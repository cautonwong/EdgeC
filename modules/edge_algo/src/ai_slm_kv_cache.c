/**
 * @file ai_slm_kv_cache.c
 * @brief Edge Small Language Model (SLM) Paged KV Cache Implementation (AEP-707 / Issue #240)
 */

#include "ai_slm_kv_cache.h"
#include "datahub.h"
#include <string.h>
#include <math.h>

#define MAX_LAYERS_INTERNAL     2
#define MAX_KV_HEADS_INTERNAL  2
#define MAX_HEAD_DIM_INTERNAL  32
#define MAX_PAGES_INTERNAL     16

/* Physical page block: holds AI_SLM_PAGE_TOKENS for (MAX_LAYERS x MAX_KV_HEADS x MAX_HEAD_DIM) */
typedef struct {
    float k[MAX_LAYERS_INTERNAL][AI_SLM_PAGE_TOKENS][MAX_KV_HEADS_INTERNAL * MAX_HEAD_DIM_INTERNAL];
    float v[MAX_LAYERS_INTERNAL][AI_SLM_PAGE_TOKENS][MAX_KV_HEADS_INTERNAL * MAX_HEAD_DIM_INTERNAL];
} kv_page_block_t;

struct ai_slm_kv_cache {
    ai_slm_kv_config_t config;

    uint32_t current_seq_len;
    uint32_t active_pages;
    uint32_t total_tokens_appended;
    uint32_t total_tokens_evicted;
    uint32_t attention_lookup_count;
    uint32_t attention_hit_count;

    /* Logical-to-physical page table */
    int32_t page_table[MAX_PAGES_INTERNAL];
    bool page_allocated[MAX_PAGES_INTERNAL];

    kv_page_block_t pages[MAX_PAGES_INTERNAL];
};

_Static_assert(sizeof(struct ai_slm_kv_cache) <= sizeof(ai_slm_kv_cache_storage_t),
               "ai_slm_kv_cache_storage_t size is smaller than internal struct ai_slm_kv_cache");

void ai_slm_apply_rope(float *vec, uint32_t head_dim, uint32_t position, float theta_base) {
    if (!vec || head_dim < 2) return;
    if (theta_base <= 0.0f) theta_base = 10000.0f;

    for (uint32_t i = 0; i < head_dim - 1; i += 2) {
        float freq = 1.0f / powf(theta_base, (float)i / (float)head_dim);
        float angle = (float)position * freq;
        float cos_val = cosf(angle);
        float sin_val = sinf(angle);

        float x0 = vec[i];
        float x1 = vec[i + 1];

        vec[i]     = x0 * cos_val - x1 * sin_val;
        vec[i + 1] = x0 * sin_val + x1 * cos_val;
    }
}

ai_slm_kv_cache_t *ai_slm_kv_cache_create_in(ai_slm_kv_cache_storage_t *storage,
                                             const ai_slm_kv_config_t *config) {
    if (!storage || !config) return NULL;

    struct ai_slm_kv_cache *cache = (struct ai_slm_kv_cache *)storage->raw;
    memset(cache, 0, sizeof(*cache));

    cache->config = *config;
    if (cache->config.num_layers == 0 || cache->config.num_layers > MAX_LAYERS_INTERNAL) {
        cache->config.num_layers = MAX_LAYERS_INTERNAL;
    }
    if (cache->config.num_heads == 0) cache->config.num_heads = 4;
    if (cache->config.num_kv_heads == 0 || cache->config.num_kv_heads > MAX_KV_HEADS_INTERNAL) {
        cache->config.num_kv_heads = (cache->config.num_heads < MAX_KV_HEADS_INTERNAL) ? cache->config.num_heads : MAX_KV_HEADS_INTERNAL;
    }
    if (cache->config.head_dim == 0 || cache->config.head_dim > MAX_HEAD_DIM_INTERNAL) {
        cache->config.head_dim = MAX_HEAD_DIM_INTERNAL;
    }
    if (cache->config.sliding_window_size == 0) {
        cache->config.sliding_window_size = MAX_PAGES_INTERNAL * AI_SLM_PAGE_TOKENS;
    }
    if (cache->config.rope_theta_base <= 0.0f) {
        cache->config.rope_theta_base = 10000.0f;
    }

    for (uint32_t i = 0; i < MAX_PAGES_INTERNAL; ++i) {
        cache->page_table[i] = -1;
        cache->page_allocated[i] = false;
    }

    return cache;
}

static int32_t allocate_physical_page(struct ai_slm_kv_cache *cache) {
    for (uint32_t i = 0; i < MAX_PAGES_INTERNAL; ++i) {
        if (!cache->page_allocated[i]) {
            cache->page_allocated[i] = true;
            cache->active_pages++;
            return (int32_t)i;
        }
    }
    return -1;
}

bool ai_slm_kv_cache_append(ai_slm_kv_cache_t *cache,
                            uint32_t layer_idx,
                            const float *k_vec,
                            const float *v_vec) {
    if (!cache || !k_vec || !v_vec || layer_idx >= cache->config.num_layers) {
        return false;
    }

    uint32_t logical_page = (cache->current_seq_len / AI_SLM_PAGE_TOKENS) % MAX_PAGES_INTERNAL;
    uint32_t token_offset = cache->current_seq_len % AI_SLM_PAGE_TOKENS;

    int32_t phys_page = cache->page_table[logical_page];
    if (phys_page < 0) {
        phys_page = allocate_physical_page(cache);
        if (phys_page < 0) {
            /* Evict oldest page */
            uint32_t oldest_logical = ((cache->current_seq_len / AI_SLM_PAGE_TOKENS) + 1) % MAX_PAGES_INTERNAL;
            phys_page = cache->page_table[oldest_logical];
            if (phys_page >= 0) {
                cache->page_table[oldest_logical] = -1;
                cache->total_tokens_evicted += AI_SLM_PAGE_TOKENS;
            } else {
                phys_page = 0;
            }
        }
        cache->page_table[logical_page] = phys_page;
    }

    size_t kv_dim = cache->config.num_kv_heads * cache->config.head_dim;
    memcpy(cache->pages[phys_page].k[layer_idx][token_offset], k_vec, kv_dim * sizeof(float));
    memcpy(cache->pages[phys_page].v[layer_idx][token_offset], v_vec, kv_dim * sizeof(float));

    if (layer_idx == cache->config.num_layers - 1) {
        cache->total_tokens_appended++;
    }

    return true;
}

bool ai_slm_kv_cache_compute_attention(ai_slm_kv_cache_t *cache,
                                       uint32_t layer_idx,
                                       const float *query,
                                       float *output) {
    if (!cache || !query || !output || layer_idx >= cache->config.num_layers) {
        return false;
    }

    uint32_t num_heads = cache->config.num_heads;
    uint32_t num_kv_heads = cache->config.num_kv_heads;
    uint32_t head_dim = cache->config.head_dim;
    uint32_t heads_per_kv = num_heads / num_kv_heads;
    if (heads_per_kv == 0) heads_per_kv = 1;

    float scale = 1.0f / sqrtf((float)head_dim);
    uint32_t seq_len = cache->current_seq_len + 1;
    uint32_t start_token = 0;
    if (seq_len > cache->config.sliding_window_size) {
        start_token = seq_len - cache->config.sliding_window_size;
    }

    cache->attention_lookup_count++;

    for (uint32_t h = 0; h < num_heads; ++h) {
        uint32_t kv_h = h / heads_per_kv;
        if (kv_h >= num_kv_heads) kv_h = num_kv_heads - 1;

        const float *q_head = &query[h * head_dim];
        float *out_head = &output[h * head_dim];
        memset(out_head, 0, head_dim * sizeof(float));

        float scores[AI_SLM_MAX_SEQ_LEN];
        float max_score = -1e9f;

        uint32_t count = 0;
        for (uint32_t t = start_token; t < seq_len; ++t) {
            uint32_t logical_page = (t / AI_SLM_PAGE_TOKENS) % MAX_PAGES_INTERNAL;
            uint32_t token_offset = t % AI_SLM_PAGE_TOKENS;
            int32_t phys_page = cache->page_table[logical_page];

            if (phys_page < 0) {
                scores[count] = -1e9f;
                count++;
                continue;
            }

            const float *k_head = &cache->pages[phys_page].k[layer_idx][token_offset][kv_h * head_dim];
            float dot = 0.0f;
            for (uint32_t d = 0; d < head_dim; ++d) {
                dot += q_head[d] * k_head[d];
            }
            dot *= scale;
            scores[count] = dot;
            if (dot > max_score) max_score = dot;
            count++;
        }

        /* Softmax */
        float exp_sum = 0.0f;
        for (uint32_t i = 0; i < count; ++i) {
            if (scores[i] <= -1e8f) {
                scores[i] = 0.0f;
            } else {
                scores[i] = expf(scores[i] - max_score);
                exp_sum += scores[i];
            }
        }
        if (exp_sum > 1e-9f) {
            float inv_sum = 1.0f / exp_sum;
            for (uint32_t i = 0; i < count; ++i) {
                scores[i] *= inv_sum;
            }
        }

        /* Weighted sum of V */
        uint32_t idx = 0;
        for (uint32_t t = start_token; t < seq_len; ++t, ++idx) {
            if (scores[idx] <= 0.0f) continue;
            uint32_t logical_page = (t / AI_SLM_PAGE_TOKENS) % MAX_PAGES_INTERNAL;
            uint32_t token_offset = t % AI_SLM_PAGE_TOKENS;
            int32_t phys_page = cache->page_table[logical_page];
            if (phys_page < 0) continue;

            const float *v_head = &cache->pages[phys_page].v[layer_idx][token_offset][kv_h * head_dim];
            for (uint32_t d = 0; d < head_dim; ++d) {
                out_head[d] += scores[idx] * v_head[d];
            }
        }
    }

    cache->attention_hit_count++;
    return true;
}

void ai_slm_kv_cache_advance_step(ai_slm_kv_cache_t *cache) {
    if (!cache) return;
    cache->current_seq_len++;
}

void ai_slm_kv_cache_reset(ai_slm_kv_cache_t *cache) {
    if (!cache) return;
    cache->current_seq_len = 0;
    cache->active_pages = 0;
    for (uint32_t i = 0; i < MAX_PAGES_INTERNAL; ++i) {
        cache->page_table[i] = -1;
        cache->page_allocated[i] = false;
    }
}

bool ai_slm_kv_cache_get_stats(const ai_slm_kv_cache_t *cache, ai_slm_kv_stats_t *stats) {
    if (!cache || !stats) return false;

    stats->total_pages = MAX_PAGES_INTERNAL;
    stats->active_pages = cache->active_pages;
    stats->current_seq_len = cache->current_seq_len;
    stats->total_tokens_appended = cache->total_tokens_appended;
    stats->total_tokens_evicted = cache->total_tokens_evicted;

    if (cache->attention_lookup_count > 0) {
        stats->cache_hit_rate_pct = 100.0f * ((float)cache->attention_hit_count / (float)cache->attention_lookup_count);
    } else {
        stats->cache_hit_rate_pct = 100.0f;
    }

    /* Savings compared to unbounded dense context */
    if (stats->total_tokens_appended > 0) {
        float dense_tokens = (float)stats->total_tokens_appended;
        float cached_tokens = (float)(cache->active_pages * AI_SLM_PAGE_TOKENS);
        if (dense_tokens > cached_tokens) {
            stats->memory_reduction_pct = 100.0f * (1.0f - (cached_tokens / dense_tokens));
        } else {
            stats->memory_reduction_pct = 0.0f;
        }
    } else {
        stats->memory_reduction_pct = 0.0f;
    }

    return true;
}

int ai_slm_kv_cache_sync_datahub(const ai_slm_kv_cache_t *cache, datahub_t *hub) {
    if (!cache || !hub) return -1;

    ai_slm_kv_stats_t stats;
    if (!ai_slm_kv_cache_get_stats(cache, &stats)) return -1;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 262170, (double)stats.active_pages, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, 262171, (double)stats.cache_hit_rate_pct, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}
