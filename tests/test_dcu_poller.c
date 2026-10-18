#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"

// 1. 验证 698.45 定时任务与方案注册
static void test_task_and_scheme_add(void** state) {
    (void)state;
    dcu_poller_t poller;
    int ret = dcu_poller_init(&poller);
    assert_int_equal(ret, 0);
    assert_int_equal(poller.task_count, 0);
    assert_int_equal(poller.scheme_count, 0);

    // 注册方案 1 (普通采集方式 0，CSD 包含电压 OAD_VOLTAGE)
    uint32_t csds[2] = {OAD_VOLTAGE, OAD_CURRENT};
    ret = dcu_poller_add_scheme(&poller, 1, 0, csds, 2);
    assert_int_equal(ret, 0);
    assert_int_equal(poller.scheme_count, 1);

    dcu_scheme_t* s = dcu_poller_get_scheme(&poller, 1);
    assert_non_null(s);
    assert_int_equal(s->scheme_id, 1);
    assert_int_equal(s->data.normal.capture_mode, 0);
    assert_int_equal(s->data.normal.csd_count, 2);
    assert_int_equal(s->data.normal.csd_list[0], OAD_VOLTAGE);

    // 注册任务 1 (执行频率 TI = 15秒)
    ret = dcu_poller_add_task(&poller, 1, 15, 1);
    assert_int_equal(ret, 0);
    assert_int_equal(poller.task_count, 1);
    assert_int_equal(poller.tasks[0].interval_s, 15);
    assert_int_equal(poller.tasks[0].scheme_id, 1);

    dcu_poller_destroy(&poller);
}

static int g_poller_pub_count = 0;
static int _poller_test_pub(void* ctx, uint32_t tag_id, uint32_t meter_idx, double val,
                            uint64_t timestamp_ns) {
    (void)ctx;
    (void)tag_id;
    (void)meter_idx;
    (void)val;
    (void)timestamp_ns;
    g_poller_pub_count++;
    return 0;
}

static int _test_poller_source_read(void* ctx, uint32_t meter_idx, uint32_t tag_id,
                                    double* out_val) {
    (void)ctx;
    (void)meter_idx;
    (void)tag_id;
    *out_val = 220.0;
    return 0;
}

// 2. 验证基于时间间隔对齐与方案队列生成机制 (TI 周期触发及防抖)
static void test_poller_pump_and_ti_alignment(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    // 1. 注册 3 台电表档案
    dcu_inventory_add(&inv, "10000001", "dlt698", 9600, 1, NULL);
    dcu_inventory_add(&inv, "10000002", "dlt698", 9600, 1, NULL);
    dcu_inventory_add(&inv, "10000003", "dlt698", 9600, 1, NULL);

    // 2. 注册周期采集方案 2 (CSD: OAD_VOLTAGE) 与任务 10 (TI = 10s)
    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 2, 0, csds, 1);
    dcu_poller_add_task(&poller, 10, 10, 2);

    poller_sink_ops_t sink = {0};
    poller_bus_ops_t bus = {.ctx = NULL, .publish_realtime = _poller_test_pub, .push_event = NULL};
    poller_source_ops_t source = {.ctx = NULL, .read_value = _test_poller_source_read};
    dcu_poller_start(&poller, source, bus, sink);

    g_poller_pub_count = 0;

    // 3. 首次调度执行 (last_run_time 为 0，无条件到期触发)
    dcu_poller_pump(&poller, &inv);

    assert_int_equal(g_poller_pub_count, 3);  // 3台档案全部入队并生成数据推送

    // 校验监控单元 (6035) 收发包数更新
    assert_int_equal(poller.tasks[0].monitor.total_meters, 3);

    // 4. 在时间未过 10s 间隔时，调度泵应防抖跳过，生成数量为 0！
    g_poller_pub_count = 0;
    dcu_poller_pump(&poller, &inv);
    assert_int_equal(g_poller_pub_count, 0);  // 周期未到，防抖跳过长度为 0

    // 5. 模拟时间流逝 15 秒，再次触发轮询！
    poller.tasks[0].last_run_time -= 15;
    if (poller.tasks[0].clock.next_tick_us >= 15000000ULL) {
        poller.tasks[0].clock.next_tick_us -= 15000000ULL;
    } else {
        poller.tasks[0].clock.next_tick_us = 0;
    }
    g_poller_pub_count = 0;
    dcu_poller_pump(&poller, &inv);
    assert_int_equal(g_poller_pub_count, 3);  // 第二次触发成功！

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_task_and_scheme_add),
        cmocka_unit_test(test_poller_pump_and_ti_alignment),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
