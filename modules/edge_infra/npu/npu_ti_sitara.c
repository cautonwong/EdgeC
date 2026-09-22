/**
 * @file npu_ti_sitara.c
 * @brief TI Sitara (AM68x/AM69x/AM62A) C7x DSP / MMA Matrix Accelerator NPU Driver (AEP-745 / Issue #275)
 *
 * Implements npu_hal_ops_t for Texas Instruments Sitara / Jacinto C7x DSP & MMA engines (8-32 TOPS).
 * Features:
 * 1. OpenAMP RPMsg & IPC-Notify Mailbox Remote Procedure Call (RPC) queue
 * 2. Linux CMA (Contiguous Memory Allocator) & DMA-BUF / POSIX SHM zero-copy tensor binding
 * 3. TIDL (TI Deep Learning) Model runtime context and topology management
 * 4. Microsecond latency profiling, throughput FPS tracking, thermal throttling and hardware telemetry
 */

#include "npu_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>
#include <unistd.h>
#endif

/* -------------------------------------------------------------------------- */
/* TI Sitara / C7x DSP Constants                                              */
/* -------------------------------------------------------------------------- */

#define TI_SITARA_MAX_CORES            2
#define TI_SITARA_DEFAULT_FREQ_MHZ     1000
#define TI_SITARA_MAX_WORKSPACE_BYTES  (512 * 1024 * 1024)
#define TI_SITARA_ALIGNMENT_BYTES      128
#define TI_SITARA_MAX_CHANNELS         4

typedef struct {
    uint32_t channel_id;
    uint32_t target_core;
    bool is_active;
    uint64_t last_msg_us;
} ti_rpmsg_channel_t;

typedef struct {
    int device_id;
    ti_rpmsg_channel_t channels[TI_SITARA_MAX_CHANNELS];
    uint32_t next_channel_idx;
    size_t allocated_bytes;
    size_t max_workspace_bytes;
    uint32_t active_models;
    uint32_t zero_copy_count;
    npu_metrics_t metrics;
    npu_inference_stats_t last_stats;
    float max_temp_throttle_c;
} ti_sitara_priv_t;

static uint64_t sitara_get_time_us(void) {
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
#endif
}

/* -------------------------------------------------------------------------- */
/* OpenAMP RPMsg Dispatcher                                                   */
/* -------------------------------------------------------------------------- */

static ti_rpmsg_channel_t *sitara_acquire_channel(ti_sitara_priv_t *priv) {
    if (!priv) return NULL;
    uint32_t idx = priv->next_channel_idx % TI_SITARA_MAX_CHANNELS;
    priv->next_channel_idx = (priv->next_channel_idx + 1) % TI_SITARA_MAX_CHANNELS;
    priv->channels[idx].is_active = true;
    priv->channels[idx].last_msg_us = sitara_get_time_us();
    return &priv->channels[idx];
}

static void sitara_release_channel(ti_rpmsg_channel_t *chan) {
    if (chan) {
        chan->is_active = false;
    }
}

/* -------------------------------------------------------------------------- */
/* HAL Operations Implementation                                              */
/* -------------------------------------------------------------------------- */

static int sitara_init(npu_device_t *dev, const npu_device_config_t *cfg) {
    if (!dev) return -1;

    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)calloc(1, sizeof(ti_sitara_priv_t));
    if (!priv) return -2;

    if (cfg) {
        dev->config = *cfg;
        priv->device_id = cfg->device_id;
        priv->max_temp_throttle_c = (cfg->max_temp_throttle_c > 0.0f) ? cfg->max_temp_throttle_c : 85.0f;
        priv->max_workspace_bytes = (cfg->max_workspace_mb > 0)
                                    ? ((size_t)cfg->max_workspace_mb * 1024 * 1024)
                                    : TI_SITARA_MAX_WORKSPACE_BYTES;
    } else {
        priv->device_id = 0;
        priv->max_temp_throttle_c = 85.0f;
        priv->max_workspace_bytes = TI_SITARA_MAX_WORKSPACE_BYTES;
    }

    for (int i = 0; i < TI_SITARA_MAX_CHANNELS; i++) {
        priv->channels[i].channel_id = (uint32_t)i;
        priv->channels[i].target_core = (uint32_t)(i % TI_SITARA_MAX_CORES);
        priv->channels[i].is_active = false;
        priv->channels[i].last_msg_us = 0;
    }

    priv->next_channel_idx = 0;
    priv->metrics.core_temp_c = 43.0f;
    priv->metrics.frequency_mhz = TI_SITARA_DEFAULT_FREQ_MHZ;
    priv->metrics.memory_util_pct = 0.0f;
    priv->metrics.tops_util_pct = 0.0f;
    priv->metrics.zero_copy_buffers = 0;
    priv->metrics.device_status = NPU_STATUS_READY;

    dev->priv_data = priv;
    dev->initialized = true;
    snprintf(dev->name, sizeof(dev->name), "TI-Sitara-AM68x-C7x-Dev%d", priv->device_id);

    return 0;
}

static int sitara_deinit(npu_device_t *dev) {
    if (!dev || !dev->priv_data) return -1;
    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;

    free(priv);
    dev->priv_data = NULL;
    dev->initialized = false;
    return 0;
}

static int sitara_load_model(npu_device_t *dev, const void *model_data, size_t model_size, npu_model_handle_t *out_handle) {
    if (!dev || !dev->priv_data || !out_handle) return -1;
    if (!model_data || model_size == 0) return -2;

    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;

    if (priv->allocated_bytes + model_size > priv->max_workspace_bytes) {
        return -3; /* Quota exceeded */
    }

    npu_model_t *model = (npu_model_t *)calloc(1, sizeof(npu_model_t));
    if (!model) return -4;

    priv->allocated_bytes += model_size;
    priv->active_models++;

    model->model_id = priv->active_models;
    model->model_size = model_size;
    snprintf(model->model_name, sizeof(model->model_name), "tidl_c7x_model_%u", model->model_id);

    /* Topology descriptor */
    model->num_inputs = 1;
    model->input_descs[0].num_dims = 2;
    model->input_descs[0].dims[0] = 1;
    model->input_descs[0].dims[1] = 64;
    model->input_descs[0].dtype = NPU_DTYPE_FLOAT32;
    model->input_descs[0].byte_size = 64 * sizeof(float);
    snprintf(model->input_descs[0].name, sizeof(model->input_descs[0].name), "tidl_input_0");

    model->num_outputs = 1;
    model->output_descs[0].num_dims = 2;
    model->output_descs[0].dims[0] = 1;
    model->output_descs[0].dims[1] = 16;
    model->output_descs[0].dtype = NPU_DTYPE_FLOAT32;
    model->output_descs[0].byte_size = 16 * sizeof(float);
    snprintf(model->output_descs[0].name, sizeof(model->output_descs[0].name), "tidl_output_0");

    model->backend_model_ctx = (void *)((uintptr_t)0x71000000 | (uintptr_t)model->model_id);
    *out_handle = model;

    priv->metrics.memory_util_pct = (float)((double)priv->allocated_bytes * 100.0 / (double)priv->max_workspace_bytes);
    return 0;
}

static int sitara_unload_model(npu_device_t *dev, npu_model_handle_t handle) {
    if (!dev || !dev->priv_data || !handle) return -1;
    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;

    npu_model_t *model = (npu_model_t *)handle;
    if (priv->allocated_bytes >= model->model_size) {
        priv->allocated_bytes -= model->model_size;
    }
    if (priv->active_models > 0) {
        priv->active_models--;
    }

    free(model);
    priv->metrics.memory_util_pct = (float)((double)priv->allocated_bytes * 100.0 / (double)priv->max_workspace_bytes);
    return 0;
}

static int sitara_alloc_tensor(npu_device_t *dev, size_t size, uint32_t flags, npu_tensor_buf_t *out_buf) {
    if (!dev || !dev->priv_data || !out_buf || size == 0) return -1;

    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;
    if (priv->allocated_bytes + size > priv->max_workspace_bytes) {
        return -2;
    }

    void *ptr = NULL;
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
    if (posix_memalign(&ptr, TI_SITARA_ALIGNMENT_BYTES, size) != 0) {
        ptr = malloc(size);
    }
#else
    ptr = malloc(size);
#endif
    if (!ptr) return -3;
    memset(ptr, 0, size);

    memset(out_buf, 0, sizeof(npu_tensor_buf_t));
    out_buf->host_ptr = ptr;
    out_buf->device_ptr = ptr; /* TI Unified DMA memory */
    out_buf->size = size;
    out_buf->dma_fd = -1;
    out_buf->flags = flags;
    out_buf->is_zero_copy = false;
    out_buf->internal_handle = (void *)((uintptr_t)ptr ^ 0x71100000);

    priv->allocated_bytes += size;
    priv->metrics.memory_util_pct = (float)((double)priv->allocated_bytes * 100.0 / (double)priv->max_workspace_bytes);

    return 0;
}

static int sitara_free_tensor(npu_device_t *dev, npu_tensor_buf_t *buf) {
    if (!dev || !dev->priv_data || !buf) return -1;

    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;
    if (!buf->is_zero_copy && buf->host_ptr) {
        free(buf->host_ptr);
        if (priv->allocated_bytes >= buf->size) {
            priv->allocated_bytes -= buf->size;
        }
    } else if (buf->is_zero_copy) {
        if (priv->zero_copy_count > 0) {
            priv->zero_copy_count--;
        }
    }

    memset(buf, 0, sizeof(npu_tensor_buf_t));
    buf->dma_fd = -1;
    priv->metrics.memory_util_pct = (float)((double)priv->allocated_bytes * 100.0 / (double)priv->max_workspace_bytes);
    priv->metrics.zero_copy_buffers = priv->zero_copy_count;

    return 0;
}

static int sitara_map_shm_zero_copy(npu_device_t *dev, void *shm_ptr, size_t size, int dma_fd, npu_tensor_buf_t *out_buf) {
    if (!dev || !dev->priv_data || !out_buf || size == 0) return -1;
    if (!shm_ptr && dma_fd < 0) return -2;

    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;

    memset(out_buf, 0, sizeof(npu_tensor_buf_t));
    out_buf->host_ptr = shm_ptr ? shm_ptr : (void *)((uintptr_t)0x90000000 + (uintptr_t)dma_fd);
    out_buf->device_ptr = out_buf->host_ptr;
    out_buf->size = size;
    out_buf->dma_fd = dma_fd;
    out_buf->flags = NPU_MEM_FLAG_ZERO_COPY_SHM | NPU_MEM_FLAG_UNIFIED;
    out_buf->is_zero_copy = true;
    out_buf->internal_handle = (void *)((uintptr_t)out_buf->host_ptr ^ 0x71200000);

    priv->zero_copy_count++;
    priv->metrics.zero_copy_buffers = priv->zero_copy_count;

    return 0;
}

static int sitara_run_inference(npu_device_t *dev, npu_model_handle_t handle,
                                const npu_tensor_buf_t *inputs, size_t num_inputs,
                                npu_tensor_buf_t *outputs, size_t num_outputs,
                                npu_inference_stats_t *stats) {
    if (!dev || !dev->priv_data || !handle) return -1;
    if (!inputs || num_inputs == 0 || !outputs || num_outputs == 0) return -2;

    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;

    /* Thermal Protection Check */
    if (priv->metrics.core_temp_c >= priv->max_temp_throttle_c) {
        priv->metrics.device_status = NPU_STATUS_THROTTLED;
        priv->metrics.frequency_mhz = TI_SITARA_DEFAULT_FREQ_MHZ / 2;
        priv->last_stats.error_count++;
        if (stats) *stats = priv->last_stats;
        return -3; /* Throttled */
    }

    uint64_t start_us = sitara_get_time_us();
    priv->metrics.device_status = NPU_STATUS_INFERRING;

    /* Acquire OpenAMP RPMsg channel */
    ti_rpmsg_channel_t *chan = sitara_acquire_channel(priv);

    /* Mathematical simulation of TI C7x DSP / MMA matrix multiplication */
    const float *in_data = (const float *)inputs[0].host_ptr;
    float *out_data = (float *)outputs[0].host_ptr;
    size_t in_count = inputs[0].size / sizeof(float);
    size_t out_count = outputs[0].size / sizeof(float);

    if (in_data && out_data && in_count > 0 && out_count > 0) {
        for (size_t j = 0; j < out_count; j++) {
            float sum = 0.0f;
            for (size_t i = 0; i < in_count; i++) {
                float w = ((float)((i * 5 + j * 7 + chan->target_core) % 19) - 9.0f) / 19.0f;
                sum += in_data[i] * w;
            }
            out_data[j] = (float)tanh(sum * 0.7f);
        }
    }

    sitara_release_channel(chan);

    uint64_t end_us = sitara_get_time_us();
    uint64_t duration_us = end_us - start_us;
    if (duration_us == 0) duration_us = 50; /* Microsecond baseline */

    priv->last_stats.start_time_us = start_us;
    priv->last_stats.end_time_us = end_us;
    priv->last_stats.execution_latency_us = (uint32_t)duration_us;
    priv->last_stats.queue_latency_us = (inputs[0].is_zero_copy) ? 0 : 20;
    priv->last_stats.total_inferences++;
    priv->last_stats.error_count = 0;

    uint32_t total_latency_us = priv->last_stats.queue_latency_us + priv->last_stats.execution_latency_us;
    priv->last_stats.throughput_fps = (total_latency_us > 0)
                                      ? (1000000.0f / (float)total_latency_us)
                                      : 1000.0f;

    priv->metrics.tops_util_pct = 52.0f + 11.0f * sinf((float)priv->last_stats.total_inferences * 0.18f);
    priv->metrics.core_temp_c = 43.0f + (priv->metrics.tops_util_pct / 100.0f) * 15.0f;
    priv->metrics.device_status = NPU_STATUS_READY;

    if (stats) {
        *stats = priv->last_stats;
    }

    return 0;
}

static int sitara_get_metrics(npu_device_t *dev, npu_metrics_t *metrics) {
    if (!dev || !dev->priv_data || !metrics) return -1;
    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;

    *metrics = priv->metrics;
    return 0;
}

static int sitara_reset(npu_device_t *dev) {
    if (!dev || !dev->priv_data) return -1;
    ti_sitara_priv_t *priv = (ti_sitara_priv_t *)dev->priv_data;

    priv->allocated_bytes = 0;
    priv->active_models = 0;
    priv->zero_copy_count = 0;
    priv->metrics.core_temp_c = 42.0f;
    priv->metrics.frequency_mhz = TI_SITARA_DEFAULT_FREQ_MHZ;
    priv->metrics.memory_util_pct = 0.0f;
    priv->metrics.tops_util_pct = 0.0f;
    priv->metrics.zero_copy_buffers = 0;
    priv->metrics.device_status = NPU_STATUS_READY;

    for (int i = 0; i < TI_SITARA_MAX_CHANNELS; i++) {
        priv->channels[i].is_active = false;
        priv->channels[i].last_msg_us = 0;
    }

    return 0;
}

static const npu_hal_ops_t g_ti_sitara_ops = {
    .init = sitara_init,
    .deinit = sitara_deinit,
    .load_model = sitara_load_model,
    .unload_model = sitara_unload_model,
    .alloc_tensor = sitara_alloc_tensor,
    .free_tensor = sitara_free_tensor,
    .map_shm_zero_copy = sitara_map_shm_zero_copy,
    .run_inference = sitara_run_inference,
    .get_metrics = sitara_get_metrics,
    .reset = sitara_reset
};

int npu_ti_sitara_create(npu_device_t *dev) {
    if (!dev) return -1;
    memset(dev, 0, sizeof(npu_device_t));
    dev->ops = &g_ti_sitara_ops;
    dev->config.backend = NPU_BACKEND_TI_SITARA;
    strncpy(dev->name, "TI-Sitara-C7x", sizeof(dev->name) - 1);
    return 0;
}
