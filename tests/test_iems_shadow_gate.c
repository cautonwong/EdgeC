/**
 * @file test_iems_shadow_gate.c
 * @brief P1-08a c1 影子模式下行硬闸：注入端口，四条控制路径被拒、零总线写、
 *        DataHub 控制值不变，非法 IEMS_SHADOW_MODE 拒绝启动。
 *
 * 期望值来自 P1-08a c1 规格与 R-20，不从实现反推：
 * - IEMS_SHADOW_MODE 未设或 "0" = 正常，"1" = 影子，其它值启动失败。
 * - 影子模式下北向（iems_control_submit）、REST/app/反射（iems_control_submit_by_tag）、
 *   节点直写（iems_node_write_tag）、紧急停机（iems_node_emergency_trip）全部返回
 *   IEMS_WRITE_DENIED_SHADOW，执行端口调用次数 0。
 * - 影子模式不修改 DataHub 控制值；采样/读路径照常。
 * - 正常模式同一写路径执行端口调用次数 1。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "datahub_priv.h"
#include "edge_api.h"
#include "edge_service.h"
#include "iems_app.h"
#include "iems_app_manager_adapter.h"
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"
#include "iems_node.h"
#include "iems_north_adapter.h"
#include "net_buf.h"
#include "rest_control_port.h"
#include "rest_server.h"

typedef struct {
    uv_loop_t loop;
    iems_node_storage_t storage;
    iems_node_t *node;
    char shm_name[64];
} fixture_t;

typedef struct {
    int calls;
    uint32_t tag_id;
    double value;
} fake_bus_t;

static int fake_write(void *ctx, int bus_id, uint32_t slave_id, uint32_t tag_id, double value,
                      const char *proto, iems_write_done_fn on_done, void *on_done_user) {
    fake_bus_t *bus = ctx;
    (void)bus_id;
    (void)slave_id;
    (void)proto;
    (void)on_done;
    (void)on_done_user;
    bus->calls++;
    bus->tag_id = tag_id;
    bus->value = value;
    return 0;
}

/* REST 边界测试的注入缝：被禁的请求不得进入 edge_api 的下行入口。 */
static int g_raw_io_calls;
static int g_write_tag_calls;

int __wrap_edge_api_raw_io(int bus_id, net_buf_t *tx_buf, raw_io_on_complete_cb cb);
int __wrap_edge_api_write_tag(int bus_id, const char *protocol, uint32_t slave_id,
                              uint32_t physical_tag_id, double value, legacy_on_complete_cb cb,
                              void *user_data);

int __wrap_edge_api_raw_io(int bus_id, net_buf_t *tx_buf, raw_io_on_complete_cb cb) {
    (void)bus_id;
    (void)cb;
    g_raw_io_calls++;
    if (tx_buf) net_buf_unref(tx_buf);
    return 0;
}

int __wrap_edge_api_write_tag(int bus_id, const char *protocol, uint32_t slave_id,
                              uint32_t physical_tag_id, double value, legacy_on_complete_cb cb,
                              void *user_data) {
    (void)bus_id;
    (void)protocol;
    (void)slave_id;
    (void)physical_tag_id;
    (void)value;
    (void)cb;
    (void)user_data;
    g_write_tag_calls++;
    return 0;
}

static int setup(void **state) {
    fixture_t *f = calloc(1, sizeof(*f));
    assert_non_null(f);
    assert_int_equal(uv_loop_init(&f->loop), 0);
    snprintf(f->shm_name, sizeof(f->shm_name), "IEMS_SHADOW_TEST_%d", (int)getpid());
    iems_node_config_t cfg = {
        .product_name = "IEMS_TEST",
        .shm_name = f->shm_name,
        .datahub_capacity = 256,
    };
    f->node = iems_node_create_in(&f->storage, &f->loop, &cfg);
    assert_non_null(f->node);
    *state = f;
    return 0;
}

static int teardown(void **state) {
    fixture_t *f = (fixture_t *)*state;
    if (f) {
        if (f->node) iems_node_destroy(f->node);
        uv_run(&f->loop, UV_RUN_NOWAIT);
        uv_loop_close(&f->loop);
        free(f);
    }
    return 0;
}

static sqlite_svc_t *open_route_db(const char *path) {
    unlink(path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *db = sqlite_svc_create(uv_default_loop(), &cfg);
    assert_non_null(db);
    sqlite3 *raw_db = (sqlite3 *)sqlite_svc_get_db_handle(db);
    assert_non_null(raw_db);
    const char *schema_sql =
        "CREATE TABLE IF NOT EXISTS inventory (id INTEGER PRIMARY KEY, addr TEXT, protocol TEXT);"
        "CREATE TABLE IF NOT EXISTS poll_tasks (bus_id INTEGER, device_id INTEGER, tag_id "
        "INTEGER);";
    assert_int_equal(sqlite3_exec(raw_db, schema_sql, NULL, NULL, NULL), SQLITE_OK);
    assert_int_equal(sqlite3_exec(raw_db,
                                  "INSERT INTO inventory (id, addr, protocol) VALUES (1, '7', "
                                  "'MODBUS');"
                                  "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES "
                                  "(0, 1, 1003);",
                                  NULL, NULL, NULL),
                     SQLITE_OK);
    return db;
}

static void test_shadow_denies_all_control_paths(void **state) {
    fixture_t *f = (fixture_t *)*state;
    datahub_t *hub = iems_node_get_datahub(f->node);
    assert_non_null(hub);

    /* 控制值镜像先有已知值；影子下任何路径都不得改写它。 */
    assert_int_equal(iems_node_ingest_sample(f->node, 0, IEMS_TAG_GRID_POWER, 123.4, 1000), 0);
    assert_int_equal(iems_node_ingest_sample(f->node, 0, IEMS_TAG_PCS_CONTROL, 55.0, 1000), 0);

    fake_bus_t bus = {0};
    iems_execution_port_t port = {.ctx = &bus, .shadow_mode = true, .write_tag = fake_write};
    assert_int_equal(iems_node_set_execution_port(f->node, port), 0);

    /* 1) 北向下行：iems_control_submit（北向适配器与 IEC104 从站共用）。 */
    iems_device_route_t route = {.bus_id = 0, .slave_id = 7, .device_id = 1};
    snprintf(route.protocol, sizeof(route.protocol), "%s", "ModbusRTU");
    assert_int_equal(iems_control_submit(f->node, &route, IEMS_TAG_GRID_POWER, 999.0, NULL, NULL),
                     IEMS_WRITE_DENIED_SHADOW);

    /* 2) REST/app/反射：iems_control_submit_by_tag 先查轮询路由，再进同一聚合根写入口。 */
    const char *db_path = "test_iems_shadow_gate.db";
    sqlite_svc_t *db = open_route_db(db_path);
    iems_control_write_ctx_t control_ctx = {.node = f->node, .db = db};
    assert_int_equal(
        iems_control_submit_by_tag(&control_ctx, 0, IEMS_TAG_GRID_POWER, 321.0, "ModbusRTU"),
        IEMS_WRITE_DENIED_SHADOW);
    sqlite_svc_destroy(db);
    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    unlink(db_path);

    /* 3) 节点直写：领域写入口自身拒绝，不走 PCS 调度或镜像写入。 */
    assert_int_equal(iems_node_write_tag(f->node, 0, 7, 1, IEMS_TAG_GRID_POWER, 777.0,
                                         "ModbusRTU", NULL, NULL),
                     IEMS_WRITE_DENIED_SHADOW);

    /* 4) 紧急停机：安全跳闸同样是下行控制写。 */
    assert_int_equal(iems_node_emergency_trip(f->node, "shadow gate test"),
                     IEMS_WRITE_DENIED_SHADOW);

    assert_int_equal(bus.calls, 0);
    assert_int_equal(bus.tag_id, 0);

    pipe_msg_t msg;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_GRID_POWER, &msg), 0);
    assert_float_equal(msg.payload.val.f, 123.4, 0.001);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_PCS_CONTROL, &msg), 0);
    assert_float_equal(msg.payload.val.f, 55.0, 0.001);

    /* 影子只挡下行写：采样/读路径照常更新镜像。 */
    assert_int_equal(iems_node_ingest_sample(f->node, 0, IEMS_TAG_VOLTAGE_PHASE_A, 230.5, 2000),
                     0);
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_VOLTAGE_PHASE_A, &msg), 0);
    assert_float_equal(msg.payload.val.f, 230.5, 0.001);
}

static void test_shadow_execution_port_is_fail_closed(void **state) {
    (void)state;
    iems_execution_config_t cfg = {.shadow_mode = true};
    iems_execution_port_t port = iems_execution_port_create(&cfg);
    assert_non_null(port.write_tag);
    assert_true(port.shadow_mode);

    /* 不初始化任何总线：拒绝必须在触碰 edge_api/edge_api_write_tag_ex 之前返回。 */
    assert_int_equal(port.write_tag(port.ctx, 0, 1, IEMS_TAG_GRID_POWER, 1.0, "ModbusRTU", NULL,
                                    NULL),
                     IEMS_WRITE_DENIED_SHADOW);
}

static void test_normal_mode_write_reaches_port_once(void **state) {
    fixture_t *f = (fixture_t *)*state;

    fake_bus_t bus = {0};
    iems_execution_port_t port = {.ctx = &bus, .shadow_mode = false, .write_tag = fake_write};
    assert_int_equal(iems_node_set_execution_port(f->node, port), 0);

    assert_int_equal(iems_node_write_tag(f->node, 0, 7, 1, IEMS_TAG_GRID_POWER, 42.5,
                                         "ModbusRTU", NULL, NULL),
                     0);
    assert_int_equal(bus.calls, 1);
    assert_int_equal(bus.tag_id, IEMS_TAG_GRID_POWER);
    assert_true(bus.value == 42.5);
}

/* IEMS_SHADOW_MODE 启动解析：未设/0 = 正常，1 = 影子，其它值拒绝启动。 */
static void test_shadow_mode_env_parse(void **state) {
    (void)state;
    bool shadow = true;

    assert_int_equal(iems_app_shadow_mode_parse(NULL, &shadow), 0);
    assert_false(shadow);

    assert_int_equal(iems_app_shadow_mode_parse("0", &shadow), 0);
    assert_false(shadow);

    assert_int_equal(iems_app_shadow_mode_parse("1", &shadow), 0);
    assert_true(shadow);

    assert_int_equal(iems_app_shadow_mode_parse("2", &shadow), -1);
    assert_int_equal(iems_app_shadow_mode_parse("", &shadow), -1);
    assert_int_equal(iems_app_shadow_mode_parse("01", &shadow), -1);
}

/* IEMS_REST_RAW_IO 只有精确 "1" 才开启；其它值一律关闭，不拒绝启动。 */
static void test_rest_raw_io_env_parse(void **state) {
    (void)state;
    bool enabled = true;

    assert_int_equal(iems_app_rest_raw_io_parse(NULL, &enabled), 0);
    assert_false(enabled);

    assert_int_equal(iems_app_rest_raw_io_parse("0", &enabled), 0);
    assert_false(enabled);

    assert_int_equal(iems_app_rest_raw_io_parse("1", &enabled), 0);
    assert_true(enabled);

    assert_int_equal(iems_app_rest_raw_io_parse("2", &enabled), 0);
    assert_false(enabled);
    assert_int_equal(iems_app_rest_raw_io_parse("01", &enabled), 0);
    assert_false(enabled);
    assert_int_equal(iems_app_rest_raw_io_parse("TRUE", &enabled), 0);
    assert_false(enabled);
    assert_int_equal(iems_app_rest_raw_io_parse("", &enabled), 0);
    assert_false(enabled);
}

/* --- REST 管理面：写策略由组合根注入，适配器不读 env --- */

static const edge_tag_meta_t k_rest_meta[] = {
    {.id = 0x1001, .name = "U_A", .type = "float", .unit = "V", .category = "GRID"},
};

typedef struct {
    uv_loop_t loop;
    arbiter_t *arb;
    datahub_t *hub;
    net_buf_pool_t pool;
    service_t *svc;
    rest_control_port_t control;
    iems_control_ctx_t control_ctx;
} rest_fixture_t;

static const char *deny_shadow_policy(void *ctx, const char *endpoint) {
    (void)ctx;
    (void)endpoint;
    return "denied_shadow";
}

static const char *allow_policy(void *ctx, const char *endpoint) {
    (void)ctx;
    (void)endpoint;
    return NULL;
}

static int rest_setup(void **state) {
    rest_fixture_t *f = calloc(1, sizeof(*f));
    assert_non_null(f);
    assert_int_equal(uv_loop_init(&f->loop), 0);
    f->hub = calloc(1, sizeof(*f->hub));
    assert_non_null(f->hub);
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "/iems_shadow_rest_test_%d", (int)getpid());
    shm_unlink(shm_name);
    assert_int_equal(net_buf_pool_init(&f->pool, 4, 512, 0, NULL, NULL), 0);
    assert_int_equal(datahub_init(f->hub, &f->loop, shm_name, 128, 65536, k_rest_meta, 1,
                                  &f->pool, NULL, NULL),
                     0);
    assert_int_equal(arbiter_init(&f->arb, &f->loop), 0);
    f->svc = service_new(&f->loop, f->hub, f->arb, NULL, NULL);
    assert_non_null(f->svc);
    f->control.ctx = f;
    f->control.control_write_denial = deny_shadow_policy;
    service_set_control_port(f->svc, &f->control);
    *state = f;
    return 0;
}

static int rest_teardown(void **state) {
    rest_fixture_t *f = (rest_fixture_t *)*state;
    if (f) {
        if (f->svc) service_delete(f->svc);
        if (f->hub) datahub_destroy(f->hub);
        if (f->arb) arbiter_destroy(f->arb);
        uv_run(&f->loop, UV_RUN_NOWAIT);
        uv_loop_close(&f->loop);
        free(f->hub);
        free(f);
    }
    return 0;
}

/* 影子下 REST 的两个下行写入口都在 edge_api 之前被拒：HTTP 明确报错，IO 入口 0 调用。 */
static void test_shadow_rest_control_writes_denied(void **state) {
    rest_fixture_t *f = (rest_fixture_t *)*state;
    g_raw_io_calls = 0;
    g_write_tag_calls = 0;

    char *resp = rest_dispatch(f->svc, "POST", "/api/v1/io/raw",
                               "{\"bus_id\":0,\"payload\":\"0106000c002ac816\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"error\""));
    assert_non_null(strstr(resp, "\"reason\":\"denied_shadow\""));
    free(resp);

    resp = rest_dispatch(f->svc, "POST", "/api/v1/actor/set",
                         "{\"dev_id\":\"pcs_01\",\"tag\":1003,\"value\":42.0}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"error\""));
    assert_non_null(strstr(resp, "\"reason\":\"denied_shadow\""));
    free(resp);

    assert_int_equal(g_raw_io_calls, 0);
    assert_int_equal(g_write_tag_calls, 0);

    /* 同一边界在策略允许时必须进入 edge_api：证明注入缝能看见调用。 */
    f->control.control_write_denial = allow_policy;
    resp = rest_dispatch(f->svc, "POST", "/api/v1/io/raw",
                         "{\"bus_id\":0,\"payload\":\"0106000c002ac816\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    resp = rest_dispatch(f->svc, "POST", "/api/v1/actor/set",
                         "{\"dev_id\":\"pcs_01\",\"tag\":1003,\"value\":42.0}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "\"status\":\"ok\""));
    free(resp);
    assert_int_equal(g_raw_io_calls, 1);
    assert_int_equal(g_write_tag_calls, 1);
}

/* 组合根策略：raw IO 默认关闭，显式开启且非影子才允许；actor/set 始终关闭；影子优先。 */
static void test_rest_write_policy_defaults_closed(void **state) {
    (void)state;
    iems_control_ctx_t ctx = {0};
    rest_control_port_t port = {0};
    iems_control_port_init(&port, &ctx, NULL, NULL, NULL);
    assert_non_null(port.control_write_denial);
    assert_string_equal(port.control_write_denial(port.ctx, "/api/v1/io/raw"),
                        "write_disabled");
    assert_string_equal(port.control_write_denial(port.ctx, "/api/v1/actor/set"),
                        "write_disabled");

    ctx.rest_raw_io_enabled = true;
    assert_null(port.control_write_denial(port.ctx, "/api/v1/io/raw"));
    assert_string_equal(port.control_write_denial(port.ctx, "/api/v1/actor/set"),
                        "write_disabled");

    ctx.shadow_mode = true;
    assert_string_equal(port.control_write_denial(port.ctx, "/api/v1/io/raw"),
                        "denied_shadow");
    assert_string_equal(port.control_write_denial(port.ctx, "/api/v1/actor/set"),
                        "denied_shadow");
}

/* 影子下公开的 dispatch_power 也必须早退：不改 PCS 状态、不写控制镜像、不下总线。 */
static void test_shadow_dispatch_power_early_exit(void **state) {
    fixture_t *f = (fixture_t *)*state;
    datahub_t *hub = iems_node_get_datahub(f->node);
    assert_non_null(hub);
    assert_int_equal(iems_node_ingest_sample(f->node, 0, IEMS_TAG_PCS_CONTROL, 55.0, 1000), 0);

    fake_bus_t bus = {0};
    iems_execution_port_t port = {.ctx = &bus, .shadow_mode = true, .write_tag = fake_write};
    assert_int_equal(iems_node_set_execution_port(f->node, port), 0);

    double actual_kw = -1.0;
    assert_int_equal(iems_node_dispatch_power(f->node, -30.0, &actual_kw),
                     IEMS_WRITE_DENIED_SHADOW);
    assert_float_equal(actual_kw, 0.0, 0.001);
    assert_int_equal(bus.calls, 0);

    pipe_msg_t msg;
    assert_int_equal(datahub_read_shadow(hub, IEMS_TAG_PCS_CONTROL, &msg), 0);
    assert_float_equal(msg.payload.val.f, 55.0, 0.001);
}

/* 影子拒绝码必须与正常模式的 BMS L3 拒绝 (-3) 可区分。 */
static void test_shadow_denied_code_is_distinct(void **state) {
    fixture_t *f = (fixture_t *)*state;
    fake_bus_t bus = {0};
    iems_execution_port_t normal = {.ctx = &bus, .shadow_mode = false, .write_tag = fake_write};
    assert_int_equal(iems_node_set_execution_port(f->node, normal), 0);

    iems_node_update_bms_rack(f->node, 1, 750.0, 10.0, 60.0, 58.0);
    double actual_kw = 0.0;
    int rc_bms = iems_node_dispatch_power(f->node, 200.0, &actual_kw);
    assert_int_equal(rc_bms, -3);

    iems_execution_port_t shadow = {.ctx = &bus, .shadow_mode = true, .write_tag = fake_write};
    assert_int_equal(iems_node_set_execution_port(f->node, shadow), 0);
    actual_kw = -1.0;
    int rc_shadow = iems_node_dispatch_power(f->node, 200.0, &actual_kw);
    assert_int_equal(rc_shadow, IEMS_WRITE_DENIED_SHADOW);
    assert_true(rc_shadow != rc_bms);
    assert_float_equal(actual_kw, 0.0, 0.001);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_shadow_denies_all_control_paths, setup, teardown),
        cmocka_unit_test_setup_teardown(test_shadow_execution_port_is_fail_closed, setup, teardown),
        cmocka_unit_test_setup_teardown(test_normal_mode_write_reaches_port_once, setup, teardown),
        cmocka_unit_test_setup_teardown(test_shadow_mode_env_parse, setup, teardown),
        cmocka_unit_test(test_rest_raw_io_env_parse),
        cmocka_unit_test_setup_teardown(test_shadow_rest_control_writes_denied, rest_setup,
                                        rest_teardown),
        cmocka_unit_test(test_rest_write_policy_defaults_closed),
        cmocka_unit_test_setup_teardown(test_shadow_dispatch_power_early_exit, setup, teardown),
        cmocka_unit_test_setup_teardown(test_shadow_denied_code_is_distinct, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
