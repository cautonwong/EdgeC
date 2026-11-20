#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "iems_north_adapter.h"

typedef struct {
    int calls;
    int bus_id;
    uint32_t slave_id;
    uint32_t tag_id;
    double value;
    char proto[16];
} fake_exec_t;

static int fake_write(void *ctx, int bus_id, uint32_t slave_id, uint32_t tag_id, double value,
                      const char *proto) {
    fake_exec_t *exec = ctx;
    exec->calls++;
    exec->bus_id = bus_id;
    exec->slave_id = slave_id;
    exec->tag_id = tag_id;
    exec->value = value;
    snprintf(exec->proto, sizeof(exec->proto), "%s", proto ? proto : "");
    return 0;
}

/* 产品侧的控制写闭环：tag -> DB 轮询路由 -> 聚合根 -> 执行端口。
 * 三段分别有测试，但北向适配器的接线此前没有一处同时跑通。 */
static void test_control_write_route_end_to_end(void **state) {
    (void)state;
    const char *db_path = "test_iems_control_route_e2e.db";
    unlink(db_path);

    uv_loop_t *db_loop = uv_default_loop();
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *db = sqlite_svc_create(db_loop, &cfg);
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

    iems_device_route_t route;
    memset(&route, 0, sizeof(route));
    assert_int_equal(iems_control_route_for_tag(db, 1003, &route), 0);
    assert_int_equal(route.bus_id, 0);
    assert_int_equal(route.slave_id, 7);
    assert_string_equal(route.protocol, "ModbusRTU");

    uv_loop_t loop;
    uv_loop_init(&loop);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "IEMS_CTRL_ROUTE_TEST_%d", (int)getpid());
    iems_node_config_t node_cfg = {
        .product_name = "IEMS_TEST",
        .shm_name = shm_name,
        .datahub_capacity = 128,
    };
    iems_node_storage_t storage;
    iems_node_t *node = iems_node_create_in(&storage, &loop, &node_cfg);
    assert_non_null(node);

    fake_exec_t exec = {0};
    assert_int_equal(iems_node_set_execution_port(
                         node, (iems_execution_port_t){.ctx = &exec, .write_tag = fake_write}),
                     0);

    iems_control_write_ctx_t ctx = {.node = node, .db = db};
    assert_int_equal(iems_control_submit_by_tag(&ctx, 0, 1003, 42.5, "ModbusRTU"), 0);
    assert_int_equal(exec.calls, 1);
    assert_int_equal(exec.bus_id, 0);
    assert_int_equal(exec.slave_id, 7);
    assert_int_equal(exec.tag_id, 1003);
    assert_true(exec.value == 42.5);
    assert_string_equal(exec.proto, "ModbusRTU");

    /* 同一 tag 出现在第二台设备：路由不唯一，必须拒绝且不碰执行端口 */
    assert_int_equal(sqlite3_exec(raw_db,
                                  "INSERT INTO inventory (id, addr, protocol) VALUES (2, '8', "
                                  "'MODBUS');"
                                  "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES "
                                  "(0, 2, 1003);",
                                  NULL, NULL, NULL),
                     SQLITE_OK);
    exec.calls = 0;
    assert_int_equal(iems_control_submit_by_tag(&ctx, 0, 1003, 1.0, "ModbusRTU"), -1);
    assert_int_equal(exec.calls, 0);

    /* 没有轮询路由的 tag 同样拒绝，不猜 bus / 从站地址 */
    exec.calls = 0;
    assert_int_equal(iems_control_submit_by_tag(&ctx, 0, 4242, 1.0, "ModbusRTU"), -1);
    assert_int_equal(exec.calls, 0);

    iems_node_destroy(node);
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
    sqlite_svc_destroy(db);
    uv_run(db_loop, UV_RUN_NOWAIT);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_control_write_route_end_to_end),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
