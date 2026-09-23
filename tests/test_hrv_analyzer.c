#include <assert.h>
#include <stdio.h>

#include "hrv_analyzer.h"

int main(void) {
    printf("[TEST] Testing HRV RMSSD/SDNN Stress Index Extractor...\n");

    hrv_analyzer_t hrv;
    hrv_analyzer_init(&hrv);

    // Push 30 normal RR interval samples (~800ms = 75bpm with normal variations)
    float rr_data[] = {800, 820, 790, 815, 805, 830, 795, 810, 825, 785, 800, 810, 790, 820, 805};
    size_t n = sizeof(rr_data) / sizeof(rr_data[0]);

    for (size_t i = 0; i < n; i++) {
        hrv_analyzer_push_rr(&hrv, rr_data[i]);
    }

    hrv_analyzer_calculate(&hrv);

    printf("SDNN: %.2f ms, RMSSD: %.2f ms, Stress Index: %.1f, Alert: %s\n", hrv.sdnn_ms,
           hrv.rmssd_ms, hrv.stress_index, hrv.high_stress_alert ? "YES" : "NO");

    assert(hrv.sdnn_ms > 0.0f);
    assert(hrv.rmssd_ms > 0.0f);
    assert(!hrv.high_stress_alert);

    printf("[TEST] HRV RMSSD/SDNN Stress Index Extractor PASSED!\n");
    return 0;
}
