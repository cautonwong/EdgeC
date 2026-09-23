/**
 * @file test_npu_self_healing.c
 * @brief Unit Tests for NPU Hardware Watchdog, Overtemp Defense & CPU Fallback (AEP-747 / Issue
 * #277)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "npu_hal.h"
#include "npu_self_healing.h"

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_npu_self_healing_normal_flow(void **state) {
    (void)state;

    npu_device_t prim_dev;
    assert_int_equal(npu_device_create(&prim_dev, NPU_BACKEND_ASCEND_ACL), 0);
    npu_device_config_t prim_cfg = {.device_id = 0,
                                    .backend = NPU_BACKEND_ASCEND_ACL,
                                    .max_workspace_mb = 256,
                                    .enable_zero_copy = true,
                                    .max_temp_throttle_c = 85.0f};
    assert_int_equal(npu_device_init(&prim_dev, &prim_cfg), 0);

    /* Load test model */
    uint8_t dummy_model[256] = {0};
    npu_model_handle_t model_handle = NULL;
    assert_int_equal(
        npu_load_model_from_mem(&prim_dev, dummy_model, sizeof(dummy_model), &model_handle), 0);
    assert_non_null(model_handle);

    npu_self_healing_t *healer = NULL;
    npu_self_healing_config_t heal_cfg = {.primary_device = &prim_dev,
                                          .max_temp_threshold_c = 85.0f,
                                          .recover_temp_threshold_c = 75.0f,
                                          .watchdog_timeout_ms = 100,
                                          .max_consecutive_errors = 3,
                                          .auto_reset_on_deadlock = true,
                                          .enable_cpu_fallback = true};
    assert_int_equal(npu_self_healing_init(&healer, &heal_cfg), 0);
    assert_non_null(healer);

    /* Allocate test tensors */
    npu_tensor_buf_t in_tensor, out_tensor;
    assert_int_equal(
        npu_alloc_tensor_buffer(&prim_dev, 64 * sizeof(float), NPU_MEM_FLAG_DEFAULT, &in_tensor),
        0);
    assert_int_equal(
        npu_alloc_tensor_buffer(&prim_dev, 64 * sizeof(float), NPU_MEM_FLAG_DEFAULT, &out_tensor),
        0);

    float *in_data = (float *)in_tensor.host_ptr;
    for (int i = 0; i < 64; i++) {
        in_data[i] = (float)i * 0.1f;
    }

    npu_inference_stats_t stats;
    assert_int_equal(
        npu_self_healing_execute(healer, model_handle, &in_tensor, 1, &out_tensor, 1, &stats), 0);

    npu_self_healing_metrics_t metrics;
    assert_int_equal(npu_self_healing_get_metrics(healer, &metrics), 0);
    assert_int_equal(metrics.total_inferences, 1);
    assert_int_equal(metrics.npi_inferences, 1);
    assert_int_equal(metrics.cpu_fallback_inferences, 0);
    assert_false(metrics.is_fallback_active);
    assert_int_equal(metrics.state, NPU_HEALING_STATE_NORMAL);

    assert_int_equal(npu_free_tensor_buffer(&prim_dev, &in_tensor), 0);
    assert_int_equal(npu_free_tensor_buffer(&prim_dev, &out_tensor), 0);
    assert_int_equal(npu_unload_model(&prim_dev, model_handle), 0);
    assert_int_equal(npu_self_healing_destroy(healer), 0);
    assert_int_equal(npu_device_deinit(&prim_dev), 0);
}

static void test_npu_self_healing_overtemp_fallback(void **state) {
    (void)state;

    npu_device_t prim_dev;
    assert_int_equal(npu_device_create(&prim_dev, NPU_BACKEND_ROCKCHIP_RKNN), 0);
    npu_device_config_t prim_cfg = {.device_id = 0,
                                    .backend = NPU_BACKEND_ROCKCHIP_RKNN,
                                    .max_workspace_mb = 128,
                                    .enable_zero_copy = true,
                                    .max_temp_throttle_c = 85.0f};
    assert_int_equal(npu_device_init(&prim_dev, &prim_cfg), 0);

    uint8_t dummy_model[256] = {0};
    npu_model_handle_t model_handle = NULL;
    assert_int_equal(
        npu_load_model_from_mem(&prim_dev, dummy_model, sizeof(dummy_model), &model_handle), 0);

    npu_self_healing_t *healer = NULL;
    npu_self_healing_config_t heal_cfg = {.primary_device = &prim_dev,
                                          .max_temp_threshold_c = 85.0f,
                                          .recover_temp_threshold_c = 75.0f,
                                          .watchdog_timeout_ms = 100,
                                          .max_consecutive_errors = 3,
                                          .auto_reset_on_deadlock = true,
                                          .enable_cpu_fallback = true};
    assert_int_equal(npu_self_healing_init(&healer, &heal_cfg), 0);

    /* Allocate tensors */
    npu_tensor_buf_t in_tensor, out_tensor;
    assert_int_equal(
        npu_alloc_tensor_buffer(&prim_dev, 32 * sizeof(float), NPU_MEM_FLAG_DEFAULT, &in_tensor),
        0);
    assert_int_equal(
        npu_alloc_tensor_buffer(&prim_dev, 32 * sizeof(float), NPU_MEM_FLAG_DEFAULT, &out_tensor),
        0);

    /* Inject Over-Temperature Fault (> 85C) */
    assert_int_equal(npu_self_healing_inject_fault(healer, true, false), 0);

    npu_inference_stats_t stats;
    assert_int_equal(
        npu_self_healing_execute(healer, model_handle, &in_tensor, 1, &out_tensor, 1, &stats), 0);

    npu_self_healing_metrics_t metrics;
    assert_int_equal(npu_self_healing_get_metrics(healer, &metrics), 0);
    assert_true(metrics.is_fallback_active);
    assert_int_equal(metrics.state, NPU_HEALING_STATE_FALLBACK_CPU);
    assert_int_equal(metrics.overtemp_events, 1);
    assert_int_equal(metrics.fallback_count, 1);
    assert_int_equal(metrics.cpu_fallback_inferences, 1);

    /* Verify seamless switch latency is strictly within 10ms (10,000 us) */
    assert_true(metrics.last_switch_latency_us < 10000);

    assert_int_equal(npu_free_tensor_buffer(&prim_dev, &in_tensor), 0);
    assert_int_equal(npu_free_tensor_buffer(&prim_dev, &out_tensor), 0);
    assert_int_equal(npu_unload_model(&prim_dev, model_handle), 0);
    assert_int_equal(npu_self_healing_destroy(healer), 0);
    assert_int_equal(npu_device_deinit(&prim_dev), 0);
}

static void test_npu_self_healing_watchdog_timeout_deadlock(void **state) {
    (void)state;

    npu_device_t prim_dev;
    assert_int_equal(npu_device_create(&prim_dev, NPU_BACKEND_HORIZON_BPU), 0);
    npu_device_config_t prim_cfg = {.device_id = 0,
                                    .backend = NPU_BACKEND_HORIZON_BPU,
                                    .max_workspace_mb = 128,
                                    .enable_zero_copy = true,
                                    .max_temp_throttle_c = 85.0f};
    assert_int_equal(npu_device_init(&prim_dev, &prim_cfg), 0);

    uint8_t dummy_model[256] = {0};
    npu_model_handle_t model_handle = NULL;
    assert_int_equal(
        npu_load_model_from_mem(&prim_dev, dummy_model, sizeof(dummy_model), &model_handle), 0);

    npu_self_healing_t *healer = NULL;
    npu_self_healing_config_t heal_cfg = {.primary_device = &prim_dev,
                                          .max_temp_threshold_c = 85.0f,
                                          .recover_temp_threshold_c = 75.0f,
                                          .watchdog_timeout_ms = 20, /* 20ms watchdog limit */
                                          .max_consecutive_errors = 2,
                                          .auto_reset_on_deadlock = true,
                                          .enable_cpu_fallback = true};
    assert_int_equal(npu_self_healing_init(&healer, &heal_cfg), 0);

    /* Allocate tensors */
    npu_tensor_buf_t in_tensor, out_tensor;
    assert_int_equal(
        npu_alloc_tensor_buffer(&prim_dev, 16 * sizeof(float), NPU_MEM_FLAG_DEFAULT, &in_tensor),
        0);
    assert_int_equal(
        npu_alloc_tensor_buffer(&prim_dev, 16 * sizeof(float), NPU_MEM_FLAG_DEFAULT, &out_tensor),
        0);

    /* Sleep to exceed watchdog timeout (simulate hardware freeze/deadlock) */
    usleep(30000); /* 30ms > 20ms timeout */

    npu_inference_stats_t stats;
    assert_int_equal(
        npu_self_healing_execute(healer, model_handle, &in_tensor, 1, &out_tensor, 1, &stats), 0);

    npu_self_healing_metrics_t metrics;
    assert_int_equal(npu_self_healing_get_metrics(healer, &metrics), 0);
    assert_true(metrics.is_fallback_active);
    assert_int_equal(metrics.watchdog_timeouts, 1);
    assert_int_equal(metrics.fallback_count, 1);
    assert_int_equal(metrics.cpu_fallback_inferences, 1);

    assert_int_equal(npu_free_tensor_buffer(&prim_dev, &in_tensor), 0);
    assert_int_equal(npu_free_tensor_buffer(&prim_dev, &out_tensor), 0);
    assert_int_equal(npu_unload_model(&prim_dev, model_handle), 0);
    assert_int_equal(npu_self_healing_destroy(healer), 0);
    assert_int_equal(npu_device_deinit(&prim_dev), 0);
}

static void test_npu_self_healing_cooldown_recovery(void **state) {
    (void)state;

    npu_device_t prim_dev;
    assert_int_equal(npu_device_create(&prim_dev, NPU_BACKEND_TI_SITARA), 0);
    npu_device_config_t prim_cfg = {.device_id = 0,
                                    .backend = NPU_BACKEND_TI_SITARA,
                                    .max_workspace_mb = 128,
                                    .enable_zero_copy = true,
                                    .max_temp_throttle_c = 85.0f};
    assert_int_equal(npu_device_init(&prim_dev, &prim_cfg), 0);

    uint8_t dummy_model[256] = {0};
    npu_model_handle_t model_handle = NULL;
    assert_int_equal(
        npu_load_model_from_mem(&prim_dev, dummy_model, sizeof(dummy_model), &model_handle), 0);

    npu_self_healing_t *healer = NULL;
    npu_self_healing_config_t heal_cfg = {.primary_device = &prim_dev,
                                          .max_temp_threshold_c = 85.0f,
                                          .recover_temp_threshold_c = 75.0f,
                                          .watchdog_timeout_ms = 100,
                                          .max_consecutive_errors = 3,
                                          .auto_reset_on_deadlock = true,
                                          .enable_cpu_fallback = true};
    assert_int_equal(npu_self_healing_init(&healer, &heal_cfg), 0);

    npu_tensor_buf_t in_tensor, out_tensor;
    assert_int_equal(
        npu_alloc_tensor_buffer(&prim_dev, 16 * sizeof(float), NPU_MEM_FLAG_DEFAULT, &in_tensor),
        0);
    assert_int_equal(
        npu_alloc_tensor_buffer(&prim_dev, 16 * sizeof(float), NPU_MEM_FLAG_DEFAULT, &out_tensor),
        0);

    /* Step 1: Inject Overtemp Fault */
    assert_int_equal(npu_self_healing_inject_fault(healer, true, false), 0);
    npu_inference_stats_t stats;
    assert_int_equal(
        npu_self_healing_execute(healer, model_handle, &in_tensor, 1, &out_tensor, 1, &stats), 0);

    npu_self_healing_metrics_t metrics;
    assert_int_equal(npu_self_healing_get_metrics(healer, &metrics), 0);
    assert_true(metrics.is_fallback_active);

    /* Step 2: Clear Fault and simulate normal temperature cool-down (< 75C) */
    assert_int_equal(npu_self_healing_inject_fault(healer, false, false), 0);
    assert_int_equal(npu_self_healing_feed_watchdog(healer), 0);
    assert_int_equal(npu_self_healing_check_health(healer), 0);

    /* Step 3: Next inference should smoothly run on recovered primary hardware NPU */
    assert_int_equal(
        npu_self_healing_execute(healer, model_handle, &in_tensor, 1, &out_tensor, 1, &stats), 0);

    assert_int_equal(npu_self_healing_get_metrics(healer, &metrics), 0);
    assert_false(metrics.is_fallback_active);
    assert_int_equal(metrics.state, NPU_HEALING_STATE_NORMAL);
    assert_int_equal(metrics.recovery_count, 1);
    assert_int_equal(metrics.npi_inferences, 1);
    assert_int_equal(metrics.cpu_fallback_inferences, 1);
    assert_int_equal(metrics.total_inferences, 2);

    assert_int_equal(npu_free_tensor_buffer(&prim_dev, &in_tensor), 0);
    assert_int_equal(npu_free_tensor_buffer(&prim_dev, &out_tensor), 0);
    assert_int_equal(npu_unload_model(&prim_dev, model_handle), 0);
    assert_int_equal(npu_self_healing_destroy(healer), 0);
    assert_int_equal(npu_device_deinit(&prim_dev), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_npu_self_healing_normal_flow),
        cmocka_unit_test(test_npu_self_healing_overtemp_fallback),
        cmocka_unit_test(test_npu_self_healing_watchdog_timeout_deadlock),
        cmocka_unit_test(test_npu_self_healing_cooldown_recovery),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
