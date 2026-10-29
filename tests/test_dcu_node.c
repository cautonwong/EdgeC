/**
 * @file test_dcu_node.c
 * @brief Unit tests for DCU Node Aggregate Root (Issue #375)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "dcu_inventory.h"
#include "dcu_node.h"

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
    const char *db_path = "/tmp/test_dcu_node.db";
    unlink(db_path);

    dcu_node_config_t cfg = {.product_name = "DCU_Test",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};

    dcu_node_t *node = dcu_node_create(loop, &cfg);
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
    const char *db_path = "/tmp/test_dcu_node_events.db";
    unlink(db_path);

    dcu_node_config_t cfg = {.product_name = "DCU_Event_Test",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};

    dcu_node_t *node = dcu_node_create(loop, &cfg);
    assert_non_null(node);

    /* 1. 注册档案电表 */
    dcu_inventory_t *inv = dcu_node_get_inventory(node);
    assert_non_null(inv);
    int rc = dcu_inventory_add(inv, "202610180001", "DLT698", 9600, 1, NULL);
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

    /* 6. 测试下行遥控动作 -> 触发 REMOTE_TRIP 动作事件 */
    rc = dcu_node_execute_control(node, "202610180001", OI_REMOTE_CONTROL);
    assert_int_equal(rc, 0);
    assert_int_equal(ev_ctx.event_count, 3);
    assert_int_equal(ev_ctx.last_event.type, DCU_EVENT_REMOTE_TRIP);

    /* 7. 测试非法未授权控制指令 -> 拒绝 */
    rc = dcu_node_execute_control(node, "202610180001", 0x1234);
    assert_int_equal(rc, -3);

    dcu_node_destroy(node);
    unlink(db_path);
}

static int g_stub_calls = 0;
static int _stub_submit(void *ctx, uint32_t meter_idx, uint32_t tag_id,
                        poller_read_done_cb cb, void *ud) {
    (void)ctx;
    (void)meter_idx;
    (void)tag_id;
    g_stub_calls++;
    if (cb) cb(ud, 0, 231.7);   /* 同步回调：pump 的 pumping 守卫防重入 */
    return 0;
}

/* setter 装进去的数据源必须真的被 poller 用上（否则组合根登记等于空转） */
static void test_dcu_node_set_source_ops_is_used(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "/tmp/test_dcu_node_srcops.db";
    unlink(db_path);

    dcu_node_config_t cfg = {.product_name = "DCU_Test",
                             .db_path = db_path,
                             .datahub_capacity = 1024,
                             .poll_interval_ms = 100};
    dcu_node_t *node = dcu_node_create(loop, &cfg);
    assert_non_null(node);

    dcu_inventory_add(dcu_node_get_inventory(node), "SRC_0001", "dlt698", 9600, 1, NULL);

    poller_source_ops_t ops = {.ctx = NULL, .submit = _stub_submit};
    dcu_node_set_source_ops(node, ops);

    g_stub_calls = 0;
    assert_int_equal(dcu_node_start(node), 0);
    (void)dcu_node_pump(node);
    assert_true(g_stub_calls > 0); // 注入的源被调用到了

    assert_int_equal(dcu_node_stop(node), 0);
    dcu_node_destroy(node);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dcu_node_lifecycle, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dcu_node_domain_methods_and_events, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dcu_node_set_source_ops_is_used, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
