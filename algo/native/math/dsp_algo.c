/**
 * @file dsp_algo.c
 * @brief 工业级信号处理实现 (内置纯C + 可选GSL加速)
 */

#include "edge_dsp.h"
#ifdef USE_GSL
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_fft_complex.h>
#endif
#include <stdlib.h>
#include <math.h>
#include <string.h>

void dsp_window_generate(dsp_window_type_t type, double *out, size_t n) {
    if (n < 1) return;
    for (size_t i = 0; i < n; i++) {
        switch (type) {
            case DSP_WINDOW_RECT: out[i] = 1.0; break;
            case DSP_WINDOW_HANN: out[i] = 0.5 * (1.0 - cos(2.0 * M_PI * i / (n - 1))); break;
            case DSP_WINDOW_HAMMING: out[i] = 0.54 - 0.46 * cos(2.0 * M_PI * i / (n - 1)); break;
            case DSP_WINDOW_BLACKMAN: 
                out[i] = 0.42 - 0.5 * cos(2.0 * M_PI * i / (n - 1)) + 0.08 * cos(4.0 * M_PI * i / (n - 1)); 
                break;
        }
    }
}

#ifdef USE_GSL
int dsp_fft_r2c(double *data, size_t n) {
    gsl_fft_real_wavetable *real = gsl_fft_real_wavetable_alloc(n);
    gsl_fft_real_workspace *work = gsl_fft_real_workspace_alloc(n);

    gsl_fft_real_transform(data, 1, n, real, work);

    gsl_fft_real_wavetable_free(real);
    gsl_fft_real_workspace_free(work);
    return 0;
}
#else
int dsp_fft_r2c(double *data, size_t n) {
    (void)data; (void)n;
    return -1; /* FFT requires GSL */
}
#endif

/**
 * @brief 2D 复数 FFT 完整实现
 * 算法：对每一行做 FFT，然后对每一列做 FFT。
 * 针对 GSL 的 stride 优化，避免显式矩阵转置。
 */
int dsp_fft2d_c2c(double *data, size_t rows, size_t cols, bool forward) {
    // 1. 对每一行做 1D FFT
    for(size_t i=0; i<rows; i++) {
        double *row_ptr = &data[i * cols * 2];
        if (forward)
            gsl_fft_complex_radix2_forward(row_ptr, 1, cols);
        else
            gsl_fft_complex_radix2_backward(row_ptr, 1, cols);
    }

    // 2. 对每一列做 1D FFT
    for(size_t j=0; j<cols; j++) {
        double *col_ptr = &data[j * 2];
        if (forward)
            gsl_fft_complex_radix2_forward(col_ptr, cols, rows);
        else
            gsl_fft_complex_radix2_backward(col_ptr, cols, rows);
    }
    
    return 0;
}

int dsp_correlate(const double *a, size_t na, const double *b, size_t nb, double *res) {
    if (!a || !b || !res) return -1;
    // 结果长度 na + nb - 1
    for (size_t i = 0; i < na + nb - 1; i++) {
        res[i] = 0;
        for (size_t j = 0; j < nb; j++) {
            if (i >= j && (i - j) < na) {
                res[i] += a[i - j] * b[j];
            }
        }
    }
    return 0;
}

void dsp_s2z_bilinear(double fs, const double *s_num, const double *s_den, 
                      double *z_num, double *z_den) {
    double T = 1.0 / fs;
    double K = 2.0 / T;

    // G(s) = (b0*s + b1) / (a0*s + a1)
    // Substitute s = K*(z-1)/(z+1)
    // G(z) = (b0*K*(z-1) + b1*(z+1)) / (a0*K*(z-1) + a1*(z+1))
    // G(z) = ((b0*K+b1)*z + (b1-b0*K)) / ((a0*K+a1)*z + (a1-a0*K))
    
    double b0 = s_num[0], b1 = s_num[1];
    double a0 = s_den[0], a1 = s_den[1];

    z_num[0] = b0 * K + b1;
    z_num[1] = b1 - b0 * K;
    z_den[0] = a0 * K + a1;
    z_den[1] = a1 - a0 * K;

    // Normalize so den[0] = 1.0
    double norm = z_den[0];
    z_num[0] /= norm; z_num[1] /= norm;
    z_den[0] /= norm; z_den[1] /= norm;
}

void dsp_dwt_haar(double *data, size_t n, uint32_t level) {
    if (n < 2 || level == 0) return;
    double *tmp = malloc(n * sizeof(double));
    size_t current_n = n;

    for (uint32_t L = 0; L < level && current_n >= 2; L++) {
        size_t half = current_n / 2;
        for (size_t i = 0; i < half; i++) {
            double a = data[2 * i];
            double b = data[2 * i + 1];
            tmp[i] = (a + b) / sqrt(2.0);        // 近似分量 (Average)
            tmp[half + i] = (a - b) / sqrt(2.0); // 细节分量 (Difference)
        }
        memcpy(data, tmp, current_n * sizeof(double));
        current_n = half; // 递归分解近似分量
    }
    free(tmp);
}

struct dsp_iir_s {
    double *b, *a;
    double *w;
    uint32_t order;
};

dsp_iir_t* dsp_iir_create(uint32_t order, const double *b, const double *a) {
    dsp_iir_t *f = malloc(sizeof(dsp_iir_t));
    f->order = order;
    f->b = malloc((order + 1) * sizeof(double));
    f->a = malloc((order + 1) * sizeof(double));
    f->w = calloc(order + 1, sizeof(double));
    memcpy(f->b, b, (order + 1) * sizeof(double));
    memcpy(f->a, a, (order + 1) * sizeof(double));
    return f;
}

double dsp_iir_process(dsp_iir_t *f, double in) {
    // 直接 II 型 (Direct Form II)
    double wn = in;
    for(uint32_t i=1; i<=f->order; i++) {
        wn -= f->a[i] * f->w[i-1];
    }
    double yn = f->b[0] * wn;
    for(uint32_t i=1; i<=f->order; i++) {
        yn += f->b[i] * f->w[i-1];
    }
    // 更新延迟链
    for(uint32_t i=f->order; i>0; i--) {
        f->w[i] = f->w[i-1];
    }
    f->w[0] = wn;
    return yn;
}

void dsp_iir_free(dsp_iir_t *f) {
    if (f) {
        free(f->b); free(f->a); free(f->w);
        free(f);
    }
}
