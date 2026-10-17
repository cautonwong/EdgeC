/**
 * @file test_ai_awq_gptq_gemm.c
 * @brief Unit tests for 4-bit AWQ/GPTQ GEMV Engine (AEP-703 / Issue #236)
 */

#include <cmocka.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include "ai_awq_gptq_gemm.h"
#include "datahub.h"
#include "edge_algo_datahub_adapter.h"

static void test_ai_awq_pack_and_fused_gemv(void **state) {
    (void)state;

    ai_awq_gptq_storage_t storage;
    memset(&storage, 0x55, sizeof(storage));

    ai_awq_gptq_engine_t *engine = ai_awq_gptq_create_in(&storage);
    assert_non_null(engine);

    /* Setup a 16 x 64 Weight Matrix */
    uint32_t N = 16;
    uint32_t K = 64;
    uint16_t group_size = 32;

    float weights_fp32[16 * 64];
    for (uint32_t i = 0; i < N * K; i++) {
        weights_fp32[i] = sinf((float)i * 0.1f) * 2.0f;
    }

    uint8_t packed_w[(16 * 64) / 2];
    float scales[(16 * 64) / 32];

    int rc = ai_awq_pack_weights(weights_fp32, N, K, group_size, AI_AWQ_FORMAT_INT4_SYMMETRIC,
                                 packed_w, scales, NULL);
    assert_int_equal(rc, 0);

    /* Construct Descriptor */
    ai_awq_weight_tensor_t tensor = {.header = {.rows_out = N,
                                                .cols_in = K,
                                                .group_size = group_size,
                                                .format = AI_AWQ_FORMAT_INT4_SYMMETRIC,
                                                .num_groups = (N * K) / group_size},
                                     .packed_weights = packed_w,
                                     .scales = scales,
                                     .zeros = NULL,
                                     .salient_channel_scales = NULL};

    /* Input vector X [1 x 64] */
    float vec_x[64];
    for (int i = 0; i < 64; i++) {
        vec_x[i] = 1.0f;
    }

    /* Compute Reference FP32 Y = X * W^T */
    float ref_y[16];
    for (uint32_t r = 0; r < N; r++) {
        float sum = 0.0f;
        for (uint32_t c = 0; c < K; c++) {
            sum += vec_x[c] * weights_fp32[r * K + c];
        }
        ref_y[r] = sum;
    }

    /* Run 4-bit Fused GEMV */
    float out_y[16];
    ai_awq_metrics_t metrics;
    rc = ai_awq_gemv_f32(engine, &tensor, vec_x, out_y, &metrics);
    assert_int_equal(rc, 0);
    assert_true(metrics.compression_ratio > 3.5f);
    assert_true(metrics.latency_ns > 0);

    /* Verify accuracy of 4-bit output vs FP32 reference */
    for (uint32_t r = 0; r < N; r++) {
        assert_true(fabsf(ref_y[r] - out_y[r]) < 2.5f);
    }
}

static void test_ai_awq_datahub_sync(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_ai_awq_hub");
    assert_int_equal(
        datahub_init(&hub, &loop, "/test_ai_awq_hub", 32, 64, NULL, 0, NULL, NULL, NULL), 0);

    ai_awq_metrics_t m = {.compression_ratio = 3.82f,
                          .gflops = 45.8f,
                          .effective_bw_gbps = 12.4f,
                          .latency_ns = 1500};
    assert_int_equal(ai_awq_sync_datahub(&m, &hub), 0);

    datahub_destroy(&hub);
    uv_loop_close(&loop);
    shm_unlink("/test_ai_awq_hub");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ai_awq_pack_and_fused_gemv),
        cmocka_unit_test(test_ai_awq_datahub_sync),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
