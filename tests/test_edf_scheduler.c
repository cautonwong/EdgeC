/**
 * @file test_edf_scheduler.c
 * @brief Unit tests for Earliest Deadline First (EDF) Dynamic Hard Real-Time Preemptive Scheduler
 * (AEP-712 / Issue #245)
 */

#include <cmocka.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_edf_scheduler.h"

typedef struct {
    uint32_t preemption_count;
    uint32_t last_preempted_id;
    uint32_t last_preempting_id;
    uint32_t miss_count;
    uint32_t last_miss_id;
    uint32_t warning_count;
    double last_warning_util;
} test_edf_event_ctx_t;

static void test_on_preempt(void *ctx, uint32_t preempted_task_id, uint32_t preempting_task_id,
                            uint64_t time_us) {
    (void)time_us;
    test_edf_event_ctx_t *evt = (test_edf_event_ctx_t *)ctx;
    if (evt) {
        evt->preemption_count++;
        evt->last_preempted_id = preempted_task_id;
        evt->last_preempting_id = preempting_task_id;
    }
}

static void test_on_miss(void *ctx, uint32_t task_id, uint64_t time_us, uint64_t overdue_us) {
    (void)time_us;
    (void)overdue_us;
    test_edf_event_ctx_t *evt = (test_edf_event_ctx_t *)ctx;
    if (evt) {
        evt->miss_count++;
        evt->last_miss_id = task_id;
    }
}

static void test_on_warning(void *ctx, double total_utilization, const char *reason) {
    (void)reason;
    test_edf_event_ctx_t *evt = (test_edf_event_ctx_t *)ctx;
    if (evt) {
        evt->warning_count++;
        evt->last_warning_util = total_utilization;
    }
}

static int dummy_task_step(void *ctx) {
    int *run_count = (int *)ctx;
    if (run_count) {
        (*run_count)++;
    }
    return 0;
}

static void test_edf_scheduler_lifecycle_and_registration(void **state) {
    (void)state;
    edge_edf_scheduler_storage_t store;
    edge_edf_config_t cfg = {
        .tick_resolution_us = 1000,
        .enable_dynamic_preemption = true,
        .drop_on_deadline_miss = true,
    };

    edge_edf_scheduler_t *sched = edge_edf_scheduler_create_in(&store, &cfg, NULL);
    assert_non_null(sched);

    // Register 3 tasks:
    // Task 1: T=10ms, C=2ms (U=0.20)
    // Task 2: T=20ms, C=5ms (U=0.25)
    // Task 3: T=50ms, C=15ms (U=0.30)
    // Total U = 0.75 <= 1.00
    edge_edf_task_config_t t1 = {
        .task_id = 101,
        .name = "foc_motor_ctrl",
        .period_us = 10000,
        .relative_deadline_us = 10000,
        .wcet_us = 2000,
    };
    edge_edf_task_config_t t2 = {
        .task_id = 102,
        .name = "vsg_inverter_ctrl",
        .period_us = 20000,
        .relative_deadline_us = 20000,
        .wcet_us = 5000,
    };
    edge_edf_task_config_t t3 = {
        .task_id = 103,
        .name = "bms_telemetry_poll",
        .period_us = 50000,
        .relative_deadline_us = 50000,
        .wcet_us = 15000,
    };

    assert_int_equal(edge_edf_scheduler_register_task(sched, &t1), 0);
    assert_int_equal(edge_edf_scheduler_register_task(sched, &t2), 0);
    assert_int_equal(edge_edf_scheduler_register_task(sched, &t3), 0);

    // Duplicate registration should fail with EEXIST
    assert_int_equal(edge_edf_scheduler_register_task(sched, &t1), -EEXIST);

    double u = edge_edf_scheduler_get_utilization(sched);
    assert_true(u > 0.749 && u < 0.751);

    edge_edf_stats_t stats;
    assert_int_equal(edge_edf_scheduler_get_stats(sched, &stats), 0);
    assert_int_equal(stats.registered_task_count, 3);
    assert_true(stats.total_utilization > 0.749 && stats.total_utilization < 0.751);

    // Unregister task 2
    assert_int_equal(edge_edf_scheduler_unregister_task(sched, 102), 0);
    u = edge_edf_scheduler_get_utilization(sched);
    assert_true(u > 0.499 && u < 0.501);
}

static void test_edf_schedulability_bound_analysis(void **state) {
    (void)state;
    test_edf_event_ctx_t evt_ctx = {0};
    edge_edf_port_t port = {
        .on_schedulability_warning = test_on_warning,
        .ctx = &evt_ctx,
    };

    edge_edf_scheduler_storage_t store;
    edge_edf_config_t cfg = {
        .tick_resolution_us = 1000,
        .enable_dynamic_preemption = true,
        .drop_on_deadline_miss = true,
    };

    edge_edf_scheduler_t *sched = edge_edf_scheduler_create_in(&store, &cfg, &port);
    assert_non_null(sched);

    // Test 1: Schedulable set (U = 0.80 <= 1.0)
    edge_edf_task_config_t t1 = {
        .task_id = 1, .name = "t1", .period_us = 10000, .wcet_us = 3000};  // U=0.30
    edge_edf_task_config_t t2 = {
        .task_id = 2, .name = "t2", .period_us = 20000, .wcet_us = 10000};  // U=0.50

    assert_int_equal(edge_edf_scheduler_register_task(sched, &t1), 0);
    assert_int_equal(edge_edf_scheduler_register_task(sched, &t2), 0);

    edge_edf_schedulability_report_t report;
    assert_int_equal(edge_edf_scheduler_check_schedulability(sched, &report), 0);
    assert_true(report.is_schedulable);
    assert_true(report.total_utilization > 0.799 && report.total_utilization < 0.801);
    assert_int_equal(report.hyperperiod_us, 20000);

    // Test 2: Overloaded set (U = 0.30 + 0.50 + 0.40 = 1.20 > 1.0)
    edge_edf_task_config_t t3 = {
        .task_id = 3, .name = "t3", .period_us = 25000, .wcet_us = 10000};  // U=0.40
    assert_int_equal(edge_edf_scheduler_register_task(sched, &t3), 0);
    assert_true(evt_ctx.warning_count > 0);

    assert_int_equal(edge_edf_scheduler_check_schedulability(sched, &report), 0);
    assert_false(report.is_schedulable);
    assert_true(report.total_utilization > 1.199 && report.total_utilization < 1.201);
    assert_non_null(strstr(report.failure_reason, "Liu & Layland"));
}

static void test_edf_preemptive_dispatch_and_execution(void **state) {
    (void)state;
    test_edf_event_ctx_t evt_ctx = {0};
    edge_edf_port_t port = {
        .on_task_preempt = test_on_preempt,
        .on_deadline_miss = test_on_miss,
        .ctx = &evt_ctx,
    };

    edge_edf_scheduler_storage_t store;
    edge_edf_config_t cfg = {
        .tick_resolution_us = 1000,
        .enable_dynamic_preemption = true,
        .drop_on_deadline_miss = false,
    };

    edge_edf_scheduler_t *sched = edge_edf_scheduler_create_in(&store, &cfg, &port);
    assert_non_null(sched);

    int t1_run = 0, t2_run = 0;
    // Task 1: Period 20ms, WCET 8ms (arrives at t=0, deadline t=20ms)
    edge_edf_task_config_t t1 = {
        .task_id = 1,
        .name = "task_background",
        .period_us = 20000,
        .wcet_us = 8000,
        .initial_phase_us = 0,
        .run = dummy_task_step,
        .task_ctx = &t1_run,
    };
    // Task 2: Period 10ms, WCET 2ms, Phase 2ms (arrives at t=2ms, deadline t=12ms < t=20ms!)
    edge_edf_task_config_t t2 = {
        .task_id = 2,
        .name = "task_urgent",
        .period_us = 10000,
        .wcet_us = 2000,
        .initial_phase_us = 2000,
        .run = dummy_task_step,
        .task_ctx = &t2_run,
    };

    assert_int_equal(edge_edf_scheduler_register_task(sched, &t1), 0);
    assert_int_equal(edge_edf_scheduler_register_task(sched, &t2), 0);

    edge_edf_dispatch_result_t res;

    // Step 1: t=0ms..1ms -> Task 1 dispatches (deadline=20000us)
    assert_int_equal(edge_edf_scheduler_step(sched, 0, 1000, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 1);
    assert_false(res.is_preemption);

    // Step 2: t=1ms..2ms -> Task 1 continues
    assert_int_equal(edge_edf_scheduler_step(sched, 1000, 1000, &res), 0);
    assert_int_equal(res.active_task_id, 1);
    assert_false(res.is_preemption);

    // Step 3: t=2ms..3ms -> Task 2 arrives with deadline t=12ms! Preempts Task 1!
    assert_int_equal(edge_edf_scheduler_step(sched, 2000, 1000, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 2);
    assert_true(res.is_preemption);
    assert_int_equal(res.preempted_task_id, 1);
    assert_int_equal(evt_ctx.preemption_count, 1);
    assert_int_equal(evt_ctx.last_preempted_id, 1);
    assert_int_equal(evt_ctx.last_preempting_id, 2);

    // Step 4: t=3ms..4ms -> Task 2 finishes its remaining 1ms
    assert_int_equal(edge_edf_scheduler_step(sched, 3000, 1000, &res), 0);
    assert_int_equal(res.active_task_id, 2);
    assert_int_equal(res.remaining_budget_us, 0);

    // Step 5: t=4ms..5ms -> Task 1 resumes!
    assert_int_equal(edge_edf_scheduler_step(sched, 4000, 1000, &res), 0);
    assert_int_equal(res.active_task_id, 1);

    edge_edf_stats_t stats;
    assert_int_equal(edge_edf_scheduler_get_stats(sched, &stats), 0);
    assert_true(stats.total_preemptions >= 1);
    assert_true(stats.total_dispatches >= 5);
}

static void test_edf_deadline_miss_isolation(void **state) {
    (void)state;
    test_edf_event_ctx_t evt_ctx = {0};
    edge_edf_port_t port = {
        .on_deadline_miss = test_on_miss,
        .ctx = &evt_ctx,
    };

    edge_edf_scheduler_storage_t store;
    edge_edf_config_t cfg = {
        .tick_resolution_us = 1000,
        .enable_dynamic_preemption = true,
        .drop_on_deadline_miss = true,
    };

    edge_edf_scheduler_t *sched = edge_edf_scheduler_create_in(&store, &cfg, &port);
    assert_non_null(sched);

    // Task with 5ms period, 8ms WCET (impossible, will overrun deadline at 5ms)
    edge_edf_task_config_t t_over = {
        .task_id = 99,
        .name = "heavy_overdue_task",
        .period_us = 5000,
        .relative_deadline_us = 5000,
        .wcet_us = 4000,
    };
    assert_int_equal(edge_edf_scheduler_register_task(sched, &t_over), 0);

    edge_edf_dispatch_result_t res;
    // Step 1ms at t=0
    edge_edf_scheduler_step(sched, 0, 1000, &res);
    // Advance simulation time past deadline to t=6ms without completing job
    edge_edf_scheduler_step(sched, 6000, 1000, &res);

    assert_true(res.is_deadline_miss);
    assert_int_equal(res.missed_task_id, 99);
    assert_int_equal(evt_ctx.miss_count, 1);
    assert_int_equal(evt_ctx.last_miss_id, 99);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_edf_scheduler_lifecycle_and_registration),
        cmocka_unit_test(test_edf_schedulability_bound_analysis),
        cmocka_unit_test(test_edf_preemptive_dispatch_and_execution),
        cmocka_unit_test(test_edf_deadline_miss_isolation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
