/**
 * @file stage_mpc_dispatcher.c
 * @brief 基于 MPC 的能量调度算子
 */

#include "stream_engine.h"
#include "edge_control.h"
#include "edge_log.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    uint32_t soc_tag;
    uint32_t out_pcs_tag;
    double dt;
    double capacity_ah;
    double target_soc;
    uint32_t horizon;

    edge_matrix_t *A, *B, *Q, *R;
    edge_matrix_t *A_cons, *b_cons;
    edge_matrix_t *x0;
    edge_matrix_t *H, *f;
    edge_matrix_t *u_opt;
} mpc_dispatcher_ctx_t;

static stage_ret_t _mpc_dispatcher_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    mpc_dispatcher_ctx_t *ctx = (mpc_dispatcher_ctx_t *)stage->priv_state;
    if (!ctx || ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_ERROR;

    if (ev->tag_id == ctx->soc_tag) {
        double current_soc = ev->payload.val.f / 100.0; // Assume input is 0-100%
        LOG_DEBUG(LOG_CAT_CORE, "MPC_DISPATCH: Received SOC %.2f%%", ev->payload.val.f);
        edge_matrix_set(ctx->x0, 0, 0, current_soc - ctx->target_soc); // Error state

        // 1. 构造 QP 问题
        control_mpc_formulate(ctx->A, ctx->B, ctx->Q, ctx->R, ctx->x0, ctx->horizon, ctx->H, ctx->f);

        // 2. 求解 QP (极小化 1/2 u'Hu + f'u)
        // 此处暂未加入约束 A_cons * u <= b_cons，Hildreth 仅演示无约束或简单界限
        control_qp_solve_hildreth(ctx->H, ctx->f, ctx->A_cons, ctx->b_cons, ctx->u_opt);

        // 3. 取第一个控制量
        double p_pcs = edge_matrix_get(ctx->u_opt, 0, 0);
        LOG_INFO(LOG_CAT_CORE, "MPC_DISPATCH: Optimal P_PCS = %.2f A", p_pcs);

        ev->tag_id = ctx->out_pcs_tag;
        ev->payload.val.f = p_pcs;
        return STAGE_OK;
    }

    return STAGE_DROP;
}

static void _mpc_dispatcher_cleanup(void *priv) {
    mpc_dispatcher_ctx_t *ctx = (mpc_dispatcher_ctx_t *)priv;
    if (ctx) {
        edge_matrix_free(ctx->A);
        edge_matrix_free(ctx->B);
        edge_matrix_free(ctx->Q);
        edge_matrix_free(ctx->R);
        edge_matrix_free(ctx->A_cons);
        edge_matrix_free(ctx->b_cons);
        edge_matrix_free(ctx->x0);
        edge_matrix_free(ctx->H);
        edge_matrix_free(ctx->f);
        edge_matrix_free(ctx->u_opt);
        free(ctx);
    }
}

/**
 * @brief MPC 调度算子工厂
 * @param def u1=soc_tag, u3=out_pcs_tag, f1=dt, f2=capacity_ah, f3=target_soc
 */
pipeline_stage_t *stage_mpc_dispatcher_create(uint32_t soc_tag, uint32_t out_pcs_tag, double dt, double capacity_ah, double target_soc) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    mpc_dispatcher_ctx_t *ctx = calloc(1, sizeof(mpc_dispatcher_ctx_t));

    ctx->soc_tag = soc_tag;
    ctx->out_pcs_tag = out_pcs_tag;
    ctx->dt = dt > 0 ? dt : 1.0;
    ctx->capacity_ah = capacity_ah > 0 ? capacity_ah : 100.0;
    ctx->target_soc = target_soc / 100.0;
    ctx->horizon = 5;

    // 状态空间模型: x(k+1) = A*x(k) + B*u(k)
    // SOC(k+1) = SOC(k) + (P * dt) / (Capacity * Voltage * 3600)
    // 简化: B = dt / Capacity_Ah_in_seconds
    ctx->A = edge_matrix_alloc(1, 1);
    edge_matrix_set(ctx->A, 0, 0, 1.0);
    ctx->B = edge_matrix_alloc(1, 1);
    edge_matrix_set(ctx->B, 0, 0, ctx->dt / (ctx->capacity_ah * 3600.0));

    // 权重矩阵
    ctx->Q = edge_matrix_alloc(1, 1);
    edge_matrix_set(ctx->Q, 0, 0, 1000.0); // 惩罚 SOC 偏差
    ctx->R = edge_matrix_alloc(1, 1);
    edge_matrix_set(ctx->R, 0, 0, 0.1);    // 惩罚控制能量

    // 约束 (演示: -100kW <= P <= 100kW)
    ctx->A_cons = edge_matrix_alloc(2, ctx->horizon); // 简化为全 0，即无约束
    ctx->b_cons = edge_matrix_alloc(2, 1);
    edge_matrix_set(ctx->b_cons, 0, 0, 1e9);
    edge_matrix_set(ctx->b_cons, 1, 0, 1e9);

    ctx->x0 = edge_matrix_alloc(1, 1);
    ctx->H = edge_matrix_alloc(ctx->horizon, ctx->horizon);
    ctx->f = edge_matrix_alloc(ctx->horizon, 1);
    ctx->u_opt = edge_matrix_alloc(ctx->horizon, 1);

    snprintf(s->name, 31, "MPC_DISPATCH");
    s->process = _mpc_dispatcher_proc;
    s->priv_state = ctx;
    s->cleanup_state = _mpc_dispatcher_cleanup;

    return s;
}

pipeline_stage_t *stage_mpc_dispatch_create_v2(const edge_stage_def_t *def, pipeline_loader_ctx_t *ctx) {
    (void)ctx;
    return stage_mpc_dispatcher_create(def->params.u1, def->params.u3, def->params.f1, def->params.f2, def->params.f3);
}
