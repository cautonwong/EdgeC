/**
 * @file edge_operator_fusion.c
 * @brief Industrial Edge Quantized Operator Fusion & Lock-Free Model Hot-Swapping Implementation
 */

#include "edge_operator_fusion.h"
#include <string.h>
#include <math.h>

static uint32_t calc_checksum(const uint8_t *data, size_t len) {
    uint32_t c = 0x811c9dc5; /* FNV-1a */
    for (size_t i = 0; i < len; i++) {
        c ^= data[i];
        c *= 0x01000193;
    }
    return c;
}

int edge_model_mgr_init(edge_model_manager_t *mgr, uint32_t initial_model_id,
                        const void *initial_weights, size_t weight_size) {
    if (!mgr) return -1;
    if (weight_size > AI_MAX_WEIGHT_SIZE) return -2;

    memset(mgr, 0, sizeof(edge_model_manager_t));

    edge_ai_model_t *m0 = &mgr->model_slots[0];
    m0->model_id = initial_model_id;
    m0->version = 1;
    m0->weight_size = weight_size;
    if (initial_weights && weight_size > 0) {
        memcpy(m0->weights, initial_weights, weight_size);
        m0->checksum = calc_checksum((const uint8_t *)initial_weights, weight_size);
    }
    atomic_init(&m0->ref_count, 0);
    m0->is_valid = true;

    atomic_init(&mgr->active_idx, 0);
    return 0;
}

edge_ai_model_t *edge_model_acquire_active(edge_model_manager_t *mgr) {
    if (!mgr) return NULL;
    uint32_t idx = atomic_load_explicit(&mgr->active_idx, memory_order_acquire);
    edge_ai_model_t *model = &mgr->model_slots[idx];

    if (!model->is_valid) return NULL;

    atomic_fetch_add_explicit(&model->ref_count, 1, memory_order_relaxed);
    mgr->total_inferences++;
    return model;
}

void edge_model_release(edge_ai_model_t *model) {
    if (model) {
        atomic_fetch_sub_explicit(&model->ref_count, 1, memory_order_release);
    }
}

int edge_model_hotswap(edge_model_manager_t *mgr, uint32_t new_version,
                       const void *new_weights, size_t weight_size,
                       uint32_t expected_checksum) {
    if (!mgr || !new_weights || weight_size > AI_MAX_WEIGHT_SIZE) return -1;

    /* Verify signature / checksum before swapping */
    uint32_t computed_chk = calc_checksum((const uint8_t *)new_weights, weight_size);
    if (expected_checksum != 0 && computed_chk != expected_checksum) {
        mgr->rejected_tampered_swaps++;
        return -2; /* Tampered / Checksum Mismatch */
    }

    uint32_t curr_idx = atomic_load_explicit(&mgr->active_idx, memory_order_relaxed);
    uint32_t next_idx = 1 - curr_idx; /* Standby buffer */

    edge_ai_model_t *standby = &mgr->model_slots[next_idx];
    standby->model_id = mgr->model_slots[curr_idx].model_id;
    standby->version = new_version;
    standby->weight_size = weight_size;
    memcpy(standby->weights, new_weights, weight_size);
    standby->checksum = computed_chk;
    standby->is_valid = true;
    atomic_store_explicit(&standby->ref_count, 0, memory_order_relaxed);

    /* Atomic release switch to standby buffer */
    atomic_store_explicit(&mgr->active_idx, next_idx, memory_order_release);
    mgr->total_hotswaps++;
    return 0;
}

int edge_fused_gemm_act_quant(const int8_t *matrix_a, const int8_t *matrix_b,
                              const int32_t *bias, const ai_fused_op_config_t *cfg,
                              int8_t *out_quant_matrix) {
    if (!matrix_a || !matrix_b || !cfg || !out_quant_matrix) return -1;

    uint32_t M = cfg->m;
    uint32_t K = cfg->k;
    uint32_t N = cfg->n;

    for (uint32_t i = 0; i < M; i++) {
        for (uint32_t j = 0; j < N; j++) {
            /* 1. Accumulate GEMM in 32-bit register */
            int32_t acc = bias ? bias[j] : 0;
            for (uint32_t p = 0; p < K; p++) {
                acc += (int32_t)matrix_a[i * K + p] * (int32_t)matrix_b[p * N + j];
            }

            /* Convert to float for fused activation */
            float fval = (float)acc;

            /* 2. Fused Activation */
            switch (cfg->act) {
                case AI_ACT_RELU:
                    if (fval < 0.0f) fval = 0.0f;
                    break;
                case AI_ACT_LEAKY_RELU:
                    if (fval < 0.0f) fval *= cfg->leaky_alpha;
                    break;
                case AI_ACT_GELU: {
                    /* Approximation: 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3))) */
                    float x3 = fval * fval * fval;
                    float tanh_in = 0.79788456f * (fval + 0.044715f * x3);
                    fval = 0.5f * fval * (1.0f + tanhf(tanh_in));
                    break;
                }
                case AI_ACT_NONE:
                default:
                    break;
            }

            /* 3. Fused INT8 Quantization: q = clamp(round(f / scale) + zero_point, -128, 127) */
            float scale = (cfg->output_scale > 0.0f) ? cfg->output_scale : 1.0f;
            int32_t q = (int32_t)roundf(fval / scale) + cfg->output_zero_point;
            if (q < -128) q = -128;
            if (q > 127) q = 127;

            out_quant_matrix[i * N + j] = (int8_t)q;
        }
    }

    return 0;
}
