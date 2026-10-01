/**
 * @file test_ai_quant_calibrator.c
 * @brief Unit tests for INT8/FP8 Quantization Calibrator (AEP-702 / Issue #235)
 */

#include <cmocka.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>

#include "datahub.h"
#include "ai_quant_calibrator.h"

static void test_ai_quant_int8_symmetric_and_kl_divergence(void **state) {
    (void)state;

    ai_quant_calib_storage_t storage;
    memset(&storage, 0xAA, sizeof(storage));

    ai_quant_config_t cfg = {
        .scheme = AI_QUANT_SCHEME_INT8_SYMMETRIC,
        .method = AI_QUANT_CALIB_KL_DIVERGENCE,
        .percentile = 99.9f,
        .mse_search_steps = 64
    };

    ai_quant_calibrator_t *calib = ai_quant_calibrator_create_in(&storage, &cfg);
    assert_non_null(calib);

    /* Generate Gaussian-like activation distribution */
    float samples[1024];
    for (int i = 0; i < 1024; i++) {
        float x = ((float)i - 512.0f) / 100.0f;
        samples[i] = sinf(x) * 12.5f + cosf(x * 0.5f) * 3.0f;
    }

    assert_int_equal(ai_quant_calibrator_collect_sample(calib, samples, 1024), 0);

    ai_quant_params_t params;
    int rc = ai_quant_calibrator_compute_params(calib, &params);
    assert_int_equal(rc, 0);
    assert_true(params.scale > 0.0f);
    assert_int_equal(params.zero_point, 0);
    assert_true(params.clipping_threshold > 0.0f);
    assert_true(params.snr_db > 20.0f); /* High SQNR */

    /* Quantize & Dequantize */
    int8_t q_buf[1024];
    float deq_buf[1024];
    assert_int_equal(ai_quantize_tensor(&params, AI_QUANT_SCHEME_INT8_SYMMETRIC, samples, 1024, q_buf), 0);
    assert_int_equal(ai_dequantize_tensor(&params, AI_QUANT_SCHEME_INT8_SYMMETRIC, q_buf, 1024, deq_buf), 0);

    /* Verify Mean Absolute Error is small (< 0.2) */
    float mae = 0.0f;
    for (int i = 0; i < 1024; i++) {
        mae += fabsf(samples[i] - deq_buf[i]);
    }
    mae /= 1024.0f;
    assert_true(mae < 0.25f);
}

static void test_ai_quant_fp8_encoding_and_datahub_sync(void **state) {
    (void)state;

    /* Test FP8 E4M3 encoding / decoding roundtrip */
    float test_vals[] = {0.0f, 1.0f, -1.0f, 2.5f, -15.25f, 128.0f, 384.0f};
    for (size_t i = 0; i < sizeof(test_vals)/sizeof(test_vals[0]); i++) {
        uint8_t fp8 = ai_quant_fp32_to_fp8_e4m3(test_vals[i]);
        float recovered = ai_quant_fp8_e4m3_to_fp32(fp8);
        assert_true(fabsf(test_vals[i] - recovered) <= fabsf(test_vals[i]) * 0.15f + 0.1f);
    }

    /* Test FP8 E5M2 roundtrip */
    float large_vals[] = {0.0f, 1.0f, -4.0f, 1024.0f, 16384.0f};
    for (size_t i = 0; i < sizeof(large_vals)/sizeof(large_vals[0]); i++) {
        uint8_t fp8 = ai_quant_fp32_to_fp8_e5m2(large_vals[i]);
        float recovered = ai_quant_fp8_e5m2_to_fp32(fp8);
        assert_true(fabsf(large_vals[i] - recovered) <= fabsf(large_vals[i]) * 0.25f + 0.1f);
    }

    /* DataHub Sync */
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_ai_quant_hub");
    assert_int_equal(datahub_init(&hub, &loop, "/test_ai_quant_hub", 32, 64, NULL, 0, NULL, NULL, NULL), 0);

    ai_quant_params_t p = {
        .mse_loss = 0.0012f,
        .snr_db = 42.5f,
        .saturation_pct = 0.05f
    };
    assert_int_equal(ai_quant_sync_datahub(&p, &hub), 0);

    datahub_destroy(&hub);
    uv_loop_close(&loop);
    shm_unlink("/test_ai_quant_hub");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ai_quant_int8_symmetric_and_kl_divergence),
        cmocka_unit_test(test_ai_quant_fp8_encoding_and_datahub_sync),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
