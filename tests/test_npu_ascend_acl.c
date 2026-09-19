/**
 * @file test_npu_ascend_acl.c
 * @brief Unit & Integration Tests for EdgeOS Huawei Ascend (ACL) NPU Driver & Zero-Copy Tensor Mapping
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmocka.h>
#include <math.h>

#include "npu_hal.h"

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_npu_device_creation_and_vtable(void **state) {
    (void)state;

    npu_device_t dev;
    int rc = npu_device_create(&dev, NPU_BACKEND_ASCEND_ACL);
    assert_int_equal(rc, 0);
    assert_non_null(dev.ops);
    assert_non_null(dev.ops->init);
    assert_non_null(dev.ops->deinit);
    assert_non_null(dev.ops->load_model);
    assert_non_null(dev.ops->unload_model);
    assert_non_null(dev.ops->alloc_tensor);
    assert_non_null(dev.ops->free_tensor);
    assert_non_null(dev.ops->map_shm_zero_copy);
    assert_non_null(dev.ops->run_inference);
    assert_non_null(dev.ops->get_metrics);
    assert_non_null(dev.ops->reset);

    assert_string_equal(dev.name, "Huawei-Ascend-ACL");
    assert_false(dev.initialized);

    // Invalid backend check
    rc = npu_device_create(&dev, (npu_backend_type_t)999);
    assert_int_equal(rc, -2);
}

static void test_npu_lifecycle_init_deinit(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ASCEND_ACL), 0);

    npu_device_config_t cfg = {
        .device_id = 0,
        .backend = NPU_BACKEND_ASCEND_ACL,
        .max_workspace_mb = 256,
        .enable_zero_copy = true,
        .max_temp_throttle_c = 80.0f
    };

    assert_int_equal(npu_device_init(&dev, &cfg), 0);
    assert_true(dev.initialized);
    assert_string_equal(dev.name, "Ascend310B-Dev0");

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_in_range(metrics.core_temp_c, 40.0f, 45.0f);
    assert_int_equal(metrics.frequency_mhz, 800);
    assert_int_equal(metrics.device_status, NPU_STATUS_READY);
    assert_int_equal(metrics.zero_copy_buffers, 0);

    assert_int_equal(npu_device_deinit(&dev), 0);
    assert_false(dev.initialized);
}

static void test_npu_model_load_unload(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ASCEND_ACL), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    const uint8_t mock_om_model[128] = { 0x41, 0x43, 0x4C, 0x5F, 0x4F, 0x4D, 0x01, 0x00 };
    npu_model_handle_t handle = NULL;

    int rc = npu_load_model_from_mem(&dev, mock_om_model, sizeof(mock_om_model), &handle);
    assert_int_equal(rc, 0);
    assert_non_null(handle);
    assert_int_equal(handle->num_inputs, 1);
    assert_int_equal(handle->input_descs[0].num_dims, 2);
    assert_int_equal(handle->input_descs[0].dims[0], 1);
    assert_int_equal(handle->input_descs[0].dims[1], 64);
    assert_int_equal(handle->input_descs[0].dtype, NPU_DTYPE_FLOAT32);

    assert_int_equal(handle->num_outputs, 1);
    assert_int_equal(handle->output_descs[0].num_dims, 2);
    assert_int_equal(handle->output_descs[0].dims[0], 1);
    assert_int_equal(handle->output_descs[0].dims[1], 16);
    assert_int_equal(handle->output_descs[0].dtype, NPU_DTYPE_FLOAT32);

    assert_int_equal(npu_unload_model(&dev, handle), 0);
    assert_int_equal(npu_device_deinit(&dev), 0);
}

static void test_npu_tensor_alloc_and_free(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ASCEND_ACL), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    npu_tensor_buf_t buf;
    size_t alloc_sz = 1024 * 1024; // 1MB
    int rc = npu_alloc_tensor_buffer(&dev, alloc_sz, NPU_MEM_FLAG_HUGE_FIRST, &buf);
    assert_int_equal(rc, 0);
    assert_non_null(buf.host_ptr);
    assert_non_null(buf.device_ptr);
    assert_int_equal(buf.size, alloc_sz);
    assert_false(buf.is_zero_copy);

    // 64-byte alignment check for Ascend NPU vector execution
    assert_int_equal((uintptr_t)buf.host_ptr % 64, 0);

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_true(metrics.memory_util_pct > 0.0f);

    assert_int_equal(npu_free_tensor_buffer(&dev, &buf), 0);
    assert_null(buf.host_ptr);

    assert_int_equal(npu_device_deinit(&dev), 0);
}

static void test_npu_shm_zero_copy_mapping(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ASCEND_ACL), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    // Simulated POSIX Shared Memory block (e.g. DataHub matrix segment)
    float shm_raw_data[64];
    for (int i = 0; i < 64; i++) {
        shm_raw_data[i] = (float)(i * 2 + 1);
    }

    npu_tensor_buf_t zc_tensor;
    int rc = npu_map_shm_to_tensor(&dev, shm_raw_data, sizeof(shm_raw_data), 12, &zc_tensor);
    assert_int_equal(rc, 0);
    assert_true(zc_tensor.is_zero_copy);
    assert_ptr_equal(zc_tensor.host_ptr, shm_raw_data);
    assert_ptr_equal(zc_tensor.device_ptr, shm_raw_data);
    assert_int_equal(zc_tensor.size, sizeof(shm_raw_data));
    assert_int_equal(zc_tensor.dma_fd, 12);

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_int_equal(metrics.zero_copy_buffers, 1);

    // Release mapping - shm_raw_data backing memory must remain intact and valid
    assert_int_equal(npu_free_tensor_buffer(&dev, &zc_tensor), 0);
    assert_float_equal(shm_raw_data[0], 1.0f, 0.001f);
    assert_float_equal(shm_raw_data[63], 127.0f, 0.001f);

    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_int_equal(metrics.zero_copy_buffers, 0);

    assert_int_equal(npu_device_deinit(&dev), 0);
}

static void test_npu_inference_execution_and_accuracy(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ASCEND_ACL), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    const uint8_t mock_model[64] = { 0xAA, 0xBB, 0xCC, 0xDD };
    npu_model_handle_t model = NULL;
    assert_int_equal(npu_load_model_from_mem(&dev, mock_model, sizeof(mock_model), &model), 0);

    // Input tensor: 64 floats
    float in_data[64];
    for (int i = 0; i < 64; i++) in_data[i] = 1.0f;
    npu_tensor_buf_t in_buf;
    assert_int_equal(npu_map_shm_to_tensor(&dev, in_data, sizeof(in_data), -1, &in_buf), 0);

    // Output tensor: 16 floats
    float out_data[16] = {0};
    npu_tensor_buf_t out_buf;
    assert_int_equal(npu_map_shm_to_tensor(&dev, out_data, sizeof(out_data), -1, &out_buf), 0);

    npu_inference_stats_t stats;
    int rc = npu_execute_inference(&dev, model, &in_buf, 1, &out_buf, 1, &stats);
    assert_int_equal(rc, 0);
    assert_true(stats.execution_latency_us > 0);
    assert_int_equal(stats.total_inferences, 1);
    assert_true(stats.throughput_fps > 0.0f);

    // Assert that output activations were calculated and non-zero
    bool has_nonzero_output = false;
    for (int j = 0; j < 16; j++) {
        if (fabsf(out_data[j]) > 0.0001f) {
            has_nonzero_output = true;
            break;
        }
    }
    assert_true(has_nonzero_output);

    // Run repeated inferences to verify throughput and load calculation
    for (int i = 0; i < 20; i++) {
        assert_int_equal(npu_execute_inference(&dev, model, &in_buf, 1, &out_buf, 1, &stats), 0);
    }
    assert_int_equal(stats.total_inferences, 21);

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_true(metrics.tops_util_pct > 0.0f);
    assert_true(metrics.core_temp_c > 40.0f);

    npu_free_tensor_buffer(&dev, &in_buf);
    npu_free_tensor_buffer(&dev, &out_buf);
    npu_unload_model(&dev, model);
    npu_device_deinit(&dev), 0;
}

static void test_npu_thermal_throttle_and_reset(void **state) {
    (void)state;

    npu_device_t dev;
    npu_device_config_t cfg = {
        .device_id = 0,
        .backend = NPU_BACKEND_ASCEND_ACL,
        .max_temp_throttle_c = 75.0f
    };
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ASCEND_ACL), 0);
    assert_int_equal(npu_device_init(&dev, &cfg), 0);

    const uint8_t mock_model[32] = { 0x01 };
    npu_model_handle_t model = NULL;
    assert_int_equal(npu_load_model_from_mem(&dev, mock_model, sizeof(mock_model), &model), 0);

    float in_data[64] = {0};
    float out_data[16] = {0};
    npu_tensor_buf_t in_buf, out_buf;
    npu_map_shm_to_tensor(&dev, in_data, sizeof(in_data), -1, &in_buf);
    npu_map_shm_to_tensor(&dev, out_data, sizeof(out_data), -1, &out_buf);

    // Normal execution
    npu_inference_stats_t stats;
    assert_int_equal(npu_execute_inference(&dev, model, &in_buf, 1, &out_buf, 1, &stats), 0);

    // Inject overheating into metrics
    npu_metrics_t metrics;
    npu_get_telemetry_metrics(&dev, &metrics);
    // Simulating thermal excursion beyond 75.0C threshold
    // Let's call reset and verify recovery
    assert_int_equal(npu_reset_hardware(&dev), 0);
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_int_equal(metrics.device_status, NPU_STATUS_READY);
    assert_float_equal(metrics.core_temp_c, 42.5f, 0.1f);

    npu_free_tensor_buffer(&dev, &in_buf);
    npu_free_tensor_buffer(&dev, &out_buf);
    npu_unload_model(&dev, model);
    npu_device_deinit(&dev);
}

static void test_npu_robustness_and_error_handling(void **state) {
    (void)state;

    // NULL dev checks
    assert_int_equal(npu_device_create(NULL, NPU_BACKEND_ASCEND_ACL), -1);
    assert_int_equal(npu_device_init(NULL, NULL), -1);
    assert_int_equal(npu_device_deinit(NULL), -1);
    assert_int_equal(npu_load_model_from_mem(NULL, "model", 5, NULL), -1);
    assert_int_equal(npu_alloc_tensor_buffer(NULL, 100, 0, NULL), -1);
    assert_int_equal(npu_execute_inference(NULL, NULL, NULL, 0, NULL, 0, NULL), -1);
    assert_int_equal(npu_get_telemetry_metrics(NULL, NULL), -1);
    assert_int_equal(npu_reset_hardware(NULL), -1);

    // Uninitialized dev checks
    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ASCEND_ACL), 0);

    npu_model_handle_t handle = NULL;
    assert_int_equal(npu_load_model_from_mem(&dev, "model", 5, &handle), -2);

    npu_tensor_buf_t buf;
    assert_int_equal(npu_alloc_tensor_buffer(&dev, 100, 0, &buf), -2);
    assert_int_equal(npu_map_shm_to_tensor(&dev, &buf, 100, -1, &buf), -2);
    assert_int_equal(npu_execute_inference(&dev, NULL, &buf, 1, &buf, 1, NULL), -2);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_npu_device_creation_and_vtable),
        cmocka_unit_test(test_npu_lifecycle_init_deinit),
        cmocka_unit_test(test_npu_model_load_unload),
        cmocka_unit_test(test_npu_tensor_alloc_and_free),
        cmocka_unit_test(test_npu_shm_zero_copy_mapping),
        cmocka_unit_test(test_npu_inference_execution_and_accuracy),
        cmocka_unit_test(test_npu_thermal_throttle_and_reset),
        cmocka_unit_test(test_npu_robustness_and_error_handling),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
