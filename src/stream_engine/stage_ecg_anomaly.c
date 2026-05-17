/**
 * @file stage_ecg_anomaly.c
 * @brief 医疗级 ECG 异常节律检测算子 (基于小波变换)
 */

#include "stream_engine.h"
#include "edge_dsp.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ECG_ALARM_EVENT_ID 0xE002

typedef struct {
    double *buffer;
    size_t size;
    size_t cursor;
    uint32_t level;
    double energy_threshold;
    uint32_t alarm_tag;
    bool buffer_full;
} ecg_anomaly_ctx_t;

static stage_ret_t _ecg_anomaly_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    ecg_anomaly_ctx_t *ctx = (ecg_anomaly_ctx_t *)stage->priv_state;
    if (!ctx || ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_ERROR;

    // 1. Buffer the incoming signal
    ctx->buffer[ctx->cursor++] = ev->payload.val.f;
    if (ctx->cursor >= ctx->size) {
        ctx->cursor = 0;
        ctx->buffer_full = true;
    }

    // 2. Perform sliding window analysis (e.g., every size/4 samples)
    if (ctx->buffer_full && ctx->cursor % (ctx->size / 4) == 0) {
        double *tmp = malloc(ctx->size * sizeof(double));
        // Rearrange circular buffer to linear
        size_t head = ctx->cursor;
        for (size_t i = 0; i < ctx->size; i++) {
            tmp[i] = ctx->buffer[(head + i) % ctx->size];
        }

        // 3. Execute Haar DWT
        dsp_dwt_haar(tmp, ctx->size, ctx->level);

        // 4. Calculate energy of the high-frequency detail bands
        // Detail coefficients for level 1 are in [size/2, size-1]
        // For level > 1, the details are from [size/(2^level), size-1]
        size_t detail_start = ctx->size / (1 << ctx->level);
        if (detail_start == 0) detail_start = 1;

        double energy = 0.0;
        for (size_t i = detail_start; i < ctx->size; i++) {
            energy += tmp[i] * tmp[i];
        }

        free(tmp);

        // 5. Detect anomaly against threshold
        if (energy > ctx->energy_threshold) {
            LOG_WARN(LOG_CAT_CORE, "ECG Anomaly Detected! High-freq energy: %.2f (Threshold: %.2f)", 
                     energy, ctx->energy_threshold);
            
            pipe_msg_t alarm;
            pipe_msg_init_event(&alarm, ECG_ALARM_EVENT_ID, (uint32_t)energy, ev->ts);
            alarm.tag_id = ctx->alarm_tag;
            
            *ev = alarm; 
            return STAGE_OK;
        }
    }

    return STAGE_DROP;
}

static void _ecg_anomaly_cleanup(void *priv) {
    ecg_anomaly_ctx_t *ctx = (ecg_anomaly_ctx_t *)priv;
    if (ctx) {
        if (ctx->buffer) free(ctx->buffer);
        free(ctx);
    }
}

/**
 * @brief ECG 异常检测算子工厂
 * @param buffer_size 窗口大小 (必须是2的幂)
 * @param level DWT 分解层数
 * @param threshold 细节频段能量阈值
 * @param alarm_tag 报警输出 Tag
 */
pipeline_stage_t *stage_ecg_anomaly_create(size_t buffer_size, uint32_t level, double threshold, uint32_t alarm_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    ecg_anomaly_ctx_t *ctx = calloc(1, sizeof(ecg_anomaly_ctx_t));

    ctx->size = buffer_size > 0 ? buffer_size : 256;
    ctx->level = level > 0 ? level : 2;
    ctx->energy_threshold = threshold > 0 ? threshold : 100.0;
    ctx->alarm_tag = alarm_tag;
    ctx->buffer = calloc(ctx->size, sizeof(double));

    strncpy(s->name, "ECG_ANOMALY", 31);
    s->process = _ecg_anomaly_proc;
    s->priv_state = ctx;
    s->cleanup_state = _ecg_anomaly_cleanup;

    LOG_INFO(LOG_CAT_CORE, "ECG Anomaly Stage Created: Window=%zu, DWT Level=%u, Threshold=%.1f", 
             ctx->size, ctx->level, ctx->energy_threshold);
    return s;
}

pipeline_stage_t *stage_ecg_anomaly_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    // f1 = buffer_size, f2 = level, f3 = threshold, u1 = alarm_tag
    return stage_ecg_anomaly_create((size_t)def->params.f1, (uint32_t)def->params.f2, def->params.f3, (uint32_t)def->params.u1);
}
