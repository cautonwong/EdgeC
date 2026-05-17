/**
 * @file stage_kinematics.c
 * @brief 工业级机器人运动学解算算子 (Inverse Kinematics)
 */

#include "stream_engine.h"
#include "edge_linalg.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    // SCARA 机械臂参数 (DH)
    double l1;          // 第一臂长
    double l2;          // 第二臂长
    uint32_t output_tag;
} kinematics_st_t;

/**
 * @brief SCARA 逆运动学解算
 * 输入向量 (PIPEMSG_TYPE_BATCH): [X, Y, Z, Yaw]
 * 输出向量 (PIPEMSG_TYPE_BATCH): [q1, q2, q3, q4]
 */
static stage_ret_t _kinematics_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    kinematics_st_t *ctx = (kinematics_st_t *)stage->priv_state;
    if (!ctx || ev->type != PIPEMSG_TYPE_BATCH || ev->payload.batch.count < 4) {
        return STAGE_ERROR;
    }

    double *data = (double *)ev->payload.batch.ptr;
    double x = data[0];
    double y = data[1];
    double z = data[2];
    double yaw = data[3];

    // 1. 计算 q2 (第二关节角)
    // cos(q2) = (x^2 + y^2 - l1^2 - l2^2) / (2 * l1 * l2)
    double dist_sq = x * x + y * y;
    double cos_q2 = (dist_sq - ctx->l1 * ctx->l1 - ctx->l2 * ctx->l2) / (2.0 * ctx->l1 * ctx->l2);
    
    // 检查工作空间
    if (fabs(cos_q2) > 1.0) {
        LOG_WARN(LOG_CAT_CORE, "Kinematics: Desired target (%.2f, %.2f) out of reach!", x, y);
        return STAGE_DROP;
    }

    double sin_q2 = sqrt(1.0 - cos_q2 * cos_q2); // 肘部向下解 (Elbow Down)
    double q2 = atan2(sin_q2, cos_q2);

    // 2. 计算 q1 (第一关节角)
    // q1 = atan2(y, x) - atan2(l2*sin_q2, l1 + l2*cos_q2)
    double k1 = ctx->l1 + ctx->l2 * cos_q2;
    double k2 = ctx->l2 * sin_q2;
    double q1 = atan2(y, x) - atan2(k2, k1);

    // 3. 计算 q3 (Z轴，直接对应)
    double q3 = z;

    // 4. 计算 q4 (末端旋转)
    double q4 = yaw - q1 - q2;

    // 5. 输出结果
    ev->tag_id = ctx->output_tag;
    data[0] = q1 * 180.0 / M_PI; // 转换为度
    data[1] = q2 * 180.0 / M_PI;
    data[2] = q3;
    data[3] = q4 * 180.0 / M_PI;
    ev->payload.batch.count = 4;

    return STAGE_OK;
}

static void _kinematics_cleanup(void *priv) {
    kinematics_st_t *ctx = (kinematics_st_t *)priv;
    if (ctx) free(ctx);
}

/**
 * @brief 运动学解算算子工厂
 * 参数: u1=output_tag, f1=l1, f2=l2
 */
pipeline_stage_t *stage_kinematics_create(uint32_t output_tag, double l1, double l2) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    kinematics_st_t *ctx = calloc(1, sizeof(kinematics_st_t));

    ctx->output_tag = output_tag;
    ctx->l1 = l1 > 0 ? l1 : 0.4;
    ctx->l2 = l2 > 0 ? l2 : 0.3;

    strncpy(s->name, "KINEMATICS_IK", 31);
    s->process = _kinematics_proc;
    s->priv_state = ctx;
    s->cleanup_state = _kinematics_cleanup;

    return s;
}

pipeline_stage_t *stage_kinematics_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    return stage_kinematics_create((uint32_t)def->params.u1, def->params.f1, def->params.f2);
}
