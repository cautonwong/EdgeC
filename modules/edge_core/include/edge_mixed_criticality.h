/**
 * @file edge_mixed_criticality.h
 * @brief Mixed-Criticality Real-Time Dispatcher and MTT Deterministic Timing (AEP-602 / Issue #221)
 *
 * Coordinates L1 Safety-Critical, L2 MTT-Aligned (15min/60min), L3 Compute/AI,
 * and L4 Cloud/Diagnostic tasks with zero-heap allocation, preventing criticality
 * inversion and providing thermal/power-aware adaptive throttling.
 */

#ifndef EDGE_MIXED_CRITICALITY_H
#define EDGE_MIXED_CRITICALITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_MIXED_CRIT_MAX_TASKS 32

/**
 * @brief Mixed Criticality Levels
 */
typedef enum {
    CRITICALITY_L1_SAFETY  = 0,  /**< Hardware protection, fault trips (highest priority, strict WCET, 0 malloc) */
    CRITICALITY_L2_MTT     = 1,  /**< 15min/60min Industrial Wall-clock aligned deterministic billing */
    CRITICALITY_L3_COMPUTE = 2,  /**< AI inference, DSP filter, stream analytics (throttlable) */
    CRITICALITY_L4_CLOUD   = 3,  /**< Log upload, diagnostics, telemetry sync (droppable on load) */
} edge_criticality_level_t;

/**
 * @brief Task Registration Configuration
 */
typedef struct {
    uint32_t task_id;
    char name[32];
    edge_criticality_level_t level;
    uint64_t period_us;
    uint64_t wcet_budget_us;
    uint64_t mtt_wall_clock_interval_us; /**< e.g. 900000000ULL for 15min, or 0 for relative periodic */
    int (*execute_fn)(void *ctx);
    void *ctx;
} edge_mixed_crit_task_config_t;

/**
 * @brief Dispatcher Configuration
 */
typedef struct {
    uint64_t tick_resolution_us;
    float thermal_warning_temp_c;      /**< Temperature to initiate L3/L4 decimation (e.g. 70.0 C) */
    float thermal_critical_temp_c;     /**< Temperature to pause L3/L4 execution (e.g. 85.0 C) */
    bool enable_thermal_throttling;
    uint32_t l3_decimation_factor;     /**< Throttle down factor for L3 (e.g. 4 -> run every 4th cycle) */
} edge_mixed_crit_config_t;

/**
 * @brief Step Dispatch Result
 */
typedef struct {
    bool task_dispatched;
    uint32_t active_task_id;
    edge_criticality_level_t active_level;
    bool preempted_lower_task;
    uint64_t execution_latency_us;
    int64_t wall_clock_drift_us;
    int task_return_code;
} edge_mixed_crit_dispatch_result_t;

/**
 * @brief Dispatcher Runtime Statistics
 */
typedef struct {
    uint32_t registered_task_count;
    uint64_t l1_dispatch_count;
    uint64_t l2_dispatch_count;
    uint64_t l3_dispatch_count;
    uint64_t l4_dispatch_count;
    uint64_t l3_skipped_throttled_count;
    uint64_t l4_dropped_count;
    uint64_t criticality_inversions_prevented;
    int64_t max_mtt_drift_us;
    float current_temperature_c;
    bool thermal_throttle_active;
} edge_mixed_crit_stats_t;

/**
 * @brief Consumer-Defined Event Port for Mixed-Criticality Dispatcher
 */
typedef struct {
    void (*on_criticality_inversion_prevented)(void *ctx, uint32_t preempting_task_id, uint32_t preempted_task_id);
    void (*on_mtt_aligned_dispatch)(void *ctx, uint32_t task_id, int64_t wall_clock_drift_us);
    void (*on_thermal_throttling_engaged)(void *ctx, float junction_temp_c, bool is_critical);
    void *ctx;
} edge_mixed_crit_port_t;

/**
 * @brief Caller-Allocated Storage for Mixed-Criticality Dispatcher (8192 bytes, 64-bit aligned)
 */
typedef struct {
    uint64_t raw[1024];
} edge_mixed_crit_storage_t;

typedef struct edge_mixed_crit_dispatcher edge_mixed_crit_dispatcher_t;

/**
 * @brief In-place placement construct Mixed-Criticality Dispatcher
 */
edge_mixed_crit_dispatcher_t *edge_mixed_crit_create_in(edge_mixed_crit_storage_t *storage,
                                                         const edge_mixed_crit_config_t *cfg,
                                                         const edge_mixed_crit_port_t *port);

/**
 * @brief Destroy dispatcher instance
 */
int edge_mixed_crit_destroy(edge_mixed_crit_dispatcher_t *disp);

/**
 * @brief Register a task with its criticality level
 */
int edge_mixed_crit_register_task(edge_mixed_crit_dispatcher_t *disp, const edge_mixed_crit_task_config_t *tcfg);

/**
 * @brief Feed thermal & power telemetry to the dispatcher
 */
int edge_mixed_crit_update_thermal(edge_mixed_crit_dispatcher_t *disp, float junction_temp_c, float cpu_load_pct);

/**
 * @brief Step dispatcher engine and execute highest ready criticality task
 */
int edge_mixed_crit_step(edge_mixed_crit_dispatcher_t *disp, uint64_t now_us, edge_mixed_crit_dispatch_result_t *res);

/**
 * @brief Query runtime statistics
 */
int edge_mixed_crit_get_stats(const edge_mixed_crit_dispatcher_t *disp, edge_mixed_crit_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_MIXED_CRITICALITY_H */
