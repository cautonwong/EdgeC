/**
 * @file npu_hal.c
 * @brief EdgeOS NPU Hardware Abstraction Layer Dispatcher Implementation
 */

#include "npu_hal.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int npu_device_create(npu_device_t *dev, npu_backend_type_t backend) {
    if (!dev) return -1;
    memset(dev, 0, sizeof(npu_device_t));

    switch (backend) {
        case NPU_BACKEND_ASCEND_ACL:
            return npu_ascend_acl_create(dev);
        case NPU_BACKEND_ROCKCHIP_RKNN:
            return npu_rockchip_rknn_create(dev);
        case NPU_BACKEND_HORIZON_BPU:
            return npu_horizon_bpu_create(dev);
        default:
            /* Fallback or unconfigured backend */
            return -2;
    }
}

int npu_device_init(npu_device_t *dev, const npu_device_config_t *cfg) {
    if (!dev || !dev->ops || !dev->ops->init) return -1;
    if (dev->initialized) return 0;
    return dev->ops->init(dev, cfg);
}

int npu_device_deinit(npu_device_t *dev) {
    if (!dev || !dev->ops || !dev->ops->deinit) return -1;
    if (!dev->initialized) return 0;
    return dev->ops->deinit(dev);
}

int npu_load_model_from_mem(npu_device_t *dev, const void *model_data, size_t model_size, npu_model_handle_t *out_handle) {
    if (!dev || !dev->ops || !dev->ops->load_model) return -1;
    if (!dev->initialized) return -2;
    return dev->ops->load_model(dev, model_data, model_size, out_handle);
}

int npu_unload_model(npu_device_t *dev, npu_model_handle_t handle) {
    if (!dev || !dev->ops || !dev->ops->unload_model) return -1;
    if (!dev->initialized) return -2;
    return dev->ops->unload_model(dev, handle);
}

int npu_alloc_tensor_buffer(npu_device_t *dev, size_t size, uint32_t flags, npu_tensor_buf_t *out_buf) {
    if (!dev || !dev->ops || !dev->ops->alloc_tensor) return -1;
    if (!dev->initialized) return -2;
    return dev->ops->alloc_tensor(dev, size, flags, out_buf);
}

int npu_free_tensor_buffer(npu_device_t *dev, npu_tensor_buf_t *buf) {
    if (!dev || !dev->ops || !dev->ops->free_tensor) return -1;
    if (!dev->initialized) return -2;
    return dev->ops->free_tensor(dev, buf);
}

int npu_map_shm_to_tensor(npu_device_t *dev, void *shm_ptr, size_t size, int dma_fd, npu_tensor_buf_t *out_buf) {
    if (!dev || !dev->ops || !dev->ops->map_shm_zero_copy) return -1;
    if (!dev->initialized) return -2;
    return dev->ops->map_shm_zero_copy(dev, shm_ptr, size, dma_fd, out_buf);
}

int npu_execute_inference(npu_device_t *dev, npu_model_handle_t handle,
                          const npu_tensor_buf_t *inputs, size_t num_inputs,
                          npu_tensor_buf_t *outputs, size_t num_outputs,
                          npu_inference_stats_t *stats) {
    if (!dev || !dev->ops || !dev->ops->run_inference) return -1;
    if (!dev->initialized) return -2;
    return dev->ops->run_inference(dev, handle, inputs, num_inputs, outputs, num_outputs, stats);
}

int npu_get_telemetry_metrics(npu_device_t *dev, npu_metrics_t *metrics) {
    if (!dev || !dev->ops || !dev->ops->get_metrics) return -1;
    if (!dev->initialized) return -2;
    return dev->ops->get_metrics(dev, metrics);
}

int npu_reset_hardware(npu_device_t *dev) {
    if (!dev || !dev->ops || !dev->ops->reset) return -1;
    if (!dev->initialized) return -2;
    return dev->ops->reset(dev);
}
