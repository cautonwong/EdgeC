/**
 * @file ai_temporal_fault_net.h
 * @brief Industrial Multi-Scale 1D-CNN + BiLSTM Operator Fusion & Fault Diagnosis Engine (AEP-704 / Issue #237)
 *
 * Implements high-throughput fused neural network operators for electrical waveform fault detection:
 * - Multi-scale parallel 1D-CNN (Kernels 3, 7, 15) for high-frequency transients, harmonics & load shifts;
 * - Fused In-place Conv + BatchNorm + GELU/ReLU + MaxPool operator;
 * - Bidirectional LSTM (BiLSTM) recurrent cell state engine with bounded memory reuse;
 * - Industrial grid fault classifier (Single-Phase Ground, Line-to-Line Short, Three-Phase, Inrush);
 * - Caller-allocated storage with compile-time assertions and DataHub telemetry synchronization.
 */

#ifndef EDGE_AI_TEMPORAL_FAULT_NET_H
#define EDGE_AI_TEMPORAL_FAULT_NET_H

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

#define AI_FAULT_WINDOW_SIZE           256   /**< 256 time steps (e.g. 5 periods @ 2.56kHz) */
#define AI_FAULT_INPUT_CHANNELS        4     /**< Va, Vb, Vc, In or Ia, Ib, Ic, In */
#define AI_FAULT_FEATURE_DIM           32
#define AI_FAULT_LSTM_HIDDEN_DIM       16
#define AI_FAULT_NUM_CLASSES           6

typedef enum {
    AI_FAULT_CLASS_NORMAL              = 0,  /**< Healthy grid steady-state */
    AI_FAULT_CLASS_SINGLE_PHASE_GROUND = 1,  /**< Single Phase-to-Ground Fault (A-G / B-G / C-G) */
    AI_FAULT_CLASS_LINE_TO_LINE_SHORT  = 2,  /**< Phase-to-Phase Short Circuit (A-B / B-C / C-A) */
    AI_FAULT_CLASS_THREE_PHASE_FAULT   = 3,  /**< Symmetrical Three-Phase Fault (A-B-C) */
    AI_FAULT_CLASS_HARMONIC_DISTORTION = 4,  /**< High THD / Power Quality Anomaly */
    AI_FAULT_CLASS_TRANSFORMER_INRUSH  = 5   /**< Transformer Magnetizing Inrush Current */
} ai_fault_class_t;

/**
 * @brief Fault Diagnosis Output Record
 */
typedef struct {
    ai_fault_class_t predicted_class;
    float confidence_pct;              /**< [0.0, 100.0] softmax confidence */
    float probabilities[AI_FAULT_NUM_CLASSES];
    float transient_energy;            /**< Feature vector L2 energy norm */
    float inference_time_us;
    uint64_t timestamp_ns;
} ai_fault_result_t;

/**
 * @brief Config for 1D-CNN + BiLSTM Model
 */
typedef struct {
    float classification_threshold;    /**< Minimum confidence to assert fault (e.g. 75.0f) */
    bool enable_bilstm;                /**< True: 1D-CNN + BiLSTM, False: 1D-CNN Only */
    float cell_state_clip;             /**< Numerical stability clip for LSTM cell state (e.g. 10.0f) */
} ai_fault_net_config_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_TEMPORAL_FAULT_STORAGE_SIZE 16384

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_TEMPORAL_FAULT_STORAGE_SIZE];
} ai_temporal_fault_storage_t;

typedef struct ai_temporal_fault_net ai_temporal_fault_net_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize Fault Diagnosis Network in caller-allocated buffer
 */
ai_temporal_fault_net_t *ai_temporal_fault_net_create_in(ai_temporal_fault_storage_t *storage,
                                                         const ai_fault_net_config_t *config);

/**
 * @brief Feed raw multi-channel waveform window and run fused inference
 *
 * @param net Network context
 * @param waveform_data Matrix of size [channels x time_steps] (4 x 256)
 * @param channels Number of channels (must be <= AI_FAULT_INPUT_CHANNELS)
 * @param time_steps Number of samples per channel (must be <= AI_FAULT_WINDOW_SIZE)
 * @param out_result Output diagnosis record
 * @return 0 on success, negative error code on failure
 */
int ai_temporal_fault_net_infer(ai_temporal_fault_net_t *net,
                                const float *waveform_data,
                                uint32_t channels,
                                uint32_t time_steps,
                                ai_fault_result_t *out_result);

#include "edge_algo_port.h"

/**
 * @brief Emit fault diagnosis metrics via Consumer-Defined Algo Sink
 */
int ai_temporal_fault_net_emit_results(const ai_fault_result_t *result, const edge_algo_sink_t *sink);


/**
 * @brief Reset temporal network recurrence state
 */
void ai_temporal_fault_net_reset(ai_temporal_fault_net_t *net);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_TEMPORAL_FAULT_NET_H */
