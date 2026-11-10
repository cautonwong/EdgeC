/**
 * @file ai_quant_calibrator.h
 * @brief INT8/FP8 Symmetric & Asymmetric Quantization Calibrator (AEP-702 / Issue #235)
 *
 * Implements industrial-grade quantization calibration for edge deep learning inference:
 * - Quantization schemes: Symmetric INT8, Asymmetric INT8 (Scale + Zero-Point), FP8 (E4M3, E5M2);
 * - Calibration algorithms: MinMax, Percentile clipping, KL Divergence (Relative Entropy minimization), MSE search;
 * - High-speed vector quantization and dequantization with saturation tracking;
 * - Caller-allocated zero-heap storage with compile-time assertions;
 * - DataHub telemetry synchronization.
 */

#ifndef EDGE_AI_QUANT_CALIBRATOR_H
#define EDGE_AI_QUANT_CALIBRATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants & Enumerations                                                   */
/* -------------------------------------------------------------------------- */

#define AI_QUANT_HISTOGRAM_BINS       2048
#define AI_QUANT_MAX_CHANNELS         512

typedef enum {
    AI_QUANT_SCHEME_INT8_SYMMETRIC     = 0,  /**< [-127, 127] with scale */
    AI_QUANT_SCHEME_INT8_ASYMMETRIC    = 1,  /**< [0, 255] with scale and zero_point */
    AI_QUANT_SCHEME_FP8_E4M3           = 2,  /**< 1 sign, 4 exponent, 3 mantissa (Max: 448.0) */
    AI_QUANT_SCHEME_FP8_E5M2           = 3   /**< 1 sign, 5 exponent, 2 mantissa (Max: 57344.0) */
} ai_quant_scheme_t;

typedef enum {
    AI_QUANT_CALIB_MINMAX              = 0,  /**< Standard absolute Min/Max range mapping */
    AI_QUANT_CALIB_PERCENTILE          = 1,  /**< Percentile clipping (e.g. 99.99%) */
    AI_QUANT_CALIB_KL_DIVERGENCE       = 2,  /**< TensorRT-style KL relative entropy minimization */
    AI_QUANT_CALIB_MSE                 = 3   /**< Mean-Squared-Error grid minimization */
} ai_quant_method_t;

/**
 * @brief Quantization Parameter Structure (Per-Tensor or Per-Channel)
 */
typedef struct {
    float scale;
    int32_t zero_point;
    float min_val;
    float max_val;
    float clipping_threshold;
    float mse_loss;
    float snr_db;
    float saturation_pct;
} ai_quant_params_t;

/**
 * @brief Calibrator Configuration
 */
typedef struct {
    ai_quant_scheme_t scheme;
    ai_quant_method_t method;
    float percentile;                  /**< For PERCENTILE method, e.g., 99.99f */
    uint32_t mse_search_steps;         /**< Grid search iterations for MSE (e.g., 64) */
    bool per_channel;                  /**< True for channel-wise, false for tensor-wise */
} ai_quant_config_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_QUANT_CALIB_STORAGE_SIZE    32768

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_QUANT_CALIB_STORAGE_SIZE];
} ai_quant_calib_storage_t;

typedef struct ai_quant_calibrator ai_quant_calibrator_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize Quantization Calibrator in caller-allocated buffer
 */
ai_quant_calibrator_t *ai_quant_calibrator_create_in(ai_quant_calib_storage_t *storage,
                                                     const ai_quant_config_t *config);

/**
 * @brief Collect activation statistics / sample tensor data
 */
int ai_quant_calibrator_collect_sample(ai_quant_calibrator_t *calib,
                                       const float *data, size_t count);

/**
 * @brief Compute optimal quantization parameters across accumulated calibration samples
 */
int ai_quant_calibrator_compute_params(ai_quant_calibrator_t *calib,
                                       ai_quant_params_t *out_params);

/**
 * @brief Quantize floating-point vector into target quantized format
 *
 * @param params Quantization parameters
 * @param scheme Target quantization scheme
 * @param src Source FP32 array
 * @param count Number of elements
 * @param dst Destination buffer (uint8_t/int8_t)
 * @return 0 on success, negative error code on failure
 */
int ai_quantize_tensor(const ai_quant_params_t *params,
                       ai_quant_scheme_t scheme,
                       const float *src, size_t count,
                       void *dst);

/**
 * @brief Dequantize quantized vector back to FP32
 */
int ai_dequantize_tensor(const ai_quant_params_t *params,
                         ai_quant_scheme_t scheme,
                         const void *src, size_t count,
                         float *dst);

/**
 * @brief Encode single FP32 to FP8 E4M3
 */
uint8_t ai_quant_fp32_to_fp8_e4m3(float val);

/**
 * @brief Decode FP8 E4M3 back to FP32
 */
float ai_quant_fp8_e4m3_to_fp32(uint8_t fp8_val);

/**
 * @brief Encode single FP32 to FP8 E5M2
 */
uint8_t ai_quant_fp32_to_fp8_e5m2(float val);

/**
 * @brief Decode FP8 E5M2 back to FP32
 */
float ai_quant_fp8_e5m2_to_fp32(uint8_t fp8_val);

#include "edge_algo_port.h"

/**
 * @brief Emit calibration metrics via Consumer-Defined Algo Sink
 */
int ai_quant_emit_params(const ai_quant_params_t *params, const edge_algo_sink_t *sink);


/**
 * @brief Reset calibrator histogram & accumulators
 */
void ai_quant_calibrator_reset(ai_quant_calibrator_t *calib);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_QUANT_CALIBRATOR_H */
