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

static int g_failing_channel = -1;
static int _mock_failing_source_read(void* ctx, uint32_t meter_idx, uint32_t tag_id,
                                    double* out_val) {
    (void)ctx;
    (void)tag_id;
    // 如果是偶数表计或指定失败通道则失败
    if ((int)meter_idx == g_failing_channel || g_failing_channel == 999) {
        return -1; // 模拟南向超时/故障
    }
    *out_val = 220.0;
    return 0;
}

// 3. 验证通道失败熔断与可观测统计 (AEP-401)
static void test_poller_circuit_breaker_and_observability(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    // 注册 1 台挂在通道 3 的电表
    dcu_inventory_add(&inv, "CH3_0001", "dlt698", 9600, 3, NULL);

    // 配置通道 3 调度策略：连续失败 3 次触发熔断
    edge_poll_policy_t policy = {
        .interval_ms = 0,
        .timeout_ms = 500,
        .max_in_flight = 1,
        .circuit_break_threshold = 3
    };
    assert_int_equal(dcu_poller_set_channel_policy(&poller, 3, &policy), 0);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1); // 0s 周期无条件到期

    poller_sink_ops_t sink = {0};
    poller_bus_ops_t bus = {0};
    g_failing_channel = 999; // 全部模拟失败
    poller_source_ops_t source = {.ctx = NULL, .read_value = _mock_failing_source_read};
    dcu_poller_start(&poller, source, bus, sink);

    // 连续触发 3 次失败
    dcu_poller_pump(&poller, &inv);
    dcu_poller_pump(&poller, &inv);
    dcu_poller_pump(&poller, &inv);

    edge_poll_stats_t stats;
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 3, &stats), 0);
    assert_int_equal(stats.consecutive_fails, 3);
    assert_true(stats.is_circuit_broken); // 触发熔断

    // 再次调用 pump，熔断通道被阻断，poll_count 不再增加
    uint64_t last_poll_count = stats.poll_count;
    dcu_poller_pump(&poller, &inv);
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 3, &stats), 0);
    assert_int_equal(stats.poll_count, last_poll_count);

    // 重置熔断恢复在线
    assert_int_equal(dcu_poller_reset_circuit_breaker(&poller, 3), 0);
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 3, &stats), 0);
    assert_false(stats.is_circuit_broken);
    assert_int_equal(stats.consecutive_fails, 0);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 4. 验证多通道隔离性 (通道 1 故障熔断不影响通道 2 正常调度)
static void test_poller_multi_channel_isolation(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    // 通道 1 电表与通道 2 电表
    dcu_inventory_add(&inv, "CH1_0001", "dlt698", 9600, 1, NULL);
    dcu_inventory_add(&inv, "CH2_0001", "dlt698", 9600, 2, NULL);

    edge_poll_policy_t policy = {.interval_ms = 0, .max_in_flight = 1, .circuit_break_threshold = 2};
    dcu_poller_set_channel_policy(&poller, 1, &policy);
    dcu_poller_set_channel_policy(&poller, 2, &policy);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    g_poller_pub_count = 0;
    poller_bus_ops_t bus = {.ctx = NULL, .publish_realtime = _poller_test_pub, .push_event = NULL};
    g_failing_channel = 0; // meter_idx 0 (通道 1) 失败，meter_idx 1 (通道 2) 成功
    poller_source_ops_t source = {.ctx = NULL, .read_value = _mock_failing_source_read};
    dcu_poller_start(&poller, source, bus, (poller_sink_ops_t){0});

    // 循环两次，让通道 1 触发熔断
    dcu_poller_pump(&poller, &inv);
    dcu_poller_pump(&poller, &inv);

    edge_poll_stats_t s1, s2;
    dcu_poller_get_channel_stats(&poller, 1, &s1);
    dcu_poller_get_channel_stats(&poller, 2, &s2);

    assert_true(s1.is_circuit_broken);   // 通道 1 熔断
    assert_false(s2.is_circuit_broken);  // 通道 2 健康
    assert_int_equal(s2.success_count, 2); // 通道 2 成功采集 2 次

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 5. 验证分步调度步进器与大规模循环非阻塞 (AEP-399)
static void test_poller_step_chunking(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    // 注册 10 台电表
    for (int i = 0; i < 10; i++) {
        char addr[16];
        snprintf(addr, sizeof(addr), "METER_%04d", i);
        dcu_inventory_add(&inv, addr, "dlt698", 9600, 0, NULL);
    }

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_source_ops_t source = {.ctx = NULL, .read_value = _test_poller_source_read};
    dcu_poller_start(&poller, source, (poller_bus_ops_t){0}, (poller_sink_ops_t){0});

    // 每次步进至多 4 台表
    int p1 = dcu_poller_step(&poller, &inv, 4);
    assert_int_equal(p1, 4);
    assert_int_equal(poller.tasks[0].cursor_meter_idx, 4);

    int p2 = dcu_poller_step(&poller, &inv, 4);
    assert_int_equal(p2, 4);
    assert_int_equal(poller.tasks[0].cursor_meter_idx, 8);

    int p3 = dcu_poller_step(&poller, &inv, 4);
    assert_int_equal(p3, 2); // 剩余 2 台 (8->10)，游标循环回 0
    assert_int_equal(poller.tasks[0].cursor_meter_idx, 0);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_task_and_scheme_add),
        cmocka_unit_test(test_poller_pump_and_ti_alignment),
        cmocka_unit_test(test_poller_circuit_breaker_and_observability),
        cmocka_unit_test(test_poller_multi_channel_isolation),
        cmocka_unit_test(test_poller_step_chunking),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
