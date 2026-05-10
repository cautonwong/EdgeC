/**
 * @file edge_dsp.h
 * @brief 工业级信号处理接口 (FFT, 滤波, 窗函数)
 */

#ifndef EDGE_DSP_H
#define EDGE_DSP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- 1. 窗函数 --- */
typedef enum {
    DSP_WINDOW_RECT,
    DSP_WINDOW_HANN,
    DSP_WINDOW_HAMMING,
    DSP_WINDOW_BLACKMAN
} dsp_window_type_t;

void dsp_window_generate(dsp_window_type_t type, double *out, size_t n);

/* --- 2. 变换 --- */
/**
 * @brief 1D 实数 FFT (结果覆盖原数组，长度为 2*n)
 */
int dsp_fft_r2c(double *data, size_t n);

/**
 * @brief 2D 复数 FFT (用于雷达 Range-Doppler Map)
 */
int dsp_fft2d_c2c(double *data, size_t rows, size_t cols, bool forward);

/* --- 3. 滤波器 --- */

// FIR 滤波器
typedef struct dsp_fir_s dsp_fir_t;
dsp_fir_t* dsp_fir_create(const double *taps, size_t num_taps);
void dsp_fir_free(dsp_fir_t *f);
double dsp_fir_process(dsp_fir_t *f, double in);

// IIR 滤波器 (直接 II 型)
typedef struct dsp_iir_s dsp_iir_t;
dsp_iir_t* dsp_iir_create(uint32_t order, const double *b, const double *a);
void dsp_iir_free(dsp_iir_t *f);
double dsp_iir_process(dsp_iir_t *f, double in);

/* --- 4. 相关与卷积 --- */
int dsp_correlate(const double *a, size_t na, const double *b, size_t nb, double *res);

#ifdef __cplusplus
}
#endif

#endif
