/**
 * @file test_mixed_criticality.c
 * @brief Unit tests for Mixed-Criticality Real-Time Dispatcher & MTT Determinism (AEP-602 / Issue
 * #221)
 */

#include <cmocka.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_mixed_criticality.h"

typedef struct {
    uint32_t inversion_prevented_count;
    uint32_t mtt_dispatch_count;
    int64_t last_mtt_drift_us;
    uint32_t thermal_throttling_count;
    bool last_thermal_critical;
} test_mixed_event_ctx_t;

static void test_on_criticality_inversion(void *ctx, uint32_t preempting_id,
                                          uint32_t preempted_id) {
    (void)preempting_id;
    (void)preempted_id;
    test_mixed_event_ctx_t *evt = (test_mixed_event_ctx_t *)ctx;
    if (evt) {
        evt->inversion_prevented_count++;
    }
}

static void test_on_mtt_dispatch(void *ctx, uint32_t task_id, int64_t wall_clock_drift_us) {
    (void)task_id;
    test_mixed_event_ctx_t *evt = (test_mixed_event_ctx_t *)ctx;
    if (evt) {
        evt->mtt_dispatch_count++;
        evt->last_mtt_drift_us = wall_clock_drift_us;
    }
}

static void test_on_thermal_throttling(void *ctx, float junction_temp_c, bool is_critical) {
    (void)junction_temp_c;
    test_mixed_event_ctx_t *evt = (test_mixed_event_ctx_t *)ctx;
    if (evt) {
        evt->thermal_throttling_count++;
        evt->last_thermal_critical = is_critical;
    }
}

static int dummy_l1_action(void *ctx) {
    int *c = (int *)ctx;
    if (c) (*c)++;
    return 0;
}

static int dummy_l2_action(void *ctx) {
    int *c = (int *)ctx;
    if (c) (*c)++;
    return 0;
}

static int dummy_l3_action(void *ctx) {
    int *c = (int *)ctx;
    if (c) (*c)++;
    return 0;
}

static void test_mixed_crit_lifecycle_and_priority_preemption(void **state) {
    (void)state;
    test_mixed_event_ctx_t evt_ctx = {0};
    edge_mixed_crit_port_t port = {
        .on_criticality_inversion_prevented = test_on_criticality_inversion,
        .on_mtt_aligned_dispatch = test_on_mtt_dispatch,
        .on_thermal_throttling_engaged = test_on_thermal_throttling,
        .ctx = &evt_ctx,
    };

    edge_mixed_crit_storage_t store;
    edge_mixed_crit_config_t cfg = {
        .tick_resolution_us = 1000,
        .thermal_warning_temp_c = 70.0f,
        .thermal_critical_temp_c = 85.0f,
        .enable_thermal_throttling = true,
        .l3_decimation_factor = 2,
    };

    edge_mixed_crit_dispatcher_t *disp = edge_mixed_crit_create_in(&store, &cfg, &port);
    assert_non_null(disp);

    int l1_runs = 0, l3_runs = 0;
    // L1 safety task (e.g. BESS overtemperature protection, period 10ms)
    edge_mixed_crit_task_config_t t_l1 = {
        .task_id = 1,
        .name = "bess_trip_protection",
        .level = CRITICALITY_L1_SAFETY,
        .period_us = 10000,
        .wcet_budget_us = 500,
        .execute_fn = dummy_l1_action,
        .ctx = &l1_runs,
    };
    // L3 compute task (e.g. AI load forecasting, period 100ms)
    edge_mixed_crit_task_config_t t_l3 = {
        .task_id = 3,
        .name = "ai_load_prediction",
        .level = CRITICALITY_L3_COMPUTE,
        .period_us = 100000,
        .wcet_budget_us = 20000,
        .execute_fn = dummy_l3_action,
        .ctx = &l3_runs,
    };

    assert_int_equal(edge_mixed_crit_register_task(disp, &t_l1), 0);
    assert_int_equal(edge_mixed_crit_register_task(disp, &t_l3), 0);

    // Both tasks ready at t = 0
    edge_mixed_crit_dispatch_result_t res;
    assert_int_equal(edge_mixed_crit_step(disp, 0, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 1);
    assert_int_equal(res.active_level, CRITICALITY_L1_SAFETY);
    assert_true(res.preempted_lower_task);
    assert_int_equal(l1_runs, 1);
    assert_int_equal(evt_ctx.inversion_prevented_count, 1);

    // Next step at t = 0, L1 done, L3 now executes
    assert_int_equal(edge_mixed_crit_step(disp, 0, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 3);
    assert_int_equal(res.active_level, CRITICALITY_L3_COMPUTE);
    assert_int_equal(l3_runs, 1);

    assert_int_equal(edge_mixed_crit_destroy(disp), 0);
}

static void test_mixed_crit_mtt_wallclock_alignment(void **state) {
    (void)state;
    test_mixed_event_ctx_t evt_ctx = {0};
    edge_mixed_crit_port_t port = {
        .on_mtt_aligned_dispatch = test_on_mtt_dispatch,
        .ctx = &evt_ctx,
    };

    edge_mixed_crit_storage_t store;
    edge_mixed_crit_config_t cfg = {0};

    edge_mixed_crit_dispatcher_t *disp = edge_mixed_crit_create_in(&store, &cfg, &port);
    assert_non_null(disp);

    int l2_runs = 0;
    // 15-minute billing task (900 seconds = 900,000,000 us)
    edge_mixed_crit_task_config_t t_mtt = {
        .task_id = 2,
        .name = "mtt_15min_tou_billing",
        .level = CRITICALITY_L2_MTT,
        .period_us = 900000000ULL,
        .mtt_wall_clock_interval_us = 900000000ULL,
        .wcet_budget_us = 5000,
        .execute_fn = dummy_l2_action,
        .ctx = &l2_runs,
    };
    assert_int_equal(edge_mixed_crit_register_task(disp, &t_mtt), 0);

    // Dispatch at exactly t = 900000000ULL (0us drift)
    edge_mixed_crit_dispatch_result_t res;
    assert_int_equal(edge_mixed_crit_step(disp, 900000000ULL, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 2);
    assert_int_equal(res.wall_clock_drift_us, 0);
    assert_int_equal(evt_ctx.mtt_dispatch_count, 1);

    assert_int_equal(edge_mixed_crit_destroy(disp), 0);
}

static void test_mixed_crit_thermal_adaptive_throttling(void **state) {
    (void)state;
    test_mixed_event_ctx_t evt_ctx = {0};
    edge_mixed_crit_port_t port = {
        .on_thermal_throttling_engaged = test_on_thermal_throttling,
        .ctx = &evt_ctx,
    };

    edge_mixed_crit_storage_t store;
    edge_mixed_crit_config_t cfg = {
        .thermal_warning_temp_c = 70.0f,
        .thermal_critical_temp_c = 85.0f,
        .enable_thermal_throttling = true,
        .l3_decimation_factor = 2,
    };

    edge_mixed_crit_dispatcher_t *disp = edge_mixed_crit_create_in(&store, &cfg, &port);
    assert_non_null(disp);

    int l1_runs = 0, l3_runs = 0;
    edge_mixed_crit_task_config_t t_l1 = {
        .task_id = 1,
        .name = "l1_safety",
        .level = CRITICALITY_L1_SAFETY,
        .period_us = 1000,
        .execute_fn = dummy_l1_action,
        .ctx = &l1_runs,
    };
    edge_mixed_crit_task_config_t t_l3 = {
        .task_id = 3,
        .name = "l3_ai",
        .level = CRITICALITY_L3_COMPUTE,
        .period_us = 1000,
        .execute_fn = dummy_l3_action,
        .ctx = &l3_runs,
    };

    assert_int_equal(edge_mixed_crit_register_task(disp, &t_l1), 0);
    assert_int_equal(edge_mixed_crit_register_task(disp, &t_l3), 0);

    // Over-temperature critical: 90.0 C
    assert_int_equal(edge_mixed_crit_update_thermal(disp, 90.0f, 95.0f), 0);
    assert_true(evt_ctx.thermal_throttling_count > 0);
    assert_true(evt_ctx.last_thermal_critical);

    // L1 safety must still execute
    edge_mixed_crit_dispatch_result_t res;
    assert_int_equal(edge_mixed_crit_step(disp, 0, &res), 0);
    assert_true(res.task_dispatched);
    assert_int_equal(res.active_task_id, 1);
    assert_int_equal(l1_runs, 1);

    // L3 AI should be skipped under critical thermal condition
    assert_int_equal(edge_mixed_crit_step(disp, 0, &res), 0);
    assert_false(res.task_dispatched);  // Skipped
    assert_int_equal(l3_runs, 0);

    edge_mixed_crit_stats_t stats;
    assert_int_equal(edge_mixed_crit_get_stats(disp, &stats), 0);
    assert_true(stats.l3_skipped_throttled_count >= 1);
    assert_true(stats.thermal_throttle_active);

    assert_int_equal(edge_mixed_crit_destroy(disp), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mixed_crit_lifecycle_and_priority_preemption),
        cmocka_unit_test(test_mixed_crit_mtt_wallclock_alignment),
        cmocka_unit_test(test_mixed_crit_thermal_adaptive_throttling),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
