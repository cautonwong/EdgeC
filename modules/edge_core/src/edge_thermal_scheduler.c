/**
 * @file edge_thermal_scheduler.c
 * @brief Industrial Thermal-Aware Dynamic Power Budgeting & Adaptive Degradation Scheduler Implementation (AEP-614)
 */

#include "edge_thermal_scheduler.h"
#include <string.h>

static inline float _max_f(float a, float b) {
    return (a >= b) ? a : b;
}

int edge_thermal_scheduler_init(edge_thermal_scheduler_ctx_t *ctx, const thermal_scheduler_config_t *cfg) {
    if (!ctx) return -1;

    memset(ctx, 0, sizeof(*ctx));

    if (cfg) {
        ctx->config = *cfg;
    } else {
        ctx->config.temp_warn_threshold_c = 70.0f;
        ctx->config.temp_degraded_threshold_c = 85.0f;
        ctx->config.temp_emergency_threshold_c = 95.0f;
        ctx->config.thermal_resistance_r_th = 1.2f;
        ctx->config.thermal_capacitance_c_th = 50.0f;
        ctx->config.max_tdp_power_w = 35.0f;
        ctx->config.base_power_idle_w = 5.0f;
    }

    if (ctx->config.temp_warn_threshold_c <= 0.0f) ctx->config.temp_warn_threshold_c = 70.0f;
    if (ctx->config.temp_degraded_threshold_c <= 0.0f) ctx->config.temp_degraded_threshold_c = 85.0f;
    if (ctx->config.temp_emergency_threshold_c <= 0.0f) ctx->config.temp_emergency_threshold_c = 95.0f;
    if (ctx->config.thermal_resistance_r_th <= 0.0f) ctx->config.thermal_resistance_r_th = 1.2f;
    if (ctx->config.thermal_capacitance_c_th <= 0.0f) ctx->config.thermal_capacitance_c_th = 50.0f;
    if (ctx->config.max_tdp_power_w <= 0.0f) ctx->config.max_tdp_power_w = 35.0f;

    ctx->temp_cpu_junction = 45.0f;
    ctx->temp_npu_junction = 45.0f;
    ctx->temp_ambient_pcb = 35.0f;
    ctx->estimated_power_w = ctx->config.base_power_idle_w;
    ctx->dynamic_power_budget_w = ctx->config.max_tdp_power_w;
    ctx->current_zone = THERMAL_ZONE_NORMAL;

    ctx->task_quota.level_a_allowed = true;
    ctx->task_quota.level_b_throttle_pct = 100.0f;
    ctx->task_quota.level_c_fps_ratio = 1.0f;
    ctx->task_quota.level_d_paused = false;
    ctx->task_quota.cpu_clock_mhz = 2000;
    ctx->task_quota.npu_clock_mhz = 1000;

    return 0;
}

int edge_thermal_scheduler_update_sensors(edge_thermal_scheduler_ctx_t *ctx, float temp_cpu, float temp_npu, float temp_ambient, float dt_sec) {
    if (!ctx) return -1;
    if (dt_sec <= 0.0f) dt_sec = 0.1f;

    ctx->temp_cpu_junction = temp_cpu;
    ctx->temp_npu_junction = temp_npu;
    ctx->temp_ambient_pcb = temp_ambient;

    float max_tj = _max_f(temp_cpu, temp_npu);

    /* 1. Thermal RC model: dT/dt = (P_in - (T_j - T_amb) / R_th) / C_th */
    float p_loss = (max_tj - temp_ambient) / ctx->config.thermal_resistance_r_th;
    if (p_loss < 0.0f) p_loss = 0.0f;

    /* Current power consumption estimate based on active load */
    float p_active = p_loss;
    if (p_active < ctx->config.base_power_idle_w) {
        p_active = ctx->config.base_power_idle_w;
    }
    ctx->estimated_power_w = p_active;

    /* Forward predictive temperature: T_next = T_j + (dT/dt) * dt */
    float dt_dt = (ctx->estimated_power_w - p_loss) / ctx->config.thermal_capacitance_c_th;
    ctx->predicted_temp_next_c = max_tj + dt_dt * dt_sec;

    /* 2. Zone Evaluation */
    thermal_zone_stage_t prev_zone = ctx->current_zone;

    if (max_tj >= ctx->config.temp_emergency_threshold_c) {
        ctx->current_zone = THERMAL_ZONE_EMERGENCY;
        ctx->emergency_tripped = true;
    } else if (max_tj >= ctx->config.temp_degraded_threshold_c) {
        ctx->current_zone = THERMAL_ZONE_DEGRADED;
    } else if (max_tj >= ctx->config.temp_warn_threshold_c) {
        ctx->current_zone = THERMAL_ZONE_WARNING;
    } else {
        ctx->current_zone = THERMAL_ZONE_NORMAL;
    }

    if (prev_zone != ctx->current_zone) {
        ctx->zone_transition_count++;
    }

    return 0;
}

int edge_thermal_scheduler_arbitrate(edge_thermal_scheduler_ctx_t *ctx, thermal_task_quota_t *out_quota) {
    if (!ctx) return -1;

    float max_tj = _max_f(ctx->temp_cpu_junction, ctx->temp_npu_junction);

    /* Compute available dynamic thermal headroom */
    float temp_headroom = ctx->config.temp_emergency_threshold_c - max_tj;
    if (temp_headroom < 0.0f) temp_headroom = 0.0f;

    float budget_factor = temp_headroom / (ctx->config.temp_emergency_threshold_c - ctx->config.temp_warn_threshold_c);
    if (budget_factor > 1.0f) budget_factor = 1.0f;
    if (budget_factor < 0.15f) budget_factor = 0.15f;

    ctx->dynamic_power_budget_w = ctx->config.base_power_idle_w +
        (ctx->config.max_tdp_power_w - ctx->config.base_power_idle_w) * budget_factor;

    /* Enforce 4-tier task criticality */
    ctx->task_quota.level_a_allowed = true; /* Level-A is NEVER throttled */

    switch (ctx->current_zone) {
    case THERMAL_ZONE_NORMAL:
        ctx->task_quota.level_b_throttle_pct = 100.0f;
        ctx->task_quota.level_c_fps_ratio = 1.0f;
        ctx->task_quota.level_d_paused = false;
        ctx->task_quota.cpu_clock_mhz = 2000;
        ctx->task_quota.npu_clock_mhz = 1000;
        break;

    case THERMAL_ZONE_WARNING:
        ctx->task_quota.level_b_throttle_pct = 90.0f;
        ctx->task_quota.level_c_fps_ratio = 0.5f; /* 50% frame rate */
        ctx->task_quota.level_d_paused = true;     /* Pause background logs */
        ctx->task_quota.cpu_clock_mhz = 1600;
        ctx->task_quota.npu_clock_mhz = 750;
        break;

    case THERMAL_ZONE_DEGRADED:
        ctx->task_quota.level_b_throttle_pct = 70.0f;
        ctx->task_quota.level_c_fps_ratio = 0.0f; /* Suspend edge AI */
        ctx->task_quota.level_d_paused = true;     /* Suspend logs */
        ctx->task_quota.cpu_clock_mhz = 1200;
        ctx->task_quota.npu_clock_mhz = 400;
        break;

    case THERMAL_ZONE_EMERGENCY:
    default:
        ctx->task_quota.level_b_throttle_pct = 50.0f;
        ctx->task_quota.level_c_fps_ratio = 0.0f;
        ctx->task_quota.level_d_paused = true;
        ctx->task_quota.cpu_clock_mhz = 800;
        ctx->task_quota.npu_clock_mhz = 0;
        break;
    }

    if (out_quota) {
        *out_quota = ctx->task_quota;
    }

    return 0;
}

int edge_thermal_scheduler_check_task_admissible(const edge_thermal_scheduler_ctx_t *ctx, task_criticality_t crit, bool *out_admissible) {
    if (!ctx || !out_admissible) return -1;

    switch (crit) {
    case CRITICALITY_LEVEL_A:
        *out_admissible = ctx->task_quota.level_a_allowed;
        break;
    case CRITICALITY_LEVEL_B:
        *out_admissible = (ctx->task_quota.level_b_throttle_pct > 0.0f);
        break;
    case CRITICALITY_LEVEL_C:
        *out_admissible = (ctx->task_quota.level_c_fps_ratio > 0.01f);
        break;
    case CRITICALITY_LEVEL_D:
        *out_admissible = !ctx->task_quota.level_d_paused;
        break;
    default:
        *out_admissible = false;
        break;
    }

    return 0;
}

int edge_thermal_scheduler_get_status(const edge_thermal_scheduler_ctx_t *ctx, thermal_zone_stage_t *out_zone, float *out_budget_w, float *out_est_power_w) {
    if (!ctx) return -1;
    if (out_zone) *out_zone = ctx->current_zone;
    if (out_budget_w) *out_budget_w = ctx->dynamic_power_budget_w;
    if (out_est_power_w) *out_est_power_w = ctx->estimated_power_w;
    return 0;
}
