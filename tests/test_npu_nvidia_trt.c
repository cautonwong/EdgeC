/**
 * @file test_npu_nvidia_trt.c
 * @brief Unit & Integration Tests for NVIDIA Jetson TensorRT & CUDA Streams Driver (AEP-744 / Issue
 * #274)
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

static void test_trt_device_creation_and_vtable(void **state) {
    (void)state;

    npu_device_t dev;
    int rc = npu_device_create(&dev, NPU_BACKEND_NVIDIA_TRT);
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

    assert_string_equal(dev.name, "NVIDIA-TensorRT");
    assert_false(dev.initialized);
}

static void test_trt_lifecycle_init_deinit(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_NVIDIA_TRT), 0);

    npu_device_config_t cfg = {.device_id = 0,
                               .backend = NPU_BACKEND_NVIDIA_TRT,
                               .max_workspace_mb = 512,
                               .enable_zero_copy = true,
                               .max_temp_throttle_c = 85.0f};

    assert_int_equal(npu_device_init(&dev, &cfg), 0);
    assert_true(dev.initialized);
    assert_string_equal(dev.name, "NVIDIA-Jetson-Orin-Dev0");

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_in_range(metrics.core_temp_c, 40.0f, 50.0f);
    assert_int_equal(metrics.frequency_mhz, 1300);
    assert_int_equal(metrics.device_status, NPU_STATUS_READY);
    assert_int_equal(metrics.zero_copy_buffers, 0);

    assert_int_equal(npu_device_deinit(&dev), 0);
    assert_false(dev.initialized);
}

static void test_trt_multimodel_load_unload(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_NVIDIA_TRT), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    const char mock_engine_a[] = "TENSORRT_ENGINE_ORIN_DEFECT_DETECTION_V1";
    const char mock_engine_b[] = "TENSORRT_ENGINE_ORIN_ACOUSTIC_ANOMALY_V2";
    npu_model_handle_t handle_a = NULL;
    npu_model_handle_t handle_b = NULL;

    assert_int_equal(npu_load_model_from_mem(&dev, mock_engine_a, sizeof(mock_engine_a), &handle_a),
                     0);
    assert_non_null(handle_a);
    assert_int_equal(npu_load_model_from_mem(&dev, mock_engine_b, sizeof(mock_engine_b), &handle_b),
                     0);
    assert_non_null(handle_b);

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_true(metrics.memory_util_pct > 0.0f);

    assert_int_equal(npu_unload_model(&dev, handle_a), 0);
    assert_int_equal(npu_unload_model(&dev, handle_b), 0);
    assert_int_equal(npu_device_deinit(&dev), 0);
}

static void test_trt_unified_memory_and_cuda_ipc_zero_copy(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_NVIDIA_TRT), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    /* 1. CUDA Unified Memory (UVA) 分配 */
    npu_tensor_buf_t tensor1;
    assert_int_equal(npu_alloc_tensor_buffer(&dev, 8192, NPU_MEM_FLAG_UNIFIED, &tensor1), 0);
    assert_non_null(tensor1.host_ptr);
    assert_ptr_equal(tensor1.host_ptr, tensor1.device_ptr);
    assert_int_equal(tensor1.size, 8192);
    assert_false(tensor1.is_zero_copy);
    assert_int_equal(tensor1.dma_fd, -1);

    /* 2. Linux DMA-BUF / CUDA IPC 零拷贝映射 (模拟 dma_fd = 33) */
    npu_tensor_buf_t dma_tensor;
    float mock_dma_mem[256];
    for (int i = 0; i < 256; i++) mock_dma_mem[i] = (float)i * 0.125f;

    assert_int_equal(
        npu_map_shm_to_tensor(&dev, mock_dma_mem, sizeof(mock_dma_mem), 33, &dma_tensor), 0);
    assert_ptr_equal(dma_tensor.host_ptr, mock_dma_mem);
    assert_int_equal(dma_tensor.dma_fd, 33);
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

static void test_trt_multistream_asynchronous_inference(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_NVIDIA_TRT), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    const char mock_model[] = "TENSORRT_MULTISTREAM_TEST";
    npu_model_handle_t handle = NULL;
    assert_int_equal(npu_load_model_from_mem(&dev, mock_model, sizeof(mock_model), &handle), 0);

    float in_data[64];
    for (int i = 0; i < 64; i++) in_data[i] = (float)i * 0.02f;

    npu_tensor_buf_t input_tensor = {
        .host_ptr = in_data, .size = sizeof(in_data), .dma_fd = -1, .is_zero_copy = true};

    float out_data[16];
    npu_tensor_buf_t output_tensor = {
        .host_ptr = out_data, .size = sizeof(out_data), .dma_fd = -1, .is_zero_copy = false};

    /* 执行 8 次推理，验证 4 个 CUDA Streams 的并发轮转调度 */
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
        cmocka_unit_test(test_trt_device_creation_and_vtable),
        cmocka_unit_test(test_trt_lifecycle_init_deinit),
        cmocka_unit_test(test_trt_multimodel_load_unload),
        cmocka_unit_test(test_trt_unified_memory_and_cuda_ipc_zero_copy),
        cmocka_unit_test(test_trt_multistream_asynchronous_inference),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
