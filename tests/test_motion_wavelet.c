#include <assert.h>
#include <stdio.h>

#include "motion_wavelet.h"

int main(void) {
    printf("[TEST] Testing Motion Wavelet Anomaly Detection Operator...\n");

    motion_wavelet_t mw;
    motion_wavelet_init(&mw);

    // Push smooth data
    for (int i = 0; i < WAVELET_WINDOW_SIZE; i++) {
        motion_wavelet_push(&mw, 10.0f);
    }
    motion_wavelet_transform_haar(&mw, 5.0f);
    assert(!mw.anomaly_detected);
    assert(mw.detail_energy == 0.0f);

    // Introduce sharp high-frequency spike
    motion_wavelet_push(&mw, 100.0f);
    motion_wavelet_transform_haar(&mw, 5.0f);

    printf("Detail Energy: %.2f, Anomaly Detected: %s\n", mw.detail_energy,
           mw.anomaly_detected ? "YES" : "NO");

    assert(mw.anomaly_detected);

    printf("[TEST] Motion Wavelet Anomaly Detection Operator PASSED!\n");
    return 0;
}
