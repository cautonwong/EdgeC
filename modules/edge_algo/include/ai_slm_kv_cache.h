/**
 * @file ai_slm_kv_cache.h
 * @brief Edge Small Language Model (SLM) Paged KV Cache & Sliding Window Attention (AEP-707 / Issue #240)
 *
 * Implements industrial edge KV Cache management for on-device SLM inference:
 * - Paged Block KV Cache allocation with zero heap fragmentation;
 * - Sliding Window Attention (SWA) with automatic page recycling;
 * - RoPE (Rotary Position Embedding) transformation;
 * - Scaled Dot-Product Multi-Head Attention computation with causal masking;
 * - Cache hit rate, active page count, and memory savings tracking;
 * - DataHub telemetry synchronization.
 */

#ifndef EDGE_AI_SLM_KV_CACHE_H
#define EDGE_AI_SLM_KV_CACHE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

/* -------------------------------------------------------------------------- */
/* Constants & Configuration                                                  */
/* -------------------------------------------------------------------------- */

#define AI_SLM_MAX_LAYERS              8
#define AI_SLM_MAX_KV_HEADS            8
#define AI_SLM_MAX_HEAD_DIM            64
#define AI_SLM_PAGE_TOKENS             8
#define AI_SLM_MAX_PAGES               32
#define AI_SLM_MAX_SEQ_LEN             (AI_SLM_MAX_PAGES * AI_SLM_PAGE_TOKENS)

typedef struct {
    uint32_t num_layers;
    uint32_t num_heads;
    uint32_t num_kv_heads;             /**< Grouped-Query Attention (GQA) support */
    uint32_t head_dim;
    uint32_t sliding_window_size;      /**< Sliding window capacity in tokens */
    float rope_theta_base;             /**< RoPE base frequency, e.g. 10000.0f */
} ai_slm_kv_config_t;

/**
 * @brief KV Cache Performance & Resource Metrics
 */
typedef struct {
    uint32_t total_pages;
    uint32_t active_pages;
    uint32_t current_seq_len;
    uint32_t total_tokens_appended;
    uint32_t total_tokens_evicted;
    float cache_hit_rate_pct;
    float memory_reduction_pct;
} ai_slm_kv_stats_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_SLM_KV_STORAGE_SIZE         (65536 * 4)

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_SLM_KV_STORAGE_SIZE];
} ai_slm_kv_cache_storage_t;

typedef struct ai_slm_kv_cache ai_slm_kv_cache_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize SLM Paged KV Cache in caller-allocated buffer
 */
ai_slm_kv_cache_t *ai_slm_kv_cache_create_in(ai_slm_kv_cache_storage_t *storage,
                                             const ai_slm_kv_config_t *config);

/**
 * @brief Apply RoPE (Rotary Position Embedding) in-place to Q or K vector
 */
void ai_slm_apply_rope(float *vec, uint32_t head_dim, uint32_t position, float theta_base);

/**
 * @brief Append new key and value states for the current token at specified layer
 */
bool ai_slm_kv_cache_append(ai_slm_kv_cache_t *cache,
                            uint32_t layer_idx,
                            const float *k_vec,
                            const float *v_vec);

/**
 * @brief Compute Multi-Head / Grouped-Query Attention with cached KV and causal mask
 * @param query [num_heads * head_dim]
 * @param output [num_heads * head_dim]
 */
bool ai_slm_kv_cache_compute_attention(ai_slm_kv_cache_t *cache,
                                       uint32_t layer_idx,
                                       const float *query,
                                       float *output);

/**
 * @brief Advance sequence position (e.g. after generating one token)
 */
void ai_slm_kv_cache_advance_step(ai_slm_kv_cache_t *cache);

/**
 * @brief Reset KV Cache state for a new prompt/context
 */
void ai_slm_kv_cache_reset(ai_slm_kv_cache_t *cache);

/**
 * @brief Get real-time KV cache statistics
 */
bool ai_slm_kv_cache_get_stats(const ai_slm_kv_cache_t *cache, ai_slm_kv_stats_t *stats);

/**
 * @brief Synchronize KV cache telemetry to DataHub
 */
int ai_slm_kv_cache_sync_datahub(const ai_slm_kv_cache_t *cache, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_SLM_KV_CACHE_H */
