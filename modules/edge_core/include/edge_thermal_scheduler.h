/**
 * @file edge_thermal_scheduler.h
 * @brief Industrial Thermal-Aware Dynamic Power Budgeting & Adaptive Degradation Scheduler (AEP-614)
 *
 * Implements a thermal RC physical model for junction temperature projection, dynamic power
 * budgeting arbitration, and 4-tier task criticality enforcement:
 *  - Level-A (Safety & Hard RT: Protection, VSG): 100% execution, 0% throttling.
 *  - Level-B (Periodic Optimization: SOC balancing, AGC): Minor frequency scaling allowed.
 *  - Level-C (Edge AI & Deep Learning): Frame rate reduction (50% -> 20% -> 0% paused).
 *  - Level-D (Telemetry & Non-critical Logging): Paused under high thermal load.
 *
 * Caller-allocated storage, zero dynamic memory allocation.
 */

#ifndef EDGE_THERMAL_SCHEDULER_H
#define EDGE_THERMAL_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CRITICALITY_LEVEL_A = 0, /* Safety & Hard RT - Never throttled / never suspended */
    CRITICALITY_LEVEL_B = 1, /* Periodic optimization - Minor throttle permissible */
    CRITICALITY_LEVEL_C = 2, /* Edge AI inference - Throttled & suspended in Degraded */
    CRITICALITY_LEVEL_D = 3  /* Telemetry / Log archiving - Suspended in Warning & Degraded */
} task_criticality_t;

typedef enum {
    THERMAL_ZONE_NORMAL    = 0, /* Tj < 70°C: Full speed, 100% budget */
    THERMAL_ZONE_WARNING   = 1, /* 70°C <= Tj < 85°C: Level-D paused, Level-C 50% fps */
    THERMAL_ZONE_DEGRADED  = 2, /* 85°C <= Tj < 95°C: Level-C/D paused, Level-B throttled, Level-A locked */
    THERMAL_ZONE_EMERGENCY = 3  /* Tj >= 95°C: Safety trip */
} thermal_zone_stage_t;

typedef struct {
    float temp_warn_threshold_c;      /* Default 70.0°C */
    float temp_degraded_threshold_c;  /* Default 85.0°C */
    float temp_emergency_threshold_c; /* Default 95.0°C */
    float thermal_resistance_r_th;    /* K/W (e.g. 1.2 K/W) */
    float thermal_capacitance_c_th;   /* J/K (e.g. 50.0 J/K) */
    float max_tdp_power_w;            /* Max thermal design power (e.g. 35.0 W) */
    float base_power_idle_w;          /* Static base idle power (e.g. 5.0 W) */
} thermal_scheduler_config_t;

typedef struct {
    bool     level_a_allowed;         /* Always true */
    float    level_b_throttle_pct;    /* 100.0% down to 50.0% */
    float    level_c_fps_ratio;       /* 1.0 down to 0.0 */
    bool     level_d_paused;          /* True in Warning / Degraded / Emergency */
    uint32_t npu_clock_mhz;           /* Target NPU frequency */
    uint32_t cpu_clock_mhz;           /* Target CPU frequency */
} thermal_task_quota_t;

typedef struct {
    thermal_scheduler_config_t config;
    float                      temp_cpu_junction;
    float                      temp_npu_junction;
    float                      temp_ambient_pcb;
    float                      estimated_power_w;
    float                      dynamic_power_budget_w;
    float                      predicted_temp_next_c;
    thermal_zone_stage_t       current_zone;
    thermal_task_quota_t       task_quota;
    uint32_t                   zone_transition_count;
    bool                       emergency_tripped;
    uint8_t                    _reserved[16];
} edge_thermal_scheduler_ctx_t;

/**
 * @brief Initialize thermal-aware scheduler context.
 */
int edge_thermal_scheduler_init(edge_thermal_scheduler_ctx_t *ctx, const thermal_scheduler_config_t *cfg);

/**
 * @brief Update temperatures from hardware sensors and estimate thermal RC trajectory.
 */
int edge_thermal_scheduler_update_sensors(edge_thermal_scheduler_ctx_t *ctx, float temp_cpu, float temp_npu, float temp_ambient, float dt_sec);

/**
 * @brief Arbitrate dynamic power budget and recalculate task quotas.
 */
int edge_thermal_scheduler_arbitrate(edge_thermal_scheduler_ctx_t *ctx, thermal_task_quota_t *out_quota);

/**
 * @brief Check if a task of a given criticality level is currently admissible.
 */
int edge_thermal_scheduler_check_task_admissible(const edge_thermal_scheduler_ctx_t *ctx, task_criticality_t crit, bool *out_admissible);

/**
 * @brief Query current thermal zone, power budget, and estimated power draw.
 */
int edge_thermal_scheduler_get_status(const edge_thermal_scheduler_ctx_t *ctx, thermal_zone_stage_t *out_zone, float *out_budget_w, float *out_est_power_w);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_THERMAL_SCHEDULER_H */
