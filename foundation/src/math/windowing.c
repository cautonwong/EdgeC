/**
 * @file windowing.c
 * @brief 窗函数实现 (Hann, Hamming, Blackman)
 */

#include <math.h>
#include <stdint.h>
#include <stddef.h>

void math_window_hann(double *out, size_t n) {
    for (size_t i = 0; i < n; i++) {
        out[i] = 0.5 * (1.0 - cos(2.0 * M_PI * i / (n - 1)));
    }
}

void math_window_hamming(double *out, size_t n) {
    for (size_t i = 0; i < n; i++) {
        out[i] = 0.54 - 0.46 * cos(2.0 * M_PI * i / (n - 1));
    }
}

void math_apply_window(double *data, const double *window, size_t n) {
    for (size_t i = 0; i < n; i++) {
        data[i] *= window[i];
    }
}
