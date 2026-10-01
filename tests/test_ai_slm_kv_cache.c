/**
 * @file test_ai_slm_kv_cache.c
 * @brief Unit tests for Edge Small Language Model (SLM) Paged KV Cache (AEP-707 / Issue #240)
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ai_slm_kv_cache.h"

static void test_ai_slm_rope_embedding(void **state) {
    (void)state;

    float vec[4] = {1.0f, 0.0f, 1.0f, 0.0f};
    ai_slm_apply_rope(vec, 4, 0, 10000.0f);
    /* At position 0, angle is 0, cos(0)=1, sin(0)=0 -> vector unchanged */
    assert_float_equal(vec[0], 1.0f, 0.001f);
    assert_float_equal(vec[1], 0.0f, 0.001f);

    /* At position > 0, vector rotates */
    ai_slm_apply_rope(vec, 4, 5, 10000.0f);
    float norm = sqrtf(vec[0] * vec[0] + vec[1] * vec[1]);
    assert_float_equal(norm, 1.0f, 0.001f); /* Rotation preserves norm */
}

static void test_ai_slm_kv_cache_attention_pipeline(void **state) {
    (void)state;

    ai_slm_kv_cache_storage_t storage;
    ai_slm_kv_config_t config = {.num_layers = 2,
                                 .num_heads = 4,
                                 .num_kv_heads = 2,
                                 .head_dim = 16,
                                 .sliding_window_size = 32,
                                 .rope_theta_base = 10000.0f};

    ai_slm_kv_cache_t *cache = ai_slm_kv_cache_create_in(&storage, &config);
    assert_non_null(cache);

    float k_vec[32];
    float v_vec[32];
    for (int i = 0; i < 32; ++i) {
        k_vec[i] = 0.1f * (float)(i + 1);
        v_vec[i] = 0.05f * (float)(i + 1);
    }

    /* Append 20 tokens */
    for (uint32_t step = 0; step < 20; ++step) {
        assert_true(ai_slm_kv_cache_append(cache, 0, k_vec, v_vec));
        assert_true(ai_slm_kv_cache_append(cache, 1, k_vec, v_vec));
        ai_slm_kv_cache_advance_step(cache);
    }

    /* Compute attention for layer 0 */
    float query[64];
    float output[64];
    for (int i = 0; i < 64; ++i) {
        query[i] = 0.2f;
    }

    assert_true(ai_slm_kv_cache_compute_attention(cache, 0, query, output));
    /* Output should be non-zero */
    float out_sum = 0.0f;
    for (int i = 0; i < 64; ++i) {
        out_sum += fabsf(output[i]);
    }
    assert_true(out_sum > 0.0f);

    ai_slm_kv_stats_t stats;
    assert_true(ai_slm_kv_cache_get_stats(cache, &stats));
    assert_int_equal(stats.current_seq_len, 20);
    assert_true(stats.active_pages >= 2);
    assert_float_equal(stats.cache_hit_rate_pct, 100.0f, 0.1f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ai_slm_rope_embedding),
        cmocka_unit_test(test_ai_slm_kv_cache_attention_pipeline),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
