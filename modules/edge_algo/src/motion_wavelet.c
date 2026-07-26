#include "motion_wavelet.h"
#include <math.h>
#include <string.h>

void motion_wavelet_init(motion_wavelet_t *mw) {
    if (!mw) return;
    memset(mw, 0, sizeof(motion_wavelet_t));
}

int motion_wavelet_push(motion_wavelet_t *mw, float val) {
    if (!mw) return -1;
    if (mw->count < WAVELET_WINDOW_SIZE) {
        mw->samples[mw->count++] = val;
    } else {
        memmove(&mw->samples[0], &mw->samples[1], (WAVELET_WINDOW_SIZE - 1) * sizeof(float));
        mw->samples[WAVELET_WINDOW_SIZE - 1] = val;
    }
    return 0;
}

void motion_wavelet_transform_haar(motion_wavelet_t *mw, float threshold) {
    if (!mw || mw->count < WAVELET_WINDOW_SIZE) return;

    float inv_sqrt2 = 0.70710678f;
    float detail_energy_sum = 0.0f;

    for (size_t i = 0; i < WAVELET_WINDOW_SIZE / 2; i++) {
        float s0 = mw->samples[2 * i];
        float s1 = mw->samples[2 * i + 1];

        // Haar Wavelet Transform formulas:
        // Approximation: (s0 + s1) / sqrt(2)
        // Detail:        (s0 - s1) / sqrt(2)
        mw->approx_coeff[i] = (s0 + s1) * inv_sqrt2;
        mw->detail_coeff[i] = (s0 - s1) * inv_sqrt2;

        detail_energy_sum += mw->detail_coeff[i] * mw->detail_coeff[i];
    }

    mw->detail_energy = detail_energy_sum;
    if (detail_energy_sum > threshold) {
        mw->anomaly_detected = true;
    } else {
        mw->anomaly_detected = false;
    }
}
