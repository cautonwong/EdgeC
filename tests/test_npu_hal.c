/**
 * @file test_npu_hal.c
 * @brief Comprehensive Integration & Unit Tests for EdgeOS NPU Hardware Abstraction Layer (AEP-610
 * / Issue #229)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dma_buf_pool.h"
#include "npu_hal.h"
#include "npu_self_healing.h"

static void test_npu_hal_device_creation_and_lifecycle(void **state) {
    (void)state;

    // 1. Caller-allocated 栈上预分配与后端创建
    npu_backend_storage_t storage;
    npu_device_t dev;

    // CPU Fallback 后端
    assert_int_equal(npu_device_create_in(&dev, NPU_BACKEND_CPU_FALLBACK, &storage), 0);
    npu_device_config_t cfg = {.device_id = 0,
                               .backend = NPU_BACKEND_CPU_FALLBACK,
                               .max_workspace_mb = 128,
                               .enable_zero_copy = true,
                               .max_temp_throttle_c = 85.0f};
    assert_int_equal(npu_device_init(&dev, &cfg), 0);
    assert_true(dev.initialized);
    assert_int_equal(npu_device_deinit(&dev), 0);

    // 2. 验证所有异构边缘 NPU 厂商后端的统一 VTable 创建
    npu_backend_type_t backends[] = {NPU_BACKEND_ASCEND_ACL, NPU_BACKEND_ROCKCHIP_RKNN,
                                     NPU_BACKEND_HORIZON_BPU, NPU_BACKEND_NVIDIA_TRT,
                                     NPU_BACKEND_TI_SITARA};

    for (size_t i = 0; i < sizeof(backends) / sizeof(backends[0]); i++) {
        npu_device_t npu_dev;
        assert_int_equal(npu_device_create(&npu_dev, backends[i]), 0);
        assert_non_null(npu_dev.ops);
        assert_non_null(npu_dev.ops->init);
        assert_non_null(npu_dev.ops->deinit);
        assert_non_null(npu_dev.ops->run_inference);
        cfg.backend = backends[i];
        assert_int_equal(npu_device_init(&npu_dev, &cfg), 0);
        assert_int_equal(npu_device_deinit(&npu_dev), 0);
    }
}

static void test_npu_hal_zero_copy_dma_buf_binding(void **state) {
    (void)state;

    // 1. 初始化 DMA-BUF 零拷贝内存池 (Issue #229 / AEP-610)
    dma_buf_pool_t *pool = NULL;
    dma_buf_pool_config_t pool_cfg = {
        .buffer_size = 4096, .pool_capacity = 4, .enable_auto_flush = true, .alignment = 64};
    assert_int_equal(dma_buf_pool_init(&pool, &pool_cfg), 0);
    assert_non_null(pool);

    dma_buf_desc_t *dma_desc = NULL;
    assert_int_equal(dma_buf_pool_acquire(pool, &dma_desc), 0);
    assert_non_null(dma_desc);

    // 2. 初始化 NPU 运行时
    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_ROCKCHIP_RKNN), 0);
    npu_device_config_t cfg = {.device_id = 0,
                               .backend = NPU_BACKEND_ROCKCHIP_RKNN,
                               .max_workspace_mb = 128,
                               .enable_zero_copy = true};
    assert_int_equal(npu_device_init(&dev, &cfg), 0);

    // 3. 将 DMA-BUF 文件描述符直接零拷贝绑定至 NPU 张量槽位
    npu_tensor_buf_t tensor_buf;
    int rc =
        npu_map_shm_to_tensor(&dev, dma_desc->vaddr, dma_desc->size, dma_desc->fd, &tensor_buf);
    assert_int_equal(rc, 0);

    // 严密断言：全链路零 memcpy，虚拟地址与文件描述符严格透传
    assert_ptr_equal(tensor_buf.host_ptr, dma_desc->vaddr);
    assert_int_equal(tensor_buf.dma_fd, dma_desc->fd);
    assert_int_equal(tensor_buf.size, dma_desc->size);
    assert_true(tensor_buf.flags & NPU_MEM_FLAG_ZERO_COPY_SHM);
    assert_true(tensor_buf.is_zero_copy);

    assert_int_equal(npu_device_deinit(&dev), 0);
    assert_int_equal(dma_buf_unref(pool, dma_desc), 0);
    assert_int_equal(dma_buf_pool_destroy(pool), 0);
}

static void test_npu_hal_sync_inference_and_metrics(void **state) {
    (void)state;

    npu_device_t dev;
    assert_int_equal(npu_device_create(&dev, NPU_BACKEND_CPU_FALLBACK), 0);
    npu_device_config_t cfg = {.device_id = 0,
                               .backend = NPU_BACKEND_CPU_FALLBACK,
                               .max_workspace_mb = 128,
                               .enable_zero_copy = true};
    assert_int_equal(npu_device_init(&dev, &cfg), 0);

    // 加载模型
    uint8_t dummy_model[256];
    memset(dummy_model, 0xAB, sizeof(dummy_model));
    npu_model_handle_t model_handle = NULL;
    assert_int_equal(npu_load_model_from_mem(&dev, dummy_model, sizeof(dummy_model), &model_handle),
                     0);
    assert_non_null(model_handle);

    // 分配输入输出张量
    npu_tensor_buf_t in_buf, out_buf;
    assert_int_equal(npu_alloc_tensor_buffer(&dev, 1024, NPU_MEM_FLAG_DEFAULT, &in_buf), 0);
    assert_int_equal(npu_alloc_tensor_buffer(&dev, 1024, NPU_MEM_FLAG_DEFAULT, &out_buf), 0);

    // 填充输入测试数据
    float *in_data = (float *)in_buf.host_ptr;
    for (int i = 0; i < 256; i++) {
        in_data[i] = (float)i * 0.1f;
    }

    // 同步推理验证与执行指标采集
    npu_inference_stats_t stats = {0};
    int ret_sync = npu_execute_inference(&dev, model_handle, &in_buf, 1, &out_buf, 1, &stats);
    assert_int_equal(ret_sync, 0);

    npu_metrics_t metrics;
    assert_int_equal(npu_get_telemetry_metrics(&dev, &metrics), 0);
    assert_true(metrics.tops_util_pct >= 0.0f);

    npu_free_tensor_buffer(&dev, &in_buf);
    npu_free_tensor_buffer(&dev, &out_buf);
    npu_unload_model(&dev, model_handle);
    npu_device_deinit(&dev);
}

static void test_npu_hal_self_healing_automatic_fallback(void **state) {
    (void)state;

    // 1. 创建主硬件 NPU (Ascend ACL)
    npu_device_t primary_npu;
    assert_int_equal(npu_device_create(&primary_npu, NPU_BACKEND_ASCEND_ACL), 0);
    npu_device_config_t cfg = {.device_id = 0,
                               .backend = NPU_BACKEND_ASCEND_ACL,
                               .max_workspace_mb = 256,
                               .enable_zero_copy = true,
                               .max_temp_throttle_c = 85.0f};
    assert_int_equal(npu_device_init(&primary_npu, &cfg), 0);

    uint8_t dummy_model[256] = {0};
    npu_model_handle_t model_handle = NULL;
    assert_int_equal(
        npu_load_model_from_mem(&primary_npu, dummy_model, sizeof(dummy_model), &model_handle), 0);
    assert_non_null(model_handle);

    // 2. 配置自愈守护调度器 (Self-Healing Watchdog)
    npu_self_healing_config_t heal_cfg = {.primary_device = &primary_npu,
                                          .max_temp_threshold_c = 85.0f,
                                          .recover_temp_threshold_c = 70.0f,
                                          .watchdog_timeout_ms = 50,
                                          .max_consecutive_errors = 3,
                                          .auto_reset_on_deadlock = true,
                                          .enable_cpu_fallback = true};
    npu_self_healing_t *healer = NULL;
    assert_int_equal(npu_self_healing_init(&healer, &heal_cfg), 0);
    assert_non_null(healer);

    npu_tensor_buf_t in_buf, out_buf;
    assert_int_equal(npu_alloc_tensor_buffer(&primary_npu, 256, NPU_MEM_FLAG_DEFAULT, &in_buf), 0);
    assert_int_equal(npu_alloc_tensor_buffer(&primary_npu, 256, NPU_MEM_FLAG_DEFAULT, &out_buf), 0);
    npu_inference_stats_t stats;

    // 正常状态：通过主 NPU 执行
    assert_int_equal(
        npu_self_healing_execute(healer, model_handle, &in_buf, 1, &out_buf, 1, &stats), 0);

    // 注入过温故障：触发平滑切入 CPU Fallback
    assert_int_equal(npu_self_healing_inject_fault(healer, true, false), 0);

    // 推理仍然成功完成，业务不受中断
    assert_int_equal(
        npu_self_healing_execute(healer, model_handle, &in_buf, 1, &out_buf, 1, &stats), 0);

    // 验证自愈指标中 fallback_count 递增
    npu_self_healing_metrics_t metrics;
    assert_int_equal(npu_self_healing_get_metrics(healer, &metrics), 0);
    assert_true(metrics.fallback_count > 0);

    // 清除故障并恢复
    assert_int_equal(npu_self_healing_inject_fault(healer, false, false), 0);
    npu_free_tensor_buffer(&primary_npu, &in_buf);
    npu_free_tensor_buffer(&primary_npu, &out_buf);
    npu_unload_model(&primary_npu, model_handle);
    assert_int_equal(npu_self_healing_destroy(healer), 0);
    assert_int_equal(npu_device_deinit(&primary_npu), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_npu_hal_device_creation_and_lifecycle),
        cmocka_unit_test(test_npu_hal_zero_copy_dma_buf_binding),
        cmocka_unit_test(test_npu_hal_sync_inference_and_metrics),
        cmocka_unit_test(test_npu_hal_self_healing_automatic_fallback),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
