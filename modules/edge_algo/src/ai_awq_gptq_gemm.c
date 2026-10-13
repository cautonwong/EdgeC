/**
 * @file ai_awq_gptq_gemm.c
 * @brief Edge 4-bit (AWQ/GPTQ) Compact Weight Quantization & Fused GEMV Implementation (AEP-703 / Issue #236)
 */

#include "datahub.h"
#include "ai_awq_gptq_gemm.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define TAG_AI_AWQ_COMPRESSION_RATIO   262163
#define TAG_AI_AWQ_GEMV_GFLOPS         262164

typedef struct ai_awq_internal {
    ai_awq_metrics_t last_metrics;
    uint64_t total_gemv_inferences;
    double cumulative_gflops_sum;
} ai_awq_internal_t;

_Static_assert(sizeof(ai_awq_internal_t) <= sizeof(ai_awq_gptq_storage_t),
               "ai_awq_gptq_storage_t size is insufficient for ai_awq_internal_t");

static inline uint64_t get_time_ns_raw(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* -------------------------------------------------------------------------- */
/* Public API Implementation                                                  */
/* -------------------------------------------------------------------------- */

ai_awq_gptq_engine_t *ai_awq_gptq_create_in(ai_awq_gptq_storage_t *storage) {
    if (!storage) return NULL;

    ai_awq_internal_t *self = (ai_awq_internal_t *)storage->raw;
    memset(self, 0, sizeof(*self));
    return (ai_awq_gptq_engine_t *)self;
}

int ai_awq_pack_weights(const float *src_fp32,
                        uint32_t rows_out, uint32_t cols_in,
                        uint16_t group_size,
                        ai_awq_weight_format_t format,
                        uint8_t *out_packed_buf,
                        float *out_scales,
                        uint8_t *out_zeros) {
    if (!src_fp32 || !out_packed_buf || !out_scales || rows_out == 0 || cols_in == 0) {
        return -1;
    }

    if (group_size == 0) group_size = AI_AWQ_DEFAULT_GROUP_SIZE;
    uint32_t total_elements = rows_out * cols_in;
    uint32_t num_groups = (total_elements + group_size - 1) / group_size;

    /* Process group by group */
    for (uint32_t g = 0; g < num_groups; g++) {
        uint32_t start_idx = g * group_size;
        uint32_t end_idx = start_idx + group_size;
        if (end_idx > total_elements) end_idx = total_elements;

        /* Find group min/max */
        float g_min = src_fp32[start_idx];
        float g_max = src_fp32[start_idx];
        for (uint32_t i = start_idx + 1; i < end_idx; i++) {
            if (src_fp32[i] < g_min) g_min = src_fp32[i];
            if (src_fp32[i] > g_max) g_max = src_fp32[i];
        }

        float scale = 1.0f;
        int32_t zp = 0;

        if (format == AI_AWQ_FORMAT_INT4_SYMMETRIC) {
            float abs_max = fmaxf(fabsf(g_min), fabsf(g_max));
            if (abs_max < 1e-7f) abs_max = 1e-7f;
            scale = abs_max / 7.0f;
            zp = 0;
        } else {
            /* Asymmetric 4-bit [0, 15] */
            if (g_max - g_min < 1e-7f) g_max = g_min + 1e-7f;
            scale = (g_max - g_min) / 15.0f;
            zp = (int32_t)roundf(-g_min / scale);
            if (zp < 0) zp = 0;
            if (zp > 15) zp = 15;
        }

        out_scales[g] = scale;
        if (out_zeros && format == AI_AWQ_FORMAT_INT4_ASYMMETRIC) {
            /* Pack zero point (2 per byte) */
            uint32_t zp_byte_idx = g / 2;
            if ((g % 2) == 0) {
                out_zeros[zp_byte_idx] = (uint8_t)(zp & 0x0F);
            } else {
                out_zeros[zp_byte_idx] |= (uint8_t)((zp & 0x0F) << 4);
            }
        }

        /* Quantize and pack pairs into nibbles */
        float inv_scale = (scale > 1e-12f) ? (1.0f / scale) : 1.0f;
        for (uint32_t i = start_idx; i < end_idx; i += 2) {
            /* First element */
            int32_t q0;
            if (format == AI_AWQ_FORMAT_INT4_SYMMETRIC) {
                q0 = (int32_t)roundf(src_fp32[i] * inv_scale);
                if (q0 > 7) q0 = 7;
                if (q0 < -8) q0 = -8;
                q0 = q0 & 0x0F;
            } else {
                q0 = (int32_t)roundf(src_fp32[i] * inv_scale) + zp;
                if (q0 > 15) q0 = 15;
                if (q0 < 0) q0 = 0;
            }

            /* Second element (if within bounds) */
            int32_t q1 = 0;
            if (i + 1 < end_idx) {
                if (format == AI_AWQ_FORMAT_INT4_SYMMETRIC) {
                    q1 = (int32_t)roundf(src_fp32[i + 1] * inv_scale);
                    if (q1 > 7) q1 = 7;
                    if (q1 < -8) q1 = -8;
                    q1 = q1 & 0x0F;
                } else {
                    q1 = (int32_t)roundf(src_fp32[i + 1] * inv_scale) + zp;
                    if (q1 > 15) q1 = 15;
                    if (q1 < 0) q1 = 0;
                }
            }

            uint32_t byte_idx = i / 2;
            out_packed_buf[byte_idx] = (uint8_t)((q0 & 0x0F) | ((q1 & 0x0F) << 4));
        }
    }

    return 0;
}

int ai_awq_gemv_f32(ai_awq_gptq_engine_t *engine,
                    const ai_awq_weight_tensor_t *weight,
                    const float *vec_x,
                    float *out_y,
                    ai_awq_metrics_t *out_metrics) {
    if (!weight || !vec_x || !out_y || !weight->packed_weights || !weight->scales) {
        return -1;
    }

    uint64_t t_start = get_time_ns_raw();

    uint32_t N = weight->header.rows_out;
    uint32_t K = weight->header.cols_in;
    uint16_t group_size = (weight->header.group_size > 0) ? weight->header.group_size : AI_AWQ_DEFAULT_GROUP_SIZE;
    ai_awq_weight_format_t fmt = (ai_awq_weight_format_t)weight->header.format;

    const uint8_t *w_packed = weight->packed_weights;
    const float *scales = weight->scales;
    const uint8_t *zeros = weight->zeros;
    const float *salient = weight->salient_channel_scales;

    /* Loop over rows (N) */
    for (uint32_t r = 0; r < N; r++) {
        double acc = 0.0;
        uint32_t row_offset = r * K;

        /* Loop over cols (K) in pairs of elements */
        for (uint32_t c = 0; c < K; c += 2) {
            uint32_t global_idx0 = row_offset + c;
            uint32_t global_idx1 = global_idx0 + 1;

            uint32_t group_idx0 = global_idx0 / group_size;
            float scale0 = scales[group_idx0];

            /* Extract 2 nibbles from 1 byte */
            uint8_t byte_val = w_packed[global_idx0 / 2];
            uint8_t nibble0 = byte_val & 0x0F;
            uint8_t nibble1 = (byte_val >> 4) & 0x0F;

            float w0, w1;
            if (fmt == AI_AWQ_FORMAT_INT4_SYMMETRIC) {
                /* Sign extend 4-bit to signed int */
                int8_t s0 = (nibble0 & 0x08) ? (int8_t)(nibble0 | 0xF0) : (int8_t)nibble0;
                int8_t s1 = (nibble1 & 0x08) ? (int8_t)(nibble1 | 0xF0) : (int8_t)nibble1;
                w0 = (float)s0 * scale0;

                uint32_t group_idx1 = global_idx1 / group_size;
                float scale1 = scales[group_idx1];
                w1 = (float)s1 * scale1;
            } else {
                /* Asymmetric with zero point */
                uint8_t zp0 = 0, zp1 = 0;
                if (zeros) {
                    zp0 = (zeros[group_idx0 / 2] >> ((group_idx0 % 2) * 4)) & 0x0F;
                }
                w0 = ((float)nibble0 - (float)zp0) * scale0;

                uint32_t group_idx1 = global_idx1 / group_size;
                float scale1 = scales[group_idx1];
                if (zeros) {
                    zp1 = (zeros[group_idx1 / 2] >> ((group_idx1 % 2) * 4)) & 0x0F;
                }
                w1 = ((float)nibble1 - (float)zp1) * scale1;
            }

            /* AWQ Salient channel scale compensation if present */
            if (salient) {
                w0 *= salient[c];
                if (c + 1 < K) {
                    w1 *= salient[c + 1];
                }
            }

            acc += (double)vec_x[c] * (double)w0;
            if (c + 1 < K) {
                acc += (double)vec_x[c + 1] * (double)w1;
            }
        }

        out_y[r] = (float)acc;
    }

    uint64_t t_end = get_time_ns_raw();
    uint64_t latency_ns = (t_end >= t_start) ? (t_end - t_start) : 1;

    /* Metrics */
    ai_awq_metrics_t m;
    memset(&m, 0, sizeof(m));
    m.latency_ns = latency_ns;

    /* 2 * N * K FLOPs */
    double flops = 2.0 * (double)N * (double)K;
    double duration_sec = (double)latency_ns / 1e9;
    if (duration_sec > 0.0) {
        m.gflops = (float)(flops / duration_sec / 1e9);
        /* 4-bit memory bandwidth: N * K * 0.5 bytes read */
        double bytes_read = (double)(N * K) * 0.5;
        m.effective_bw_gbps = (float)(bytes_read / duration_sec / 1e9);
    }

    /* FP32 is 4 bytes, 4-bit + scales is ~0.55 bytes => ~7.27x or ~3.8x */
    double original_bytes = (double)(N * K) * 4.0;
    double compressed_bytes = (double)(N * K) * 0.5 + (double)weight->header.num_groups * 4.0;
    m.compression_ratio = (compressed_bytes > 0.0) ? (float)(original_bytes / compressed_bytes) : 8.0f;

    if (engine) {
        ai_awq_internal_t *self = (ai_awq_internal_t *)engine;
        self->last_metrics = m;
        self->total_gemv_inferences++;
        self->cumulative_gflops_sum += (double)m.gflops;
    }

    if (out_metrics) {
        *out_metrics = m;
    }

    return 0;
}

int ai_awq_mmap_weights(const char *file_path,
                        ai_awq_weight_tensor_t *out_tensor,
                        void **out_mmap_handle,
                        size_t *out_mmap_size) {
    if (!file_path || !out_tensor || !out_mmap_handle || !out_mmap_size) {
        return -1;
    }

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) return -2;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return -3;
    }

    size_t file_size = (size_t)st.st_size;
    if (file_size < sizeof(ai_awq_matrix_header_t)) {
        close(fd);
        return -4;
    }

    void *mapped = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (mapped == MAP_FAILED) return -5;

    const ai_awq_matrix_header_t *hdr = (const ai_awq_matrix_header_t *)mapped;
    out_tensor->header = *hdr;

    const uint8_t *ptr = (const uint8_t *)mapped + sizeof(ai_awq_matrix_header_t);
    out_tensor->packed_weights = ptr;

    size_t packed_len = (hdr->rows_out * hdr->cols_in) / 2;
    ptr += packed_len;

    out_tensor->scales = (const float *)ptr;
    ptr += hdr->num_groups * sizeof(float);

    if (hdr->format == AI_AWQ_FORMAT_INT4_ASYMMETRIC) {
        out_tensor->zeros = ptr;
    } else {
        out_tensor->zeros = NULL;
    }
    out_tensor->salient_channel_scales = NULL;

    *out_mmap_handle = mapped;
    *out_mmap_size = file_size;
    return 0;
}

int ai_awq_munmap_weights(void *mmap_handle, size_t mmap_size) {
    if (!mmap_handle || mmap_size == 0) return -1;
    return munmap(mmap_handle, mmap_size);
}

int ai_awq_sync_datahub(const ai_awq_metrics_t *metrics, datahub_t *hub) {
    if (!metrics || !hub) return -1;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, TAG_AI_AWQ_COMPRESSION_RATIO, (double)metrics->compression_ratio, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, TAG_AI_AWQ_GEMV_GFLOPS, (double)metrics->gflops, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}
