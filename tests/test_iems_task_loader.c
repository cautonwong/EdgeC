#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "edge_guard.h"
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"
#include "modbus_codec.h"
#include "iems_task_loader.h"
#include "sqlite_service.h"
#include "stream_engine.h"

extern pipeline_stage_t *stage_vpp_logger_create(const char *secret, bool production);

static int g_callback_task_count = 0;
static uint32_t g_last_tag_id = 0;

static void test_task_loader_cb(const iems_loaded_task_t *task, void *user_data) {
    (void)task;
    (void)user_data;
    if (task) {
        g_callback_task_count++;
        g_last_tag_id = task->tag_id;
    }
}

static int g_protocol_task_count = 0;

static const struct {
    uint32_t tag_id;
    const char *protocol;
} g_expected_protocols[] = {
    {1001, "ModbusRTU"},
    {1002, "ModbusTCP"},
    {3001, "DLT645"},
    {4001, "CAN"},
    {5001, "IEC104"},
};

static void test_protocol_loader_cb(const iems_loaded_task_t *task, void *user_data) {
    (void)user_data;
    assert_non_null(task);
    int matched = 0;
    for (size_t i = 0; i < sizeof(g_expected_protocols) / sizeof(g_expected_protocols[0]); i++) {
        if (task->tag_id == g_expected_protocols[i].tag_id) {
            assert_string_equal(task->protocol, g_expected_protocols[i].protocol);
            matched = 1;
        }
    }
    // 未知或空协议的行必须被跳过，不能走到这里。
    assert_true(matched);
    g_protocol_task_count++;
}

static void test_iems_task_loading_direct_product_code(void **state) {
    (void)state;
    const char *db_path = "test_iems_task_loader_direct.db";
    unlink(db_path);

    uv_loop_t *loop = uv_default_loop();
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);

    // 1. Initial state: schema missing -> query fails, returns -1 (no tasks, no callback)
    g_callback_task_count = 0;
    int rc = iems_poll_tasks_load(svc, test_task_loader_cb, NULL);
    assert_int_equal(rc, -1);
    assert_int_equal(g_callback_task_count, 0);

    // 2. Setup schema & seed data via sqlite_svc_exec
    sqlite3 *raw_db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_non_null(raw_db);

    const char *schema_sql =
        "CREATE TABLE IF NOT EXISTS inventory (id INTEGER PRIMARY KEY, addr TEXT, protocol TEXT);"
        "CREATE TABLE IF NOT EXISTS poll_tasks (bus_id INTEGER, device_id INTEGER, tag_id INTEGER);";
    rc = sqlite3_exec(raw_db, schema_sql, NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);

    // 3. 有表无数据 -> 0 条任务,不是错误(启动重试循环靠这个返回值区分)
    g_callback_task_count = 0;
    rc = iems_poll_tasks_load(svc, test_task_loader_cb, NULL);
    assert_int_equal(rc, 0);
    assert_int_equal(g_callback_task_count, 0);

    const char *seed_sql =
        "INSERT INTO inventory (id, addr, protocol) VALUES (1, '01', 'MODBUS');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (2, '02', 'MODBUS');"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 1, 1001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 2, 1002);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (10, 1, 9999);";  // Bus 10
                                                                                     // should be
                                                                                     // filtered
    rc = sqlite3_exec(raw_db, seed_sql, NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);

    // 4. Load tasks -> should load 2 tasks (bus 0 tag 1001, 1002; bus 10 ignored)
    g_callback_task_count = 0;
    rc = iems_poll_tasks_load(svc, test_task_loader_cb, NULL);
    assert_int_equal(rc, 2);
    assert_int_equal(g_callback_task_count, 2);
    assert_int_equal(g_last_tag_id, 1002);

    /* 北向控制写的路由：bus / 从站地址来自 poll_tasks + inventory，协议规范化 */
    iems_device_route_t route;
    memset(&route, 0, sizeof(route));
    assert_int_equal(iems_poll_task_route(svc, 1001, &route), 0);
    assert_int_equal(route.bus_id, 0);
    assert_int_equal(route.slave_id, 1);
    assert_string_equal(route.protocol, "ModbusRTU");
    /* bus 10 是北向镜像，不参与控制写 */
    assert_int_equal(iems_poll_task_route(svc, 9999, &route), -1);
    assert_int_equal(iems_poll_task_route(svc, 4242, &route), -1);

    rc = sqlite3_exec(raw_db,
                      "INSERT INTO inventory (id, addr, protocol) VALUES (3, '03', 'modbus_rtu');"
                      "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (3, 3, 1003);",
                      NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);
    assert_int_equal(iems_poll_task_route(svc, 1003, &route), 0);
    assert_int_equal(route.bus_id, 3);
    assert_int_equal(route.slave_id, 3);
    assert_string_equal(route.protocol, "ModbusRTUHiCRC");

    /* addr 按整数解析；0 沿用 loader 的默认从站 1 */
    rc = sqlite3_exec(raw_db,
                      "INSERT INTO inventory (id, addr, protocol) VALUES (4, '0', 'MODBUS');"
                      "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 4, 1004);",
                      NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);
    assert_int_equal(iems_poll_task_route(svc, 1004, &route), 0);
    assert_int_equal(route.slave_id, 1);

    /* 同一设备重复登记轮询任务：仍是同一台设备，路由可用 */
    rc = sqlite3_exec(raw_db,
                      "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 2, 1002);",
                      NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);
    assert_int_equal(iems_poll_task_route(svc, 1002, &route), 0);
    assert_int_equal(route.bus_id, 0);
    assert_int_equal(route.slave_id, 2);
    assert_string_equal(route.protocol, "ModbusRTU");

    /* 同一 tag 映射到两台不同设备：无法确定写哪台，拒绝 */
    rc = sqlite3_exec(raw_db,
                      "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (3, 1, 1001);",
                      NULL, NULL, NULL);
    assert_int_equal(rc, SQLITE_OK);
    assert_int_equal(iems_poll_task_route(svc, 1001, &route), -2);

    sqlite_svc_destroy(svc);
    unlink(db_path);
}

static void test_stage_vpp_logger_edge_env_guard(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    const char *db_path = "test_vpp_logger_guard.db";
    unlink(db_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);

    // Case 1: EDGE_ENV=production without VPP_AUDIT_SECRET -> Fail-fast (returns NULL)
    setenv("EDGE_ENV", "production", 1);
    unsetenv("VPP_AUDIT_SECRET");
    pipeline_stage_t *stage_prod_missing = stage_vpp_logger_create(NULL, true);
    assert_null(stage_prod_missing);

    // Case 2: EDGE_ENV=development without VPP_AUDIT_SECRET -> Dev fallback (returns valid stage)
    setenv("EDGE_ENV", "development", 1);
    unsetenv("VPP_AUDIT_SECRET");
    pipeline_stage_t *stage_dev = stage_vpp_logger_create(NULL, false);
    assert_non_null(stage_dev);
    assert_string_equal(stage_dev->name, "VPP_LOGGER");
    if (stage_dev->cleanup_state) {
        stage_dev->cleanup_state(stage_dev->priv_state);
    }
    free(stage_dev);

    // Case 3: EDGE_ENV=production with valid VPP_AUDIT_SECRET -> Success
    setenv("EDGE_ENV", "production", 1);
    setenv("VPP_AUDIT_SECRET", "custom_prod_vpp_secret_2026", 1);
    pipeline_stage_t *stage_prod_valid = stage_vpp_logger_create("custom_prod_vpp_secret_2026", true);
    assert_non_null(stage_prod_valid);
    assert_string_equal(stage_prod_valid->name, "VPP_LOGGER");
    if (stage_prod_valid->cleanup_state) {
        stage_prod_valid->cleanup_state(stage_prod_valid->priv_state);
    }
    free(stage_prod_valid);

    // Clean up
    unsetenv("EDGE_ENV");
    unsetenv("VPP_AUDIT_SECRET");
    sqlite_svc_destroy(svc);
    unlink(db_path);
}

/* 轮询层把产品 tag 翻成驱动认识的原生标识：DLT645 是 DI，Modbus 是地址+个数，
 * 高位字节序变体与标准 ModbusRTU 走同一张点表。 */
static void test_native_id_translation(void **state) {
    (void)state;
    assert_int_equal(iems_driver_adapter_native_id("DLT645", IEMS_TAG_GRID_POWER), 0x02030000u);
    assert_int_equal(iems_driver_adapter_native_id("ModbusTCP", IEMS_TAG_GRID_POWER),
                     MODBUS_NATIVE(12, 2));
    assert_int_equal(iems_driver_adapter_native_id("ModbusRTUHiCRC", IEMS_TAG_GRID_POWER),
                     MODBUS_NATIVE(12, 2));
    /* 点表里没有的 tag 维持原样，避免静默换号 */
    assert_int_equal(iems_driver_adapter_native_id("DLT645", 999999u), 999999u);

    /* 量纲：DI 格式 XX.XXXX kW 换算到 tag 契约的 W，乘数 0.1。
     * scaler 是 float，容差按 float 精度放宽。 */
    assert_true(fabs(iems_driver_adapter_tag_scale("DLT645", IEMS_TAG_GRID_POWER) - 0.1) < 1e-6);
    /* 用一个明确不在点表里的 tag，避免用真实 tag 加偏移拼出意外撞号的值 */
    assert_true(fabs(iems_driver_adapter_tag_scale("DLT645", 0xFFFFu) - 1.0) < 1e-6);
}

/* 读路径乘 tag_scale，写路径除以同一系数；TCP 与 RTU 共用一张点表，口径必须
 * 一致。台架给寄存器 2317 时必须还原成 231.7V，而不是 23170。 */
static void test_tag_scale_modbus_family_is_one_calibration(void **state) {
    (void)state;
    const double rtu = iems_driver_adapter_tag_scale("ModbusRTU", IEMS_TAG_VOLTAGE_PHASE_A);
    const double tcp = iems_driver_adapter_tag_scale("ModbusTCP", IEMS_TAG_VOLTAGE_PHASE_A);
    assert_true(fabs(rtu - 0.1) < 1e-6);
    assert_true(fabs(tcp - rtu) < 1e-9);

    /* 写 231.7V 下发寄存器 2317；按同一系数读回仍是 231.7V */
    uint32_t native_id = 0;
    double raw = 0.0;
    const void *desc = NULL;
    size_t desc_len = 0;
    assert_int_equal(iems_exec_translate_write("ModbusTCP", IEMS_TAG_VOLTAGE_PHASE_A, 231.7,
                                               &native_id, &raw, &desc, &desc_len),
                     0);
    assert_true(fabs(raw - 2317.0) < 1e-9);
    assert_true(fabs(raw * tcp - 231.7) < 1e-3);
}

static void test_protocol_canonical(void **state) {
    (void)state;
    static const struct {
        const char *raw;
        const char *canonical;
    } good[] = {
        {"MODBUS", "ModbusRTU"},    {"Modbus", "ModbusRTU"},
        {"ModbusRTU", "ModbusRTU"}, {"  modbus  ", "ModbusRTU"},
        {"ModbusTCP", "ModbusTCP"}, {"IEC104", "IEC104"},
        {"CAN", "CAN"},             {"DLT645", "DLT645"},
        {"DLT645-2007", "DLT645"},  {" dlt645-2007\t", "DLT645"},
        /* v3 配置串：默认类型是高字节先发 CRC，rcrc 才是标准序 */
        {"modbus_rtu", "ModbusRTUHiCRC"},
        {"modbus_rtu_rcrc", "ModbusRTU"},
        {"ModbusRTUHiCRC", "ModbusRTUHiCRC"},
    };
    char out[16];
    for (size_t i = 0; i < sizeof(good) / sizeof(good[0]); i++) {
        memset(out, 0, sizeof(out));
        assert_int_equal(iems_driver_protocol_canonical(good[i].raw, out), 0);
        assert_string_equal(out, good[i].canonical);
    }

    static const char *bad[] = {NULL, "", "   ", "DLT645-1997", "FOO", "ModbusRTU2"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        assert_int_equal(iems_driver_protocol_canonical(bad[i], out), -1);
    }
    assert_int_equal(iems_driver_protocol_canonical("CAN", NULL), -1);
}

static void test_protocol_selection_from_inventory(void **state) {
    (void)state;
    const char *db_path = "test_iems_task_loader_protocol.db";
    unlink(db_path);

    uv_loop_t *loop = uv_default_loop();
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);

    sqlite3 *raw_db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_non_null(raw_db);
    const char *schema_sql =
        "CREATE TABLE IF NOT EXISTS inventory (id INTEGER PRIMARY KEY, addr TEXT, protocol TEXT);"
        "CREATE TABLE IF NOT EXISTS poll_tasks (bus_id INTEGER, device_id INTEGER, tag_id INTEGER);";
    assert_int_equal(sqlite3_exec(raw_db, schema_sql, NULL, NULL, NULL), SQLITE_OK);

    const char *seed_sql =
        "INSERT INTO inventory (id, addr, protocol) VALUES (1, '01', ' modbus ');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (2, '02', 'ModbusTCP');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (3, '03', 'DLT645-2007');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (4, '04', 'CAN');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (5, '05', 'iec104');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (6, '06', 'UNKNOWN');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (7, '07', '');"
        "INSERT INTO inventory (id, addr, protocol) VALUES (8, '08', 'DLT645-1997');"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 1, 1001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 2, 1002);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 3, 3001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 4, 4001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 5, 5001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 6, 6001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 7, 7001);"
        "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (0, 8, 3002);";
    assert_int_equal(sqlite3_exec(raw_db, seed_sql, NULL, NULL, NULL), SQLITE_OK);

    g_protocol_task_count = 0;
    int rc = iems_poll_tasks_load(svc, test_protocol_loader_cb, NULL);
    assert_int_equal(rc, 5);
    assert_int_equal(g_protocol_task_count, 5);

    sqlite_svc_destroy(svc);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_iems_task_loading_direct_product_code),
        cmocka_unit_test(test_stage_vpp_logger_edge_env_guard),
        cmocka_unit_test(test_native_id_translation),
        cmocka_unit_test(test_tag_scale_modbus_family_is_one_calibration),
        cmocka_unit_test(test_protocol_canonical),
        cmocka_unit_test(test_protocol_selection_from_inventory),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
