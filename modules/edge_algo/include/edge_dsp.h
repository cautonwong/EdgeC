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

/* --- 5. 域变换 (Transformations) --- */

/**
 * @brief 双线性变换 (Bilinear Transform / Tustin)
 * 将 S 域传递函数 G(s) = (b0*s + b1) / (a0*s + a1) 转换为 Z 域
 * @param fs 采样频率
 * @param s_num [b0, b1]
 * @param s_den [a0, a1]
 * @param z_num 输出 [b0z, b1z]
 * @param z_den 输出 [a0z, a1z]
 */
void dsp_s2z_bilinear(double fs, const double *s_num, const double *s_den, 
                      double *z_num, double *z_den);

/**
 * @brief 离散小波变换 (DWT - Haar 小波)
 * @param data 输入数据 (长度必须是 2 的幂)
 * @param n 数据长度
 * @param level 分解层数
 */
void dsp_dwt_haar(double *data, size_t n, uint32_t level);

#ifdef __cplusplus
}
#endif

#endif
