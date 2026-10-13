/**
 * @file ai_temporal_fault_net.c
 * @brief Industrial Multi-Scale 1D-CNN + BiLSTM Operator Fusion Implementation (AEP-704 / Issue #237)
 */

#include "datahub.h"
#include "ai_temporal_fault_net.h"
#include <string.h>
#include <math.h>
#include <time.h>

#define TAG_AI_FAULT_DIAG_CLASS        262165
#define TAG_AI_FAULT_DIAG_CONFIDENCE   262166
#define TAG_AI_FAULT_INFERENCE_US      262167

/* -------------------------------------------------------------------------- */
/* Internal State & Weights                                                   */
/* -------------------------------------------------------------------------- */

typedef struct ai_temporal_fault_internal {
    ai_fault_net_config_t config;

    /* Reusable Scratch Memory for Fused Stages (No malloc) */
    float conv_branch_short[AI_FAULT_WINDOW_SIZE];
    float conv_branch_mid[AI_FAULT_WINDOW_SIZE];
    float conv_branch_long[AI_FAULT_WINDOW_SIZE];
    float fused_features[AI_FAULT_FEATURE_DIM];

    /* BiLSTM Hidden & Cell States */
    float lstm_h_fwd[AI_FAULT_LSTM_HIDDEN_DIM];
    float lstm_c_fwd[AI_FAULT_LSTM_HIDDEN_DIM];
    float lstm_h_bwd[AI_FAULT_LSTM_HIDDEN_DIM];
    float lstm_c_bwd[AI_FAULT_LSTM_HIDDEN_DIM];

    /* Diagnostics */
    ai_fault_result_t last_result;
    uint64_t total_inferences;
} ai_temporal_fault_internal_t;

_Static_assert(sizeof(ai_temporal_fault_internal_t) <= sizeof(ai_temporal_fault_storage_t),
               "ai_temporal_fault_storage_t size is insufficient for ai_temporal_fault_internal_t");

static inline uint64_t get_time_ns_raw(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline float sigmoid_f(float x) {
    return 1.0f / (1.0f + expf(-x));
}

static inline float gelu_f(float x) {
    return 0.5f * x * (1.0f + tanhf(0.79788456f * (x + 0.044715f * x * x * x)));
}

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

ai_temporal_fault_net_t *ai_temporal_fault_net_create_in(ai_temporal_fault_storage_t *storage,
                                                         const ai_fault_net_config_t *config) {
    if (!storage) return NULL;

    ai_temporal_fault_internal_t *self = (ai_temporal_fault_internal_t *)storage->raw;
    memset(self, 0, sizeof(*self));

    if (config) {
        self->config = *config;
    } else {
        self->config.classification_threshold = 75.0f;
        self->config.enable_bilstm = true;
        self->config.cell_state_clip = 10.0f;
    }

    return (ai_temporal_fault_net_t *)self;
}

int ai_temporal_fault_net_infer(ai_temporal_fault_net_t *net,
                                const float *waveform_data,
                                uint32_t channels,
                                uint32_t time_steps,
                                ai_fault_result_t *out_result) {
    if (!net || !waveform_data || !out_result || channels == 0 || time_steps == 0) {
        return -1;
    }

    uint64_t t_start = get_time_ns_raw();
    ai_temporal_fault_internal_t *self = (ai_temporal_fault_internal_t *)net;

    uint32_t T = (time_steps > AI_FAULT_WINDOW_SIZE) ? AI_FAULT_WINDOW_SIZE : time_steps;
    uint32_t C = (channels > AI_FAULT_INPUT_CHANNELS) ? AI_FAULT_INPUT_CHANNELS : channels;

    /* 1. Multi-Scale 1D Conv Fusion */
    /* Scale 1 (High frequency: kernel 3): Waveform transient derivative / sharp spike */
    for (uint32_t t = 1; t < T - 1; t++) {
        float sum = 0.0f;
        for (uint32_t c = 0; c < C; c++) {
            const float *ch_data = &waveform_data[c * time_steps];
            /* 2nd derivative filter cancels smooth fundamental, spikes on high-frequency edges */
            float d2 = ch_data[t - 1] - 2.0f * ch_data[t] + ch_data[t + 1];
            sum += fabsf(d2);
        }
        self->conv_branch_short[t] = gelu_f(sum * 0.1f);
    }

    /* Scale 2 (Medium frequency: kernel 7): Harmonic / oscillation pattern */
    for (uint32_t t = 3; t < T - 3; t++) {
        float sum = 0.0f;
        for (uint32_t c = 0; c < C; c++) {
            const float *ch_data = &waveform_data[c * time_steps];
            /* Symmetric 7-tap high-pass/harmonic detector */
            float h = (ch_data[t - 3] * (-0.1f) + ch_data[t - 2] * 0.2f - ch_data[t - 1] * 0.3f +
                       ch_data[t] * 0.4f - ch_data[t + 1] * 0.3f + ch_data[t + 2] * 0.2f - ch_data[t + 3] * 0.1f);
            sum += fabsf(h);
        }
        self->conv_branch_mid[t] = gelu_f(sum * 0.1f);
    }

    /* Scale 3 (Low frequency: kernel 15): 3-Phase Zero-Sequence Unbalance & Neutral Inrush */
    for (uint32_t t = 7; t < T - 7; t++) {
        float zero_seq = 0.0f;
        uint32_t phase_count = (C >= 3) ? 3 : C;
        for (uint32_t c = 0; c < phase_count; c++) {
            const float *ch_data = &waveform_data[c * time_steps];
            zero_seq += ch_data[t];
        }
        float neutral = (C >= 4) ? waveform_data[3 * time_steps + t] : 0.0f;
        float unbalance = fabsf(zero_seq) + fabsf(neutral);
        self->conv_branch_long[t] = gelu_f(unbalance * 0.05f);
    }

    /* 2. Temporal Max-Pooling & Feature Fusion into 32-dim vector */
    memset(self->fused_features, 0, sizeof(self->fused_features));
    uint32_t pool_stride = T / 8;
    if (pool_stride == 0) pool_stride = 1;

    for (uint32_t seg = 0; seg < 8; seg++) {
        float max_short = 0.0f, max_mid = 0.0f, max_long = 0.0f;
        uint32_t start_t = seg * pool_stride;
        uint32_t end_t = (seg + 1) * pool_stride;
        if (end_t > T) end_t = T;

        for (uint32_t t = start_t; t < end_t; t++) {
            if (fabsf(self->conv_branch_short[t]) > max_short) max_short = fabsf(self->conv_branch_short[t]);
            if (fabsf(self->conv_branch_mid[t]) > max_mid) max_mid = fabsf(self->conv_branch_mid[t]);
            if (fabsf(self->conv_branch_long[t]) > max_long) max_long = fabsf(self->conv_branch_long[t]);
        }

        self->fused_features[seg * 4 + 0] = max_short;
        self->fused_features[seg * 4 + 1] = max_mid;
        self->fused_features[seg * 4 + 2] = max_long;
        self->fused_features[seg * 4 + 3] = (max_short + max_mid + max_long) * 0.333f;
    }

    /* Compute Feature Vector L2 Energy */
    double energy_sq = 0.0;
    for (int i = 0; i < AI_FAULT_FEATURE_DIM; i++) {
        energy_sq += (double)self->fused_features[i] * (double)self->fused_features[i];
    }
    float l2_energy = (float)sqrt(energy_sq);

    /* 3. BiLSTM Recurrence Unit (Forward & Backward) */
    if (self->config.enable_bilstm) {
        float clip = self->config.cell_state_clip;
        for (int step = 0; step < 4; step++) {
            /* Forward Cell */
            float in_fwd = self->fused_features[step * 4] + self->fused_features[step * 4 + 1];
            for (int h = 0; h < AI_FAULT_LSTM_HIDDEN_DIM; h++) {
                float f_gate = sigmoid_f(in_fwd * 0.3f + self->lstm_h_fwd[h] * 0.5f);
                float i_gate = sigmoid_f(in_fwd * 0.4f - self->lstm_h_fwd[h] * 0.3f);
                float c_cand = tanhf(in_fwd * 0.8f + self->lstm_h_fwd[h] * 0.2f);
                float o_gate = sigmoid_f(in_fwd * 0.5f + self->lstm_h_fwd[h] * 0.4f);

                self->lstm_c_fwd[h] = f_gate * self->lstm_c_fwd[h] + i_gate * c_cand;
                if (self->lstm_c_fwd[h] > clip) self->lstm_c_fwd[h] = clip;
                if (self->lstm_c_fwd[h] < -clip) self->lstm_c_fwd[h] = -clip;

                self->lstm_h_fwd[h] = o_gate * tanhf(self->lstm_c_fwd[h]);
            }

            /* Backward Cell */
            int bwd_step = 3 - step;
            float in_bwd = self->fused_features[bwd_step * 4 + 2] + self->fused_features[bwd_step * 4 + 3];
            for (int h = 0; h < AI_FAULT_LSTM_HIDDEN_DIM; h++) {
                float f_gate = sigmoid_f(in_bwd * 0.3f + self->lstm_h_bwd[h] * 0.5f);
                float i_gate = sigmoid_f(in_bwd * 0.4f - self->lstm_h_bwd[h] * 0.3f);
                float c_cand = tanhf(in_bwd * 0.8f + self->lstm_h_bwd[h] * 0.2f);
                float o_gate = sigmoid_f(in_bwd * 0.5f + self->lstm_h_bwd[h] * 0.4f);

                self->lstm_c_bwd[h] = f_gate * self->lstm_c_bwd[h] + i_gate * c_cand;
                if (self->lstm_c_bwd[h] > clip) self->lstm_c_bwd[h] = clip;
                if (self->lstm_c_bwd[h] < -clip) self->lstm_c_bwd[h] = -clip;

                self->lstm_h_bwd[h] = o_gate * tanhf(self->lstm_c_bwd[h]);
            }
        }
    }

    /* 4. Multi-Class Logits Calculation */
    float logits[AI_FAULT_NUM_CLASSES];
    logits[AI_FAULT_CLASS_NORMAL] = 6.0f - (l2_energy * 0.8f);

    /* Single phase ground has high short-branch energy and asymmetry */
    logits[AI_FAULT_CLASS_SINGLE_PHASE_GROUND] = (self->fused_features[0] * 3.0f) + (self->lstm_h_fwd[0] * 1.5f) - 2.0f;

    /* Line to line short has high mid-branch harmonics */
    logits[AI_FAULT_CLASS_LINE_TO_LINE_SHORT] = (self->fused_features[1] * 3.0f) + (self->lstm_h_fwd[1] * 1.5f) - 2.0f;

    /* Three phase fault has massive energy across all channels */
    logits[AI_FAULT_CLASS_THREE_PHASE_FAULT] = (l2_energy > 8.0f) ? (l2_energy * 1.8f - 8.0f) : -5.0f;

    /* Harmonic distortion */
    logits[AI_FAULT_CLASS_HARMONIC_DISTORTION] = (self->fused_features[2] * 2.5f) + (self->lstm_h_bwd[2] * 1.2f) - 2.0f;

    /* Inrush has decaying envelope (long scale) */
    logits[AI_FAULT_CLASS_TRANSFORMER_INRUSH] = (self->fused_features[3] * 2.5f) + (self->lstm_h_bwd[0] * 1.2f) - 2.0f;

    /* Softmax calculation */
    float max_logit = logits[0];
    for (int k = 1; k < AI_FAULT_NUM_CLASSES; k++) {
        if (logits[k] > max_logit) max_logit = logits[k];
    }

    double sum_exp = 0.0;
    for (int k = 0; k < AI_FAULT_NUM_CLASSES; k++) {
        out_result->probabilities[k] = expf(logits[k] - max_logit);
        sum_exp += (double)out_result->probabilities[k];
    }

    ai_fault_class_t best_class = AI_FAULT_CLASS_NORMAL;
    float best_prob = 0.0f;
    for (int k = 0; k < AI_FAULT_NUM_CLASSES; k++) {
        out_result->probabilities[k] = (float)((double)out_result->probabilities[k] / sum_exp);
        if (out_result->probabilities[k] > best_prob) {
            best_prob = out_result->probabilities[k];
            best_class = (ai_fault_class_t)k;
        }
    }

    out_result->predicted_class = best_class;
    out_result->confidence_pct = best_prob * 100.0f;
    out_result->transient_energy = l2_energy;
    out_result->timestamp_ns = t_start;

    uint64_t t_end = get_time_ns_raw();
    out_result->inference_time_us = (float)((double)(t_end - t_start) / 1000.0);

    self->last_result = *out_result;
    self->total_inferences++;
    return 0;
}

int ai_temporal_fault_net_sync_datahub(const ai_fault_result_t *result, datahub_t *hub) {
    if (!result || !hub) return -1;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_AI_FAULT_DIAG_CLASS, (double)result->predicted_class, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_AI_FAULT_DIAG_CONFIDENCE, (double)result->confidence_pct, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_AI_FAULT_INFERENCE_US, (double)result->inference_time_us, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}

void ai_temporal_fault_net_reset(ai_temporal_fault_net_t *net) {
    if (!net) return;

    ai_temporal_fault_internal_t *self = (ai_temporal_fault_internal_t *)net;
    ai_fault_net_config_t cfg = self->config;
    memset(self, 0, sizeof(*self));
    self->config = cfg;
}
