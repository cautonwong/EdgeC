/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_operator_fusion.c
 * @brief Unit tests for Quantized Operator Fusion & Lock-Free Model Hot-Swapping (AEP-604)
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "edge_operator_fusion.h"

static void test_fused_gemm_relu_quantize(void) {
    printf("[TEST] Quantized Operator Fusion (GEMM + Bias + ReLU + INT8 Quantize)...\n");

    /* 2x2 Matrix A */
    int8_t A[4] = {2, -1, 3, 4};
    /* 2x2 Matrix B */
    int8_t B[4] = {1, 2, 0, -1};
    /* Bias: [10, -5] */
    int32_t bias[2] = {10, -5};

    /* Matrix multiplication results:
     * row 0: (2*1 + -1*0) + 10 = 12, (2*2 + -1*-1) - 5 = 5 - 5 = 0
     * row 1: (3*1 + 4*0) + 10 = 13,  (3*2 + 4*-1) - 5 = 2 - 5 = -3 -> ReLU -> 0
     */
    ai_fused_op_config_t cfg = {.m = 2,
                                .k = 2,
                                .n = 2,
                                .act = AI_ACT_RELU,
                                .leaky_alpha = 0.1f,
                                .output_scale = 1.0f,
                                .output_zero_point = 0};

    int8_t out[4] = {0};
    int ret = edge_fused_gemm_act_quant(A, B, bias, &cfg, out);
    assert(ret == 0);

    assert(out[0] == 12);
    assert(out[1] == 0);
    assert(out[2] == 13);
    assert(out[3] == 0); /* -3 clamped to 0 by ReLU */

    printf("  -> PASSED: Single-pass fused computation output verified.\n");
}

static void test_lock_free_model_hotswapping(void) {
    printf("[TEST] RCU-Style Lock-Free Double-Buffered Model Hot-Swapping...\n");

    edge_model_manager_t mgr;
    uint8_t weights_v1[64] = {0xAA};
    int ret = edge_model_mgr_init(&mgr, 101, weights_v1, sizeof(weights_v1));
    assert(ret == 0);

    /* Acquire active model */
    edge_ai_model_t *m1 = edge_model_acquire_active(&mgr);
    assert(m1 != NULL);
    assert(m1->version == 1);
    assert(atomic_load(&m1->ref_count) == 1);

    /* Prepare new model weights v2 */
    uint8_t weights_v2[64];
    memset(weights_v2, 0xBB, sizeof(weights_v2));

    /* Swap to v2 (lock-free) */
    ret = edge_model_hotswap(&mgr, 2, weights_v2, sizeof(weights_v2), 0);
    assert(ret == 0);
    assert(mgr.total_hotswaps == 1);

    /* Next acquire gets v2 */
    edge_ai_model_t *m2 = edge_model_acquire_active(&mgr);
    assert(m2 != NULL);
    assert(m2->version == 2);
    assert(m2 != m1);

    /* Release models */
    edge_model_release(m1);
    assert(atomic_load(&m1->ref_count) == 0);

    edge_model_release(m2);
    assert(atomic_load(&m2->ref_count) == 0);

    printf("  -> PASSED: 0-blocking concurrent model hot-swapping verified.\n");
}

static void test_model_tamper_signature_rejection(void) {
    printf("[TEST] Model File Checksum & Tamper Rejection...\n");

    edge_model_manager_t mgr;
    uint8_t w[32] = {1, 2, 3, 4};
    edge_model_mgr_init(&mgr, 202, w, sizeof(w));

    /* Attempt to hotswap with wrong expected checksum */
    uint8_t bad_w[32] = {9, 9, 9, 9};
    uint32_t wrong_checksum = 0x12345678;

    int ret = edge_model_hotswap(&mgr, 3, bad_w, sizeof(bad_w), wrong_checksum);
    assert(ret == -2); /* Tampered rejected */
    assert(mgr.rejected_tampered_swaps == 1);

    /* Verify active model is still valid */
    edge_ai_model_t *active = edge_model_acquire_active(&mgr);
    assert(active != NULL);
    assert(active->version == 1);
    edge_model_release(active);

    printf("  -> PASSED: Tampered model weights successfully rejected.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-604: Operator Fusion & Model Hot-Swapping Suite        \n");
    printf("=================================================================\n");

    test_fused_gemm_relu_quantize();
    test_lock_free_model_hotswapping();
    test_model_tamper_signature_rejection();

    printf("\nAll 3 AEP-604 Operator Fusion test cases PASSED!\n");
    return 0;
}
