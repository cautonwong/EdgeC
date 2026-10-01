/**
 * @file test_rms_scheduler.c
 * @brief Unit tests for Rate Monotonic Scheduling (RMS) Static Deterministic Scheduler & Budget
 * Watchdog (AEP-713 / Issue #246)
 */

#include <cmocka.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_rms_scheduler.h"

typedef struct {
    uint32_t preemption_count;
    uint32_t last_preempted_id;
    uint32_t last_preempting_id;
    uint32_t budget_overrun_count;
    uint32_t last_overrun_id;
    edge_rms_action_t last_action;
    uint32_t miss_count;
    uint32_t last_miss_id;
    uint32_t warning_count;
    double last_warn_util;
} test_rms_event_ctx_t;

static void test_on_preempt(void *ctx, uint32_t preempted_task_id, uint32_t preempting_task_id,
                            uint64_t time_us) {
    (void)time_us;
    test_rms_event_ctx_t *evt = (test_rms_event_ctx_t *)ctx;
    if (evt) {
        evt->preemption_count++;
        evt->last_preempted_id = preempted_task_id;
        evt->last_preempting_id = preempting_task_id;
    }
}

static void test_on_budget_exceeded(void *ctx, uint32_t task_id, uint64_t exec_duration_us,
                                    uint64_t budget_us, edge_rms_action_t action_taken) {
    (void)exec_duration_us;
    (void)budget_us;
    test_rms_event_ctx_t *evt = (test_rms_event_ctx_t *)ctx;
    if (evt) {
        evt->budget_overrun_count++;
        evt->last_overrun_id = task_id;
        evt->last_action = action_taken;
    }
}

static void test_on_miss(void *ctx, uint32_t task_id, uint64_t time_us, uint64_t overdue_us) {
    (void)time_us;
    (void)overdue_us;
    test_rms_event_ctx_t *evt = (test_rms_event_ctx_t *)ctx;
    if (evt) {
        evt->miss_count++;
        evt->last_miss_id = task_id;
    }
}

static void test_on_warning(void *ctx, double total_utilization, double bound_utilization,
                            const char *reason) {
    (void)bound_utilization;
    (void)reason;
    test_rms_event_ctx_t *evt = (test_rms_event_ctx_t *)ctx;
    if (evt) {
        evt->warning_count++;
        evt->last_warn_util = total_utilization;
    }
}

static void test_rms_scheduler_lifecycle_and_priority_assignment(void **state) {
    (void)state;
    edge_rms_scheduler_storage_t store;
    edge_rms_config_t cfg = {
        .tick_resolution_us = 1000,
        .auto_assign_priorities = true,
        .enable_budget_watchdog = true,
    };

    edge_rms_scheduler_t *sched = edge_rms_scheduler_create_in(&store, &cfg, NULL);
    assert_non_null(sched);

    // Register 3 tasks:
    // Task A: T=50ms, C=10ms
    // Task B: T=10ms, C=2ms (Should get Priority 1 - highest!)
    // Task C: T=20ms, C=5ms (Should get Priority 2)
    edge_rms_task_config_t t_a = {
        .task_id = 10, .name = "slow_task", .period_us = 50000, .wcet_budget_us = 10000};
    edge_rms_task_config_t t_b = {
        .task_id = 20, .name = "fast_task", .period_us = 10000, .wcet_budget_us = 2000};
    edge_rms_task_config_t t_c = {
        .task_id = 30, .name = "med_task", .period_us = 20000, .wcet_budget_us = 5000};

    assert_int_equal(edge_rms_scheduler_register_task(sched, &t_a), 0);
    assert_int_equal(edge_rms_scheduler_register_task(sched, &t_b), 0);
    assert_int_equal(edge_rms_scheduler_register_task(sched, &t_c), 0);

    // Verify Rate Monotonic Priority ordering: fast_task (T=10ms) has highest priority (1)
    edge_rms_schedulability_report_t report;
    assert_int_equal(edge_rms_scheduler_check_schedulability(sched, &report), 0);
    assert_int_equal(report.task_count, 3);

    for (uint32_t i = 0; i < report.task_count; i++) {
        if (report.task_rta[i].task_id == 20) {
            assert_int_equal(report.task_rta[i].static_priority, 1);
        } else if (report.task_rta[i].task_id == 30) {
            assert_int_equal(report.task_rta[i].static_priority, 2);
        } else if (report.task_rta[i].task_id == 10) {
            assert_int_equal(report.task_rta[i].static_priority, 3);
        }
    }

    double u_rms = edge_rms_scheduler_get_utilization(sched);
    // U = 10/50 + 2/10 + 5/20 = 0.20 + 0.20 + 0.25 = 0.65
    assert_true(u_rms > 0.649 && u_rms < 0.651);

    // Bound for 3 tasks: 3(2^(1/3) - 1) = 0.77976
    double bound = edge_rms_scheduler_get_bound(sched);
    assert_true(bound > 0.779 && bound < 0.781);
    assert_true(report.passed_sufficient_bound);
    assert_true(report.is_schedulable);
}

static void test_rms_exact_response_time_analysis(void **state) {
    (void)state;
    test_rms_event_ctx_t evt_ctx = {0};
    edge_rms_port_t port = {
        .on_schedulability_warning = test_on_warning,
        .ctx = &evt_ctx,
    };

    edge_rms_scheduler_storage_t store;
    edge_rms_config_t cfg = {
        .tick_resolution_us = 1000,
        .auto_assign_priorities = true,
        .enable_budget_watchdog = true,
    };

    edge_rms_scheduler_t *sched = edge_rms_scheduler_create_in(&store, &cfg, &port);
    assert_non_null(sched);

    // Task set exceeding Liu & Layland sufficient bound (U_bound = 0.7798) but schedulable via
    // exact RTA: T1: T=100ms, C=40ms (U=0.40) T2: T=150ms, C=40ms (U=0.267) T3: T=350ms, C=80ms
    // (U=0.228) Total U = 0.895 > 0.7798 Exact RTA: R1 = 40ms <= 100ms R2 = 40 + ceil(40/100)*40 =
    // 80ms <= 150ms R3 = 80 + ceil(80/100)*40 + ceil(80/150)*40 = 80 + 40 + 40 = 160ms ->
    // ceil(160/100)*40 + ceil(160/150)*40 + 80 = 80 + 80 + 80 = 240ms <= 350ms!
    edge_rms_task_config_t t1 = {
        .task_id = 1, .name = "t1", .period_us = 100000, .wcet_budget_us = 40000};
    edge_rms_task_config_t t2 = {
        .task_id = 2, .name = "t2", .period_us = 150000, .wcet_budget_us = 40000};
    edge_rms_task_config_t t3 = {
        .task_id = 3, .name = "t3", .period_us = 350000, .wcet_budget_us = 80000};

    assert_int_equal(edge_rms_scheduler_register_task(sched, &t1), 0);
    assert_int_equal(edge_rms_scheduler_register_task(sched, &t2), 0);
    assert_int_equal(edge_rms_scheduler_register_task(sched, &t3), 0);

    assert_true(evt_ctx.warning_count > 0);  // Warning because U > U_bound

    edge_rms_schedulability_report_t report;
    assert_int_equal(edge_rms_scheduler_check_schedulability(sched, &report), 0);

    assert_false(report.passed_sufficient_bound);  // Failed sufficient test
    assert_true(report.passed_exact_rta);          // Passed exact RTA!
    assert_true(report.is_schedulable);

    for (uint32_t i = 0; i < report.task_count; i++) {
        assert_true(report.task_rta[i].is_task_schedulable);
        if (report.task_rta[i].task_id == 1) {
            assert_int_equal(report.task_rta[i].worst_case_response_time_us, 40000);
        } else if (report.task_rta[i].task_id == 2) {
            assert_int_equal(report.task_rta[i].worst_case_response_time_us, 80000);
        } else if (report.task_rta[i].task_id == 3) {
            assert_int_equal(report.task_rta[i].worst_case_response_time_us, 280000);
        }
    }
}

static void test_rms_preemptive_dispatch_and_watchdog(void **state) {
    (void)state;
    test_rms_event_ctx_t evt_ctx = {0};
    edge_rms_port_t port = {
        .on_task_preempt = test_on_preempt,
        .on_budget_exceeded = test_on_budget_exceeded,
        .on_deadline_miss = test_on_miss,
        .ctx = &evt_ctx,
    };

    edge_rms_scheduler_storage_t store;
    edge_rms_config_t cfg = {
        .tick_resolution_us = 1000,
        .auto_assign_priorities = true,
        .enable_budget_watchdog = true,
    };

    edge_rms_scheduler_t *sched = edge_rms_scheduler_create_in(&store, &cfg, &port);
    assert_non_null(sched);

    // Task 1 (Low Priority): T=50ms, C=10ms, Throttle on budget
    edge_rms_task_config_t t_low = {
        .task_id = 1,
        .name = "task_low_prio",
        .period_us = 50000,
        .wcet_budget_us = 10000,
        .budget_action = EDGE_RMS_ACTION_THROTTLE,
    };
    // Task 2 (High Priority): T=10ms, C=2ms, Phase 2ms
    edge_rms_task_config_t t_high = {
        .task_id = 2,
        .name = "task_high_prio",
        .period_us = 10000,
        .wcet_budget_us = 2000,
        .initial_phase_us = 2000,
        .budget_action = EDGE_RMS_ACTION_WARN_ONLY,
    };

    assert_int_equal(edge_rms_scheduler_register_task(sched, &t_low), 0);
    assert_int_equal(edge_rms_scheduler_register_task(sched, &t_high), 0);

    edge_rms_dispatch_result_t res;

    // Step 1: t=0ms -> Task 1 runs
    assert_int_equal(edge_rms_scheduler_step(sched, 0, 1000, &res), 0);
    assert_int_equal(res.active_task_id, 1);
    assert_false(res.is_preemption);

    // Step 2: t=1ms -> Task 1 continues
    assert_int_equal(edge_rms_scheduler_step(sched, 1000, 1000, &res), 0);
    assert_int_equal(res.active_task_id, 1);

    // Step 3: t=2ms -> Task 2 arrives (T=10ms, static priority 1). Preempts Task 1!
    assert_int_equal(edge_rms_scheduler_step(sched, 2000, 1000, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 2);
    assert_true(res.is_preemption);
    assert_int_equal(res.preempted_task_id, 1);
    assert_int_equal(evt_ctx.preemption_count, 1);

    // Step 4: t=3ms -> Task 2 finishes remaining 1ms
    assert_int_equal(edge_rms_scheduler_step(sched, 3000, 1000, &res), 0);
    assert_int_equal(res.active_task_id, 2);
    assert_int_equal(res.remaining_budget_us, 0);

    // Step 5: t=4ms -> Task 1 resumes
    assert_int_equal(edge_rms_scheduler_step(sched, 4000, 1000, &res), 0);
    assert_int_equal(res.active_task_id, 1);

    // Advance Task 1 until its 10ms budget is fully exhausted
    for (uint64_t t = 5000; t <= 12000; t += 1000) {
        edge_rms_scheduler_step(sched, t, 1000, &res);
    }

    // Budget watchdog should trigger throttle for Task 1
    assert_true(evt_ctx.budget_overrun_count >= 1);
    assert_int_equal(evt_ctx.last_overrun_id, 1);
    assert_int_equal(evt_ctx.last_action, EDGE_RMS_ACTION_THROTTLE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_rms_scheduler_lifecycle_and_priority_assignment),
        cmocka_unit_test(test_rms_exact_response_time_analysis),
        cmocka_unit_test(test_rms_preemptive_dispatch_and_watchdog),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
