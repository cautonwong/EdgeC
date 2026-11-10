#ifndef FMCW_DSP_NEON_H
#define FMCW_DSP_NEON_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float real;
    float imag;
} complex_f32_t;

/**
 * @brief 2D-FFT Range-Doppler SIMD Accelerated Complex Vector Subtraction (MTI Filter)
 */
int fmcw_mti_filter_simd(const complex_f32_t *frame_prev, const complex_f32_t *frame_curr, 
                         complex_f32_t *frame_out, size_t num_samples);

/**
 * @brief Complex Magnitude Spectrum Calculation (Range-Doppler Power Map)
 */
int fmcw_rdm_power_spectrum_simd(const complex_f32_t *rdm_in, float *power_out, size_t num_samples);

#ifdef __cplusplus
}
#endif

#endif /* FMCW_DSP_NEON_H */
