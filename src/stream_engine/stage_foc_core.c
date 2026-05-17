/**
 * @file stage_foc_core.c
 * @brief 工业级高性能电机 FOC 核心算子
 */

#include "stream_engine.h"
#include "edge_motor.h"
#include "edge_control.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    // 算法参数
    double rs;          // 相电阻
    double ls;          // 相电感
    double v_dc;        // 母线电压
    double dt;          // 采样周期
    
    // PI 控制器
    pid_adv_t id_pid;
    pid_adv_t iq_pid;
    
    // 状态记录
    double last_theta;
    uint32_t output_tag;
} foc_core_ctx_t;

/**
 * @brief FOC 核心处理函数
 * 输入向量 (PIPEMSG_TYPE_BATCH): [Ia, Ib, Theta, Id_ref, Iq_ref]
 * 输出向量 (PIPEMSG_TYPE_BATCH): [DutyA, DutyB, DutyC]
 */
static stage_ret_t _foc_core_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    foc_core_ctx_t *ctx = (foc_core_ctx_t *)stage->priv_state;
    if (!ctx || ev->type != PIPEMSG_TYPE_BATCH || ev->payload.batch.count < 5) {
        return STAGE_ERROR;
    }

    double *data = (double *)ev->payload.batch.ptr;
    double ia = data[0];
    double ib = data[1];
    double theta = data[2];
    double id_ref = data[3];
    double iq_ref = data[4];

    // 1. Clark Transform (Ic = -(Ia + Ib))
    coord_alphabeta_t alpha_beta_i;
    motor_clark(ia, ib, -(ia + ib), &alpha_beta_i);

    // 2. Park Transform
    coord_dq_t dq_i;
    motor_park(&alpha_beta_i, theta, &dq_i);

    // 3. Current PI Control (Inner Loop)
    ctx->id_pid.setpoint = id_ref;
    ctx->iq_pid.setpoint = iq_ref;
    double vd_ref = control_pid_compute(&ctx->id_pid, dq_i.d, ctx->dt);
    double vq_ref = control_pid_compute(&ctx->iq_pid, dq_i.q, ctx->dt);

    // 4. Inverse Park Transform
    coord_dq_t dq_v = {vd_ref, vq_ref};
    coord_alphabeta_t alpha_beta_v;
    motor_inv_park(&dq_v, theta, &alpha_beta_v);

    // 5. SVPWM
    double duties[3];
    motor_svpwm(&alpha_beta_v, ctx->v_dc, duties);

    // 6. Write Result to Vector (Reuse pointer if possible, but safe here)
    ev->tag_id = ctx->output_tag;
    data[0] = duties[0];
    data[1] = duties[1];
    data[2] = duties[2];
    ev->payload.batch.count = 3;

    return STAGE_OK;
}

static void _foc_core_cleanup(void *priv) {
    foc_core_ctx_t *ctx = (foc_core_ctx_t *)priv;
    if (ctx) free(ctx);
}

/**
 * @brief FOC 核心算子工厂
 */
pipeline_stage_t *stage_foc_core_create(uint32_t output_tag, double v_dc, double dt, double kp_i, double ki_i) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    foc_core_ctx_t *ctx = calloc(1, sizeof(foc_core_ctx_t));

    ctx->output_tag = output_tag;
    ctx->v_dc = v_dc > 0 ? v_dc : 24.0;
    ctx->dt = dt > 0 ? dt : 0.0001; // 10kHz

    // 初始化 PI 参数
    control_pid_init(&ctx->id_pid, kp_i, ki_i, 0);
    ctx->id_pid.out_max = ctx->v_dc;
    ctx->id_pid.out_min = -ctx->v_dc;

    control_pid_init(&ctx->iq_pid, kp_i, ki_i, 0);
    ctx->iq_pid.out_max = ctx->v_dc;
    ctx->iq_pid.out_min = -ctx->v_dc;

    strncpy(s->name, "FOC_CORE", 31);
    s->process = _foc_core_proc;
    s->priv_state = ctx;
    s->cleanup_state = _foc_core_cleanup;

    return s;
}

pipeline_stage_t *stage_foc_core_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *loader_ctx) {
    (void)loader_ctx;
    return stage_foc_core_create((uint32_t)def->params.u1, def->params.f1, def->params.f2, def->params.f3, def->params.f3 / 10.0);
}
