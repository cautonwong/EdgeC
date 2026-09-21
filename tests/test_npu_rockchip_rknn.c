/**
 * @file test_npu_rockchip_rknn.c
 * @brief Unit & Integration Tests for Rockchip RKNN Multi-Core Load Balancer & DMA-BUF Tensor Mapping (Issue #272)
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

static void test_rknn_device_creation_and_vtable(void **state) {
    (void)state;

    npu_device_t dev;
    int rc = npu_device_create(&dev, NPU_BACKEND_ROCKCHIP_RKNN);
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

    assert_string_equal(dev.name, "Rockchip-RKNN");
    assert_false(dev.initialized);
}

static void test_rknn_lifecycle_init_deinit(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ROCKCHIP_RKNN), 0);

    npu_device_config_t cfg = {
        .device_id = 0,
        .backend = NPU_BACKEND_ROCKCHIP_RKNN,
        .max_workspace_mb = 128,
        .enable_zero_copy = true,
        .max_temp_throttle_c = 85.0f
    };

    assert_int_equal(npu_device_init(&dev, &cfg), 0);
    assert_true(dev.initialized);
    assert_string_equal(dev.name, "Rockchip-RK3588-NPU-Dev0");

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_in_range(metrics.core_temp_c, 35.0f, 45.0f);
    assert_int_equal(metrics.frequency_mhz, 1000);
    assert_int_equal(metrics.device_status, NPU_STATUS_READY);
    assert_int_equal(metrics.zero_copy_buffers, 0);

    assert_int_equal(npu_device_deinit(&dev), 0);
    assert_false(dev.initialized);
}

static void test_rknn_model_load_unload(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ROCKCHIP_RKNN), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    const char mock_rknn_model[] = "RKNN_MODEL_RK3588_BMS_FAULT_DIAGNOSTICS_V2";
    npu_model_handle_t handle = NULL;

    assert_int_equal(npu_load_model_from_mem(&dev, mock_rknn_model, sizeof(mock_rknn_model), &handle), 0);
    assert_non_null(handle);

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_true(metrics.memory_util_pct > 0.0f);

    assert_int_equal(npu_unload_model(&dev, handle), 0);
    assert_int_equal(npu_device_deinit(&dev), 0);
}

static void test_rknn_dmabuf_and_shm_zero_copy_mapping(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ROCKCHIP_RKNN), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    /* 1. 普通 Tensor 内存分配 */
    npu_tensor_buf_t tensor1;
    assert_int_equal(npu_alloc_tensor_buffer(&dev, 4096, NPU_MEM_FLAG_DEFAULT, &tensor1), 0);
    assert_non_null(tensor1.host_ptr);
    assert_int_equal(tensor1.size, 4096);
    assert_false(tensor1.is_zero_copy);
    assert_int_equal(tensor1.dma_fd, -1);

    /* 2. Linux DMA-BUF 零拷贝映射 (模拟 dma_fd = 12) */
    npu_tensor_buf_t dma_tensor;
    float mock_dma_mem[64];
    for (int i = 0; i < 64; i++) mock_dma_mem[i] = (float)i * 0.5f;

    assert_int_equal(npu_map_shm_to_tensor(&dev, mock_dma_mem, sizeof(mock_dma_mem), 12, &dma_tensor), 0);
    assert_ptr_equal(dma_tensor.host_ptr, mock_dma_mem);
    assert_int_equal(dma_tensor.dma_fd, 12);
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

static void test_rknn_multicore_load_balancing_inference(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ROCKCHIP_RKNN), 0);
    assert_int_equal(npu_device_init(&dev, NULL), 0);

    const char mock_model[] = "RKNN_MODEL_LOAD_BALANCE";
    npu_model_handle_t handle = NULL;
    assert_int_equal(npu_load_model_from_mem(&dev, mock_model, sizeof(mock_model), &handle), 0);

    float in_data[32];
    for (int i = 0; i < 32; i++) in_data[i] = (float)i * 0.1f;

    npu_tensor_buf_t input_tensor = {
        .host_ptr = in_data,
        .size = sizeof(in_data),
        .dma_fd = -1,
        .is_zero_copy = true
    };

    float out_data[32];
    npu_tensor_buf_t output_tensor = {
        .host_ptr = out_data,
        .size = sizeof(out_data),
        .dma_fd = -1,
        .is_zero_copy = false
    };

    /* 执行 6 次推理，验证 3 核负载均衡多任务轮询调度 */
    for (int step = 0; step < 6; step++) {
        npu_inference_stats_t stats;
        assert_int_equal(npu_execute_inference(&dev, handle, &input_tensor, 1, &output_tensor, 1, &stats), 0);
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
        cmocka_unit_test(test_rknn_device_creation_and_vtable),
        cmocka_unit_test(test_rknn_lifecycle_init_deinit),
        cmocka_unit_test(test_rknn_model_load_unload),
        cmocka_unit_test(test_rknn_dmabuf_and_shm_zero_copy_mapping),
        cmocka_unit_test(test_rknn_multicore_load_balancing_inference),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
