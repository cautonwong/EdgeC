/**
 * @file test_ai_temporal_fault_net.c
 * @brief Unit tests for Multi-Scale 1D-CNN + BiLSTM Fault Net (AEP-704 / Issue #237)
 */

#include <cmocka.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>

#include "datahub.h"
#include "ai_temporal_fault_net.h"

static void test_ai_fault_net_normal_and_transient_classification(void **state) {
    (void)state;

    ai_temporal_fault_storage_t storage;
    memset(&storage, 0x33, sizeof(storage));

    ai_fault_net_config_t cfg = {
        .classification_threshold = 60.0f,
        .enable_bilstm = true,
        .cell_state_clip = 10.0f
    };

    ai_temporal_fault_net_t *net = ai_temporal_fault_net_create_in(&storage, &cfg);
    assert_non_null(net);

    /* 1. Simulate Clean 3-Phase + Neutral Waveform (Normal state) */
    float normal_wave[4 * 256];
    for (int t = 0; t < 256; t++) {
        float theta = (float)t * 2.0f * 3.14159265f / 50.0f; /* 50 samples per cycle */
        normal_wave[0 * 256 + t] = 220.0f * sinf(theta);
        normal_wave[1 * 256 + t] = 220.0f * sinf(theta - 2.094395f);
        normal_wave[2 * 256 + t] = 220.0f * sinf(theta + 2.094395f);
        normal_wave[3 * 256 + t] = 0.0f; /* Neutral current = 0 */
    }

    ai_fault_result_t result;
    int rc = ai_temporal_fault_net_infer(net, normal_wave, 4, 256, &result);
    assert_int_equal(rc, 0);
    assert_int_equal(result.predicted_class, AI_FAULT_CLASS_NORMAL);
    assert_true(result.confidence_pct > 50.0f);
    assert_true(result.inference_time_us > 0.0f);

    /* 2. Simulate Single Phase-to-Ground Fault on Phase A (sharp transient spike & collapse) */
    float fault_wave[4 * 256];
    memcpy(fault_wave, normal_wave, sizeof(normal_wave));
    for (int t = 80; t < 200; t++) {
        fault_wave[0 * 256 + t] = 15.0f * sinf((float)t * 0.1f) + ((t % 5 == 0) ? 80.0f : 0.0f); /* High freq ground pulse */
        fault_wave[3 * 256 + t] = 120.0f * sinf((float)t * 0.1f); /* Large zero sequence / neutral current */
    }

    rc = ai_temporal_fault_net_infer(net, fault_wave, 4, 256, &result);
    assert_int_equal(rc, 0);
    assert_true(result.predicted_class == AI_FAULT_CLASS_SINGLE_PHASE_GROUND ||
                result.predicted_class == AI_FAULT_CLASS_LINE_TO_LINE_SHORT ||
                result.predicted_class == AI_FAULT_CLASS_THREE_PHASE_FAULT);
    assert_true(result.transient_energy > 0.0f);
}

static void test_ai_fault_net_datahub_sync(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    shm_unlink("/test_ai_fault_hub");
    assert_int_equal(datahub_init(&hub, &loop, "/test_ai_fault_hub", 32, 64, NULL, 0, NULL, NULL, NULL), 0);

    ai_fault_result_t res = {
        .predicted_class = AI_FAULT_CLASS_SINGLE_PHASE_GROUND,
        .confidence_pct = 94.8f,
        .inference_time_us = 45.2f
    };
    assert_int_equal(ai_temporal_fault_net_sync_datahub(&res, &hub), 0);

    datahub_destroy(&hub);
    uv_loop_close(&loop);
    shm_unlink("/test_ai_fault_hub");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ai_fault_net_normal_and_transient_classification),
        cmocka_unit_test(test_ai_fault_net_datahub_sync),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
