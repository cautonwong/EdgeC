/**
 * @file stage_notch_filter.c
 * @brief 工业级 DSP 陷波滤波器算子 (针对 50/60Hz 工频干扰)
 */

#include "stream_engine.h"
#include "edge_dsp.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    dsp_iir_t *iir;
    uint32_t output_tag;
} notch_filter_ctx_t;

/**
 * @brief 计算陷波器 IIR 系数
 * @param f0 陷波频率 (Hz)
 * @param fs 采样频率 (Hz)
 * @param Q Q 因子
 * @param b 输出 b 系数 (长度3)
 * @param a 输出 a 系数 (长度3)
 */
static void calc_notch_coeffs(double f0, double fs, double Q, double *b, double *a) {
    double w0 = 2.0 * M_PI * f0 / fs;
    double alpha = sin(w0) / (2.0 * Q);
    
    double a0 = 1.0 + alpha;
    
    b[0] = 1.0 / a0;
    b[1] = (-2.0 * cos(w0)) / a0;
    b[2] = 1.0 / a0;
    
    a[0] = 1.0;
    a[1] = (-2.0 * cos(w0)) / a0;
    a[2] = (1.0 - alpha) / a0;
}

static stage_ret_t _notch_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    notch_filter_ctx_t *ctx = (notch_filter_ctx_t *)stage->priv_state;
    if (!ctx || !ctx->iir) return STAGE_ERROR;

    if (ev->type == PIPEMSG_TYPE_SCALAR) {
        ev->payload.val.f = dsp_iir_process(ctx->iir, ev->payload.val.f);
        ev->tag_id = ctx->output_tag;
        return STAGE_OK;
    } 
    else if (ev->type == PIPEMSG_TYPE_BATCH) {
        double *data = (double *)ev->payload.batch.ptr;
        for (uint32_t i = 0; i < ev->payload.batch.count; i++) {
            data[i] = dsp_iir_process(ctx->iir, data[i]);
        }
        ev->tag_id = ctx->output_tag;
        return STAGE_OK;
    }

    return STAGE_DROP;
}

static void _notch_cleanup(void *priv) {
    notch_filter_ctx_t *ctx = (notch_filter_ctx_t *)priv;
    if (ctx) {
        if (ctx->iir) dsp_iir_free(ctx->iir);
        free(ctx);
    }
}

/**
 * @brief 陷波器算子工厂
 * @param f0 目标频率 (通常 50 或 60)
 * @param fs 采样率 (例如 1000)
 * @param q_factor Q 因子 (例如 30.0)
 * @param output_tag 输出 Tag
 */
pipeline_stage_t *stage_notch_filter_create(double f0, double fs, double q_factor, uint32_t output_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    notch_filter_ctx_t *ctx = calloc(1, sizeof(notch_filter_ctx_t));

    ctx->output_tag = output_tag;

    double b[3], a[3];
    calc_notch_coeffs(f0, fs, q_factor, b, a);
    
    // Order is 2, so arrays are size 3
    ctx->iir = dsp_iir_create(2, b, a);

    strncpy(s->name, "NOTCH_FILTER", 31);
    s->process = _notch_proc;
    s->priv_state = ctx;
    s->cleanup_state = _notch_cleanup;

    LOG_INFO(LOG_CAT_CORE, "Notch Filter Stage Created: F0=%.1fHz, Fs=%.1fHz, Q=%.1f", f0, fs, q_factor);
    return s;
}

pipeline_stage_t *stage_notch_filter_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    // f1 = F0 (50/60), f2 = Fs (e.g. 1000), f3 = Q-factor
    return stage_notch_filter_create(def->params.f1, def->params.f2, def->params.f3, (uint32_t)def->params.u1);
}
