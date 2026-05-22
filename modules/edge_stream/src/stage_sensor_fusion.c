/**
 * @file stage_sensor_fusion.c
 * @brief 工业级传感器融合算子 (IMU + GPS EKF)
 */

#include "stream_engine.h"
#include "edge_control.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    control_ekf_t ekf;
    uint32_t imu_tag;
    uint32_t gps_tag;
    uint32_t output_tag;
    
    edge_matrix_t *F, *H, *B, *u, *z;
    uint64_t last_ts;
} fusion_st_t;

/**
 * @brief 传感器融合处理
 */
static stage_ret_t _fusion_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    fusion_st_t *ctx = (fusion_st_t *)stage->priv_state;
    if (!ctx) return STAGE_ERROR;

    uint64_t now = ev->ts;
    double dt = (ctx->last_ts == 0) ? 0.01 : (double)(now - ctx->last_ts) / 1000.0;
    if (dt <= 0) dt = 0.001;
    ctx->last_ts = now;

    if (ev->tag_id == ctx->imu_tag && ev->type == PIPEMSG_TYPE_BATCH) {
        double *data = (double *)ev->payload.batch.ptr;
        double acc = data[0];
        
        // 1. 手动更新状态 x (f(x, u))
        // pos = pos + vel*dt + 0.5*acc*dt^2
        // vel = vel + acc*dt
        double pos = edge_matrix_get(ctx->ekf.x, 0, 0);
        double vel = edge_matrix_get(ctx->ekf.x, 1, 0);
        edge_matrix_set(ctx->ekf.x, 0, 0, pos + vel * dt + 0.5 * acc * dt * dt);
        edge_matrix_set(ctx->ekf.x, 1, 0, vel + acc * dt);

        // 2. 更新协方差 P (预测步)
        edge_matrix_set(ctx->F, 0, 1, dt);
        control_ekf_predict(&ctx->ekf, ctx->F, NULL, NULL);
        
        return STAGE_DROP;
    } 
    else if (ev->tag_id == ctx->gps_tag && ev->type == PIPEMSG_TYPE_BATCH) {
        double *data = (double *)ev->payload.batch.ptr;
        edge_matrix_set(ctx->z, 0, 0, data[0]); // Observed Pos
        
        // 3. 计算残差并校正
        edge_matrix_t *hx = edge_matrix_alloc(1, 1);
        edge_matrix_mul(ctx->H, ctx->ekf.x, hx);
        edge_matrix_sub(ctx->z, hx, hx); // reuse hx for residual y
        
        control_ekf_correct(&ctx->ekf, ctx->H, hx);
        edge_matrix_free(hx);
        
        ev->tag_id = ctx->output_tag;
        data[0] = edge_matrix_get(ctx->ekf.x, 0, 0); // Fused Pos
        data[1] = edge_matrix_get(ctx->ekf.x, 1, 0); // Fused Vel
        ev->payload.batch.count = 2;
        return STAGE_OK;
    }

    return STAGE_DROP;
}

static void _fusion_cleanup(void *priv) {
    fusion_st_t *ctx = (fusion_st_t *)priv;
    if (ctx) {
        if (ctx->ekf.x) edge_matrix_free(ctx->ekf.x);
        if (ctx->ekf.P) edge_matrix_free(ctx->ekf.P);
        if (ctx->ekf.Q) edge_matrix_free(ctx->ekf.Q);
        if (ctx->ekf.R) edge_matrix_free(ctx->ekf.R);
        if (ctx->F) edge_matrix_free(ctx->F);
        if (ctx->H) edge_matrix_free(ctx->H);
        if (ctx->B) edge_matrix_free(ctx->B);
        if (ctx->u) edge_matrix_free(ctx->u);
        if (ctx->z) edge_matrix_free(ctx->z);
        free(ctx);
    }
}

pipeline_stage_t *stage_sensor_fusion_create(uint32_t imu_tag, uint32_t gps_tag, uint32_t out_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    fusion_st_t *ctx = calloc(1, sizeof(fusion_st_t));

    ctx->imu_tag = imu_tag;
    ctx->gps_tag = gps_tag;
    ctx->output_tag = out_tag;

    ctx->ekf.n = 2;
    ctx->ekf.x = edge_matrix_alloc(2, 1);
    ctx->ekf.P = edge_matrix_alloc(2, 2);
    ctx->ekf.Q = edge_matrix_alloc(2, 2);
    ctx->ekf.R = edge_matrix_alloc(1, 1);
    
    edge_matrix_identity(ctx->ekf.P);
    edge_matrix_scale(ctx->ekf.P, 1.0);
    edge_matrix_set(ctx->ekf.Q, 0, 0, 0.0001);
    edge_matrix_set(ctx->ekf.Q, 1, 1, 0.0001);
    edge_matrix_set(ctx->ekf.R, 0, 0, 0.1); 

    ctx->F = edge_matrix_alloc(2, 2);
    edge_matrix_identity(ctx->F);
    ctx->H = edge_matrix_alloc(1, 2);
    edge_matrix_set(ctx->H, 0, 0, 1.0);
    ctx->B = edge_matrix_alloc(2, 1);
    ctx->u = edge_matrix_alloc(1, 1);
    ctx->z = edge_matrix_alloc(1, 1);

    strncpy(s->name, "SENSOR_FUSION", 31);
    s->process = _fusion_proc;
    s->priv_state = ctx;
    s->cleanup_state = _fusion_cleanup;

    return s;
}

pipeline_stage_t *stage_sensor_fusion_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    return stage_sensor_fusion_create(def->params.u1, def->params.u2, def->params.u3);
}
