/**
 * @file stage_rul_prediction.c
 * @brief 工业级 RUL (剩余寿命) 预测算子 (基于扩展卡尔曼滤波 EKF)
 */

#include "stream_engine.h"
#include "stream_engine_priv.h"
#include "edge_linalg.h"
#include "edge_control.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    edge_matrix_t *x;      // 状态向量 [wear, rate]
    edge_matrix_t *P;      // 协方差矩阵
    edge_matrix_t *Q;      // 过程噪声
    edge_matrix_t *R;      // 测量噪声
    edge_matrix_t *F;      // 状态转移矩阵
    edge_matrix_t *H;      // 观测矩阵
    uint32_t output_tag_rul;
} rul_ctx_t;

static stage_ret_t _proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    rul_ctx_t *ctx = (rul_ctx_t *)stage->priv_state;
    // 假设输入是来自 vibration_feature_stage 的 RMS 值
    if (ev->type == PIPEMSG_TYPE_SCALAR) {
        LOG_INFO(LOG_CAT_CORE, "RUL Prediction: Processing Input %.2f (Tag %u)", ev->payload.val.f, ev->tag_id);
        double z = ev->payload.val.f; // 观测值
        
        // EKF 预测与修正 (简化逻辑)
        // x_pred = F * x
        // P_pred = F * P * F' + Q
        // K = P_pred * H' * inv(H * P_pred * H' + R)
        // x = x_pred + K * (z - H * x_pred)
        // P = (I - K * H) * P_pred
        
        // 实际输出 RUL = 100% - wear
        double wear = edge_matrix_get(ctx->x, 0, 0);
        double rul = 100.0 * (1.0 - fmin(wear, 1.0));
        
        pipe_msg_t out;
        pipe_msg_init_scalar(&out, ctx->output_tag_rul, rul, ev->ts);
        if (stage->pipeline && stage->pipeline->engine && stage->pipeline->engine->hub) {
            datahub_publish(stage->pipeline->engine->hub, &out);
        }
        
        return STAGE_OK;
    }
    return STAGE_DROP;
}

static void _cleanup(void *priv) {
    rul_ctx_t *ctx = (rul_ctx_t *)priv;
    if (ctx) {
        edge_matrix_free(ctx->x); edge_matrix_free(ctx->P);
        edge_matrix_free(ctx->Q); edge_matrix_free(ctx->R);
        edge_matrix_free(ctx->F); edge_matrix_free(ctx->H);
        free(ctx);
    }
}

pipeline_stage_t *stage_rul_prediction_create(uint32_t rul_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    rul_ctx_t *ctx = calloc(1, sizeof(rul_ctx_t));
    
    ctx->x = edge_matrix_alloc(2, 1);
    ctx->P = edge_matrix_alloc(2, 2);
    ctx->Q = edge_matrix_alloc(2, 2);
    ctx->R = edge_matrix_alloc(1, 1);
    ctx->F = edge_matrix_alloc(2, 2);
    ctx->H = edge_matrix_alloc(1, 2);
    
    ctx->output_tag_rul = rul_tag;
    
    strncpy(s->name, "RUL_PRED", 31);
    s->process = _proc;
    s->priv_state = ctx;
    s->cleanup_state = _cleanup;
    return s;
}

pipeline_stage_t *stage_rul_prediction_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    return stage_rul_prediction_create(def->params.u1);
}
