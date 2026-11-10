/**
 * @file ai_awq_gptq_gemm.h
 * @brief Edge 4-bit (AWQ/GPTQ) Compact Weight Quantization & Fused GEMV Kernel (AEP-703 / Issue #236)
 *
 * Implements 4-bit quantized matrix-vector / matrix-matrix multiplication (GEMV / GEMM):
 * - 4-bit packed weights (2 weights per byte) with group-wise scale & zero-point;
 * - Activation-Aware Weight Quantization (AWQ) salient channel protection;
 * - GPTQ Hessian-guided weight rounding error compensation;
 * - Zero-copy mmap weight stream loader without cold-start memory spikes;
 * - High-speed fused C99 SIMD-friendly dequantization + inner product kernel;
 * - Caller-allocated storage and DataHub telemetry synchronization.
 */

#ifndef EDGE_AI_AWQ_GPTQ_GEMM_H
#define EDGE_AI_AWQ_GPTQ_GEMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants & Enums                                                          */
/* -------------------------------------------------------------------------- */

#define AI_AWQ_DEFAULT_GROUP_SIZE      64
#define AI_AWQ_MAX_DIM                 4096

typedef enum {
    AI_AWQ_FORMAT_INT4_SYMMETRIC       = 0,  /**< [-8, 7] packed with per-group scale */
    AI_AWQ_FORMAT_INT4_ASYMMETRIC      = 1,  /**< [0, 15] packed with per-group scale & zero-point */
    AI_AWQ_FORMAT_GPTQ_2BIT            = 2   /**< [0, 3] 2-bit ultra-compact */
} ai_awq_weight_format_t;

/**
 * @brief Quantized Weight Matrix Header
 */
typedef struct {
    uint32_t rows_out;                 /**< Output dimensions (N) */
    uint32_t cols_in;                  /**< Input dimensions (K) */
    uint16_t group_size;               /**< Group quantization size (e.g. 32, 64, 128) */
    uint8_t format;                    /**< ai_awq_weight_format_t */
    uint8_t reserved;
    uint32_t num_groups;               /**< Total scaling groups = (N * K) / group_size */
} ai_awq_matrix_header_t;

/**
 * @brief Quantized 4-bit Matrix Descriptor
 */
typedef struct {
    ai_awq_matrix_header_t header;
    const uint8_t *packed_weights;     /**< Packed 4-bit data (N * K / 2 bytes) */
    const float *scales;               /**< Group scales (num_groups float entries) */
    const uint8_t *zeros;              /**< Group zero points (optional, num_groups/2 bytes) */
    const float *salient_channel_scales; /**< AWQ salient channel scale multipliers */
} ai_awq_weight_tensor_t;

/**
 * @brief GEMV Execution Performance Metrics
 */
typedef struct {
    float gflops;
    float compression_ratio;
    float effective_bw_gbps;
    uint64_t latency_ns;
} ai_awq_metrics_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_AWQ_GPTQ_STORAGE_SIZE       8192

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_AWQ_GPTQ_STORAGE_SIZE];
} ai_awq_gptq_storage_t;

typedef struct ai_awq_gptq_engine ai_awq_gptq_engine_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize AWQ/GPTQ inference engine in caller-allocated buffer
 */
ai_awq_gptq_engine_t *ai_awq_gptq_create_in(ai_awq_gptq_storage_t *storage);

/**
 * @brief Pack FP32 weight matrix into 4-bit AWQ representation
 *
 * @param src_fp32 Source FP32 weights [rows_out x cols_in]
 * @param rows_out Output rows (N)
 * @param cols_in Input columns (K)
 * @param group_size Elements per group (e.g., 64)
 * @param format ai_awq_weight_format_t
 * @param out_packed_buf Destination buffer for packed weights (N*K/2 bytes)
 * @param out_scales Destination buffer for scales (num_groups * sizeof(float))
 * @param out_zeros Destination buffer for zero points (num_groups/2 bytes)
 * @return 0 on success, negative error code on failure
 */
int ai_awq_pack_weights(const float *src_fp32,
                        uint32_t rows_out, uint32_t cols_in,
                        uint16_t group_size,
                        ai_awq_weight_format_t format,
                        uint8_t *out_packed_buf,
                        float *out_scales,
                        uint8_t *out_zeros);

/**
 * @brief High-Performance Fused 4-bit GEMV: Y = X * W^T
 *
 * @param engine Engine context
 * @param weight Quantized weight tensor descriptor
 * @param vec_x Input activation vector X [1 x cols_in]
 * @param out_y Output vector Y [1 x rows_out]
 * @param out_metrics Optional pointer to receive runtime execution metrics
 * @return 0 on success, negative error code on failure
 */
int ai_awq_gemv_f32(ai_awq_gptq_engine_t *engine,
                    const ai_awq_weight_tensor_t *weight,
                    const float *vec_x,
                    float *out_y,
                    ai_awq_metrics_t *out_metrics);

/**
 * @brief Memory-map 4-bit weight file directly from disk/flash (Zero cold-start RAM)
 */
int ai_awq_mmap_weights(const char *file_path,
                        ai_awq_weight_tensor_t *out_tensor,
                        void **out_mmap_handle,
                        size_t *out_mmap_size);

/**
 * @brief Unmap previously mapped weights
 */
int ai_awq_munmap_weights(void *mmap_handle, size_t mmap_size);

#include "edge_algo_port.h"

/**
 * @brief Emit AWQ execution metrics via Consumer-Defined Algo Sink
 */
int ai_awq_emit_metrics(const ai_awq_metrics_t *metrics, const edge_algo_sink_t *sink);


#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_AWQ_GPTQ_GEMM_H */
