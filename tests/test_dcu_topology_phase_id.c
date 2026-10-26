/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_dcu_topology_phase_id.c
 * @brief Unit tests for HPLC Dual-Mode Carrier Topology & Phase Identification (AEP-461 / Issue
 * #148)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcu_topology_phase_id.h"

static void test_hplc_single_phase_identification(void) {
    printf("[TEST] Single Meter Phase Identification by Zero-Crossing Offset...\n");

    float conf = 0.0f;
    // Phase A nominal 0us (+ 50us jitter)
    dcu_phase_t ph = dcu_hplc_identify_single_phase(50.0, &conf);
    assert(ph == PHASE_A);
    assert(conf > 0.95f);

    // Phase B nominal 6666.67us (- 40us jitter)
    ph = dcu_hplc_identify_single_phase(6626.67, &conf);
    assert(ph == PHASE_B);
    assert(conf > 0.95f);

    // Phase C nominal 13333.33us (+ 80us jitter)
    ph = dcu_hplc_identify_single_phase(13413.33, &conf);
    assert(ph == PHASE_C);
    assert(conf > 0.95f);

    // Phase A wrap-around (19950us)
    ph = dcu_hplc_identify_single_phase(19950.0, &conf);
    assert(ph == PHASE_A);
    assert(conf > 0.95f);

    printf("  -> PASSED: Single meter phase identification verified.\n");
}

static void test_hplc_100_meters_batch_accuracy(void) {
    printf("[TEST] Batch Phase Identification for 100 Meters (Accuracy >= 99%%)...\n");

    const uint32_t meter_count = 100;
    dcu_hplc_meter_sample_t samples[100];
    dcu_phase_t ground_truth[100];

    for (uint32_t i = 0; i < meter_count; i++) {
        snprintf(samples[i].meter_addr, sizeof(samples[i].meter_addr), "MTR_%05u", i);
        samples[i].device_id = i;
        samples[i].branch_box_id = 1 + (i % 4);  // 4 branch boxes
        samples[i].meter_box_id = 1 + (i % 16);  // 16 meter boxes

        // Alternate phases: A, B, C
        int ph_idx = i % 3;
        double base_ref = 0.0;
        if (ph_idx == 0) {
            ground_truth[i] = PHASE_A;
            base_ref = PHASE_A_REF_US;
        } else if (ph_idx == 1) {
            ground_truth[i] = PHASE_B;
            base_ref = PHASE_B_REF_US;
        } else {
            ground_truth[i] = PHASE_C;
            base_ref = PHASE_C_REF_US;
        }

        // Add realistic industrial network jitter: +/- 150us
        double jitter = (double)((int)(i * 17) % 301) - 150.0;
        samples[i].zero_crossing_us = base_ref + jitter;
        if (samples[i].zero_crossing_us < 0.0)
            samples[i].zero_crossing_us += GRID_NOMINAL_PERIOD_US;
    }

    dcu_phase_id_summary_t summary = {0};
    int rc = dcu_hplc_solve_topology_and_phases(samples, meter_count, &summary);
    assert(rc == 0);
    assert(summary.total_meters == meter_count);

    uint32_t correct_count = 0;
    for (uint32_t i = 0; i < meter_count; i++) {
        if (samples[i].identified_phase == ground_truth[i]) {
            correct_count++;
        }
    }

    float accuracy = (float)correct_count / (float)meter_count;
    printf("  -> Identified: Phase A=%u, Phase B=%u, Phase C=%u, Unknown=%u\n",
           summary.phase_a_count, summary.phase_b_count, summary.phase_c_count,
           summary.unknown_count);
    printf("  -> Accuracy: %.2f%% (Target >= 99.00%%), Avg Confidence: %.2f%%\n", accuracy * 100.0f,
           summary.avg_confidence * 100.0f);

    assert(accuracy >= 0.99f);
    assert(summary.avg_confidence > 0.90f);

    printf("  -> PASSED: 100-meter batch phase accuracy satisfies AEP-461 requirement.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("     AEP-461: DCU HPLC Topology & Phase Identification Test     \n");
    printf("=================================================================\n");

    test_hplc_single_phase_identification();
    test_hplc_100_meters_batch_accuracy();

    printf("\nAll AEP-461 HPLC Phase Identification tests PASSED!\n");
    return 0;
}
