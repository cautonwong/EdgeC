#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "edge_periodic_clock.h"

typedef struct {
    uint64_t current_time_us;
} mock_time_ctx_t;

static uint64_t mock_clock_fn(void *ctx) {
    mock_time_ctx_t *m = (mock_time_ctx_t *)ctx;
    return m ? m->current_time_us : 0;
}

// 1. [DoD] 1000 周期抗漂移严密验证：断言累计漂移 < 1 个 tick (可证伪)
static void test_clock_zero_drift_1000_cycles(void **state) {
    (void)state;

    mock_time_ctx_t time_ctx = {.current_time_us = 10000000ULL}; // 10 秒基准
    uint64_t interval = 1000ULL; // 1ms (1000us) 周期

    edge_periodic_clock_config_t cfg = {
        .interval_us = interval,
        .policy = EDGE_OVERRUN_SKIP,
        .wall_clock_align = true,
        .custom_clock = mock_clock_fn,
        .clock_ctx = &time_ctx,
    };

    edge_periodic_clock_t clk;
    int ret = edge_periodic_clock_init(&clk, &cfg);
    assert_int_equal(ret, 0);

    uint64_t start_lattice = clk.next_tick_us;
    assert_int_equal(start_lattice, 10000000ULL + interval);

    // 运行 1000 个周期，模拟在每个周期内存在各种非确定性执行耗时 (jitter: -100us ~ +200us)
    for (int i = 0; i < 1000; i++) {
        // 时间行进到当前预定触发时刻 + 动态抖动 (模拟执行耗时)
        uint64_t jitter = (uint64_t)((i % 7) * 30); // 0 ~ 180us 抖动
        time_ctx.current_time_us = clk.next_tick_us + jitter;

        uint64_t ticks = 0;
        int fired = edge_periodic_clock_check(&clk, &ticks);
        assert_int_equal(fired, 1);
        assert_int_equal(ticks, 1);
    }

    assert_int_equal(clk.ticks_count, 1000);
    assert_int_equal(clk.missed_ticks, 0);

    // 严密断言：经过 1000 个周期的随机执行抖动后，下一次预定时刻必须严格锁定在理想格点上！
    uint64_t expected_lattice = start_lattice + 1000 * interval;
    int64_t drift = (int64_t)clk.next_tick_us - (int64_t)expected_lattice;
    
    // 累计漂移严格等于 0 微秒，且必然 < 1 个 tick (1000us)
    assert_true(drift < (int64_t)interval);
    assert_int_equal(drift, 0);
}

// 2. [DoD] 漏拍策略可声明验证：EDGE_OVERRUN_SKIP
static void test_clock_overrun_policy_skip(void **state) {
    (void)state;

    mock_time_ctx_t time_ctx = {.current_time_us = 5000000ULL};
    uint64_t interval = 1000ULL;

    edge_periodic_clock_config_t cfg = {
        .interval_us = interval,
        .policy = EDGE_OVERRUN_SKIP,
        .wall_clock_align = false,
        .custom_clock = mock_clock_fn,
        .clock_ctx = &time_ctx,
    };

    edge_periodic_clock_t clk;
    edge_periodic_clock_init(&clk, &cfg);
    uint64_t first_tick = clk.next_tick_us; // 5001000

    // 模拟现场严重卡顿/阻塞：直接跳过 6.5 个周期 (6500us)
    time_ctx.current_time_us += 6500ULL;

    uint64_t ticks = 0;
    int fired = edge_periodic_clock_check(&clk, &ticks);
    assert_int_equal(fired, 1);
    assert_int_equal(ticks, 1); // SKIP 策略仅触发当前拍

    // 漏拍计数为 5
    assert_int_equal(clk.missed_ticks, 5);
    assert_int_equal(clk.ticks_count, 1);

    // 下一拍必须对齐到未来的第 7 个周期格点
    uint64_t expected_next = first_tick + 6 * interval;
    assert_int_equal(clk.next_tick_us, expected_next);
}

// 3. [DoD] 漏拍策略可声明验证：EDGE_OVERRUN_CATCHUP
static void test_clock_overrun_policy_catchup(void **state) {
    (void)state;

    mock_time_ctx_t time_ctx = {.current_time_us = 8000000ULL};
    uint64_t interval = 2000ULL;

    edge_periodic_clock_config_t cfg = {
        .interval_us = interval,
        .policy = EDGE_OVERRUN_CATCHUP,
        .wall_clock_align = false,
        .custom_clock = mock_clock_fn,
        .clock_ctx = &time_ctx,
    };

    edge_periodic_clock_t clk;
    edge_periodic_clock_init(&clk, &cfg);

    // 模拟离线断网或总线拥塞后恢复：落后 5 个完整周期
    time_ctx.current_time_us += 5 * interval + 500ULL;

    uint64_t ticks = 0;
    int fired = edge_periodic_clock_check(&clk, &ticks);
    // CATCHUP 策略必须完整补齐所有错过的周期！
    assert_int_equal(fired, 5);
    assert_int_equal(ticks, 5);
    assert_int_equal(clk.missed_ticks, 4);
    assert_int_equal(clk.ticks_count, 5);
}

// 4. 自然格点/挂钟对齐验证
static void test_clock_wall_clock_alignment(void **state) {
    (void)state;

    mock_time_ctx_t time_ctx = {.current_time_us = 1234567ULL};
    uint64_t interval = 10000ULL; // 10ms

    edge_periodic_clock_config_t cfg = {
        .interval_us = interval,
        .policy = EDGE_OVERRUN_COALESCE,
        .wall_clock_align = true,
        .custom_clock = mock_clock_fn,
        .clock_ctx = &time_ctx,
    };

    edge_periodic_clock_t clk;
    edge_periodic_clock_init(&clk, &cfg);

    // 1234567 / 10000 = 123; (123 + 1) * 10000 = 1240000
    assert_int_equal(clk.next_tick_us, 1240000ULL);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_clock_zero_drift_1000_cycles),
        cmocka_unit_test(test_clock_overrun_policy_skip),
        cmocka_unit_test(test_clock_overrun_policy_catchup),
        cmocka_unit_test(test_clock_wall_clock_alignment),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
