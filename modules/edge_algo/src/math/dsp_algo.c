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
// 纯 C Radix-2 FFT 后备实现
static void bit_reverse_copy(double *data, size_t n) {
    size_t j = 0;
    for (size_t i = 0; i < n - 1; i++) {
        if (i < j) {
            double temp_re = data[2 * i];
            double temp_im = data[2 * i + 1];
            data[2 * i] = data[2 * j];
            data[2 * i + 1] = data[2 * j + 1];
            data[2 * j] = temp_re;
            data[2 * j + 1] = temp_im;
        }
        size_t k = n >> 1;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
}

int dsp_fft_r2c(double *data, size_t n) {
    if (!data || n == 0 || (n & (n - 1)) != 0) return -1; // n 必须是 2 的幂

    // 将长度为 n 的实数数组 (前 n 个元素存放实数，由于数组长为 2n)
    // 扩展为长度为 n 的复数数组 (交替的实部/虚部)
    for (size_t i = n; i > 0; i--) {
        data[2 * (i - 1)] = data[i - 1]; // 实部
        data[2 * (i - 1) + 1] = 0.0;     // 虚部
    }

    bit_reverse_copy(data, n);

    for (size_t s = 1; s <= (size_t)log2(n); s++) {
        size_t m = 1 << s;
        double theta = -2.0 * M_PI / m;
        double w_re = cos(theta);
        double w_im = sin(theta);

        for (size_t k = 0; k < n; k += m) {
            double w_k_re = 1.0;
            double w_k_im = 0.0;
            for (size_t j = 0; j < m / 2; j++) {
                double t_re = w_k_re * data[2 * (k + j + m / 2)] - w_k_im * data[2 * (k + j + m / 2) + 1];
                double t_im = w_k_re * data[2 * (k + j + m / 2) + 1] + w_k_im * data[2 * (k + j + m / 2)];
                double u_re = data[2 * (k + j)];
                double u_im = data[2 * (k + j) + 1];
                
                data[2 * (k + j)] = u_re + t_re;
                data[2 * (k + j) + 1] = u_im + t_im;
                data[2 * (k + j + m / 2)] = u_re - t_re;
                data[2 * (k + j + m / 2) + 1] = u_im - t_im;

                double next_w_k_re = w_k_re * w_re - w_k_im * w_im;
                double next_w_k_im = w_k_re * w_im + w_k_im * w_re;
                w_k_re = next_w_k_re;
                w_k_im = next_w_k_im;
            }
        }
    }
    return 0;
}
#endif

/**
 * @brief 2D 复数 FFT 完整实现
 * 算法：对每一行做 FFT，然后对每一列做 FFT。
 * 针对 GSL 的 stride 优化，避免显式矩阵转置。
 */
int dsp_fft2d_c2c(double *data, size_t rows, size_t cols, bool forward) {
#ifdef USE_GSL
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
#else
    if (!data || rows == 0 || cols == 0 || (rows & (rows - 1)) != 0 || (cols & (cols - 1)) != 0) return -1;
    // 1. 行变换
    for (size_t r = 0; r < rows; r++) {
        double *row = &data[r * cols * 2];
        bit_reverse_copy(row, cols);
        for (size_t s = 1; s <= (size_t)log2((double)cols); s++) {
            size_t m = 1 << s;
            double theta = (forward ? -2.0 : 2.0) * M_PI / (double)m;
            double w_re = cos(theta), w_im = sin(theta);
            for (size_t k = 0; k < cols; k += m) {
                double w_k_re = 1.0, w_k_im = 0.0;
                for (size_t j = 0; j < m / 2; j++) {
                    double t_re = w_k_re * row[2 * (k + j + m / 2)] - w_k_im * row[2 * (k + j + m / 2) + 1];
                    double t_im = w_k_re * row[2 * (k + j + m / 2) + 1] + w_k_im * row[2 * (k + j + m / 2)];
                    double u_re = row[2 * (k + j)], u_im = row[2 * (k + j) + 1];
                    row[2 * (k + j)] = u_re + t_re;
                    row[2 * (k + j) + 1] = u_im + t_im;
                    row[2 * (k + j + m / 2)] = u_re - t_re;
                    row[2 * (k + j + m / 2) + 1] = u_im - t_im;
                    double next_w_k_re = w_k_re * w_re - w_k_im * w_im;
                    double next_w_k_im = w_k_re * w_im + w_k_im * w_re;
                    w_k_re = next_w_k_re; w_k_im = next_w_k_im;
                }
            }
        }
    }
    // 2. 列变换
    double col_stack[2048];
    double *col_temp = (rows * 2 <= 2048) ? col_stack : (double *)malloc(rows * 2 * sizeof(double));
    if (!col_temp) return -1;
    for (size_t c = 0; c < cols; c++) {
        for (size_t r = 0; r < rows; r++) {
            col_temp[2 * r] = data[(r * cols + c) * 2];
            col_temp[2 * r + 1] = data[(r * cols + c) * 2 + 1];
        }
        bit_reverse_copy(col_temp, rows);
        for (size_t s = 1; s <= (size_t)log2((double)rows); s++) {
            size_t m = 1 << s;
            double theta = (forward ? -2.0 : 2.0) * M_PI / (double)m;
            double w_re = cos(theta), w_im = sin(theta);
            for (size_t k = 0; k < rows; k += m) {
                double w_k_re = 1.0, w_k_im = 0.0;
                for (size_t j = 0; j < m / 2; j++) {
                    double t_re = w_k_re * col_temp[2 * (k + j + m / 2)] - w_k_im * col_temp[2 * (k + j + m / 2) + 1];
                    double t_im = w_k_re * col_temp[2 * (k + j + m / 2) + 1] + w_k_im * col_temp[2 * (k + j + m / 2)];
                    double u_re = col_temp[2 * (k + j)], u_im = col_temp[2 * (k + j) + 1];
                    col_temp[2 * (k + j)] = u_re + t_re;
                    col_temp[2 * (k + j) + 1] = u_im + t_im;
                    col_temp[2 * (k + j + m / 2)] = u_re - t_re;
                    col_temp[2 * (k + j + m / 2) + 1] = u_im - t_im;
                    double next_w_k_re = w_k_re * w_re - w_k_im * w_im;
                    double next_w_k_im = w_k_re * w_im + w_k_im * w_re;
                    w_k_re = next_w_k_re; w_k_im = next_w_k_im;
                }
            }
        }
        for (size_t r = 0; r < rows; r++) {
            data[(r * cols + c) * 2] = col_temp[2 * r];
            data[(r * cols + c) * 2 + 1] = col_temp[2 * r + 1];
        }
    }
    if (col_temp != col_stack) {
        free(col_temp);
    }
    return 0;
#endif
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
    double stack_tmp[1024];
    double *tmp = (n <= 1024) ? stack_tmp : malloc(n * sizeof(double));
    if (!tmp) return;
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
    if (tmp != stack_tmp) {
        free(tmp);
    }
}

struct dsp_fir_s {
    double *taps;
    double *delay_line;
    size_t num_taps;
    size_t head;
};

dsp_fir_t* dsp_fir_create(const double *taps, size_t num_taps) {
    if (!taps || num_taps == 0) return NULL;
    dsp_fir_t *f = malloc(sizeof(dsp_fir_t));
    f->num_taps = num_taps;
    f->taps = malloc(num_taps * sizeof(double));
    f->delay_line = calloc(num_taps, sizeof(double));
    f->head = 0;
    memcpy(f->taps, taps, num_taps * sizeof(double));
    return f;
}

double dsp_fir_process(dsp_fir_t *f, double in) {
    if (!f) return 0.0;
    f->delay_line[f->head] = in;
    double out = 0.0;
    size_t idx = f->head;
    for (size_t i = 0; i < f->num_taps; i++) {
        out += f->taps[i] * f->delay_line[idx];
        idx = (idx > 0) ? (idx - 1) : (f->num_taps - 1);
    }
    f->head = (f->head + 1) % f->num_taps;
    return out;
}

void dsp_fir_free(dsp_fir_t *f) {
    if (f) {
        free(f->taps);
        free(f->delay_line);
        free(f);
    }
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
