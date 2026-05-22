/**
 * @file stage_advanced_pid.c
 * @brief 高级 PID 控制算子 (支持抗积分饱和)
 */

#include "stream_engine.h"
#include "edge_control.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    pid_adv_t pid;
    uint32_t output_tag;
    uint64_t last_ts;
} advanced_pid_ctx_t;

static stage_ret_t _pid_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    advanced_pid_ctx_t *ctx = (advanced_pid_ctx_t *)stage->priv_state;
    if (!ctx || ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_ERROR;

    uint64_t now_ms = ev->ts;
    double dt = 0.0;
    if (ctx->last_ts != 0) {
        dt = (double)(now_ms - ctx->last_ts) / 1000.0; // convert ms to seconds
    }
    ctx->last_ts = now_ms;

    // First sample doesn't have a valid dt, so we just set last_input and return 0 (or drop)
    if (dt <= 0.0) {
        ctx->pid.last_input = ev->payload.val.f;
        return STAGE_DROP;
    }

    double output = control_pid_compute(&ctx->pid, ev->payload.val.f, dt);

    ev->tag_id = ctx->output_tag;
    ev->payload.val.f = output;
    
    return STAGE_OK;
}

static void _pid_cleanup(void *priv) {
    if (priv) free(priv);
}

/**
 * @brief PID 算子工厂
 * @param def f1=Kp, f2=Ki, f3=Kd, u1=output_tag, s1=JSON(setpoint, out_min, out_max)
 */
pipeline_stage_t *stage_advanced_pid_create(double kp, double ki, double kd, double setpoint, double out_min, double out_max, uint32_t out_tag) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    advanced_pid_ctx_t *ctx = calloc(1, sizeof(advanced_pid_ctx_t));
    
    control_pid_init(&ctx->pid, kp, ki, kd);
    ctx->pid.setpoint = setpoint;
    ctx->pid.out_min = out_min;
    ctx->pid.out_max = out_max;
    ctx->output_tag = out_tag;
    ctx->last_ts = 0;
    
    snprintf(s->name, 31, "Advanced_PID");
    s->process = _pid_proc;
    s->priv_state = ctx;
    s->cleanup_state = _pid_cleanup;
    
    return s;
}
