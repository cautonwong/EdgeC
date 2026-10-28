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
    g_failing_channel = -1;
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
    assert_true(stats.is_circuit_broken);                 // 本通道唯一表熔断 → 通道不可用
    assert_true(dcu_poller_meter_breaker_open(&poller, "CH3_0001")); // 熔断落在表上

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
    assert_false(dcu_poller_meter_breaker_open(&poller, "CH3_0001"));

    // 复位必须真的重新入轮询（不只是清标志）
    uint64_t poll_after_reset = stats.poll_count;
    dcu_poller_pump(&poller, &inv);
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 3, &stats), 0);
    assert_int_equal(stats.poll_count, poll_after_reset + 1);
    assert_int_equal(stats.consecutive_fails, 1);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 3b. 故障用例 4：单表静默不得拖累同通道其余表 (聚合根落在表上)
static void test_poller_single_meter_breaker_isolation(void** state) {
    (void)state;
    g_failing_channel = -1;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    // 两台表挂在同一条通道 3：表 0 永远失败，表 1 永远成功
    dcu_inventory_add(&inv, "CH3_0001", "dlt698", 9600, 3, NULL);
    dcu_inventory_add(&inv, "CH3_0002", "dlt698", 9600, 3, NULL);
    assert_int_equal(inv.count, 2);

    edge_poll_policy_t policy = {
        .interval_ms = 0, .timeout_ms = 500, .max_in_flight = 8, .circuit_break_threshold = 3};
    assert_int_equal(dcu_poller_set_channel_policy(&poller, 3, &policy), 0);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    g_failing_channel = 0; // 只有表 0 失败
    poller_source_ops_t source = {.ctx = NULL, .read_value = _mock_failing_source_read};
    dcu_poller_start(&poller, source, (poller_bus_ops_t){0}, (poller_sink_ops_t){0});

    dcu_poller_pump(&poller, &inv);
    dcu_poller_pump(&poller, &inv);
    dcu_poller_pump(&poller, &inv);

    // 表 0 连败 3 次 → 只断表 0
    assert_true(dcu_poller_meter_breaker_open(&poller, "CH3_0001"));
    assert_false(dcu_poller_meter_breaker_open(&poller, "CH3_0002"));

    edge_poll_stats_t stats;
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 3, &stats), 0);
    assert_false(stats.is_circuit_broken); // 通道未熔断
    assert_int_equal(stats.success_count, 3);

    // 第 4 轮：表 0 被跳过，表 1 照常轮询
    uint64_t poll_before = stats.poll_count;
    dcu_poller_pump(&poller, &inv);
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 3, &stats), 0);
    assert_int_equal(stats.poll_count, poll_before + 1); // 只轮询表 1
    assert_int_equal(stats.success_count, 4);

    // 复位后表 0 重新入轮询
    assert_int_equal(dcu_poller_reset_circuit_breaker(&poller, 3), 0);
    assert_false(dcu_poller_meter_breaker_open(&poller, "CH3_0001"));
    uint64_t poll_after_reset = stats.poll_count;
    dcu_poller_pump(&poller, &inv);
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 3, &stats), 0);
    assert_int_equal(stats.poll_count, poll_after_reset + 2); // 两台表都回来了

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 3c. 熔断的是最后一台表时，游标不得卡死、其余表必须继续轮询
static void test_poller_breaker_on_last_index_keeps_polling(void** state) {
    (void)state;
    g_failing_channel = -1;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "A001", "dlt698", 9600, 5, NULL);
    dcu_inventory_add(&inv, "A002", "dlt698", 9600, 5, NULL);
    dcu_inventory_add(&inv, "A003", "dlt698", 9600, 5, NULL); // 最后一台会熔断
    assert_int_equal(inv.count, 3);

    edge_poll_policy_t policy = {
        .interval_ms = 0, .timeout_ms = 500, .max_in_flight = 8, .circuit_break_threshold = 1};
    assert_int_equal(dcu_poller_set_channel_policy(&poller, 5, &policy), 0);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    g_failing_channel = 2; // 只让下标 2 失败
    poller_source_ops_t source = {.ctx = NULL, .read_value = _mock_failing_source_read};
    dcu_poller_start(&poller, source, (poller_bus_ops_t){0}, (poller_sink_ops_t){0});

    dcu_poller_pump(&poller, &inv);
    assert_true(dcu_poller_meter_breaker_open(&poller, "A003"));

    // 后续每轮必须继续采集健康表，不得卡在熔断表上
    edge_poll_stats_t stats = {0};
    dcu_poller_get_channel_stats(&poller, 5, &stats);
    uint64_t success_before = stats.success_count;
    for (int i = 0; i < 3; i++) {
        dcu_poller_pump(&poller, &inv);
    }
    dcu_poller_get_channel_stats(&poller, 5, &stats);
    assert_int_equal(stats.success_count, success_before + 6); // 2 台健康表 × 3 轮

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 4. 验证多通道隔离性 (通道 1 故障熔断不影响通道 2 正常调度)
static void test_poller_multi_channel_isolation(void** state) {
    (void)state;
    g_failing_channel = -1;
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

    assert_true(s1.is_circuit_broken);   // 通道 1 唯一表熔断 → 通道不可用
    assert_true(dcu_poller_meter_breaker_open(&poller, "CH1_0001")); // 熔断落在表上
    assert_false(s2.is_circuit_broken);  // 通道 2 健康
    assert_int_equal(s2.success_count, 2); // 通道 2 成功采集 2 次

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 5. 验证分步调度步进器与大规模循环非阻塞 (AEP-399)
// 6. 分步调度时跳过熔断表：健康表照常采集，游标越过被跳过的表
static void test_poller_chunked_step_skips_breaker(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    for (int i = 0; i < 6; i++) {
        char addr[32];
        snprintf(addr, sizeof(addr), "METER_%04d", i);
        dcu_inventory_add(&inv, addr, "dlt698", 9600, 0, NULL);
    }

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    edge_poll_policy_t policy = {0};
    policy.timeout_ms = 500;
    policy.circuit_break_threshold = 1;
    assert_int_equal(dcu_poller_set_channel_policy(&poller, 0, &policy), 0);

    g_failing_channel = 2;
    poller_source_ops_t source = {.ctx = NULL, .read_value = _mock_failing_source_read};
    dcu_poller_start(&poller, source, (poller_bus_ops_t){0}, (poller_sink_ops_t){0});

    // 前三步每步 2 台：m2 在第二步失败并熔断
    assert_int_equal(dcu_poller_step(&poller, &inv, 2), 2);
    assert_int_equal(dcu_poller_step(&poller, &inv, 2), 2);
    assert_int_equal(dcu_poller_step(&poller, &inv, 2), 2);
    assert_true(dcu_poller_meter_breaker_open(&poller, "METER_0002"));

    // 之后 m2 被跳过，健康表补足配额，游标继续前进
    edge_poll_stats_t stats;
    assert_int_equal(dcu_poller_step(&poller, &inv, 2), 2); // m0 m1
    assert_int_equal(dcu_poller_step(&poller, &inv, 2), 2); // 跳 m2 → m3 m4
    assert_int_equal(poller.tasks[0].cursor_meter_idx, 5);
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 0, &stats), 0);
    assert_int_equal(stats.success_count, 9); // 5 + 2 + 2
    assert_false(stats.is_circuit_broken);    // 单表熔断不拖累通道

    g_failing_channel = -1;
    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 7. 档案变更后退休已删表的条目，且不破坏探测链（存活表的熔断状态不丢）
static void test_poller_meter_state_gc_on_archive_change(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "GC_AAAA", "dlt698", 9600, 0, NULL);
    dcu_inventory_add(&inv, "GC_BBBB", "dlt698", 9600, 0, NULL);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    edge_poll_policy_t policy = {0};
    policy.timeout_ms = 500;
    policy.circuit_break_threshold = 1;
    assert_int_equal(dcu_poller_set_channel_policy(&poller, 0, &policy), 0);

    g_failing_channel = 999; // 全部失败，两只表都熔断
    poller_source_ops_t source = {.ctx = NULL, .read_value = _mock_failing_source_read};
    dcu_poller_start(&poller, source, (poller_bus_ops_t){0}, (poller_sink_ops_t){0});
    assert_int_equal(dcu_poller_step(&poller, &inv, 8), 2);
    assert_true(dcu_poller_meter_breaker_open(&poller, "GC_AAAA"));
    assert_true(dcu_poller_meter_breaker_open(&poller, "GC_BBBB"));

    // 删一只、增一只：count 不变，只能靠 revision 察觉
    assert_int_equal(dcu_inventory_remove(&inv, "GC_AAAA"), 0);
    assert_int_equal(dcu_inventory_add(&inv, "GC_CCCC", "dlt698", 9600, 0, NULL), 0);
    assert_int_equal(inv.count, 2);

    g_failing_channel = -1;
    assert_int_equal(dcu_poller_step(&poller, &inv, 8), 1); // GC_BBBB 已熔断被跳过，只采 GC_CCCC

    assert_false(dcu_poller_meter_breaker_open(&poller, "GC_AAAA")); // 已退休
    assert_true(dcu_poller_meter_breaker_open(&poller, "GC_BBBB"));  // 探测链未被墓碑截断
    assert_false(dcu_poller_meter_breaker_open(&poller, "GC_CCCC"));

    uint32_t tracked = 0;
    for (uint32_t i = 0; i < METER_TABLE_CAP; i++) {
        if (poller.meter_poll_state[i].tracked) tracked++;
    }
    assert_int_equal(tracked, 2); // 无重复条目

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 8. 删掉最后一只表：count==0 早返回之前必须先退休，否则残留状态永久占位
static void test_poller_meter_state_gc_on_empty_archive(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);
    dcu_inventory_add(&inv, "GC_LAST", "dlt698", 9600, 0, NULL);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    dcu_poller_add_task(&poller, 1, 0, 1);

    edge_poll_policy_t policy = {0};
    policy.timeout_ms = 500;
    policy.circuit_break_threshold = 1;
    assert_int_equal(dcu_poller_set_channel_policy(&poller, 0, &policy), 0);

    g_failing_channel = 999;
    poller_source_ops_t source = {.ctx = NULL, .read_value = _mock_failing_source_read};
    dcu_poller_start(&poller, source, (poller_bus_ops_t){0}, (poller_sink_ops_t){0});
    assert_int_equal(dcu_poller_step(&poller, &inv, 8), 1);
    assert_true(dcu_poller_meter_breaker_open(&poller, "GC_LAST"));

    assert_int_equal(dcu_inventory_remove(&inv, "GC_LAST"), 0);
    assert_int_equal(inv.count, 0);

    g_failing_channel = -1;
    (void)dcu_poller_step(&poller, &inv, 8); // count==0：本轮不采集，但必须完成退休

    assert_false(dcu_poller_meter_breaker_open(&poller, "GC_LAST"));
    uint32_t tracked = 0;
    for (uint32_t i = 0; i < METER_TABLE_CAP; i++) {
        if (poller.meter_poll_state[i].tracked) tracked++;
    }
    assert_int_equal(tracked, 0);

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 8. PENDING 语义：全部在途→中性；有成功→算成功并清连败
static int g_pending_mode = 0; // 1=全在途, 2=仅首个 CSD 返回成功
static int _mock_pending_source_read(void* ctx, uint32_t meter_idx, uint32_t tag_id,
                                     double* out_val) {
    (void)ctx;
    (void)meter_idx;
    if (g_pending_mode == 1) return POLL_READ_PENDING;
    if (g_pending_mode == 2 && tag_id != OAD_VOLTAGE) return POLL_READ_PENDING;
    *out_val = 220.0;
    return 0;
}

static void test_poller_pending_result_is_neutral(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;

    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);
    dcu_inventory_add(&inv, "PEND_0001", "dlt698", 9600, 0, NULL);

    uint32_t csds[2] = {OAD_VOLTAGE, OAD_CURRENT};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 2);
    dcu_poller_add_task(&poller, 1, 0, 1);

    poller_source_ops_t source = {.ctx = NULL, .read_value = _mock_pending_source_read};
    dcu_poller_start(&poller, source, (poller_bus_ops_t){0}, (poller_sink_ops_t){0});

    edge_poll_stats_t stats;

    // 全部在途：不记成功也不记失败
    g_pending_mode = 1;
    dcu_poller_step(&poller, &inv, 8);
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 0, &stats), 0);
    assert_int_equal(stats.success_count, 0);
    assert_int_equal(stats.error_count, 0);

    // 一个成功 + 一个在途：算成功，连败清零
    g_pending_mode = 2;
    dcu_poller_step(&poller, &inv, 8);
    assert_int_equal(dcu_poller_get_channel_stats(&poller, 0, &stats), 0);
    assert_int_equal(stats.success_count, 1);

    g_pending_mode = 0;
    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

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
    assert_int_equal(p3, 4); // 尾部 2 台 + 绕回头 2 台，单步不欠额
    assert_int_equal(poller.tasks[0].cursor_meter_idx, 2); // 已走到 (8+4)%10

    dcu_poller_destroy(&poller);
    dcu_inventory_destroy(&inv);
}

// 12. 项维度：任务只抄方案里的部分 CSD
static void test_poller_task_item_mask(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "20000001", "dlt698", 9600, 1, NULL);
    dcu_inventory_add(&inv, "20000002", "dlt698", 9600, 1, NULL);

    uint32_t csds[2] = {OAD_VOLTAGE, OAD_CURRENT};
    dcu_poller_add_scheme(&poller, 3, 0, csds, 2);
    // 只抄 csd_list[0]（电压），跳过 csd_list[1]（电流）
    assert_int_equal(dcu_poller_add_task_ex(&poller, 11, 10, 3, 0x1u), 0);

    poller_sink_ops_t sink = {0};
    poller_bus_ops_t bus = {.ctx = NULL, .publish_realtime = _poller_test_pub, .push_event = NULL};
    poller_source_ops_t source = {.ctx = NULL, .read_value = _test_poller_source_read};
    dcu_poller_start(&poller, source, bus, sink);

    g_poller_pub_count = 0;
    dcu_poller_pump(&poller, &inv);

    // 2 台表 x 1 个项 = 2 次推送（旧行为会是 2 台 x 2 项 = 4 次）
    assert_int_equal(g_poller_pub_count, 2);

    dcu_poller_destroy(&poller);
}

// 13. 表维度：显式绑定后，任务只抄绑定给它的表
static void test_poller_meter_task_binding(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "30000001", "dlt698", 9600, 1, NULL);
    dcu_inventory_add(&inv, "30000002", "dlt698", 9600, 1, NULL);
    dcu_inventory_add(&inv, "30000003", "dlt698", 9600, 1, NULL);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 2, 0, csds, 1);
    dcu_poller_add_task(&poller, 10, 10, 2);

    // 前两块表归任务 0；第三块显式声明"不参与任何任务"
    assert_int_equal(dcu_poller_set_meter_tasks(&poller, "30000001", 1, 0x1u), 0);
    assert_int_equal(dcu_poller_set_meter_tasks(&poller, "30000002", 1, 0x1u), 0);
    assert_int_equal(dcu_poller_set_meter_tasks(&poller, "30000003", 1, 0x0u), 0);

    poller_sink_ops_t sink = {0};
    poller_bus_ops_t bus = {.ctx = NULL, .publish_realtime = _poller_test_pub, .push_event = NULL};
    poller_source_ops_t source = {.ctx = NULL, .read_value = _test_poller_source_read};
    dcu_poller_start(&poller, source, bus, sink);

    g_poller_pub_count = 0;
    dcu_poller_pump(&poller, &inv);

    // 只有绑定过的两块表被抄（旧行为是 3 块全抄）
    assert_int_equal(g_poller_pub_count, 2);

    dcu_poller_destroy(&poller);
}

// 14. 跨任务去重：两个任务项集重叠时，同一步内同一数据项只抄一次
static void test_poller_cross_task_dedup(void** state) {
    (void)state;
    dcu_inventory_t inv;
    dcu_poller_t poller;
    dcu_inventory_init(&inv);
    dcu_poller_init(&poller);

    dcu_inventory_add(&inv, "40000001", "dlt698", 9600, 1, NULL);
    dcu_inventory_add(&inv, "40000002", "dlt698", 9600, 1, NULL);

    uint32_t csds[1] = {OAD_VOLTAGE};
    dcu_poller_add_scheme(&poller, 1, 0, csds, 1);
    // 两个任务，同一方案、同一数据项，且都立即到期
    dcu_poller_add_task(&poller, 1, 0, 1);
    dcu_poller_add_task(&poller, 2, 0, 1);

    poller_sink_ops_t sink = {0};
    poller_bus_ops_t bus = {.ctx = NULL, .publish_realtime = _poller_test_pub, .push_event = NULL};
    poller_source_ops_t source = {.ctx = NULL, .read_value = _test_poller_source_read};
    dcu_poller_start(&poller, source, bus, sink);

    g_poller_pub_count = 0;
    dcu_poller_pump(&poller, &inv);

    // 2 台表 x 1 项 = 2 次（没有去重会是 2 个任务 x 2 台 = 4 次）
    assert_int_equal(g_poller_pub_count, 2);

    // 下一步位图翻页，重新允许抄读（不是永久压制）
    g_poller_pub_count = 0;
    dcu_poller_pump(&poller, &inv);
    assert_int_equal(g_poller_pub_count, 2);

    dcu_poller_destroy(&poller);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_task_and_scheme_add),
        cmocka_unit_test(test_poller_pump_and_ti_alignment),
        cmocka_unit_test(test_poller_circuit_breaker_and_observability),
        cmocka_unit_test(test_poller_single_meter_breaker_isolation),
        cmocka_unit_test(test_poller_breaker_on_last_index_keeps_polling),
        cmocka_unit_test(test_poller_multi_channel_isolation),
        cmocka_unit_test(test_poller_step_chunking),
        cmocka_unit_test(test_poller_task_item_mask),
        cmocka_unit_test(test_poller_meter_task_binding),
        cmocka_unit_test(test_poller_cross_task_dedup),
        cmocka_unit_test(test_poller_chunked_step_skips_breaker),
        cmocka_unit_test(test_poller_meter_state_gc_on_archive_change),
        cmocka_unit_test(test_poller_meter_state_gc_on_empty_archive),
        cmocka_unit_test(test_poller_pending_result_is_neutral),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
