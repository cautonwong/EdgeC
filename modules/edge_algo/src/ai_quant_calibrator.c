/**
 * @file ai_quant_calibrator.c
 * @brief INT8/FP8 Symmetric & Asymmetric Quantization Calibrator Implementation (AEP-702 / Issue #235)
 */

#include "datahub.h"
#include "ai_quant_calibrator.h"
#include <string.h>
#include <math.h>
#include <float.h>

#define TAG_AI_QUANT_CALIB_MSE_LOSS   262160
#define TAG_AI_QUANT_CALIB_SNR_DB     262161
#define TAG_AI_QUANT_SATURATION_PCT   262162

typedef struct ai_quant_internal {
    ai_quant_config_t config;

    /* Global sample statistics */
    float global_min;
    float global_max;
    uint64_t total_samples;
    double sum_val;
    double sum_sq_val;

    /* Dynamic Range Histogram for KL Divergence */
    uint32_t histogram[AI_QUANT_HISTOGRAM_BINS];
    float hist_bin_width;
    float hist_max_abs;

    /* Computed Parameters */
    ai_quant_params_t last_params;
    bool is_calibrated;
} ai_quant_internal_t;

_Static_assert(sizeof(ai_quant_internal_t) <= sizeof(ai_quant_calib_storage_t),
               "ai_quant_calib_storage_t size is insufficient for ai_quant_internal_t");

/* -------------------------------------------------------------------------- */
/* Helper Functions                                                           */
/* -------------------------------------------------------------------------- */

static inline float clip_f32(float val, float min_val, float max_val) {
    if (val < min_val) return min_val;
    if (val > max_val) return max_val;
    return val;
}

static inline int32_t round_to_int32(float val) {
    return (val >= 0.0f) ? (int32_t)(val + 0.5f) : (int32_t)(val - 0.5f);
}

/* -------------------------------------------------------------------------- */
/* FP8 E4M3 & E5M2 Bit Conversion Utilities                                    */
/* -------------------------------------------------------------------------- */

uint8_t ai_quant_fp32_to_fp8_e4m3(float val) {
    union { float f; uint32_t u; } u32;
    u32.f = val;

    uint32_t sign = (u32.u >> 31) & 0x01;
    int32_t exp = (int32_t)((u32.u >> 23) & 0xFF) - 127;
    uint32_t mant = u32.u & 0x7FFFFF;

    /* Max positive representable in E4M3 is 448.0 */
    if (fabsf(val) > 448.0f) {
        return (uint8_t)((sign << 7) | 0x7E); /* Max finite */
    }

    if (exp < -9) {
        return (uint8_t)(sign << 7); /* Subnormal underflow to zero */
    }

    /* Target E4M3: Bias = 7 */
    int32_t target_exp = exp + 7;
    if (target_exp <= 0) {
        /* Subnormal handling */
        int32_t shift = 1 - target_exp;
        uint32_t sub_mant = (1U << 3) | (mant >> 20);
        uint8_t m = (uint8_t)(sub_mant >> shift);
        return (uint8_t)((sign << 7) | (m & 0x07));
    }

    if (target_exp > 15) {
        target_exp = 15;
        mant = 0x700000;
    }

    uint8_t target_mant = (uint8_t)((mant >> 20) & 0x07);
    return (uint8_t)((sign << 7) | ((target_exp & 0x0F) << 3) | target_mant);
}

float ai_quant_fp8_e4m3_to_fp32(uint8_t fp8_val) {
    uint32_t sign = (fp8_val >> 7) & 0x01;
    uint32_t exp = (fp8_val >> 3) & 0x0F;
    uint32_t mant = fp8_val & 0x07;

    if (exp == 0 && mant == 0) {
        return sign ? -0.0f : 0.0f;
    }

    float result;
    if (exp == 0) {
        /* Subnormal */
        result = ldexpf((float)mant / 8.0f, -6);
    } else if (exp == 15 && mant == 7) {
        /* NaN or max */
        result = 448.0f;
    } else {
        result = ldexpf(1.0f + (float)mant / 8.0f, (int)exp - 7);
    }

    return sign ? -result : result;
}

uint8_t ai_quant_fp32_to_fp8_e5m2(float val) {
    union { float f; uint32_t u; } u32;
    u32.f = val;

    uint32_t sign = (u32.u >> 31) & 0x01;
    int32_t exp = (int32_t)((u32.u >> 23) & 0xFF) - 127;
    uint32_t mant = u32.u & 0x7FFFFF;

    if (fabsf(val) > 57344.0f) {
        return (uint8_t)((sign << 7) | 0x7B); /* Max finite in E5M2 */
    }

    if (exp < -16) {
        return (uint8_t)(sign << 7);
    }

    /* Target E5M2: Bias = 15 */
    int32_t target_exp = exp + 15;
    if (target_exp <= 0) {
        int32_t shift = 1 - target_exp;
        uint32_t sub_mant = (1U << 2) | (mant >> 21);
        uint8_t m = (uint8_t)(sub_mant >> shift);
        return (uint8_t)((sign << 7) | (m & 0x03));
    }

    if (target_exp > 30) {
        target_exp = 30;
        mant = 0x600000;
    }

    uint8_t target_mant = (uint8_t)((mant >> 21) & 0x03);
    return (uint8_t)((sign << 7) | ((target_exp & 0x1F) << 2) | target_mant);
}

float ai_quant_fp8_e5m2_to_fp32(uint8_t fp8_val) {
    uint32_t sign = (fp8_val >> 7) & 0x01;
    uint32_t exp = (fp8_val >> 2) & 0x1F;
    uint32_t mant = fp8_val & 0x03;

    if (exp == 0 && mant == 0) {
        return sign ? -0.0f : 0.0f;
    }

    float result;
    if (exp == 0) {
        result = ldexpf((float)mant / 4.0f, -14);
    } else if (exp == 31) {
        result = 57344.0f;
    } else {
        result = ldexpf(1.0f + (float)mant / 4.0f, (int)exp - 15);
    }

    return sign ? -result : result;
}

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

ai_quant_calibrator_t *ai_quant_calibrator_create_in(ai_quant_calib_storage_t *storage,
                                                     const ai_quant_config_t *config) {
    if (!storage) return NULL;

    ai_quant_internal_t *self = (ai_quant_internal_t *)storage->raw;
    memset(self, 0, sizeof(*self));

    if (config) {
        self->config = *config;
    } else {
        self->config.scheme = AI_QUANT_SCHEME_INT8_SYMMETRIC;
        self->config.method = AI_QUANT_CALIB_KL_DIVERGENCE;
        self->config.percentile = 99.99f;
        self->config.mse_search_steps = 64;
    }

    self->global_min = FLT_MAX;
    self->global_max = -FLT_MAX;
    self->hist_max_abs = 1e-4f;
    return (ai_quant_calibrator_t *)self;
}

int ai_quant_calibrator_collect_sample(ai_quant_calibrator_t *calib,
                                       const float *data, size_t count) {
    if (!calib || !data || count == 0) return -1;

    ai_quant_internal_t *self = (ai_quant_internal_t *)calib;

    for (size_t i = 0; i < count; i++) {
        float v = data[i];
        if (isnan(v) || isinf(v)) continue;

        if (v < self->global_min) self->global_min = v;
        if (v > self->global_max) self->global_max = v;

        float abs_v = fabsf(v);
        if (abs_v > self->hist_max_abs) {
            self->hist_max_abs = abs_v;
        }

        self->sum_val += (double)v;
        self->sum_sq_val += (double)v * (double)v;
        self->total_samples++;
    }

    /* Populate histogram bins dynamically */
    self->hist_bin_width = self->hist_max_abs / (float)AI_QUANT_HISTOGRAM_BINS;
    if (self->hist_bin_width > 0.0f) {
        for (size_t i = 0; i < count; i++) {
            float abs_v = fabsf(data[i]);
            uint32_t bin = (uint32_t)(abs_v / self->hist_bin_width);
            if (bin >= AI_QUANT_HISTOGRAM_BINS) {
                bin = AI_QUANT_HISTOGRAM_BINS - 1;
            }
            self->histogram[bin]++;
        }
    }

    return 0;
}

static float compute_kl_divergence_threshold(ai_quant_internal_t *self) {
    /* TensorRT-like KL divergence search between quantized distribution (128 bins) and reference P */
    const uint32_t target_bins = 128;
    uint32_t num_bins = AI_QUANT_HISTOGRAM_BINS;

    double min_kl = DBL_MAX;
    uint32_t optimal_threshold_bin = num_bins - 1;

    /* Search threshold bin from target_bins up to num_bins */
    for (uint32_t t = target_bins; t < num_bins; t += 8) {
        /* Slice P[0..t] */
        double p_sum = 0.0;
        for (uint32_t i = 0; i < t; i++) {
            p_sum += (double)self->histogram[i];
        }
        /* Add outlier counts to last bin t-1 */
        for (uint32_t i = t; i < num_bins; i++) {
            p_sum += (double)self->histogram[i];
        }

        if (p_sum <= 0.0) continue;

        /* Quantize into target_bins and expand back to Q[0..t] */
        double bin_step = (double)t / (double)target_bins;
        double q_sum = 0.0;
        double q_dist[AI_QUANT_HISTOGRAM_BINS];
        memset(q_dist, 0, sizeof(double) * t);

        for (uint32_t qb = 0; qb < target_bins; qb++) {
            uint32_t start_i = (uint32_t)(qb * bin_step);
            uint32_t end_i = (uint32_t)((qb + 1) * bin_step);
            if (end_i > t) end_i = t;

            double count = 0.0;
            uint32_t nonzeros = 0;
            for (uint32_t j = start_i; j < end_i; j++) {
                double c = (double)self->histogram[j];
                if (j == t - 1) {
                    for (uint32_t k = t; k < num_bins; k++) c += (double)self->histogram[k];
                }
                count += c;
                if (c > 0.0) nonzeros++;
            }

            if (nonzeros > 0 && count > 0.0) {
                double avg = count / (double)(end_i - start_i);
                for (uint32_t j = start_i; j < end_i; j++) {
                    if (self->histogram[j] > 0 || j == t - 1) {
                        q_dist[j] = avg;
                        q_sum += avg;
                    }
                }
            }
        }

        /* Calculate KL(P || Q) over all num_bins */
        double kl = 0.0;
        for (uint32_t i = 0; i < num_bins; i++) {
            double p_val = (double)self->histogram[i];
            if (p_val > 0.0) {
                double p_prob = p_val / p_sum;
                double q_prob = 1e-7;
                if (i < t && q_sum > 0.0 && q_dist[i] > 0.0) {
                    q_prob = q_dist[i] / q_sum;
                }
                kl += p_prob * log(p_prob / q_prob);
            }
        }

        if (kl < min_kl) {
            min_kl = kl;
            optimal_threshold_bin = t;
        }
    }

    return ((float)optimal_threshold_bin + 1.0f) * self->hist_bin_width;
}

int ai_quant_calibrator_compute_params(ai_quant_calibrator_t *calib,
                                       ai_quant_params_t *out_params) {
    if (!calib || !out_params) return -1;

    ai_quant_internal_t *self = (ai_quant_internal_t *)calib;
    if (self->total_samples == 0) return -2;

    ai_quant_params_t p;
    memset(&p, 0, sizeof(p));
    p.min_val = self->global_min;
    p.max_val = self->global_max;

    float abs_max = fmaxf(fabsf(self->global_min), fabsf(self->global_max));
    if (abs_max < 1e-6f) abs_max = 1e-6f;

    /* Determine Threshold */
    switch (self->config.method) {
        case AI_QUANT_CALIB_MINMAX:
            p.clipping_threshold = abs_max;
            break;

        case AI_QUANT_CALIB_PERCENTILE: {
            uint64_t target_count = (uint64_t)((double)self->total_samples * ((double)self->config.percentile / 100.0));
            uint64_t accum = 0;
            uint32_t p_bin = AI_QUANT_HISTOGRAM_BINS - 1;
            for (uint32_t b = 0; b < AI_QUANT_HISTOGRAM_BINS; b++) {
                accum += self->histogram[b];
                if (accum >= target_count) {
                    p_bin = b;
                    break;
                }
            }
            p.clipping_threshold = ((float)p_bin + 1.0f) * self->hist_bin_width;
            break;
        }

        case AI_QUANT_CALIB_KL_DIVERGENCE:
            p.clipping_threshold = compute_kl_divergence_threshold(self);
            break;

        case AI_QUANT_CALIB_MSE: {
            /* Grid search for scale that minimizes MSE */
            float best_thresh = abs_max;
            double min_mse = DBL_MAX;
            uint32_t steps = (self->config.mse_search_steps > 0) ? self->config.mse_search_steps : 64;

            for (uint32_t s = 1; s <= steps; s++) {
                float trial_thresh = abs_max * ((float)s / (float)steps);
                float trial_scale = trial_thresh / 127.0f;
                if (trial_scale <= 0.0f) continue;

                double mse = 0.0;
                for (uint32_t b = 0; b < AI_QUANT_HISTOGRAM_BINS; b++) {
                    float val = ((float)b + 0.5f) * self->hist_bin_width;
                    float q = clip_f32(val, -trial_thresh, trial_thresh);
                    int32_t q_int = round_to_int32(q / trial_scale);
                    float deq = (float)q_int * trial_scale;
                    float err = val - deq;
                    mse += (double)err * (double)err * (double)self->histogram[b];
                }

                if (mse < min_mse) {
                    min_mse = mse;
                    best_thresh = trial_thresh;
                }
            }
            p.clipping_threshold = best_thresh;
            break;
        }
    }

    if (p.clipping_threshold < 1e-6f) p.clipping_threshold = abs_max;

    /* Compute Scale & Zero Point */
    if (self->config.scheme == AI_QUANT_SCHEME_INT8_SYMMETRIC) {
        p.scale = p.clipping_threshold / 127.0f;
        p.zero_point = 0;
    } else if (self->config.scheme == AI_QUANT_SCHEME_INT8_ASYMMETRIC) {
        float r_min = self->global_min;
        float r_max = self->global_max;
        if (r_max - r_min < 1e-6f) r_max = r_min + 1e-6f;
        p.scale = (r_max - r_min) / 255.0f;
        p.zero_point = round_to_int32(-r_min / p.scale);
        if (p.zero_point < 0) p.zero_point = 0;
        if (p.zero_point > 255) p.zero_point = 255;
    } else {
        /* FP8 Direct Normalized Scale */
        p.scale = (self->config.scheme == AI_QUANT_SCHEME_FP8_E4M3) ? (p.clipping_threshold / 448.0f) : (p.clipping_threshold / 57344.0f);
        p.zero_point = 0;
    }

    /* Compute Quality Metrics (MSE & SNR) */
    uint64_t saturated_count = 0;
    double total_noise_pwr = 0.0;
    double total_sig_pwr = self->sum_sq_val;

    for (uint32_t b = 0; b < AI_QUANT_HISTOGRAM_BINS; b++) {
        float val = ((float)b + 0.5f) * self->hist_bin_width;
        if (val > p.clipping_threshold) {
            saturated_count += self->histogram[b];
        }
        float q_val = (val > p.clipping_threshold) ? p.clipping_threshold : val;
        float deq_val = 0.0f;
        if (self->config.scheme == AI_QUANT_SCHEME_INT8_SYMMETRIC) {
            int32_t q = round_to_int32(q_val / p.scale);
            deq_val = (float)q * p.scale;
        } else {
            deq_val = q_val;
        }
        float err = val - deq_val;
        total_noise_pwr += (double)err * (double)err * (double)self->histogram[b];
    }

    p.mse_loss = (self->total_samples > 0) ? (float)(total_noise_pwr / (double)self->total_samples) : 0.0f;
    p.saturation_pct = (self->total_samples > 0) ? ((float)saturated_count / (float)self->total_samples * 100.0f) : 0.0f;

    if (total_noise_pwr > 1e-12 && total_sig_pwr > 1e-12) {
        p.snr_db = (float)(10.0 * log10(total_sig_pwr / total_noise_pwr));
    } else {
        p.snr_db = 99.0f; /* High fidelity */
    }

    self->last_params = p;
    self->is_calibrated = true;
    *out_params = p;
    return 0;
}

int ai_quantize_tensor(const ai_quant_params_t *params,
                       ai_quant_scheme_t scheme,
                       const float *src, size_t count,
                       void *dst) {
    if (!params || !src || !dst || count == 0) return -1;

    float inv_scale = (params->scale > 1e-12f) ? (1.0f / params->scale) : 1.0f;

    if (scheme == AI_QUANT_SCHEME_INT8_SYMMETRIC) {
        int8_t *d = (int8_t *)dst;
        for (size_t i = 0; i < count; i++) {
            float v = src[i] * inv_scale;
            int32_t q = round_to_int32(v);
            if (q > 127) q = 127;
            if (q < -127) q = -127;
            d[i] = (int8_t)q;
        }
    } else if (scheme == AI_QUANT_SCHEME_INT8_ASYMMETRIC) {
        uint8_t *d = (uint8_t *)dst;
        for (size_t i = 0; i < count; i++) {
            float v = src[i] * inv_scale + (float)params->zero_point;
            int32_t q = round_to_int32(v);
            if (q > 255) q = 255;
            if (q < 0) q = 0;
            d[i] = (uint8_t)q;
        }
    } else if (scheme == AI_QUANT_SCHEME_FP8_E4M3) {
        uint8_t *d = (uint8_t *)dst;
        for (size_t i = 0; i < count; i++) {
            d[i] = ai_quant_fp32_to_fp8_e4m3(src[i] * inv_scale);
        }
    } else if (scheme == AI_QUANT_SCHEME_FP8_E5M2) {
        uint8_t *d = (uint8_t *)dst;
        for (size_t i = 0; i < count; i++) {
            d[i] = ai_quant_fp32_to_fp8_e5m2(src[i] * inv_scale);
        }
    }

    return 0;
}

int ai_dequantize_tensor(const ai_quant_params_t *params,
                         ai_quant_scheme_t scheme,
                         const void *src, size_t count,
                         float *dst) {
    if (!params || !src || !dst || count == 0) return -1;

    if (scheme == AI_QUANT_SCHEME_INT8_SYMMETRIC) {
        const int8_t *s = (const int8_t *)src;
        for (size_t i = 0; i < count; i++) {
            dst[i] = (float)s[i] * params->scale;
        }
    } else if (scheme == AI_QUANT_SCHEME_INT8_ASYMMETRIC) {
        const uint8_t *s = (const uint8_t *)src;
        for (size_t i = 0; i < count; i++) {
            dst[i] = ((float)s[i] - (float)params->zero_point) * params->scale;
        }
    } else if (scheme == AI_QUANT_SCHEME_FP8_E4M3) {
        const uint8_t *s = (const uint8_t *)src;
        for (size_t i = 0; i < count; i++) {
            dst[i] = ai_quant_fp8_e4m3_to_fp32(s[i]) * params->scale;
        }
    } else if (scheme == AI_QUANT_SCHEME_FP8_E5M2) {
        const uint8_t *s = (const uint8_t *)src;
        for (size_t i = 0; i < count; i++) {
            dst[i] = ai_quant_fp8_e5m2_to_fp32(s[i]) * params->scale;
        }
    }

    return 0;
}

int ai_quant_sync_datahub(const ai_quant_params_t *params, datahub_t *hub) {
    if (!params || !hub) return -1;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_AI_QUANT_CALIB_MSE_LOSS, (double)params->mse_loss, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_AI_QUANT_CALIB_SNR_DB, (double)params->snr_db, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_AI_QUANT_SATURATION_PCT, (double)params->saturation_pct, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}

void ai_quant_calibrator_reset(ai_quant_calibrator_t *calib) {
    if (!calib) return;

    ai_quant_internal_t *self = (ai_quant_internal_t *)calib;
    ai_quant_config_t cfg = self->config;
    memset(self, 0, sizeof(*self));
    self->config = cfg;
    self->global_min = FLT_MAX;
    self->global_max = -FLT_MAX;
    self->hist_max_abs = 1e-4f;
}
