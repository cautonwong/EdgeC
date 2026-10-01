/**
 * @file test_ai_dynamic_tensor_pool.c
 * @brief Unit tests for Dynamic Tensor Shape Static Memory Pool (AEP-706 / Issue #239)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ai_dynamic_tensor_pool.h"

static void test_tensor_desc_calc_bytes(void **state) {
    (void)state;

    ai_tensor_desc_t desc1 = {.dtype = AI_TENSOR_DTYPE_FLOAT32, .ndim = 3, .dims = {1, 32, 64, 0}};
    /* 1 * 32 * 64 * 4 = 8192 bytes (aligned 64) */
    assert_int_equal(ai_tensor_desc_calc_bytes(&desc1), 8192);

    ai_tensor_desc_t desc_fp16 = {
        .dtype = AI_TENSOR_DTYPE_FLOAT16, .ndim = 2, .dims = {10, 20, 0, 0}};
    /* 200 * 2 = 400 -> rounded up to 448 (multiple of 64) */
    assert_int_equal(ai_tensor_desc_calc_bytes(&desc_fp16), 448);

    ai_tensor_desc_t desc_int4 = {.dtype = AI_TENSOR_DTYPE_INT4, .ndim = 1, .dims = {128, 0, 0, 0}};
    /* 128 * 0.5 = 64 bytes */
    assert_int_equal(ai_tensor_desc_calc_bytes(&desc_int4), 64);
}

static void test_dyn_pool_alloc_and_free(void **state) {
    (void)state;

    ai_dyn_pool_storage_t storage;
    ai_dyn_pool_t *pool = ai_dyn_pool_create_in(&storage, 32768);
    assert_non_null(pool);

    ai_dyn_pool_stats_t stats;
    assert_true(ai_dyn_pool_get_stats(pool, &stats));
    assert_int_equal(stats.current_allocated_bytes, 0);
    assert_int_equal(stats.active_allocations, 0);

    ai_tensor_desc_t desc_act = {.dtype = AI_TENSOR_DTYPE_FLOAT32,
                                 .ndim = 2,
                                 .dims = {4, 64, 0, 0}}; /* 4 * 64 * 4 = 1024 bytes */

    ai_dyn_tensor_t *t1 = ai_dyn_pool_alloc(pool, &desc_act, AI_POOL_SCOPE_ACTIVATION);
    assert_non_null(t1);
    assert_non_null(t1->data);
    assert_int_equal(t1->size_bytes, 1024);

    ai_tensor_desc_t desc_scratch = {
        .dtype = AI_TENSOR_DTYPE_INT8, .ndim = 2, .dims = {16, 128, 0, 0}}; /* 2048 bytes */

    ai_dyn_tensor_t *t2 = ai_dyn_pool_alloc(pool, &desc_scratch, AI_POOL_SCOPE_EPHEMERAL);
    assert_non_null(t2);
    assert_int_equal(t2->size_bytes, 2048);

    assert_true(ai_dyn_pool_get_stats(pool, &stats));
    assert_int_equal(stats.current_allocated_bytes, 3072);
    assert_int_equal(stats.active_allocations, 2);

    /* Free single ephemeral tensor */
    assert_true(ai_dyn_pool_free(pool, t2));
    assert_true(ai_dyn_pool_get_stats(pool, &stats));
    assert_int_equal(stats.current_allocated_bytes, 1024);
    assert_int_equal(stats.active_allocations, 1);

    /* Batch free scope */
    size_t freed = ai_dyn_pool_free_scope(pool, AI_POOL_SCOPE_ACTIVATION);
    assert_int_equal(freed, 1);
    assert_true(ai_dyn_pool_get_stats(pool, &stats));
    assert_int_equal(stats.current_allocated_bytes, 0);
    assert_int_equal(stats.active_allocations, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_tensor_desc_calc_bytes),
        cmocka_unit_test(test_dyn_pool_alloc_and_free),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
