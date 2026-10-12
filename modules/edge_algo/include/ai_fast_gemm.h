/**
 * @file ai_fast_gemm.h
 * @brief High-Speed Quantized GEMM Engine Optimized for Edge ARM/RISC-V (AEP-705 / Issue #238)
 *
 * Implements industrial-grade GEMM matrix kernel optimizations:
 * - Fixed-point INT8/INT16 and FP32 matrix multiplication with bias addition and activation fusion (ReLU/Linear);
 * - 4x4 / 2x2 register tiling and cache blocking for maximum L1/L2 memory bandwidth efficiency;
 * - Fixed-point quantized scaling with integer arithmetic right shift and saturation;
 * - Zero-heap caller-allocated storage;
 * - Throughput (GFLOPS) and cache efficiency telemetry synchronization to DataHub.
 */

#ifndef EDGE_AI_FAST_GEMM_H
#define EDGE_AI_FAST_GEMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants & Types                                                          */
/* -------------------------------------------------------------------------- */

typedef enum {
    AI_GEMM_PRECISION_INT8   = 0,  /**< Quantized INT8 with int32 accumulator & scale/shift */
    AI_GEMM_PRECISION_INT16  = 1,  /**< Quantized INT16 high precision */
    AI_GEMM_PRECISION_FP32   = 2   /**< Single precision floating point */
} ai_gemm_precision_t;

typedef enum {
    AI_GEMM_ACT_NONE = 0,
    AI_GEMM_ACT_RELU = 1,
    AI_GEMM_ACT_RELU6 = 2
} ai_gemm_activation_t;

typedef struct {
    uint32_t m;                        /**< Rows of A and C */
    uint32_t k;                        /**< Cols of A, Rows of B */
    uint32_t n;                        /**< Cols of B and C */
    ai_gemm_precision_t precision;
    ai_gemm_activation_t activation;
    int32_t output_multiplier;         /**< Fixed-point quantized multiplier (Q31) */
    int32_t output_shift;              /**< Arithmetic right shift */
    int32_t output_offset;             /**< Quantization zero-point offset */
} ai_gemm_config_t;

typedef struct {
    float gflops;
    float efficiency_pct;
    uint64_t total_ops;
    uint32_t compute_time_us;
} ai_gemm_metrics_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_FAST_GEMM_STORAGE_SIZE      16384

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_FAST_GEMM_STORAGE_SIZE];
} ai_fast_gemm_storage_t;

typedef struct ai_fast_gemm ai_fast_gemm_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize Fast GEMM engine in caller-allocated buffer
 */
ai_fast_gemm_t *ai_fast_gemm_create_in(ai_fast_gemm_storage_t *storage,
                                       const ai_gemm_config_t *config);

/**
 * @brief Execute Quantized INT8 Matrix Multiplication: C = sat( (A * B + bias) * scale >> shift + offset )
 * @param a [M x K] row-major INT8 matrix
 * @param b [K x N] row-major INT8 matrix
 * @param bias [N] optional INT32 bias vector (can be NULL)
 * @param c [M x N] row-major INT8 destination matrix
 */
int ai_fast_gemm_compute_int8(ai_fast_gemm_t *gemm,
                              const int8_t *a,
                              const int8_t *b,
                              const int32_t *bias,
                              int8_t *c,
                              uint32_t m, uint32_t k, uint32_t n);

/**
 * @brief Execute Floating Point Matrix Multiplication: C = (A * B + bias)
 */
int ai_fast_gemm_compute_fp32(ai_fast_gemm_t *gemm,
                              const float *a,
                              const float *b,
                              const float *bias,
                              float *c,
                              uint32_t m, uint32_t k, uint32_t n);

/**
 * @brief Retrieve performance metrics
 */
bool ai_fast_gemm_get_metrics(const ai_fast_gemm_t *gemm, ai_gemm_metrics_t *metrics);

/**
 * @brief Synchronize performance metrics to DataHub
 */
int ai_fast_gemm_sync_datahub(const ai_fast_gemm_t *gemm, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_FAST_GEMM_H */
