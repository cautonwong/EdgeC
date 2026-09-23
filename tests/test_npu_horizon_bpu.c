/**
 * @file test_npu_horizon_bpu.c
 * @brief Unit & Integration Tests for Horizon BPU DNN Driver & Context Pool (AEP-743 / Issue #273)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "npu_hal.h"

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_horizon_device_creation_and_vtable(void **state) {
    (void)state;

    npu_device_t dev;
    int rc = npu_device_create(&dev, NPU_BACKEND_HORIZON_BPU);
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

    assert_string_equal(dev.name, "Horizon-BPU");
    assert_false(dev.initialized);
}

static void test_horizon_lifecycle_init_deinit(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_HORIZON_BPU), 0);

    npu_device_config_t cfg = {.device_id = 0,
                               .backend = NPU_BACKEND_HORIZON_BPU,
                               .max_workspace_mb = 256,
                               .enable_zero_copy = true,
                               .max_temp_throttle_c = 85.0f};

    assert_int_equal(npu_device_init(&dev, &cfg), 0);
    assert_true(dev.initialized);
    assert_string_equal(dev.name, "Horizon-BPU-X3-Dev0");

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_in_range(metrics.core_temp_c, 38.0f, 45.0f);
    assert_int_equal(metrics.frequency_mhz, 1200);
    assert_int_equal(metrics.device_status, NPU_STATUS_READY);
    assert_int_equal(metrics.zero_copy_buffers, 0);

    assert_int_equal(npu_device_deinit(&dev), 0);
    assert_false(dev.initialized);
}

static void test_horizon_multimodel_preloading_and_unloading(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_HORIZON_BPU), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    const char mock_bpu_model_a[] = "HORIZON_BPU_MODEL_PV_SURGE_PREDICTION_V1";
    const char mock_bpu_model_b[] = "HORIZON_BPU_MODEL_TRANSFORMER_HOTSPOT_V2";
    npu_model_handle_t handle_a = NULL;
    npu_model_handle_t handle_b = NULL;

    assert_int_equal(
        npu_load_model_from_mem(&dev, mock_bpu_model_a, sizeof(mock_bpu_model_a), &handle_a), 0);
    assert_non_null(handle_a);
    assert_int_equal(
        npu_load_model_from_mem(&dev, mock_bpu_model_b, sizeof(mock_bpu_model_b), &handle_b), 0);
    assert_non_null(handle_b);

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_true(metrics.memory_util_pct > 0.0f);

    assert_int_equal(npu_unload_model(&dev, handle_a), 0);
    assert_int_equal(npu_unload_model(&dev, handle_b), 0);
    assert_int_equal(npu_device_deinit(&dev), 0);
}

static void test_horizon_dmabuf_and_shm_zero_copy(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_HORIZON_BPU), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    /* 1. 普通 Tensor 内存分配 */
    npu_tensor_buf_t tensor1;
    assert_int_equal(npu_alloc_tensor_buffer(&dev, 4096, NPU_MEM_FLAG_DEFAULT, &tensor1), 0);
    assert_non_null(tensor1.host_ptr);
    assert_int_equal(tensor1.size, 4096);
    assert_false(tensor1.is_zero_copy);
    assert_int_equal(tensor1.dma_fd, -1);

    /* 2. Linux DMA-BUF 零拷贝映射 (模拟 dma_fd = 25) */
    npu_tensor_buf_t dma_tensor;
    float mock_dma_mem[128];
    for (int i = 0; i < 128; i++) mock_dma_mem[i] = (float)i * 0.25f;

    assert_int_equal(
        npu_map_shm_to_tensor(&dev, mock_dma_mem, sizeof(mock_dma_mem), 25, &dma_tensor), 0);
    assert_ptr_equal(dma_tensor.host_ptr, mock_dma_mem);
    assert_int_equal(dma_tensor.dma_fd, 25);
    assert_true(dma_tensor.is_zero_copy);

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_int_equal(metrics.zero_copy_buffers, 1);

    /* 3. 释放 */
    assert_int_equal(npu_free_tensor_buffer(&dev, &tensor1), 0);
    assert_int_equal(npu_free_tensor_buffer(&dev, &dma_tensor), 0);

    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_int_equal(metrics.zero_copy_buffers, 0);

    assert_int_equal(npu_device_deinit(&dev), 0);
}

static void test_horizon_context_pool_and_multitask_inference(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_HORIZON_BPU), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    const char mock_model[] = "HORIZON_BPU_CONTEXT_POOL_TEST";
    npu_model_handle_t handle = NULL;
    assert_int_equal(npu_load_model_from_mem(&dev, mock_model, sizeof(mock_model), &handle), 0);

    float in_data[64];
    for (int i = 0; i < 64; i++) in_data[i] = (float)i * 0.05f;

    npu_tensor_buf_t input_tensor = {
        .host_ptr = in_data, .size = sizeof(in_data), .dma_fd = -1, .is_zero_copy = true};

    float out_data[16];
    npu_tensor_buf_t output_tensor = {
        .host_ptr = out_data, .size = sizeof(out_data), .dma_fd = -1, .is_zero_copy = false};

    /* 执行 8 次推理，充分复用上下文线程池 (HORIZON_BPU_MAX_CONTEXT_POOL = 8) */
    for (int step = 0; step < 8; step++) {
        npu_inference_stats_t stats;
        assert_int_equal(
            npu_execute_inference(&dev, handle, &input_tensor, 1, &output_tensor, 1, &stats), 0);
        assert_true(stats.execution_latency_us > 0);
        assert_true(stats.throughput_fps > 0.0f);
        assert_int_equal(stats.total_inferences, (uint32_t)(step + 1));
    }

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_int_equal(metrics.device_status, NPU_STATUS_READY);
    assert_true(metrics.tops_util_pct > 0.0f);

    /* 硬件复位测试 */
    assert_int_equal(npu_reset_hardware(&dev), 0);
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_float_equal(metrics.tops_util_pct, 0.0f, 0.01f);

    assert_int_equal(npu_unload_model(&dev, handle), 0);
    assert_int_equal(npu_device_deinit(&dev), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_horizon_device_creation_and_vtable),
        cmocka_unit_test(test_horizon_lifecycle_init_deinit),
        cmocka_unit_test(test_horizon_multimodel_preloading_and_unloading),
        cmocka_unit_test(test_horizon_dmabuf_and_shm_zero_copy),
        cmocka_unit_test(test_horizon_context_pool_and_multitask_inference),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
