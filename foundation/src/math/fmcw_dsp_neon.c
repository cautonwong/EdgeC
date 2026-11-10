#include "fmcw_dsp_neon.h"
#include <math.h>

#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#define USE_ARM_NEON 1
#else
#define USE_ARM_NEON 0
#endif

int fmcw_mti_filter_simd(const complex_f32_t *frame_prev, const complex_f32_t *frame_curr, 
                         complex_f32_t *frame_out, size_t num_samples) {
    if (!frame_prev || !frame_curr || !frame_out || num_samples == 0) {
        return -1;
    }

    size_t i = 0;

#if USE_ARM_NEON
    // Process 2 complex_f32_t numbers (4 floats: prev_real, prev_imag, prev2_real, prev2_imag) per iteration
    for (; i + 1 < num_samples; i += 2) {
        float32x4_t v_prev = vld1q_f32((const float *)&frame_prev[i]);
        float32x4_t v_curr = vld1q_f32((const float *)&frame_curr[i]);
        float32x4_t v_out  = vsubq_f32(v_curr, v_prev);
        vst1q_f32((float *)&frame_out[i], v_out);
    }
#endif

    // Remainder loop
    for (; i < num_samples; i++) {
        frame_out[i].real = frame_curr[i].real - frame_prev[i].real;
        frame_out[i].imag = frame_curr[i].imag - frame_prev[i].imag;
    }

    return 0;
}

int fmcw_rdm_power_spectrum_simd(const complex_f32_t *rdm_in, float *power_out, size_t num_samples) {
    if (!rdm_in || !power_out || num_samples == 0) {
        return -1;
    }

    size_t i = 0;

#if USE_ARM_NEON
    for (; i + 1 < num_samples; i += 2) {
        // Load 2 complex numbers: [r0, i0, r1, i1]
        float32x4_t v_cmplx = vld1q_f32((const float *)&rdm_in[i]);
        // Square each component: [r0^2, i0^2, r1^2, i1^2]
        float32x4_t v_sq = vmulq_f32(v_cmplx, v_cmplx);
        
        // Extract components and sum real^2 + imag^2
        float r0_sq = vgetq_lane_f32(v_sq, 0);
        float i0_sq = vgetq_lane_f32(v_sq, 1);
        float r1_sq = vgetq_lane_f32(v_sq, 2);
        float i1_sq = vgetq_lane_f32(v_sq, 3);

        power_out[i]     = sqrtf(r0_sq + i0_sq);
        power_out[i + 1] = sqrtf(r1_sq + i1_sq);
    }
#endif

    // Remainder loop
    for (; i < num_samples; i++) {
        float r = rdm_in[i].real;
        float im = rdm_in[i].imag;
        power_out[i] = sqrtf(r * r + im * im);
    }

    return 0;
}
