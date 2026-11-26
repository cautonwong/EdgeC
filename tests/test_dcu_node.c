/**
 * @file test_dcu_node.c
 * @brief Unit tests for DCU Node Aggregate Root (Issue #375)
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
#include <uv.h>

#include "dcu_698_object_model.h"
#include "dcu_inventory.h"
#include "dcu_node.h"
#include "dcu_relay_map.h"
#include "edge_slave_plane.h"

static const char *unique_db_path(char *buf, size_t cap, const char *tag) {
    snprintf(buf, cap, "/tmp/%s_%ld.db", tag, (long)getpid());
    return buf;
}

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int setup(void **state) {
    uv_loop_t *loop = (uv_loop_t *)malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    uv_walk(loop, on_walk_close, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    free(loop);
    return 0;
}

static void test_dcu_node_lifecycle(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_node");
    unlink(db_path);

    dcu_node_config_t cfg = {.product_name = "DCU_Test",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};

    size_t node_storage_size = dcu_node_storage_size(&cfg);
    void *node_mem = malloc(node_storage_size);
    dcu_node_t *node = dcu_node_create_in(node_mem, node_storage_size, loop, &cfg);
    assert_non_null(node);

    assert_non_null(dcu_node_get_inventory(node));
    assert_non_null(dcu_node_get_poller(node));
    assert_non_null(dcu_node_get_storage(node));
    assert_non_null(dcu_node_get_datahub(node));
    assert_non_null(dcu_node_get_container(node));

    /* Start the node polling loop */
    int rc = dcu_node_start(node);
    assert_int_equal(rc, 0);

    /* Pump active poller */
    rc = dcu_node_pump(node);
    assert_int_equal(rc, 0);

    /* Stop the node */
    rc = dcu_node_stop(node);
    assert_int_equal(rc, 0);

    /* Clean destruction */
    dcu_node_destroy(node);

    free(node_mem);
    unlink(db_path);
}

typedef struct {
    int event_count;
    dcu_domain_event_t last_event;
} test_event_ctx_t;

static void on_test_domain_event(const dcu_domain_event_t *event, void *user_data) {
    test_event_ctx_t *ctx = (test_event_ctx_t *)user_data;
    if (ctx && event) {
        ctx->event_count++;
        ctx->last_event = *event;
    }
}

static void test_dcu_node_domain_methods_and_events(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_node_events");
    unlink(db_path);

    dcu_node_config_t cfg = {.product_name = "DCU_Event_Test",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};

    size_t node_storage_size = dcu_node_storage_size(&cfg);
    void *node_mem = malloc(node_storage_size);
    dcu_node_t *node = dcu_node_create_in(node_mem, node_storage_size, loop, &cfg);
    assert_non_null(node);

    /* 1. 注册档案电表 */
    dcu_inventory_t *inv = dcu_node_get_inventory(node);
    assert_non_null(inv);
    int rc = dcu_inventory_add(inv, "202610180001", "DLT698", 9600, 1);
    assert_int_equal(rc, 0);

    /* 2. 挂接微服务领域事件监听器 */
    test_event_ctx_t ev_ctx = {0};
    rc = dcu_node_set_event_handler(node, on_test_domain_event, &ev_ctx);
    assert_int_equal(rc, 0);

    /* 3. 测试未注册电表的非法注入 -> 领域不变量拒绝 */
    rc = dcu_node_feed_meter_reading(node, "999999999999", OAD_VOLTAGE, 220.0, 0);
    assert_int_equal(rc, -2);
    assert_int_equal(ev_ctx.event_count, 0);

    /* 4. 测试合法电表正常遥测注入 -> 触发 TELEMETRY 领域事件 */
    rc = dcu_node_feed_meter_reading(node, "202610180001", OAD_VOLTAGE, 220.5, 1000000ULL);
    assert_int_equal(rc, 0);
    assert_int_equal(ev_ctx.event_count, 1);
    assert_int_equal(ev_ctx.last_event.type, DCU_EVENT_TELEMETRY);
    assert_string_equal(ev_ctx.last_event.meter_addr, "202610180001");
    assert_float_equal((float)ev_ctx.last_event.value, 220.5f, 0.01f);
    assert_false(ev_ctx.last_event.is_alarm_active);

    /* 5. 测试失压异常注入 -> 触发 VOLTAGE_DROP 告警事件 */
    rc = dcu_node_feed_meter_reading(node, "202610180001", OAD_VOLTAGE, 150.0, 2000000ULL);
    assert_int_equal(rc, 0);
    assert_int_equal(ev_ctx.event_count, 2);
    assert_int_equal(ev_ctx.last_event.type, DCU_EVENT_VOLTAGE_DROP);
    assert_true(ev_ctx.last_event.is_alarm_active);

    /* 6. 下行遥控动作 -> 派发 REMOTE_TRIP 事件，并且**真的去下发 645 控制帧**。
     *
     * 本测试没有总线（没调 edge_api_init），所以帧一定发不出去，返回 -5。
     * 这正是要的语义：事件记了账 ≠ 命令出了门。以前这里断言 0，而实现只发事件、
     * 什么都不下发 —— 那是最典型的"假成功"。 */
    rc = dcu_node_execute_control(node, "202610180001", OI_REMOTE_CONTROL, true);
    /* 本测试没装配总线：-4 = 拿不到发送缓冲，-5 = 总线拒绝。两者都表示"没下发"，
     * 关键是**不能是 0** —— 0 会让上层以为命令出了门。 */
    assert_true(rc < 0);
    assert_int_equal(ev_ctx.event_count, 3);
    assert_int_equal(ev_ctx.last_event.type, DCU_EVENT_REMOTE_TRIP);

    /* 7. 测试非法未授权控制指令 -> 拒绝（连事件都不该发） */
    rc = dcu_node_execute_control(node, "202610180001", 0x1234, true);
    assert_int_equal(rc, -3);
    assert_int_equal(ev_ctx.event_count, 3);

    dcu_node_destroy(node);


    free(node_mem);
    unlink(db_path);
}

static int g_stub_calls = 0;
static int _stub_submit(void *ctx, uint32_t meter_idx, uint32_t tag_id, uint32_t task_mask,
                        poller_read_done_cb cb, void *ud) {
    (void)ctx;
    (void)meter_idx;
    (void)tag_id;
    (void)task_mask;
    g_stub_calls++;
    if (cb) cb(ud, 0, 231.7);   /* 同步回调：pump 的 pumping 守卫防重入 */
    return 0;
}

/* setter 装进去的数据源必须真的被 poller 用上（否则组合根登记等于空转） */
static void test_dcu_node_set_source_ops_is_used(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_node_srcops");
    unlink(db_path);

    dcu_node_config_t cfg = {.product_name = "DCU_Test",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};
    size_t node_storage_size = dcu_node_storage_size(&cfg);
    void *node_mem = malloc(node_storage_size);
    dcu_node_t *node = dcu_node_create_in(node_mem, node_storage_size, loop, &cfg);
    assert_non_null(node);

    dcu_inventory_add(dcu_node_get_inventory(node), "SRC_0001", "dlt698", 9600, 1);

    poller_source_ops_t ops = {.ctx = NULL, .submit = _stub_submit};
    dcu_node_set_source_ops(node, ops);

    g_stub_calls = 0;
    assert_int_equal(dcu_node_start(node), 0);
    (void)dcu_node_pump(node);
    assert_true(g_stub_calls > 0); // 注入的源被调用到了

    assert_int_equal(dcu_node_stop(node), 0);
    dcu_node_destroy(node);

    free(node_mem);
    unlink(db_path);
}

/* dcu_telemetry_latest 按 (meter_address, oad) 主键落最近值；这里只数地址下的行。 */
static int latest_count(dcu_storage_t *storage, const char *addr) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(storage->db,
                           "SELECT COUNT(*) FROM dcu_telemetry_latest WHERE meter_address = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, addr, -1, SQLITE_TRANSIENT);
    int count = (sqlite3_step(stmt) == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : -1;
    sqlite3_finalize(stmt);
    return count;
}

/* 档案删除用末元素覆盖删除位：删 B 后 D 落到下标 1，再新增的 E 占下标 3（slot 4）。
 * record_reading 的 device_id 是 slot_id，落库地址必须归 slot 对应的表。 */
static void test_dcu_node_record_reading_uses_slot_id(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_node_slot_reading");
    unlink(db_path);

    dcu_node_config_t cfg = {.product_name = "DCU_Slot_Test",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};
    size_t node_storage_size = dcu_node_storage_size(&cfg);
    void *node_mem = malloc(node_storage_size);
    dcu_node_t *node = dcu_node_create_in(node_mem, node_storage_size, loop, &cfg);
    assert_non_null(node);

    dcu_inventory_t *inv = dcu_node_get_inventory(node);
    assert_int_equal(dcu_inventory_add(inv, "10000001", "DLT645", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(inv, "10000002", "DLT645", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(inv, "10000003", "DLT645", 9600, 1), 0);
    assert_int_equal(dcu_inventory_add(inv, "10000004", "DLT645", 9600, 1), 0);
    assert_int_equal(dcu_inventory_remove(inv, "10000002"), 0);
    assert_int_equal(dcu_inventory_add(inv, "10000005", "DLT645", 9600, 1), 0);
    assert_int_equal(inv->count, 4);
    assert_string_equal(inv->meters[1].address, "10000004");
    assert_int_equal(inv->meters[1].slot_id, 3);
    assert_string_equal(inv->meters[3].address, "10000005");
    assert_int_equal(inv->meters[3].slot_id, 4);

    dcu_storage_t *storage = dcu_node_get_storage(node);
    double value = 231.7;
    /* D 的 slot=3 落在下标 1：按下标取会记到下标 3 的 E 名下。 */
    assert_int_equal(
        dcu_node_record_reading(node, 3, 0x02010100, (const uint8_t *)&value, sizeof(value)), 0);
    assert_int_equal(dcu_storage_flush(storage), 0);
    assert_int_equal(latest_count(storage, "10000004"), 1);
    assert_int_equal(latest_count(storage, "10000005"), 0);

    /* E 的 slot=4 已超出 count=4：按 slot 当下标会被直接拒收。 */
    assert_int_equal(
        dcu_node_record_reading(node, 4, 0x02010100, (const uint8_t *)&value, sizeof(value)), 0);
    assert_int_equal(dcu_storage_flush(storage), 0);
    assert_int_equal(latest_count(storage, "10000005"), 1);

    dcu_node_destroy(node);
    free(node_mem);
    unlink(db_path);
}

/* 8000 方法 129/130/131：空映射诚实回 DAR 2；聚合根填表后可解析到表地址/DO，
 * 真实 DO 执行未接线，仍按 DAR 5 拒绝，不返回成功。 */
static void test_relay_map_resolution_keeps_execution_rejected(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    char db_file[96];
    const char *db_path = unique_db_path(db_file, sizeof(db_file), "test_dcu_node_relay");
    unlink(db_path);

    dcu_node_config_t cfg = {.product_name = "DCU_Test",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};
    size_t node_storage_size = dcu_node_storage_size(&cfg);
    void *node_mem = malloc(node_storage_size);
    dcu_node_t *node = dcu_node_create_in(node_mem, node_storage_size, loop, &cfg);
    assert_non_null(node);

    dcu_action_param_t param = {.count = 1, .relay_oad = 0x20000400u};
    const uint32_t methods[] = {129, 130, 131};
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
        assert_int_equal(dcu_698_call(((uint32_t)OI_REMOTE_CONTROL << 16) | (methods[i] << 8),
                                      &param, NULL),
                         EDGE_SLAVE_SRC_TEMPORARY);
    }

    assert_int_equal(dcu_node_set_relay(node, 0x20000400u, "202610180001", 5), 0);
    const dcu_relay_entry_t *target = dcu_node_find_relay(node, 0x20000400u);
    assert_non_null(target);
    assert_string_equal(target->meter_addr, "202610180001");
    assert_int_equal(target->do_channel, 5);
    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
        assert_int_equal(dcu_698_call(((uint32_t)OI_REMOTE_CONTROL << 16) | (methods[i] << 8),
                                      &param, NULL),
                         EDGE_SLAVE_SRC_CLASS_MISMATCH);
    }

    /* 未映射的继电器 OAD 不按表地址猜，仍回 DAR 2。 */
    param.relay_oad = 0x20000499u;
    assert_int_equal(
        dcu_698_call(((uint32_t)OI_REMOTE_CONTROL << 16) | (129u << 8), &param, NULL),
        EDGE_SLAVE_SRC_TEMPORARY);

    dcu_node_destroy(node);
    free(node_mem);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dcu_node_lifecycle, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dcu_node_domain_methods_and_events, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dcu_node_set_source_ops_is_used, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dcu_node_record_reading_uses_slot_id, setup, teardown),
        cmocka_unit_test_setup_teardown(test_relay_map_resolution_keeps_execution_rejected, setup,
                                        teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
