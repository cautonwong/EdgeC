#ifndef MOTION_WAVELET_H
#define MOTION_WAVELET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WAVELET_WINDOW_SIZE 64

typedef struct {
    float samples[WAVELET_WINDOW_SIZE];
    size_t count;
    float approx_coeff[WAVELET_WINDOW_SIZE / 2];
    float detail_coeff[WAVELET_WINDOW_SIZE / 2];
    float detail_energy;
    bool anomaly_detected;
} motion_wavelet_t;

void motion_wavelet_init(motion_wavelet_t *mw);

int motion_wavelet_push(motion_wavelet_t *mw, float val);

void motion_wavelet_transform_haar(motion_wavelet_t *mw, float threshold);

#ifdef __cplusplus
}
#endif

#endif // MOTION_WAVELET_H
