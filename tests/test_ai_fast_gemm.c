/**
 * @file test_ai_fast_gemm.c
 * @brief Unit tests for Fast Quantized GEMM Engine (AEP-705 / Issue #238)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ai_fast_gemm.h"

static void test_gemm_int8_quantized_multiplication(void **state) {
    (void)state;

    ai_fast_gemm_storage_t storage;
    ai_gemm_config_t config = {.m = 2,
                               .k = 4,
                               .n = 2,
                               .precision = AI_GEMM_PRECISION_INT8,
                               .activation = AI_GEMM_ACT_RELU,
                               .output_multiplier = 1073741824, /* 0.5 in Q31 */
                               .output_shift = 31,
                               .output_offset = 0};

    ai_fast_gemm_t *gemm = ai_fast_gemm_create_in(&storage, &config);
    assert_non_null(gemm);

    /* A [2 x 4] */
    int8_t a[8] = {10, 20, 30, 40, -10, 20, -30, 40};

    /* B [4 x 2] */
    int8_t b[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    /* Bias [2] */
    int32_t bias[2] = {100, 200};

    int8_t c[4] = {0};

    /* Compute */
    assert_int_equal(ai_fast_gemm_compute_int8(gemm, a, b, bias, c, 2, 4, 2), 0);

    /* Verify row 0 col 0:
     * dot = 10*1 + 20*3 + 30*5 + 40*7 = 10 + 60 + 150 + 280 = 500
     * acc = 500 + 100 = 600
     * scaled = 600 * 0.5 = 300 -> clamped to 127
     */
    assert_int_equal(c[0], 127);

    ai_gemm_metrics_t metrics;
    assert_true(ai_fast_gemm_get_metrics(gemm, &metrics));
    assert_true(metrics.gflops > 0.0f);
    assert_true(metrics.efficiency_pct >= 90.0f);
}

static void test_gemm_fp32_multiplication(void **state) {
    (void)state;

    ai_fast_gemm_storage_t storage;
    ai_gemm_config_t config = {.m = 2,
                               .k = 2,
                               .n = 2,
                               .precision = AI_GEMM_PRECISION_FP32,
                               .activation = AI_GEMM_ACT_NONE,
                               .output_multiplier = 0,
                               .output_shift = 0,
                               .output_offset = 0};

    ai_fast_gemm_t *gemm = ai_fast_gemm_create_in(&storage, &config);
    assert_non_null(gemm);

    float a[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    float b[4] = {0.5f, 1.5f, -0.5f, 2.5f};
    float c[4] = {0};

    assert_int_equal(ai_fast_gemm_compute_fp32(gemm, a, b, NULL, c, 2, 2, 2), 0);

    /* c[0] = 1*0.5 + 2*(-0.5) = 0.5 - 1.0 = -0.5 */
    assert_float_equal(c[0], -0.5f, 0.001f);
    /* c[1] = 1*1.5 + 2*2.5 = 1.5 + 5.0 = 6.5 */
    assert_float_equal(c[1], 6.5f, 0.001f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_gemm_int8_quantized_multiplication),
        cmocka_unit_test(test_gemm_fp32_multiplication),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
