/**
 * @file test_coop_guard.c
 * @brief Unit tests for Cooperative Timeslice Overrun Watchdog & Soft Circuit Breaker (AEP-719 /
 * Issue #250)
 */

#include <cmocka.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_coop_guard.h"

typedef struct {
    uint32_t overrun_count;
    uint32_t last_overrun_task_id;
    uint64_t last_actual_us;
    uint32_t transition_count;
    edge_coop_state_t last_prev_state;
    edge_coop_state_t last_new_state;
} test_guard_event_ctx_t;

static void test_on_timeslice_overrun(void *ctx, uint32_t task_id, uint64_t actual_us,
                                      uint64_t budget_us) {
    (void)budget_us;
    test_guard_event_ctx_t *evt = (test_guard_event_ctx_t *)ctx;
    if (evt) {
        evt->overrun_count++;
        evt->last_overrun_task_id = task_id;
        evt->last_actual_us = actual_us;
    }
}

static void test_on_state_transition(void *ctx, uint32_t task_id, edge_coop_state_t prev_state,
                                     edge_coop_state_t new_state, const char *reason) {
    (void)task_id;
    (void)reason;
    test_guard_event_ctx_t *evt = (test_guard_event_ctx_t *)ctx;
    if (evt) {
        evt->transition_count++;
        evt->last_prev_state = prev_state;
        evt->last_new_state = new_state;
    }
}

static void test_coop_guard_lifecycle_and_normal_execution(void **state) {
    (void)state;
    test_guard_event_ctx_t evt_ctx = {0};
    edge_coop_guard_port_t port = {
        .on_timeslice_overrun = test_on_timeslice_overrun,
        .on_state_transition = test_on_state_transition,
        .ctx = &evt_ctx,
    };

    edge_coop_guard_storage_t store;
    edge_coop_guard_config_t cfg = {
        .default_budget_us = 5000,
        .default_degrade_threshold = 2,
        .default_trip_threshold = 4,
        .default_cooldown_us = 50000,
    };

    edge_coop_guard_t *guard = edge_coop_guard_create_in(&store, &cfg, &port);
    assert_non_null(guard);

    edge_coop_task_config_t t1 = {
        .task_id = 101,
        .name = "ai_anomaly_detector",
        .timeslice_budget_us = 10000,  // 10ms
        .max_consecutive_overruns_to_degrade = 2,
        .max_consecutive_overruns_to_trip = 4,
        .recovery_cooldown_us = 50000,
    };
    assert_int_equal(edge_coop_guard_register_task(guard, &t1), 0);

    // Normal execution within 8ms budget (< 10ms)
    assert_true(edge_coop_guard_can_execute(guard, 101, 1000000ULL));
    assert_int_equal(edge_coop_guard_enter_slice(guard, 101, 1000000ULL), 0);
    assert_int_equal(edge_coop_guard_exit_slice(guard, 101, 1008000ULL), 0);

    assert_int_equal(evt_ctx.overrun_count, 0);
    assert_int_equal(evt_ctx.transition_count, 0);

    edge_coop_task_status_t status;
    assert_int_equal(edge_coop_guard_get_task_status(guard, 101, &status), 0);
    assert_int_equal(status.state, EDGE_COOP_STATE_HEALTHY);
    assert_int_equal(status.last_execution_us, 8000);
    assert_int_equal(status.consecutive_overruns, 0);

    assert_int_equal(edge_coop_guard_destroy(guard), 0);
}

static void test_coop_guard_consecutive_overruns_and_trip(void **state) {
    (void)state;
    test_guard_event_ctx_t evt_ctx = {0};
    edge_coop_guard_port_t port = {
        .on_timeslice_overrun = test_on_timeslice_overrun,
        .on_state_transition = test_on_state_transition,
        .ctx = &evt_ctx,
    };

    edge_coop_guard_storage_t store;
    edge_coop_guard_config_t cfg = {0};

    edge_coop_guard_t *guard = edge_coop_guard_create_in(&store, &cfg, &port);
    assert_non_null(guard);

    edge_coop_task_config_t t1 = {
        .task_id = 202,
        .name = "untrusted_mpc_solver",
        .timeslice_budget_us = 5000,
        .max_consecutive_overruns_to_degrade = 2,
        .max_consecutive_overruns_to_trip = 3,
        .recovery_cooldown_us = 100000,  // 100ms
    };
    assert_int_equal(edge_coop_guard_register_task(guard, &t1), 0);

    // Overrun 1: 8ms (> 5ms) -> Still Healthy
    assert_int_equal(edge_coop_guard_enter_slice(guard, 202, 1000000ULL), 0);
    assert_int_equal(edge_coop_guard_exit_slice(guard, 202, 1008000ULL), 0);
    assert_int_equal(evt_ctx.overrun_count, 1);
    assert_int_equal(evt_ctx.transition_count, 0);

    // Overrun 2: 9ms (> 5ms) -> Transitions to DEGRADED
    assert_int_equal(edge_coop_guard_enter_slice(guard, 202, 1020000ULL), 0);
    assert_int_equal(edge_coop_guard_exit_slice(guard, 202, 1029000ULL), 0);
    assert_int_equal(evt_ctx.overrun_count, 2);
    assert_int_equal(evt_ctx.last_new_state, EDGE_COOP_STATE_DEGRADED);

    // Overrun 3: 10ms (> 5ms) -> Transitions to TRIPPED
    assert_int_equal(edge_coop_guard_enter_slice(guard, 202, 1040000ULL), 0);
    assert_int_equal(edge_coop_guard_exit_slice(guard, 202, 1050000ULL), 0);
    assert_int_equal(evt_ctx.overrun_count, 3);
    assert_int_equal(evt_ctx.last_new_state, EDGE_COOP_STATE_TRIPPED);

    // Immediately check execution permission at t=1060000 (only 10ms after trip < 100ms cooldown)
    assert_false(edge_coop_guard_can_execute(guard, 202, 1060000ULL));

    assert_int_equal(edge_coop_guard_destroy(guard), 0);
}

static void test_coop_guard_cooldown_and_half_open_recovery(void **state) {
    (void)state;
    test_guard_event_ctx_t evt_ctx = {0};
    edge_coop_guard_port_t port = {
        .on_timeslice_overrun = test_on_timeslice_overrun,
        .on_state_transition = test_on_state_transition,
        .ctx = &evt_ctx,
    };

    edge_coop_guard_storage_t store;
    edge_coop_guard_config_t cfg = {0};

    edge_coop_guard_t *guard = edge_coop_guard_create_in(&store, &cfg, &port);
    assert_non_null(guard);

    edge_coop_task_config_t t1 = {
        .task_id = 303,
        .name = "third_party_analytics",
        .timeslice_budget_us = 5000,
        .max_consecutive_overruns_to_degrade = 1,
        .max_consecutive_overruns_to_trip = 2,
        .recovery_cooldown_us = 50000,  // 50ms cooldown
    };
    assert_int_equal(edge_coop_guard_register_task(guard, &t1), 0);

    // Force trip: Overrun 1 & 2 at t=1000000, 1010000
    edge_coop_guard_enter_slice(guard, 303, 1000000ULL);
    edge_coop_guard_exit_slice(guard, 303, 1008000ULL);
    edge_coop_guard_enter_slice(guard, 303, 1010000ULL);
    edge_coop_guard_exit_slice(guard, 303, 1018000ULL);

    // At t=1020000: Blocked (within 50ms cooldown)
    assert_false(edge_coop_guard_can_execute(guard, 303, 1020000ULL));

    // At t=1075000: Cooldown satisfied (57ms > 50ms) -> Transition to HALF_OPEN probe run
    assert_true(edge_coop_guard_can_execute(guard, 303, 1075000ULL));
    assert_int_equal(evt_ctx.last_new_state, EDGE_COOP_STATE_HALF_OPEN);

    // Successful probe run within 3ms budget
    assert_int_equal(edge_coop_guard_enter_slice(guard, 303, 1075000ULL), 0);
    assert_int_equal(edge_coop_guard_exit_slice(guard, 303, 1078000ULL), 0);

    // Successfully recovered to HEALTHY!
    assert_int_equal(evt_ctx.last_new_state, EDGE_COOP_STATE_HEALTHY);

    edge_coop_task_status_t status;
    assert_int_equal(edge_coop_guard_get_task_status(guard, 303, &status), 0);
    assert_int_equal(status.state, EDGE_COOP_STATE_HEALTHY);

    assert_int_equal(edge_coop_guard_destroy(guard), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_coop_guard_lifecycle_and_normal_execution),
        cmocka_unit_test(test_coop_guard_consecutive_overruns_and_trip),
        cmocka_unit_test(test_coop_guard_cooldown_and_half_open_recovery),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
