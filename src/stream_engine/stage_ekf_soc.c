/**
 * @file stage_ekf_soc.c
 * @brief 电池 SOC 估算算子 (基于扩展卡尔曼滤波 EKF)
 */

#include "stream_engine.h"
#include "edge_control.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// 简化的电池等效电路模型 (ECM) 参数
#define BATT_CAPACITY_AH 100.0
#define BATT_R0 0.05
#define BATT_R1 0.02
#define BATT_C1 1000.0

typedef struct {
    control_ekf_t ekf;
    uint32_t current_tag;
    uint32_t voltage_tag;
    uint32_t soc_out_tag;
    double last_current;
    uint64_t last_ts;
    
    // 临时矩阵
    edge_matrix_t *F;
    edge_matrix_t *B;
    edge_matrix_t *u;
    edge_matrix_t *H;
    edge_matrix_t *y;
} ekf_soc_ctx_t;

// OCV(SOC) 曲线近似: OCV = 3.0 + 1.2 * SOC
static double get_ocv(double soc) {
    if (soc < 0) soc = 0;
    if (soc > 1) soc = 1;
    return 3.0 + 1.2 * soc;
}

static double get_docv_dsoc(double soc) {
    (void)soc;
    return 1.2;
}

static stage_ret_t _ekf_soc_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    ekf_soc_ctx_t *ctx = (ekf_soc_ctx_t *)stage->priv_state;
    if (!ctx || ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_ERROR;

    uint64_t now_ms = ev->ts;
    double dt = 0.0;
    if (ctx->last_ts != 0) {
        dt = (double)(now_ms - ctx->last_ts) / 1000.0;
    }
    ctx->last_ts = now_ms;

    if (ev->tag_id == ctx->current_tag) {
        ctx->last_current = ev->payload.val.f;
        return STAGE_DROP; // Wait for voltage to run correction
    }

    if (ev->tag_id == ctx->voltage_tag && dt > 0.0) {
        double vt = ev->payload.val.f;
        double i = ctx->last_current; // 放电为正

        // 1. 预测步
        // x = [SOC, Vp]^T
        // SOC_{k+1} = SOC_k - (I_k * dt) / (3600 * Cn)
        // Vp_{k+1} = Vp_k * exp(-dt / (R1*C1)) + R1 * I_k * (1 - exp(-dt / (R1*C1)))
        
        double soc_k = edge_matrix_get(ctx->ekf.x, 0, 0);
        double vp_k = edge_matrix_get(ctx->ekf.x, 1, 0);
        
        double rc_factor = exp(-dt / (BATT_R1 * BATT_C1));
        double soc_next = soc_k - (i * dt) / (3600.0 * BATT_CAPACITY_AH);
        double vp_next = vp_k * rc_factor + BATT_R1 * i * (1.0 - rc_factor);
        
        // F = 雅可比矩阵 df/dx
        edge_matrix_set(ctx->F, 0, 0, 1.0);
        edge_matrix_set(ctx->F, 0, 1, 0.0);
        edge_matrix_set(ctx->F, 1, 0, 0.0);
        edge_matrix_set(ctx->F, 1, 1, rc_factor);
        
        // 更新状态 (Prediction skips internal x update, we do it here since f is non-linear)
        edge_matrix_set(ctx->ekf.x, 0, 0, soc_next);
        edge_matrix_set(ctx->ekf.x, 1, 0, vp_next);
        
        control_ekf_predict(&ctx->ekf, ctx->F, NULL, NULL);

        // 2. 校正步
        // y_hat = OCV(SOC) - Vp - I*R0
        double y_hat = get_ocv(soc_next) - vp_next - i * BATT_R0;
        
        // H = 雅可比矩阵 dh/dx
        edge_matrix_set(ctx->H, 0, 0, get_docv_dsoc(soc_next));
        edge_matrix_set(ctx->H, 0, 1, -1.0);
        
        edge_matrix_set(ctx->y, 0, 0, vt - y_hat);
        
        control_ekf_correct(&ctx->ekf, ctx->H, ctx->y);
        
        // 3. 输出结果
        double final_soc = edge_matrix_get(ctx->ekf.x, 0, 0);
        
        ev->tag_id = ctx->soc_out_tag;
        ev->payload.val.f = final_soc * 100.0; // output percentage
        return STAGE_OK;
    }

    return STAGE_DROP;
}

static void _ekf_soc_cleanup(void *priv) {
    ekf_soc_ctx_t *ctx = (ekf_soc_ctx_t *)priv;
    if (ctx) {
        edge_matrix_free(ctx->ekf.x);
        edge_matrix_free(ctx->ekf.P);
        edge_matrix_free(ctx->ekf.Q);
        edge_matrix_free(ctx->ekf.R);
        edge_matrix_free(ctx->F);
        edge_matrix_free(ctx->B);
        edge_matrix_free(ctx->u);
        edge_matrix_free(ctx->H);
        edge_matrix_free(ctx->y);
        free(ctx);
    }
}

/**
 * @brief EKF SOC 算子工厂
 * @param def u1=current_tag, u2=voltage_tag, u3=soc_out_tag
 */
pipeline_stage_t *stage_ekf_soc_create(uint32_t current_tag, uint32_t voltage_tag, uint32_t soc_out_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    ekf_soc_ctx_t *ctx = calloc(1, sizeof(ekf_soc_ctx_t));
    
    ctx->current_tag = current_tag;
    ctx->voltage_tag = voltage_tag;
    ctx->soc_out_tag = soc_out_tag;
    
    ctx->ekf.n = 2; // SOC, Vp
    ctx->ekf.x = edge_matrix_alloc(2, 1);
    edge_matrix_set(ctx->ekf.x, 0, 0, 0.5); // 初始 SOC 50%
    edge_matrix_set(ctx->ekf.x, 1, 0, 0.0); // 初始 Vp
    
    ctx->ekf.P = edge_matrix_alloc(2, 2);
    edge_matrix_identity(ctx->ekf.P);
    
    ctx->ekf.Q = edge_matrix_alloc(2, 2);
    edge_matrix_identity(ctx->ekf.Q);
    edge_matrix_scale(ctx->ekf.Q, 1e-5);
    
    ctx->ekf.R = edge_matrix_alloc(1, 1);
    edge_matrix_set(ctx->ekf.R, 0, 0, 1e-2);
    
    ctx->F = edge_matrix_alloc(2, 2);
    ctx->B = edge_matrix_alloc(2, 1);
    ctx->u = edge_matrix_alloc(1, 1);
    ctx->H = edge_matrix_alloc(1, 2);
    ctx->y = edge_matrix_alloc(1, 1);
    
    snprintf(s->name, 31, "EKF_SOC");
    s->process = _ekf_soc_proc;
    s->priv_state = ctx;
    s->cleanup_state = _ekf_soc_cleanup;
    
    return s;
}

pipeline_stage_t *stage_ekf_soc_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    return stage_ekf_soc_create(def->params.u1, def->params.u2, def->params.u3);
}
