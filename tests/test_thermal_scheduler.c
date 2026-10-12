/**
 * @file test_thermal_scheduler.c
 * @brief Unit tests for Thermal-Aware Dynamic Power Budgeting & Adaptive Degradation Scheduler
 * (AEP-614)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_thermal_scheduler.h"

static void test_thermal_scheduler_normal_operation(void **state) {
    (void)state;
    edge_thermal_scheduler_ctx_t thermal_ctx;
    thermal_scheduler_config_t cfg = {.temp_warn_threshold_c = 70.0f,
                                      .temp_degraded_threshold_c = 85.0f,
                                      .temp_emergency_threshold_c = 95.0f,
                                      .thermal_resistance_r_th = 1.2f,
                                      .thermal_capacitance_c_th = 50.0f,
                                      .max_tdp_power_w = 35.0f,
                                      .base_power_idle_w = 5.0f};
    assert_int_equal(edge_thermal_scheduler_init(&thermal_ctx, &cfg), 0);

    /* Normal operating temperatures: CPU 55°C, NPU 50°C, Ambient 30°C */
    edge_thermal_scheduler_update_sensors(&thermal_ctx, 55.0f, 50.0f, 30.0f, 0.1f);

    thermal_task_quota_t quota;
    int rc = edge_thermal_scheduler_arbitrate(&thermal_ctx, &quota);
    assert_int_equal(0, rc);

    thermal_zone_stage_t zone;
    float budget_w = 0.0f, est_power_w = 0.0f;
    edge_thermal_scheduler_get_status(&thermal_ctx, &zone, &budget_w, &est_power_w);

    assert_int_equal(THERMAL_ZONE_NORMAL, zone);
    assert_true(quota.level_a_allowed);
    assert_true(fabsf(quota.level_b_throttle_pct - 100.0f) < 1.0f);
    assert_true(fabsf(quota.level_c_fps_ratio - 1.0f) < 0.02f);
    assert_false(quota.level_d_paused);

    /* All criticality levels admissible */
    bool adm_a = false, adm_c = false, adm_d = false;
    edge_thermal_scheduler_check_task_admissible(&thermal_ctx, CRITICALITY_LEVEL_A, &adm_a);
    edge_thermal_scheduler_check_task_admissible(&thermal_ctx, CRITICALITY_LEVEL_C, &adm_c);
    edge_thermal_scheduler_check_task_admissible(&thermal_ctx, CRITICALITY_LEVEL_D, &adm_d);
    assert_true(adm_a);
    assert_true(adm_c);
    assert_true(adm_d);
}

static void test_thermal_scheduler_warning_degradation(void **state) {
    (void)state;
    edge_thermal_scheduler_ctx_t thermal_ctx;
    thermal_scheduler_config_t cfg = {.temp_warn_threshold_c = 70.0f,
                                      .temp_degraded_threshold_c = 85.0f,
                                      .temp_emergency_threshold_c = 95.0f,
                                      .thermal_resistance_r_th = 1.2f,
                                      .thermal_capacitance_c_th = 50.0f,
                                      .max_tdp_power_w = 35.0f,
                                      .base_power_idle_w = 5.0f};
    edge_thermal_scheduler_init(&thermal_ctx, &cfg);

    /* Warning stage: CPU 75°C (>= 70°C, < 85°C) */
    edge_thermal_scheduler_update_sensors(&thermal_ctx, 75.0f, 72.0f, 35.0f, 0.1f);

    thermal_task_quota_t quota;
    edge_thermal_scheduler_arbitrate(&thermal_ctx, &quota);

    thermal_zone_stage_t zone;
    edge_thermal_scheduler_get_status(&thermal_ctx, &zone, NULL, NULL);

    assert_int_equal(THERMAL_ZONE_WARNING, zone);
    assert_true(quota.level_a_allowed);
    assert_true(fabsf(quota.level_c_fps_ratio - 0.5f) < 0.02f); /* AI throttled 50% */
    assert_true(quota.level_d_paused);                          /* Logs paused */

    bool adm_a = false, adm_c = false, adm_d = false;
    edge_thermal_scheduler_check_task_admissible(&thermal_ctx, CRITICALITY_LEVEL_A, &adm_a);
    edge_thermal_scheduler_check_task_admissible(&thermal_ctx, CRITICALITY_LEVEL_C, &adm_c);
    edge_thermal_scheduler_check_task_admissible(&thermal_ctx, CRITICALITY_LEVEL_D, &adm_d);
    assert_true(adm_a);
    assert_true(adm_c);  /* 50% ratio is still admissible */
    assert_false(adm_d); /* Level-D paused */
}

static void test_thermal_scheduler_degraded_stage(void **state) {
    (void)state;
    edge_thermal_scheduler_ctx_t thermal_ctx;
    thermal_scheduler_config_t cfg = {.temp_warn_threshold_c = 70.0f,
                                      .temp_degraded_threshold_c = 85.0f,
                                      .temp_emergency_threshold_c = 95.0f,
                                      .thermal_resistance_r_th = 1.2f,
                                      .thermal_capacitance_c_th = 50.0f,
                                      .max_tdp_power_w = 35.0f,
                                      .base_power_idle_w = 5.0f};
    edge_thermal_scheduler_init(&thermal_ctx, &cfg);

    /* Degraded stage: CPU 88°C (>= 85°C, < 95°C) */
    edge_thermal_scheduler_update_sensors(&thermal_ctx, 88.0f, 86.0f, 40.0f, 0.1f);

    thermal_task_quota_t quota;
    edge_thermal_scheduler_arbitrate(&thermal_ctx, &quota);

    thermal_zone_stage_t zone;
    edge_thermal_scheduler_get_status(&thermal_ctx, &zone, NULL, NULL);

    assert_int_equal(THERMAL_ZONE_DEGRADED, zone);
    assert_true(quota.level_a_allowed);                         /* Level-A 100% locked */
    assert_true(fabsf(quota.level_c_fps_ratio - 0.0f) < 0.02f); /* AI suspended */
    assert_true(quota.level_d_paused);                          /* Logs paused */

    bool adm_a = false, adm_c = false;
    edge_thermal_scheduler_check_task_admissible(&thermal_ctx, CRITICALITY_LEVEL_A, &adm_a);
    edge_thermal_scheduler_check_task_admissible(&thermal_ctx, CRITICALITY_LEVEL_C, &adm_c);
    assert_true(adm_a);
    assert_false(adm_c);
}

static void test_thermal_scheduler_emergency_trip(void **state) {
    (void)state;
    edge_thermal_scheduler_ctx_t thermal_ctx;
    thermal_scheduler_config_t cfg = {.temp_warn_threshold_c = 70.0f,
                                      .temp_degraded_threshold_c = 85.0f,
                                      .temp_emergency_threshold_c = 95.0f,
                                      .thermal_resistance_r_th = 1.2f,
                                      .thermal_capacitance_c_th = 50.0f,
                                      .max_tdp_power_w = 35.0f,
                                      .base_power_idle_w = 5.0f};
    edge_thermal_scheduler_init(&thermal_ctx, &cfg);

    /* Emergency stage: CPU 98°C (>= 95°C) */
    edge_thermal_scheduler_update_sensors(&thermal_ctx, 98.0f, 96.0f, 50.0f, 0.1f);

    thermal_task_quota_t quota;
    edge_thermal_scheduler_arbitrate(&thermal_ctx, &quota);

    assert_true(thermal_ctx.emergency_tripped);
    assert_int_equal(THERMAL_ZONE_EMERGENCY, thermal_ctx.current_zone);
    assert_int_equal(0, quota.npu_clock_mhz);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_thermal_scheduler_normal_operation),
        cmocka_unit_test(test_thermal_scheduler_warning_degradation),
        cmocka_unit_test(test_thermal_scheduler_degraded_stage),
        cmocka_unit_test(test_thermal_scheduler_emergency_trip),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
