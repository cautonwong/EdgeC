/**
 * @file npu_self_healing.h
 * @brief EdgeOS NPU Hardware Anomaly Watchdog, Overtemp Defense & Seamless CPU Fallback (AEP-747 / Issue #277)
 *
 * Provides real-time hardware watchdog monitoring, over-temperature protection, deadlock detection,
 * < 10ms seamless fallback to CPU INT8/FP32 execution kernel, and automatic cool-down self-healing.
 */

#ifndef EDGE_NPU_SELF_HEALING_H
#define EDGE_NPU_SELF_HEALING_H

#include "npu_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants & Enums                                                          */
/* -------------------------------------------------------------------------- */

#define NPU_DEFAULT_MAX_TEMP_C         85.0f
#define NPU_DEFAULT_RECOVER_TEMP_C     75.0f
#define NPU_DEFAULT_WATCHDOG_TIMEOUT_MS 50
#define NPU_MAX_CONSECUTIVE_ERRORS     3

typedef enum {
    NPU_HEALING_STATE_NORMAL       = 0, /**< Normal execution on primary hardware NPU */
    NPU_HEALING_STATE_THROTTLED    = 1, /**< Approaching temperature threshold, frequency lowered */
    NPU_HEALING_STATE_FALLBACK_CPU = 2, /**< Switched seamlessly to CPU fallback kernel */
    NPU_HEALING_STATE_RESETTING    = 3, /**< Hardware reset in progress */
    NPU_HEALING_STATE_RECOVERING   = 4  /**< Cooldown verified, returning to primary NPU */
} npu_self_healing_state_t;

/* -------------------------------------------------------------------------- */
/* Configuration & Telemetry Structures                                       */
/* -------------------------------------------------------------------------- */

/**
 * @brief Self-Healing Configuration
 */
typedef struct {
    npu_device_t *primary_device;       /**< Primary Hardware NPU Accelerator (Ascend/RKNN/BPU/TRT/DSP) */
    float max_temp_threshold_c;         /**< Over-temperature threshold in C (default: 85.0C) */
    float recover_temp_threshold_c;     /**< Cooldown recovery threshold in C (default: 75.0C) */
    uint32_t watchdog_timeout_ms;       /**< Heartbeat/execution timeout in ms (default: 50ms) */
    uint32_t max_consecutive_errors;    /**< Error threshold to trigger fallback (default: 3) */
    bool auto_reset_on_deadlock;        /**< Automatically call npu_reset_hardware on watchdog bite */
    bool enable_cpu_fallback;           /**< Enable seamless CPU INT8/FP32 fallback */
} npu_self_healing_config_t;

/**
 * @brief Self-Healing Telemetry Metrics
 */
typedef struct {
    npu_self_healing_state_t state;     /**< Current self-healing lifecycle state */
    bool is_fallback_active;            /**< True if currently routing to CPU fallback kernel */
    uint64_t total_inferences;          /**< Total inferences handled by self-healing orchestrator */
    uint64_t npi_inferences;            /**< Inferences successfully processed by primary NPU */
    uint64_t cpu_fallback_inferences;   /**< Inferences processed by CPU fallback */
    uint32_t fallback_count;            /**< Total times fallback was triggered */
    uint32_t recovery_count;            /**< Total times hardware self-healing recovered */
    uint32_t watchdog_timeouts;         /**< Total watchdog timeout occurrences */
    uint32_t overtemp_events;           /**< Total over-temperature trip events */
    uint32_t last_switch_latency_us;    /**< Latency of the most recent switch in microseconds */
    float last_recorded_temp_c;         /**< Most recent core temperature reading in C */
} npu_self_healing_metrics_t;

typedef struct npu_self_healing npu_self_healing_t;

/* -------------------------------------------------------------------------- */
/* Public APIs                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the NPU Self-Healing & Watchdog Orchestrator
 */
int npu_self_healing_init(npu_self_healing_t **out_healer, const npu_self_healing_config_t *cfg);

/**
 * @brief Destroy the Self-Healing Orchestrator and release resources
 */
int npu_self_healing_destroy(npu_self_healing_t *healer);

/**
 * @brief Execute inference with automatic anomaly detection, < 10ms CPU fallback and self-healing
 */
int npu_self_healing_execute(npu_self_healing_t *healer,
                             npu_model_handle_t handle,
                             const npu_tensor_buf_t *inputs, size_t num_inputs,
                             npu_tensor_buf_t *outputs, size_t num_outputs,
                             npu_inference_stats_t *stats);

/**
 * @brief Feed the hardware watchdog timer to prevent deadlock timeout
 */
int npu_self_healing_feed_watchdog(npu_self_healing_t *healer);

/**
 * @brief Periodic health check: checks core temperature, timeout and triggers auto-recovery
 */
int npu_self_healing_check_health(npu_self_healing_t *healer);

/**
 * @brief Manually inject hardware fault / simulated overtemp for testing
 */
int npu_self_healing_inject_fault(npu_self_healing_t *healer, bool force_overtemp, bool force_deadlock);

/**
 * @brief Retrieve real-time telemetry metrics of self-healing engine
 */
int npu_self_healing_get_metrics(npu_self_healing_t *healer, npu_self_healing_metrics_t *metrics);

/**
 * @brief CPU Fallback Reference Backend Factory
 */
int npu_cpu_fallback_create(npu_device_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_NPU_SELF_HEALING_H */
