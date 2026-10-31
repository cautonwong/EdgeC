#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"

static datahub_t g_hub;
static int received_publish_count = 0;
static double last_received_val = 0.0;
static int g_source_read_calls = 0;
static double g_injected_sample_value = 231.45;

static int _test_poller_publish(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                                uint64_t timestamp_ns) {
    (void)ctx;
    (void)tag_id;
    (void)meter_idx;
    (void)timestamp_ns;
    received_publish_count++;
    last_received_val = val;
    return 0;
}

/* 链式接口：submit 只负责提交，结果走回调。测试用同步回调，
 * poller 的 pumping 守卫保证不会递归打穿栈。 */
static int _test_poller_source_submit(void* ctx, uint32_t meter_idx, uint32_t tag_id,
                                      poller_read_done_cb cb, void* ud) {
    (void)ctx;
    (void)meter_idx;
    (void)tag_id;
    g_source_read_calls++;
    if (cb) cb(ud, 0, g_injected_sample_value);
    return 0;
}

static int _test_poller_source_fail(void* ctx, uint32_t meter_idx, uint32_t tag_id,
                                    poller_read_done_cb cb, void* ud) {
    (void)ctx;
    (void)meter_idx;
    (void)tag_id;
    (void)cb;
    (void)ud;
    return -1;  // Physical communication failure
}

// 验证轮询调度通过显式 source adapter 取得真实读数并推向 DataHub (可证伪)
static void test_polling_simulation(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    received_publish_count = 0;
    last_received_val = 0.0;
    g_source_read_calls = 0;
    g_injected_sample_value = 231.45;

    // 1. 初始化 DataHub，作为总线载体
    int ret = datahub_init(&g_hub, loop, "dcu_bus", 128, 0, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    // 2. 初始化电表档案并增加 1 只表
    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1);

    // 3. 初始化轮询调度引擎，注册定时任务与普通采集方案
    dcu_poller_t poller;
    dcu_poller_init(&poller);
    uint32_t oads[1] = {OAD_VOLTAGE};  // 电压 OAD
    dcu_poller_add_scheme(&poller, 1, 0, oads, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);  // 0秒周期，确保立即执行

    poller_sink_ops_t poller_sink = {0};
    poller_bus_ops_t poller_bus = {
        .ctx = NULL, .publish_realtime = _test_poller_publish, .push_event = NULL};
    poller_source_ops_t source_ops = {.ctx = NULL, .submit = _test_poller_source_submit};

    // 4. 启动动态轮询调度并注入源适配器
    ret = dcu_poller_start(&poller, source_ops, poller_bus, poller_sink);
    assert_int_equal(ret, 0);

    // 5. 调用数采数据泵，驱动一次真实抄表
    dcu_poller_pump(&poller, &inv);

    // 6. 验证：读数必须严格源于 source adapter，调度器绝不自行伪造数值
    assert_int_equal(g_source_read_calls, 1);
    assert_int_equal(received_publish_count, 1);
    assert_float_equal(last_received_val, 231.45f, 0.001f);

    // 销毁
    dcu_poller_stop(&poller);
    dcu_inventory_destroy(&inv);
    datahub_destroy(&g_hub);
}

// 验证数据源读取失败时，调度器绝不伪造常数（零 Mock 原则）
static void test_polling_source_failure_no_fake_data(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    received_publish_count = 0;
    last_received_val = 0.0;

    int ret = datahub_init(&g_hub, loop, "dcu_bus_fail", 128, 0, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(ret, 0);

    dcu_inventory_t inv;
    dcu_inventory_init(&inv);
    dcu_inventory_add(&inv, "10000002", "dlt698", 9600, 1);

    dcu_poller_t poller;
    dcu_poller_init(&poller);
    uint32_t oads[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, oads, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_sink_ops_t poller_sink = {0};
    poller_bus_ops_t poller_bus = {
        .ctx = NULL, .publish_realtime = _test_poller_publish, .push_event = NULL};
    poller_source_ops_t source_ops = {.ctx = NULL, .submit = _test_poller_source_fail};

    ret = dcu_poller_start(&poller, source_ops, poller_bus, poller_sink);
    assert_int_equal(ret, 0);

    dcu_poller_pump(&poller, &inv);

    // 读数失败时不应有任何数据发布
    assert_int_equal(received_publish_count, 0);

    dcu_poller_stop(&poller);
    dcu_inventory_destroy(&inv);
    datahub_destroy(&g_hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_polling_simulation),
        cmocka_unit_test(test_polling_source_failure_no_fake_data),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
