/**
 * @file stage_wavelet_anomaly.c
 * @brief 基于小波变换 (DWT Haar) 的信号异常检测算子
 */

#include "stream_engine.h"
#include "edge_dsp.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ALARM_EVENT_ID 0xE001

typedef struct {
    double *buffer;
    size_t size;
    size_t cursor;
    uint32_t level;
    double threshold;
    uint32_t alarm_tag;
    bool buffer_full;
} wavelet_st_t;

/**
 * @brief 小波异常检测处理
 * 输入: 标量采样
 * 输出: 当检测到高频异常时发布事件
 */
static stage_ret_t _wavelet_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    wavelet_st_t *ctx = (wavelet_st_t *)stage->priv_state;
    if (!ctx || ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_ERROR;

    // 1. 累积缓存
    ctx->buffer[ctx->cursor++] = ev->payload.val.f;
    if (ctx->cursor >= ctx->size) {
        ctx->cursor = 0;
        ctx->buffer_full = true;
    }

    // 2. 当缓存写满一轮时进行分析 (滑动窗口分析)
    if (ctx->buffer_full && ctx->cursor % (ctx->size / 4) == 0) {
        // 复制一份数据，因为 DWT 是覆盖式的
        double *tmp = malloc(ctx->size * sizeof(double));
        memcpy(tmp, ctx->buffer, ctx->size * sizeof(double));

        // 3. 执行 Haar 小波变换
        dsp_dwt_haar(tmp, ctx->size, ctx->level);

        // 4. 分析高频分量 (细节系数)
        // 对于 1 层分解，后一半 [n/2, n-1] 是细节系数
        double max_detail = 0;
        for (size_t i = ctx->size / 2; i < ctx->size; i++) {
            double val = fabs(tmp[i]);
            if (val > max_detail) max_detail = val;
        }

        free(tmp);

        // 5. 阈值判定
        if (max_detail > ctx->threshold) {
            LOG_WARN(LOG_CAT_CORE, "Wavelet Anomaly Detected! Max Detail: %.2f (Threshold: %.2f)", 
                     max_detail, ctx->threshold);
            
            // 发布异常事件
            pipe_msg_t alarm;
            pipe_msg_init_event(&alarm, ALARM_EVENT_ID, (uint32_t)(max_detail * 100), ev->ts);
            alarm.tag_id = ctx->alarm_tag;
            
            // 直接将事件注入 DataHub (通过 StreamEngine 上下文通常更佳，但这里演示 Stage 返回)
            *ev = alarm; 
            return STAGE_OK;
        }
    }

    return STAGE_DROP;
}

static void _wavelet_cleanup(void *priv) {
    wavelet_st_t *ctx = (wavelet_st_t *)priv;
    if (ctx) {
        if (ctx->buffer) free(ctx->buffer);
        free(ctx);
    }
}

/**
 * @brief 小波异常检测算子工厂
 * 参数: u1=alarm_tag, f1=threshold, f2=size(power of 2), f3=level
 */
pipeline_stage_t *stage_wavelet_anomaly_create(uint32_t alarm_tag, double threshold, size_t size, uint32_t level) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    wavelet_st_t *ctx = calloc(1, sizeof(wavelet_st_t));

    ctx->alarm_tag = alarm_tag;
    ctx->threshold = threshold > 0 ? threshold : 1.0;
    ctx->size = size > 0 ? size : 256;
    ctx->level = level > 0 ? level : 1;
    ctx->buffer = calloc(ctx->size, sizeof(double));

    strncpy(s->name, "WAVELET_ANOMALY", 31);
    s->process = _wavelet_proc;
    s->priv_state = ctx;
    s->cleanup_state = _wavelet_cleanup;

    return s;
}

pipeline_stage_t *stage_wavelet_anomaly_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    return stage_wavelet_anomaly_create((uint32_t)def->params.u1, def->params.f1, 
                                       (size_t)def->params.f2, (uint32_t)def->params.f3);
}
