/**
 * @file stage_ahrs_quaternion.c
 * @brief 航空航天级 AHRS 姿态解算算子 (基于四元数避免万向节死锁)
 */

#include "stream_engine.h"
#include "edge_linalg.h"
#include "edge_control.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    edge_matrix_t *q;      // 当前四元数状态 [w, x, y, z]
    edge_matrix_t *dq;     // 临时四元数变化量
    edge_matrix_t *q_new;  // 临时四元数新状态
    uint32_t gyro_tag;     // 陀螺仪输入 Tag
    uint32_t output_tag;   // 姿态输出 Tag
    uint64_t last_ts;
    control_ekf_t ekf;     // 可选的 EKF 融合
} ahrs_ctx_t;

/**
 * @brief AHRS 姿态更新处理
 * 输入: 角速度 [gx, gy, gz] (rad/s)
 * 输出: 四元数 [w, x, y, z]
 */
static stage_ret_t _ahrs_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    ahrs_ctx_t *ctx = (ahrs_ctx_t *)stage->priv_state;
    if (!ctx) return STAGE_ERROR;

    if (ev->tag_id == ctx->gyro_tag && ev->type == PIPEMSG_TYPE_BATCH && ev->payload.batch.count >= 3) {
        uint64_t now = ev->ts;
        double dt = (ctx->last_ts == 0) ? 0.01 : (double)(now - ctx->last_ts) / 1000.0;
        if (dt <= 0) dt = 0.001;
        ctx->last_ts = now;

        double *gyro = (double *)ev->payload.batch.ptr;
        double gx = gyro[0], gy = gyro[1], gz = gyro[2];

        // 构造角速度四元数 dq = [0, gx*dt/2, gy*dt/2, gz*dt/2]
        // 由于是高频小角度近似: q_new = q * dq
        // 也可以使用更精确的 q_new = q + 0.5 * q * [0, gx, gy, gz] * dt
        edge_matrix_set(ctx->dq, 0, 0, 1.0); // w 约等于 1
        edge_matrix_set(ctx->dq, 1, 0, gx * dt * 0.5);
        edge_matrix_set(ctx->dq, 2, 0, gy * dt * 0.5);
        edge_matrix_set(ctx->dq, 3, 0, gz * dt * 0.5);
        edge_matrix_quat_normalize(ctx->dq);

        // 更新状态: q_new = q * dq
        edge_matrix_quat_mul(ctx->q, ctx->dq, ctx->q_new);
        edge_matrix_quat_normalize(ctx->q_new);

        // 覆盖回 q
        edge_matrix_copy(ctx->q, ctx->q_new);

        // 发布输出
        ev->tag_id = ctx->output_tag;
        gyro[0] = edge_matrix_get(ctx->q, 0, 0); // w
        gyro[1] = edge_matrix_get(ctx->q, 1, 0); // x
        gyro[2] = edge_matrix_get(ctx->q, 2, 0); // y
        gyro[3] = edge_matrix_get(ctx->q, 3, 0); // z
        ev->payload.batch.count = 4;
        
        return STAGE_OK;
    }

    return STAGE_DROP;
}

static void _ahrs_cleanup(void *priv) {
    ahrs_ctx_t *ctx = (ahrs_ctx_t *)priv;
    if (ctx) {
        if (ctx->q) edge_matrix_free(ctx->q);
        if (ctx->dq) edge_matrix_free(ctx->dq);
        if (ctx->q_new) edge_matrix_free(ctx->q_new);
        free(ctx);
    }
}

/**
 * @brief AHRS 算子工厂
 */
pipeline_stage_t *stage_ahrs_quaternion_create(uint32_t gyro_tag, uint32_t output_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    ahrs_ctx_t *ctx = calloc(1, sizeof(ahrs_ctx_t));

    ctx->gyro_tag = gyro_tag;
    ctx->output_tag = output_tag;

    ctx->q = edge_matrix_alloc(4, 1);
    ctx->dq = edge_matrix_alloc(4, 1);
    ctx->q_new = edge_matrix_alloc(4, 1);
    
    // 初始化为单位四元数
    edge_matrix_set(ctx->q, 0, 0, 1.0);
    edge_matrix_set(ctx->q, 1, 0, 0.0);
    edge_matrix_set(ctx->q, 2, 0, 0.0);
    edge_matrix_set(ctx->q, 3, 0, 0.0);

    strncpy(s->name, "AHRS_QUATERNION", 31);
    s->process = _ahrs_proc;
    s->priv_state = ctx;
    s->cleanup_state = _ahrs_cleanup;

    LOG_INFO(LOG_CAT_CORE, "AHRS Quaternion Stage Created: GyroTag=%u, OutTag=%u", gyro_tag, output_tag);
    return s;
}

pipeline_stage_t *stage_ahrs_quaternion_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    return stage_ahrs_quaternion_create(def->params.u1, def->params.u2);
}
