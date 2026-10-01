/**
 * @file npu_power_manager.h
 * @brief NPU Dynamic Clock Gating, DVFS & Ultra-low Power Wakeup Driver (AEP-750 / Issue #280)
 *
 * Provides microsecond-level hardware clock gating state machines (ACTIVE -> IDLE_GATE -> STANDBY -> DEEP_SLEEP),
 * Dynamic Voltage and Frequency Scaling (DVFS), thermal-adaptive throttling, energy accounting,
 * fast tensor arrival wakeup (< 50us), and DataHub power telemetry synchronization.
 */

#ifndef EDGE_NPU_POWER_MANAGER_H
#define EDGE_NPU_POWER_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include "npu_hal.h"
#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants & Enums                                                          */
/* -------------------------------------------------------------------------- */

typedef enum {
    NPU_PWR_OFF            = 0,
    NPU_PWR_ACTIVE         = 1, /**< Running at active frequency (e.g. 1000MHz), ~8500mW */
    NPU_PWR_IDLE_GATE      = 2, /**< Clock dynamically gated on idle, ~450mW, wakeup <= 15us */
    NPU_PWR_STANDBY_SLEEP  = 3, /**< Compute core power gated, ~80mW, wakeup <= 45us */
    NPU_PWR_DEEP_SLEEP     = 4  /**< Deep retention sleep, ~5mW, wakeup <= 200us */
} npu_power_state_t;

typedef enum {
    NPU_DVFS_POWERSAVE        = 0, /**< Lowest stable operating frequency */
    NPU_DVFS_PERFORMANCE      = 1, /**< Fixed maximum frequency */
    NPU_DVFS_ONDEMAND         = 2, /**< Dynamic load-based frequency scaling */
    NPU_DVFS_THERMAL_ADAPTIVE = 3  /**< Thermal envelope closed-loop throttling */
} npu_dvfs_governor_t;

/**
 * @brief Power & Frequency Level Profile
 */
typedef struct {
    uint32_t freq_mhz;
    uint32_t voltage_mv;
    float active_power_mw;
} npu_opp_point_t;

/**
 * @brief Power Manager Configuration
 */
typedef struct {
    npu_dvfs_governor_t governor;
    uint32_t idle_gate_timeout_ms;       /**< Idle duration before entering IDLE_GATE (default: 5ms) */
    uint32_t standby_timeout_ms;         /**< Idle duration before entering STANDBY (default: 50ms) */
    uint32_t deep_sleep_timeout_ms;      /**< Idle duration before entering DEEP_SLEEP (default: 500ms) */
    float thermal_warning_temp_c;        /**< Temperature threshold for adaptive downclocking (default: 75C) */
    float thermal_critical_temp_c;       /**< Temperature threshold for force idle gate (default: 85C) */
    uint32_t max_freq_mhz;               /**< Max operating frequency (default: 1000MHz) */
    uint32_t min_freq_mhz;               /**< Min operating frequency (default: 300MHz) */
} npu_power_config_t;

/**
 * @brief Power Telemetry & Energy Metrics
 */
typedef struct {
    npu_power_state_t current_state;
    uint32_t current_freq_mhz;
    float dynamic_power_mw;
    float last_wakeup_latency_us;
    uint64_t total_active_time_us;
    uint64_t total_gated_time_us;
    uint64_t total_sleep_time_us;
    uint32_t state_transition_count;
    double cumulative_energy_mj;         /**< Total energy consumed in milli-Joules */
    double cumulative_energy_mwh;        /**< Total energy consumed in milli-Watt-hours */
} npu_power_metrics_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define NPU_POWER_MGR_STORAGE_SIZE 4096

typedef struct {
    alignas(max_align_t) uint8_t raw[NPU_POWER_MGR_STORAGE_SIZE];
} npu_power_mgr_storage_t;

typedef struct npu_power_mgr npu_power_mgr_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief In-place initialize NPU power management governor
 */
npu_power_mgr_t *npu_power_mgr_create_in(npu_power_mgr_storage_t *storage, const npu_power_config_t *config);

/**
 * @brief Notify power manager of incoming tensor / inference dispatch (fast wakeup)
 *
 * @param mgr Power manager instance
 * @param out_wakeup_latency_us Optional pointer to receive measured wakeup latency
 * @return 0 on success
 */
int npu_power_mgr_notify_tensor_arrival(npu_power_mgr_t *mgr, float *out_wakeup_latency_us);

/**
 * @brief Periodic step ticker to update idle timeouts, thermal throttling and DVFS
 *
 * @param mgr Power manager instance
 * @param delta_time_ms Elapsed time since last tick in ms
 * @param current_core_temp_c Current NPU core temperature
 * @param pending_queue_depth Number of queued tensors awaiting inference
 */
int npu_power_mgr_tick(npu_power_mgr_t *mgr, uint32_t delta_time_ms,
                       float current_core_temp_c, uint32_t pending_queue_depth);

/**
 * @brief Explicitly request power state transition
 */
int npu_power_mgr_set_state(npu_power_mgr_t *mgr, npu_power_state_t target_state);

/**
 * @brief Retrieve real-time power and energy metrics
 */
int npu_power_mgr_get_metrics(const npu_power_mgr_t *mgr, npu_power_metrics_t *out_metrics);

/**
 * @brief Synchronize power state and energy metrics with DataHub shadow bus
 */
int npu_power_mgr_sync_datahub(const npu_power_mgr_t *mgr, datahub_t *hub);

/**
 * @brief Reset power and energy accumulators
 */
void npu_power_mgr_reset(npu_power_mgr_t *mgr);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_NPU_POWER_MANAGER_H */
