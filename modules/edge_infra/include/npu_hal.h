/**
 * @file npu_hal.h
 * @brief EdgeOS NPU Hardware Abstraction Layer (HAL) & Heterogeneous AI Driver VTable
 *
 * Provides a standardized consumer-defined VTable interface for edge AI accelerators
 * including Huawei Ascend (ACL), Rockchip (RKNN), Horizon (BPU), NVIDIA (TensorRT),
 * and TI Sitara (C7x DSP). Supports zero-copy tensor memory mapping from POSIX SHM
 * and Linux DMA-BUF.
 */

#ifndef EDGE_NPU_HAL_H
#define EDGE_NPU_HAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants and Enumerations                                                 */
/* -------------------------------------------------------------------------- */

#define NPU_MAX_TENSOR_DIMS    8
#define NPU_MAX_MODEL_IO       16
#define NPU_NAME_MAX_LEN       64

/**
 * @brief NPU Chipset / Acceleration Backend Type
 */
typedef enum {
    NPU_BACKEND_CPU_FALLBACK   = 0,
    NPU_BACKEND_ASCEND_ACL     = 1, /* Huawei Ascend 310B / 910 ACL */
    NPU_BACKEND_ROCKCHIP_RKNN   = 2, /* Rockchip RK3588 / RV1126 RKNN */
    NPU_BACKEND_HORIZON_BPU    = 3, /* Horizon Journey / Sunrise BPU */
    NPU_BACKEND_NVIDIA_TRT     = 4, /* NVIDIA Jetson TensorRT */
    NPU_BACKEND_TI_SITARA      = 5  /* TI AM62A / TDA4 C7x DSP */
} npu_backend_type_t;

/**
 * @brief Tensor Data Types
 */
typedef enum {
    NPU_DTYPE_FLOAT32 = 0,
    NPU_DTYPE_FLOAT16 = 1,
    NPU_DTYPE_INT8    = 2,
    NPU_DTYPE_UINT8   = 3,
    NPU_DTYPE_INT32   = 4,
    NPU_DTYPE_INT64   = 5
} npu_dtype_t;

/**
 * @brief Memory Allocation and Mapping Flags
 */
#define NPU_MEM_FLAG_DEFAULT       0x00000000
#define NPU_MEM_FLAG_HUGE_FIRST    0x00000001
#define NPU_MEM_FLAG_ZERO_COPY_SHM 0x00000002
#define NPU_MEM_FLAG_DEVICE_ONLY   0x00000004
#define NPU_MEM_FLAG_UNIFIED       0x00000008

/**
 * @brief NPU Device Execution / Health Status
 */
typedef enum {
    NPU_STATUS_OFFLINE    = 0,
    NPU_STATUS_READY      = 1,
    NPU_STATUS_INFERRING  = 2,
    NPU_STATUS_THROTTLED  = 3,
    NPU_STATUS_ERROR      = 4
} npu_device_status_t;

/* -------------------------------------------------------------------------- */
/* Data Structures                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Tensor Dimension & Shape Descriptor
 */
typedef struct {
    uint32_t dims[NPU_MAX_TENSOR_DIMS];
    uint32_t num_dims;
    npu_dtype_t dtype;
    size_t byte_size;
    char name[NPU_NAME_MAX_LEN];
} npu_tensor_desc_t;

/**
 * @brief Tensor Buffer Structure with Zero-Copy DMA-BUF & SHM metadata
 */
typedef struct npu_tensor_buf {
    void *host_ptr;             /**< Host accessible virtual pointer */
    void *device_ptr;           /**< Device physical or virtual address */
    size_t size;                /**< Size in bytes */
    int dma_fd;                 /**< Linux DMA-BUF file descriptor (-1 if standard heap/SHM) */
    uint32_t flags;             /**< Memory allocation flags */
    bool is_zero_copy;          /**< True if mapped zero-copy from SHM/DMA-BUF */
    npu_tensor_desc_t desc;     /**< Tensor shape & dtype metadata */
    void *internal_handle;      /**< Backend specific handle (e.g. aclDataBuffer*) */
} npu_tensor_buf_t;

/**
 * @brief Inference Timing and Execution Statistics
 */
typedef struct {
    uint64_t start_time_us;        /**< Timestamp when inference was dispatched (us) */
    uint64_t end_time_us;          /**< Timestamp when inference completed (us) */
    uint32_t execution_latency_us; /**< Kernel execution latency (us) */
    uint32_t queue_latency_us;     /**< Queue and dispatch latency (us) */
    uint32_t total_inferences;     /**< Cumulative inference count */
    uint32_t error_count;          /**< Cumulative error count */
    float throughput_fps;          /**< Estimated real-time throughput */
} npu_inference_stats_t;

/**
 * @brief Hardware Health & Telemetry Metrics
 */
typedef struct {
    float core_temp_c;          /**< NPU core temperature in Celsius */
    float memory_util_pct;      /**< On-chip / unified memory utilization % */
    float tops_util_pct;        /**< Compute load utilization % */
    uint32_t frequency_mhz;     /**< Current operating core clock frequency in MHz */
    uint32_t zero_copy_buffers; /**< Active mapped zero-copy buffers count */
    int device_status;          /**< npu_device_status_t */
} npu_metrics_t;

/**
 * @brief Model Handle & Introspection Structure
 */
typedef struct npu_model {
    uint32_t model_id;
    char model_name[NPU_NAME_MAX_LEN];
    size_t model_size;
    uint32_t num_inputs;
    npu_tensor_desc_t input_descs[NPU_MAX_MODEL_IO];
    uint32_t num_outputs;
    npu_tensor_desc_t output_descs[NPU_MAX_MODEL_IO];
    void *backend_model_ctx;    /**< Backend specific model context (e.g. aclmdlDesc*) */
} npu_model_t;

typedef npu_model_t *npu_model_handle_t;

/* Forward declaration */
typedef struct npu_device npu_device_t;

/**
 * @brief NPU Device Configuration
 */
typedef struct {
    int device_id;              /**< Physical device index (e.g., 0) */
    npu_backend_type_t backend; /**< Backend type */
    uint32_t max_workspace_mb;  /**< Workspace memory quota in MB */
    bool enable_zero_copy;      /**< Enable zero-copy DMA-BUF / SHM mapping */
    float max_temp_throttle_c;  /**< Thermal throttle threshold in C (default: 85.0C) */
} npu_device_config_t;

/**
 * @brief NPU HAL Virtual Dispatch Table (VTable)
 */
typedef struct npu_hal_ops {
    int (*init)(npu_device_t *dev, const npu_device_config_t *cfg);
    int (*deinit)(npu_device_t *dev);
    int (*load_model)(npu_device_t *dev, const void *model_data, size_t model_size, npu_model_handle_t *out_handle);
    int (*unload_model)(npu_device_t *dev, npu_model_handle_t handle);
    int (*alloc_tensor)(npu_device_t *dev, size_t size, uint32_t flags, npu_tensor_buf_t *out_buf);
    int (*free_tensor)(npu_device_t *dev, npu_tensor_buf_t *buf);
    int (*map_shm_zero_copy)(npu_device_t *dev, void *shm_ptr, size_t size, int dma_fd, npu_tensor_buf_t *out_buf);
    int (*run_inference)(npu_device_t *dev, npu_model_handle_t handle,
                         const npu_tensor_buf_t *inputs, size_t num_inputs,
                         npu_tensor_buf_t *outputs, size_t num_outputs,
                         npu_inference_stats_t *stats);
    int (*get_metrics)(npu_device_t *dev, npu_metrics_t *metrics);
    int (*reset)(npu_device_t *dev);
} npu_hal_ops_t;

/**
 * @brief NPU Device Context
 */
struct npu_device {
    char name[NPU_NAME_MAX_LEN];
    npu_device_config_t config;
    const npu_hal_ops_t *ops;
    void *priv_data;            /**< Backend private state */
    bool initialized;
};

/* -------------------------------------------------------------------------- */
/* Factory & High Level Unified APIs                                          */
/* -------------------------------------------------------------------------- */

int npu_device_create(npu_device_t *dev, npu_backend_type_t backend);
int npu_device_init(npu_device_t *dev, const npu_device_config_t *cfg);
int npu_device_deinit(npu_device_t *dev);

int npu_load_model_from_mem(npu_device_t *dev, const void *model_data, size_t model_size, npu_model_handle_t *out_handle);
int npu_unload_model(npu_device_t *dev, npu_model_handle_t handle);
int npu_alloc_tensor_buffer(npu_device_t *dev, size_t size, uint32_t flags, npu_tensor_buf_t *out_buf);
int npu_free_tensor_buffer(npu_device_t *dev, npu_tensor_buf_t *buf);
int npu_map_shm_to_tensor(npu_device_t *dev, void *shm_ptr, size_t size, int dma_fd, npu_tensor_buf_t *out_buf);
int npu_execute_inference(npu_device_t *dev, npu_model_handle_t handle,
                          const npu_tensor_buf_t *inputs, size_t num_inputs,
                          npu_tensor_buf_t *outputs, size_t num_outputs,
                          npu_inference_stats_t *stats);
int npu_get_telemetry_metrics(npu_device_t *dev, npu_metrics_t *metrics);
int npu_reset_hardware(npu_device_t *dev);

/* Specific Backend Factory */
int npu_ascend_acl_create(npu_device_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_NPU_HAL_H */
