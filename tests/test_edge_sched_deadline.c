/**
 * @file test_edge_sched_deadline.c
 * @brief 确定性调度死线超限监控器与熔断器单元测试 (AEP-720 / Issue #251)
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmocka.h>

#include "edge_sched_deadline.h"

/* -------------------------------------------------------------------------- */
/* Mock Clock & Ports Context                                                 */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint64_t current_time_us;
    uint32_t last_violation_task_id;
    edge_sched_action_t last_violation_action;
    uint32_t violation_count;
} mock_sched_env_t;

static uint64_t mock_get_now_us(void *ctx) {
    mock_sched_env_t *env = (mock_sched_env_t *)ctx;
    return env ? env->current_time_us : 0;
}

static void mock_on_violation(void *ctx, uint32_t task_id, const edge_sched_task_stats_t *stats, edge_sched_action_t action) {
    (void)stats;
    mock_sched_env_t *env = (mock_sched_env_t *)ctx;
    if (env) {
        env->last_violation_task_id = task_id;
        env->last_violation_action = action;
        env->violation_count++;
    }
}

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_edge_sched_create_in_and_init(void **state) {
    (void)state;
    edge_sched_deadline_storage_t storage;
    memset(&storage, 0xAA, sizeof(storage));

    mock_sched_env_t env = {.current_time_us = 1000000ULL};
    edge_sched_deadline_ports_t ports = {
        .get_now_us = mock_get_now_us,
        .on_violation = mock_on_violation,
        .ctx = &env
    };

    edge_sched_deadline_monitor_t *mon = edge_sched_deadline_create_in(&storage, &ports);
    assert_non_null(mon);
    assert_ptr_equal(mon, (void *)storage.raw);
    assert_int_equal(edge_sched_deadline_get_task_count(mon), 0);

    edge_sched_task_config_t cfg = {
        .task_id = 101,
        .name = "vsg_control_loop",
        .period_us = 10000,
        .deadline_us = 2000,
        .max_allowed_jitter_us = 500,
        .max_consecutive_overruns = 3,
        .fallback_action = EDGE_SCHED_ACTION_SAFE_FALLBACK
    };

    int rc = edge_sched_deadline_register_task(mon, &cfg);
    assert_int_equal(rc, 0);
    assert_int_equal(edge_sched_deadline_get_task_count(mon), 1);

    /* Duplicate registration should fail */
    rc = edge_sched_deadline_register_task(mon, &cfg);
    assert_int_equal(rc, -2);
}

static void test_edge_sched_normal_execution_metrics(void **state) {
    (void)state;
    edge_sched_deadline_storage_t storage;
    mock_sched_env_t env = {.current_time_us = 1000000ULL};
    edge_sched_deadline_ports_t ports = {
        .get_now_us = mock_get_now_us,
        .on_violation = mock_on_violation,
        .ctx = &env
    };

    edge_sched_deadline_monitor_t *mon = edge_sched_deadline_create_in(&storage, &ports);
    assert_non_null(mon);

    edge_sched_task_config_t cfg = {
        .task_id = 1,
        .name = "ekf_estimator",
        .period_us = 10000,
        .deadline_us = 2000,
        .max_allowed_jitter_us = 500,
        .max_consecutive_overruns = 3,
        .fallback_action = EDGE_SCHED_ACTION_SAFE_FALLBACK
    };
    assert_int_equal(edge_sched_deadline_register_task(mon, &cfg), 0);

    /* Run 5 normal cycles: 1000us execution time each (well below 2000us deadline) */
    for (uint64_t i = 0; i < 5; ++i) {
        env.current_time_us = 1000000ULL + i * 10000ULL;
        edge_sched_token_t token;
        int begin_rc = edge_sched_deadline_task_begin(mon, 1, &token);
        assert_int_equal(begin_rc, 0);
        assert_true(token.active);

        /* Simulate 1000 us work */
        env.current_time_us += 1000ULL;
        edge_sched_action_t action = EDGE_SCHED_ACTION_WARN;
        int end_rc = edge_sched_deadline_task_end(mon, &token, &action);
        assert_int_equal(end_rc, 0);
        assert_int_equal(action, EDGE_SCHED_ACTION_LOG_ONLY);
        assert_false(token.active);
    }

    edge_sched_task_stats_t stats;
    assert_int_equal(edge_sched_deadline_get_task_stats(mon, 1, &stats), 0);
    assert_int_equal(stats.total_runs, 5);
    assert_int_equal(stats.deadline_misses, 0);
    assert_int_equal(stats.consecutive_misses, 0);
    assert_int_equal(stats.state, EDGE_SCHED_STATE_NORMAL);
    assert_int_equal(stats.min_duration_us, 1000);
    assert_int_equal(stats.max_duration_us, 1000);
    assert_int_equal((int)stats.avg_duration_us, 1000);
    assert_int_equal(env.violation_count, 0);
}

static void test_edge_sched_deadline_overrun_and_recovery(void **state) {
    (void)state;
    edge_sched_deadline_storage_t storage;
    mock_sched_env_t env = {.current_time_us = 1000000ULL};
    edge_sched_deadline_ports_t ports = {
        .get_now_us = mock_get_now_us,
        .on_violation = mock_on_violation,
        .ctx = &env
    };

    edge_sched_deadline_monitor_t *mon = edge_sched_deadline_create_in(&storage, &ports);

    edge_sched_task_config_t cfg = {
        .task_id = 2,
        .name = "mpc_solver",
        .period_us = 20000,
        .deadline_us = 5000,
        .max_allowed_jitter_us = 1000,
        .max_consecutive_overruns = 3,
        .fallback_action = EDGE_SCHED_ACTION_SAFE_FALLBACK
    };
    assert_int_equal(edge_sched_deadline_register_task(mon, &cfg), 0);

    /* 1. Run with 6000 us execution (Exceeds 5000 us deadline) */
    edge_sched_token_t token;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 2, &token), 0);
    env.current_time_us += 6000ULL;
    edge_sched_action_t action;
    int end_rc = edge_sched_deadline_task_end(mon, &token, &action);
    assert_int_equal(end_rc, 1); /* 1: Single miss */
    assert_int_equal(action, EDGE_SCHED_ACTION_WARN);
    assert_int_equal(env.violation_count, 1);
    assert_int_equal(env.last_violation_task_id, 2);

    edge_sched_task_stats_t stats;
    assert_int_equal(edge_sched_deadline_get_task_stats(mon, 2, &stats), 0);
    assert_int_equal(stats.state, EDGE_SCHED_STATE_WARNING);
    assert_int_equal(stats.deadline_misses, 1);
    assert_int_equal(stats.consecutive_misses, 1);

    /* 2. Next run completes normally in 3000 us -> Recovery to NORMAL */
    env.current_time_us += 14000ULL;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 2, &token), 0);
    env.current_time_us += 3000ULL;
    end_rc = edge_sched_deadline_task_end(mon, &token, &action);
    assert_int_equal(end_rc, 0);
    assert_int_equal(action, EDGE_SCHED_ACTION_LOG_ONLY);

    assert_int_equal(edge_sched_deadline_get_task_stats(mon, 2, &stats), 0);
    assert_int_equal(stats.state, EDGE_SCHED_STATE_NORMAL);
    assert_int_equal(stats.consecutive_misses, 0);
    assert_int_equal(stats.deadline_misses, 1);
}

static void test_edge_sched_circuit_breaker_trip_and_reset(void **state) {
    (void)state;
    edge_sched_deadline_storage_t storage;
    mock_sched_env_t env = {.current_time_us = 1000000ULL};
    edge_sched_deadline_ports_t ports = {
        .get_now_us = mock_get_now_us,
        .on_violation = mock_on_violation,
        .ctx = &env
    };

    edge_sched_deadline_monitor_t *mon = edge_sched_deadline_create_in(&storage, &ports);

    edge_sched_task_config_t cfg = {
        .task_id = 3,
        .name = "vsg_inertia_loop",
        .period_us = 10000,
        .deadline_us = 2000,
        .max_allowed_jitter_us = 500,
        .max_consecutive_overruns = 3,
        .fallback_action = EDGE_SCHED_ACTION_SAFE_FALLBACK
    };
    assert_int_equal(edge_sched_deadline_register_task(mon, &cfg), 0);

    /* Miss 1 */
    edge_sched_token_t token;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 3, &token), 0);
    env.current_time_us += 3000ULL;
    edge_sched_action_t act;
    assert_int_equal(edge_sched_deadline_task_end(mon, &token, &act), 1);
    assert_int_equal(act, EDGE_SCHED_ACTION_WARN);

    /* Miss 2 */
    env.current_time_us += 7000ULL;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 3, &token), 0);
    env.current_time_us += 3000ULL;
    assert_int_equal(edge_sched_deadline_task_end(mon, &token, &act), 1);
    assert_int_equal(act, EDGE_SCHED_ACTION_WARN);

    /* Miss 3 -> Tripped! */
    env.current_time_us += 7000ULL;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 3, &token), 0);
    env.current_time_us += 3000ULL;
    assert_int_equal(edge_sched_deadline_task_end(mon, &token, &act), 2);
    assert_int_equal(act, EDGE_SCHED_ACTION_SAFE_FALLBACK);
    assert_int_equal(env.last_violation_action, EDGE_SCHED_ACTION_SAFE_FALLBACK);

    edge_sched_task_stats_t stats;
    assert_int_equal(edge_sched_deadline_get_task_stats(mon, 3, &stats), 0);
    assert_int_equal(stats.state, EDGE_SCHED_STATE_TRIPPED);
    assert_int_equal(stats.consecutive_misses, 3);
    assert_int_equal(stats.deadline_misses, 3);

    /* Subsequent task_begin indicates TRIPPED (rc = 1) */
    env.current_time_us += 7000ULL;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 3, &token), 1);
    assert_int_equal(edge_sched_deadline_task_end(mon, &token, &act), 0);

    /* Reset Circuit Breaker */
    assert_int_equal(edge_sched_deadline_reset_circuit(mon, 3), 0);
    assert_int_equal(edge_sched_deadline_get_task_stats(mon, 3, &stats), 0);
    assert_int_equal(stats.state, EDGE_SCHED_STATE_NORMAL);
    assert_int_equal(stats.consecutive_misses, 0);

    /* After reset, task_begin returns 0 (NORMAL) */
    assert_int_equal(edge_sched_deadline_task_begin(mon, 3, &token), 0);
    assert_int_equal(edge_sched_deadline_task_end(mon, &token, &act), 0);
}

static void test_edge_sched_jitter_detection(void **state) {
    (void)state;
    edge_sched_deadline_storage_t storage;
    mock_sched_env_t env = {.current_time_us = 1000000ULL};
    edge_sched_deadline_ports_t ports = {
        .get_now_us = mock_get_now_us,
        .on_violation = mock_on_violation,
        .ctx = &env
    };

    edge_sched_deadline_monitor_t *mon = edge_sched_deadline_create_in(&storage, &ports);

    edge_sched_task_config_t cfg = {
        .task_id = 4,
        .name = "grid_phase_tracker",
        .period_us = 10000,
        .deadline_us = 2000,
        .max_allowed_jitter_us = 300,
        .max_consecutive_overruns = 3,
        .fallback_action = EDGE_SCHED_ACTION_SAFE_FALLBACK
    };
    assert_int_equal(edge_sched_deadline_register_task(mon, &cfg), 0);

    /* Run 1: at 1000000 us */
    edge_sched_token_t token;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 4, &token), 0);
    env.current_time_us += 500ULL;
    assert_int_equal(edge_sched_deadline_task_end(mon, &token, NULL), 0);

    /* Run 2: expected at 1010000 us, but started at 1010800 us (Jitter = 800 us > 300 us) */
    env.current_time_us = 1010800ULL;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 4, &token), 0);
    env.current_time_us += 500ULL;
    assert_int_equal(edge_sched_deadline_task_end(mon, &token, NULL), 0);

    edge_sched_task_stats_t stats;
    assert_int_equal(edge_sched_deadline_get_task_stats(mon, 4, &stats), 0);
    assert_int_equal(stats.last_jitter_us, 800);
    assert_int_equal(stats.max_jitter_us, 800);
    assert_int_equal(stats.state, EDGE_SCHED_STATE_WARNING);
}

static void test_edge_sched_multi_task_isolation(void **state) {
    (void)state;
    edge_sched_deadline_storage_t storage;
    mock_sched_env_t env = {.current_time_us = 1000000ULL};
    edge_sched_deadline_ports_t ports = {
        .get_now_us = mock_get_now_us,
        .on_violation = mock_on_violation,
        .ctx = &env
    };

    edge_sched_deadline_monitor_t *mon = edge_sched_deadline_create_in(&storage, &ports);

    edge_sched_task_config_t cfg1 = {
        .task_id = 11,
        .name = "fast_pwm",
        .period_us = 1000,
        .deadline_us = 200,
        .max_allowed_jitter_us = 50,
        .max_consecutive_overruns = 2,
        .fallback_action = EDGE_SCHED_ACTION_CIRCUIT_TRIP
    };
    edge_sched_task_config_t cfg2 = {
        .task_id = 22,
        .name = "slow_soh",
        .period_us = 100000,
        .deadline_us = 20000,
        .max_allowed_jitter_us = 5000,
        .max_consecutive_overruns = 3,
        .fallback_action = EDGE_SCHED_ACTION_SAFE_FALLBACK
    };
    assert_int_equal(edge_sched_deadline_register_task(mon, &cfg1), 0);
    assert_int_equal(edge_sched_deadline_register_task(mon, &cfg2), 0);
    assert_int_equal(edge_sched_deadline_get_task_count(mon), 2);

    /* Trip Task 11 */
    edge_sched_token_t t1;
    for (int i = 0; i < 2; ++i) {
        assert_int_equal(edge_sched_deadline_task_begin(mon, 11, &t1), 0);
        env.current_time_us += 500ULL; /* Exceeds 200 us */
        edge_sched_deadline_task_end(mon, &t1, NULL);
    }

    /* Execute Task 22 normally */
    edge_sched_token_t t2;
    assert_int_equal(edge_sched_deadline_task_begin(mon, 22, &t2), 0);
    env.current_time_us += 10000ULL; /* Well below 20000 us */
    assert_int_equal(edge_sched_deadline_task_end(mon, &t2, NULL), 0);

    edge_sched_task_stats_t s1, s2;
    assert_int_equal(edge_sched_deadline_get_task_stats(mon, 11, &s1), 0);
    assert_int_equal(edge_sched_deadline_get_task_stats(mon, 22, &s2), 0);

    assert_int_equal(s1.state, EDGE_SCHED_STATE_TRIPPED);
    assert_int_equal(s2.state, EDGE_SCHED_STATE_NORMAL);
}

/* -------------------------------------------------------------------------- */
/* Main Entry                                                                 */
/* -------------------------------------------------------------------------- */
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_edge_sched_create_in_and_init),
        cmocka_unit_test(test_edge_sched_normal_execution_metrics),
        cmocka_unit_test(test_edge_sched_deadline_overrun_and_recovery),
        cmocka_unit_test(test_edge_sched_circuit_breaker_trip_and_reset),
        cmocka_unit_test(test_edge_sched_jitter_detection),
        cmocka_unit_test(test_edge_sched_multi_task_isolation),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
