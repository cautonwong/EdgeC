/**
 * @file test_npu_power_manager.c
 * @brief Unit Tests for NPU Power Manager, DVFS & Dynamic Clock Gating (AEP-750 / Issue #280)
 */

#include <cmocka.h>
#include <string.h>
#include <sys/mman.h>

#include "datahub.h"
#include "npu_power_manager.h"

static void test_npu_power_state_machine_and_timeouts(void **state) {
    (void)state;

    npu_power_mgr_storage_t storage;
    memset(&storage, 0xEE, sizeof(storage));

    npu_power_config_t cfg = {.governor = NPU_DVFS_ONDEMAND,
                              .idle_gate_timeout_ms = 5,
                              .standby_timeout_ms = 50,
                              .deep_sleep_timeout_ms = 500,
                              .thermal_warning_temp_c = 75.0f,
                              .thermal_critical_temp_c = 85.0f,
                              .max_freq_mhz = 1000,
                              .min_freq_mhz = 300};

    npu_power_mgr_t *mgr = npu_power_mgr_create_in(&storage, &cfg);
    assert_non_null(mgr);

    /* 1. Initial State: ACTIVE at 1000MHz */
    npu_power_metrics_t metrics;
    assert_int_equal(npu_power_mgr_get_metrics(mgr, &metrics), 0);
    assert_int_equal(metrics.current_state, NPU_PWR_ACTIVE);
    assert_int_equal(metrics.current_freq_mhz, 1000);
    assert_true(metrics.dynamic_power_mw > 5000.0f);

    /* 2. Step 1ms idle: Still ACTIVE */
    npu_power_mgr_tick(mgr, 1, 45.0f, 0);
    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_state, NPU_PWR_ACTIVE);

    /* 3. Step 5ms idle: Transition to IDLE_GATE */
    npu_power_mgr_tick(mgr, 5, 45.0f, 0);
    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_state, NPU_PWR_IDLE_GATE);
    assert_true(metrics.dynamic_power_mw < 1000.0f); /* Low power clock gated */

    /* 4. Step 50ms idle: Transition to STANDBY_SLEEP */
    npu_power_mgr_tick(mgr, 50, 45.0f, 0);
    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_state, NPU_PWR_STANDBY_SLEEP);
    assert_true(metrics.dynamic_power_mw < 100.0f);

    /* 5. Step 500ms idle: Transition to DEEP_SLEEP */
    npu_power_mgr_tick(mgr, 500, 45.0f, 0);
    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_state, NPU_PWR_DEEP_SLEEP);
    assert_true(metrics.dynamic_power_mw < 10.0f);

    /* 6. Tensor Arrival: Microsecond Fast Wakeup */
    float wakeup_lat_us = 0.0f;
    assert_int_equal(npu_power_mgr_notify_tensor_arrival(mgr, &wakeup_lat_us), 0);
    assert_true(wakeup_lat_us > 0.0f && wakeup_lat_us < 200.0f);

    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_state, NPU_PWR_ACTIVE);
    assert_true(metrics.cumulative_energy_mj > 0.0);
}

static void test_npu_dvfs_thermal_throttling_and_datahub_sync(void **state) {
    (void)state;

    npu_power_mgr_storage_t storage;
    npu_power_config_t cfg = {.governor = NPU_DVFS_THERMAL_ADAPTIVE,
                              .idle_gate_timeout_ms = 5,
                              .standby_timeout_ms = 50,
                              .deep_sleep_timeout_ms = 500,
                              .thermal_warning_temp_c = 75.0f,
                              .thermal_critical_temp_c = 85.0f,
                              .max_freq_mhz = 1000,
                              .min_freq_mhz = 300};

    npu_power_mgr_t *mgr = npu_power_mgr_create_in(&storage, &cfg);
    assert_non_null(mgr);

    /* High load: 10 queue depth -> Turbo 1200MHz */
    npu_power_mgr_tick(mgr, 10, 50.0f, 10);
    npu_power_metrics_t metrics;
    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_freq_mhz, 1200);

    /* Thermal warning 78C -> Downclock to 600MHz */
    npu_power_mgr_tick(mgr, 10, 78.0f, 10);
    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_freq_mhz, 600);

    /* Thermal critical 86C -> Force IDLE_GATE and Min Freq (300MHz) */
    npu_power_mgr_tick(mgr, 10, 86.0f, 10);
    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_state, NPU_PWR_IDLE_GATE);
    assert_int_equal(metrics.current_freq_mhz, 300);

    /* DataHub Sync */
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_npu_power_hub");
    assert_int_equal(
        datahub_init(&hub, &loop, "/test_npu_power_hub", 32, 64, NULL, 0, NULL, NULL, NULL), 0);
    assert_int_equal(npu_power_mgr_sync_datahub(mgr, &hub), 0);
    datahub_destroy(&hub);
    uv_loop_close(&loop);
    shm_unlink("/test_npu_power_hub");

    /* Reset */
    npu_power_mgr_reset(mgr);
    npu_power_mgr_get_metrics(mgr, &metrics);
    assert_int_equal(metrics.current_state, NPU_PWR_ACTIVE);
    assert_int_equal(metrics.current_freq_mhz, 1000);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_npu_power_state_machine_and_timeouts),
        cmocka_unit_test(test_npu_dvfs_thermal_throttling_and_datahub_sync),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
