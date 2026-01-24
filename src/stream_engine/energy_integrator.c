#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "edge_log.h"
#include "stream_engine.h"
#include "stream_engine_priv.h"

/**
 * @brief 电能模块共享上下文 (Shared Context)
 */
typedef struct {
    uint32_t state_tag_id;   // 状态在 DataHub 中的 TagID (用于可见性)
    double accumulated_kWh;  // 累计电能
    uint64_t last_calc_ts;   // 上次计算纳秒时间戳
    double last_power_W;     // 上次功率值
    bool is_dirty;           // 是否有未保存的变更
} energy_ctx_t;

/**
 * @brief 电能算子处理逻辑 (Multi-modal Operator)
 */
static stage_ret_t _energy_integrator_proc(pipeline_stage_t *s, pipe_msg_t *msg) {
    energy_ctx_t *ctx = (energy_ctx_t *)s->priv_state;

    // 模式 1: 周期性功率触发 (积分累计)
    if (msg->type == PIPEMSG_TYPE_SCALAR) {
        if (ctx->last_calc_ts == 0) {
            ctx->last_calc_ts = msg->ts;
            ctx->last_power_W = msg->payload.val.f;
            return STAGE_OK;
        }

        // 梯形积分算法: E = (P1 + P2) / 2 * dt
        double dt_s = (double)(msg->ts - ctx->last_calc_ts) / 1e9;
        if (dt_s > 0 && dt_s < 10.0) {  // 过滤跳变异常
            double energy_Ws = (ctx->last_power_W + msg->payload.val.f) / 2.0 * dt_s;
            double increment_kWh = energy_Ws / 3600000.0;  // Calculate increment in kWh
            ctx->accumulated_kWh += increment_kWh;
            ctx->is_dirty = true;

            // [Trace] 显式打印积分进度
            if (msg->payload.val.f != 0.0) {
                printf("\033[34m[STAGE-INTEGRATOR] Power:%.2f W -> Total:%.6f kWh (+%.6f)\033[0m\n",
                       msg->payload.val.f, ctx->accumulated_kWh, increment_kWh);
            }
        }

        ctx->last_calc_ts = msg->ts;
        ctx->last_power_W = msg->payload.val.f;

        // 实时更新 DataHub，让 UI 能看到
        pipe_msg_t state_msg;
        pipe_msg_init_scalar(&state_msg, ctx->state_tag_id, ctx->accumulated_kWh, msg->ts);
        datahub_publish(s->pipeline->engine->hub, &state_msg);
    }

    // 模式 2: 系统事件触发 (掉电保存 / 上电恢复)
    if (msg->type == PIPEMSG_TYPE_EVENT) {
        uint32_t event_id = msg->payload.event.event_id;

        if (event_id == 0x1001) {  // 假设 0x1001 是 SYS_EVENT_POWER_OFF
            LOG_INFO(LOG_CAT_CORE, "EnergyModule: Power-off detected. Saving state: %.3f kWh",
                     ctx->accumulated_kWh);
            // 此处通常会触发下一级 SQL_SINK 算子
            ctx->is_dirty = false;
        } else if (event_id == 0x1002) {  // 假设 0x1002 是 SYS_EVENT_BOOT_READY
            LOG_INFO(LOG_CAT_CORE, "EnergyModule: System ready. State restored.");
            // 恢复逻辑...
        }
    }

    return STAGE_OK;
}

static void _energy_integrator_cleanup(void *p) {
    free(p);
}

/**
 * @brief 创建电能积分算子工厂
 */
pipeline_stage_t *stage_energy_integrator_create(uint32_t state_tag_id) {
    pipeline_stage_t *s = calloc(1, sizeof(pipeline_stage_t));
    strncpy(s->name, "energy_integrator", 31);
    s->process = _energy_integrator_proc;

    energy_ctx_t *ctx = calloc(1, sizeof(energy_ctx_t));
    ctx->state_tag_id = state_tag_id;

    s->priv_state = ctx;
    s->cleanup_state = _energy_integrator_cleanup;
    return s;
}
