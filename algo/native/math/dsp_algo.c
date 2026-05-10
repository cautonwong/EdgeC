/**
 * @file dsp_algo.c
 * @brief 工业级信号处理实现 (GSL 加速版)
 */

#include "edge_dsp.h"
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_fft_complex.h>
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

int dsp_fft_r2c(double *data, size_t n) {
    gsl_fft_real_wavetable *real = gsl_fft_real_wavetable_alloc(n);
    gsl_fft_real_workspace *work = gsl_fft_real_workspace_alloc(n);
    
    gsl_fft_real_transform(data, 1, n, real, work);
    
    gsl_fft_real_wavetable_free(real);
    gsl_fft_real_workspace_free(work);
    return 0;
}

int dsp_fft2d_c2c(double *data, size_t rows, size_t cols, bool forward) {
    // data 存储格式: [re, im, re, im...]
    // 行优先处理
    for(size_t i=0; i<rows; i++) {
        gsl_fft_complex_radix2_forward(&data[i * cols * 2], 1, cols);
    }
    // TODO: 列处理需要转置矩阵
    return 0;
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
