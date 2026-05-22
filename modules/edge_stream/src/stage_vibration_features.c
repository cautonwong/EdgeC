/**
 * @file stage_vibration_features.c
 * @brief 工业级振动特征提取算子 (RMS, Kurtosis, Skewness)
 */

#include "stream_engine.h"
#include "stream_engine_priv.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    double *window;
    size_t window_size;
    size_t cursor;
    uint32_t output_tag_rms;
    uint32_t output_tag_kurt;
    uint32_t output_tag_skew;
} vibra_features_ctx_t;

static void calc_stats(const double *data, size_t n, double *rms, double *kurt, double *skew) {
    if (n == 0) return;
    double sum = 0.0, sum_sq = 0.0;
    
    for (size_t i = 0; i < n; i++) {
        double v = data[i];
        sum += v;
        sum_sq += v * v;
    }
    
    double mean = sum / (double)n;
    double var = (sum_sq / (double)n) - (mean * mean);
    double std = sqrt(var > 0 ? var : 0.0);
    
    *rms = sqrt(sum_sq / (double)n);
    
    if (std > 1e-9) {
        double m3 = 0.0, m4 = 0.0;
        for (size_t i = 0; i < n; i++) {
            double diff = data[i] - mean;
            m3 += diff * diff * diff;
            m4 += diff * diff * diff * diff;
        }
        *skew = (m3 / (double)n) / (std * std * std);
        *kurt = (m4 / (double)n) / (std * std * std * std);
    } else {
        *skew = 0.0;
        *kurt = 0.0;
    }
}

static stage_ret_t _proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    vibra_features_ctx_t *ctx = (vibra_features_ctx_t *)stage->priv_state;
    if (ev->type == PIPEMSG_TYPE_SCALAR) {
        ctx->window[ctx->cursor++] = ev->payload.val.f;
        if (ctx->cursor >= ctx->window_size) {
            ctx->cursor = 0;
            double rms, kurt, skew;
            calc_stats(ctx->window, ctx->window_size, &rms, &kurt, &skew);
            
            pipe_msg_t m1, m2, m3;
            pipe_msg_init_scalar(&m1, ctx->output_tag_rms, rms, ev->ts);
            pipe_msg_init_scalar(&m2, ctx->output_tag_kurt, kurt, ev->ts);
            pipe_msg_init_scalar(&m3, ctx->output_tag_skew, skew, ev->ts);
            
            // 安全发布到 DataHub
            if (stage->pipeline && stage->pipeline->engine && stage->pipeline->engine->hub) {
                datahub_publish(stage->pipeline->engine->hub, &m1);
                datahub_publish(stage->pipeline->engine->hub, &m2);
                datahub_publish(stage->pipeline->engine->hub, &m3);
            }
        }
        return STAGE_DROP;
    }
    return STAGE_DROP;
}

static void _cleanup(void *priv) {
    vibra_features_ctx_t *ctx = (vibra_features_ctx_t *)priv;
    if (ctx) {
        if (ctx->window) free(ctx->window);
        free(ctx);
    }
}

pipeline_stage_t *stage_vibration_features_create(size_t window_size, uint32_t rms_tag, uint32_t kurt_tag, uint32_t skew_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    vibra_features_ctx_t *ctx = calloc(1, sizeof(vibra_features_ctx_t));
    
    ctx->window_size = window_size > 0 ? window_size : 128;
    ctx->window = calloc(ctx->window_size, sizeof(double));
    ctx->output_tag_rms = rms_tag;
    ctx->output_tag_kurt = kurt_tag;
    ctx->output_tag_skew = skew_tag;
    
    strncpy(s->name, "VIBRA_FEAT", 31);
    s->process = _proc;
    s->priv_state = ctx;
    s->cleanup_state = _cleanup;
    return s;
}

pipeline_stage_t *stage_vibration_features_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    return stage_vibration_features_create((size_t)def->params.f1, (uint32_t)def->params.u1, (uint32_t)def->params.u2, (uint32_t)def->params.u3);
}
