/**
 * @file edge_rms_scheduler.h
 * @brief Rate Monotonic Scheduling (RMS) Static Deterministic Periodic Scheduler & Budget Watchdog (AEP-713 / Issue #246)
 * @details Implements fixed-priority Rate Monotonic Scheduling, Liu & Layland utilization bound
 *          verification (U <= n(2^(1/n) - 1)), exact Response Time Analysis (RTA),
 *          per-task WCET execution budget watchdogs, and consumer-defined ports.
 */

#ifndef EDGE_RMS_SCHEDULER_H
#define EDGE_RMS_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_RMS_MAX_TASKS 32
#define EDGE_RMS_TASK_NAME_LEN 32

/**
 * @brief Task lifecycle states under Rate Monotonic Scheduling
 */
typedef enum {
    EDGE_RMS_STATE_DORMANT = 0,   /**< Registered but not released */
    EDGE_RMS_STATE_READY,         /**< Released in period, waiting for CPU */
    EDGE_RMS_STATE_RUNNING,       /**< Currently executing */
    EDGE_RMS_STATE_COMPLETED,     /**< Current period job finished */
    EDGE_RMS_STATE_THROTTLED,     /**< Execution paused due to budget exhaustion */
    EDGE_RMS_STATE_TRIPPED        /**< Task circuit breaker tripped */
} edge_rms_task_state_t;

/**
 * @brief Watchdog actions when task exceeds execution time budget (WCET)
 */
typedef enum {
    EDGE_RMS_ACTION_WARN_ONLY = 0,/**< Log and notify port only, continue running */
    EDGE_RMS_ACTION_THROTTLE,     /**< Suspend task until next period release */
    EDGE_RMS_ACTION_SUSPEND,      /**< Permanently suspend task until operator reset */
    EDGE_RMS_ACTION_CIRCUIT_BREAK /**< Trip system safety fallback mode */
} edge_rms_action_t;

/**
 * @brief RMS Task Configuration
 */
typedef struct {
    uint32_t task_id;                         /**< Unique task identifier */
    char name[EDGE_RMS_TASK_NAME_LEN];        /**< Task descriptive name */
    uint64_t period_us;                       /**< Task period T_i (us) */
    uint64_t relative_deadline_us;            /**< Relative deadline D_i (us), default T_i */
    uint64_t wcet_budget_us;                  /**< Execution time budget C_i per period (us) */
    uint64_t initial_phase_us;                /**< Initial phase offset (us) */
    edge_rms_action_t budget_action;          /**< Watchdog action on budget overrun */
    int (*run)(void *task_ctx);               /**< Job step callback */
    void *task_ctx;                           /**< User context pointer passed to run */
} edge_rms_task_config_t;

/**
 * @brief Per-Task Response Time Analysis (RTA) Detail
 */
typedef struct {
    uint32_t task_id;                         /**< Task identifier */
    uint32_t static_priority;                /**< Assigned static priority (1 = highest) */
    double task_utilization;                  /**< C_i / T_i */
    uint64_t worst_case_response_time_us;     /**< Calculated exact R_i (us) via fixed-point RTA */
    bool is_task_schedulable;                 /**< True if R_i <= D_i */
    uint64_t slack_margin_us;                 /**< D_i - R_i (us) */
} edge_rms_task_rta_info_t;

/**
 * @brief Schedulability Analysis Report
 */
typedef struct {
    bool is_schedulable;                      /**< Overall schedulability verdict */
    bool passed_sufficient_bound;             /**< True if U <= n(2^(1/n) - 1) */
    bool passed_exact_rta;                    /**< True if all R_i <= D_i */
    double total_utilization;                 /**< Total task set utilization */
    double liu_layland_bound;                 /**< n(2^(1/n) - 1) utilization upper bound */
    uint32_t task_count;                      /**< Number of active tasks */
    edge_rms_task_rta_info_t task_rta[EDGE_RMS_MAX_TASKS]; /**< Per-task RTA breakdown */
    char failure_reason[128];                 /**< Diagnostic failure reason if unschedulable */
} edge_rms_schedulability_report_t;

/**
 * @brief RMS Dispatch Step Result
 */
typedef struct {
    bool task_dispatched;                     /**< True if a task ran */
    uint32_t active_task_id;                  /**< Active task ID */
    char active_task_name[EDGE_RMS_TASK_NAME_LEN]; /**< Active task name */
    uint32_t static_priority;                /**< Priority of active task */
    uint64_t remaining_budget_us;             /**< Remaining WCET budget in current period */
    bool is_preemption;                       /**< True if higher-priority task preempted current */
    uint32_t preempted_task_id;               /**< Task ID of preempted task */
    bool is_budget_overrun;                   /**< True if task budget watchdog triggered */
    bool is_deadline_miss;                    /**< True if deadline was exceeded */
    uint32_t affected_task_id;                /**< Task ID affected by overrun/miss */
} edge_rms_dispatch_result_t;

/**
 * @brief RMS Scheduler Statistics
 */
typedef struct {
    uint32_t registered_task_count;           /**< Number of registered tasks */
    double total_utilization;                 /**< Total CPU utilization */
    double bound_utilization;                 /**< Theoretical Liu & Layland bound */
    uint64_t total_dispatches;                /**< Total task dispatch events */
    uint64_t total_preemptions;               /**< Total static priority preemptions */
    uint64_t total_budget_overruns;           /**< Total budget watchdog violations */
    uint64_t total_deadline_misses;           /**< Total deadline overruns */
    uint64_t total_completions;               /**< Total completed jobs */
    uint64_t peak_response_time_us;           /**< Maximum observed response time (us) */
} edge_rms_stats_t;

/**
 * @brief Consumer-Defined Port for RMS Scheduler Events (AEP-751)
 */
typedef struct {
    /**
     * @brief Callback invoked on task preemption
     */
    void (*on_task_preempt)(void *ctx, uint32_t preempted_task_id, uint32_t preempting_task_id, uint64_t time_us);

    /**
     * @brief Callback invoked when a task exceeds its allocated WCET execution budget
     */
    void (*on_budget_exceeded)(void *ctx, uint32_t task_id, uint64_t exec_duration_us, uint64_t budget_us, edge_rms_action_t action_taken);

    /**
     * @brief Callback invoked when a task job misses its relative deadline
     */
    void (*on_deadline_miss)(void *ctx, uint32_t task_id, uint64_t time_us, uint64_t overdue_us);

    /**
     * @brief Callback invoked on schedulability warnings
     */
    void (*on_schedulability_warning)(void *ctx, double total_utilization, double bound_utilization, const char *reason);

    void *ctx;
} edge_rms_port_t;

/**
 * @brief Global RMS Scheduler Configuration
 */
typedef struct {
    uint64_t tick_resolution_us;              /**< Step tick resolution (us) */
    bool auto_assign_priorities;              /**< Automatically assign priorities inversely by period T_i */
    bool enable_budget_watchdog;              /**< Enable WCET execution budget watchdog */
} edge_rms_config_t;

/**
 * @brief Opaque RMS Scheduler Handle
 */
typedef struct edge_rms_scheduler edge_rms_scheduler_t;

/**
 * @brief Caller-Allocated Storage for RMS Scheduler (6144 bytes, zero heap allocation)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[6144];
} edge_rms_scheduler_storage_t;

/**
 * @brief In-place placement constructor for RMS Scheduler
 *
 * @param store Caller-provided static/stack storage buffer
 * @param cfg Scheduler configuration (ticks, watchdog)
 * @param port Injected audit port callbacks (optional, may be NULL)
 * @return edge_rms_scheduler_t* handle pointing into store, or NULL on error
 */
edge_rms_scheduler_t *edge_rms_scheduler_create_in(edge_rms_scheduler_storage_t *store,
                                                  const edge_rms_config_t *cfg,
                                                  const edge_rms_port_t *port);

/**
 * @brief Register a periodic real-time task into the RMS task set
 *
 * @param sched Pointer to RMS scheduler instance
 * @param task_cfg Task configuration (period, budget, action)
 * @return 0 on success, negative error code on failure
 */
int edge_rms_scheduler_register_task(edge_rms_scheduler_t *sched,
                                    const edge_rms_task_config_t *task_cfg);

/**
 * @brief Unregister a task from the RMS task set
 *
 * @param sched Pointer to RMS scheduler instance
 * @param task_id Task identifier to remove
 * @return 0 on success, negative error code on failure
 */
int edge_rms_scheduler_unregister_task(edge_rms_scheduler_t *sched, uint32_t task_id);

/**
 * @brief Query current total processor utilization U = sum(C_i / T_i)
 *
 * @param sched Pointer to RMS scheduler instance
 * @return Total utilization (0.0 ~ 1.0), or negative on error
 */
double edge_rms_scheduler_get_utilization(const edge_rms_scheduler_t *sched);

/**
 * @brief Query theoretical Liu & Layland bound U_bound = n(2^(1/n) - 1)
 *
 * @param sched Pointer to RMS scheduler instance
 * @return Bound utilization (e.g. 0.693 for large n), or negative on error
 */
double edge_rms_scheduler_get_bound(const edge_rms_scheduler_t *sched);

/**
 * @brief Perform full mathematical schedulability analysis (Bound Test + Exact RTA)
 *
 * @param sched Pointer to RMS scheduler instance
 * @param report Pointer to output schedulability report structure
 * @return 0 on success, negative error code on invalid parameters
 */
int edge_rms_scheduler_check_schedulability(const edge_rms_scheduler_t *sched,
                                           edge_rms_schedulability_report_t *report);

/**
 * @brief Advance the RMS scheduler by a time step, manage periods, enforce budgets, and dispatch
 *
 * @param sched Pointer to RMS scheduler instance
 * @param current_time_us Current wall-clock time (us)
 * @param step_duration_us Duration of execution slice (us)
 * @param result Pointer to receive dispatch outcome
 * @return 0 on success, negative error code on failure
 */
int edge_rms_scheduler_step(edge_rms_scheduler_t *sched,
                           uint64_t current_time_us,
                           uint64_t step_duration_us,
                           edge_rms_dispatch_result_t *result);

/**
 * @brief Explicitly signal completion of the active period job for a task
 *
 * @param sched Pointer to RMS scheduler instance
 * @param task_id Task identifier completing period
 * @param finish_time_us Completion timestamp (us)
 * @return 0 on success, negative error code on failure
 */
int edge_rms_scheduler_complete_job(edge_rms_scheduler_t *sched,
                                   uint32_t task_id,
                                   uint64_t finish_time_us);

/**
 * @brief Query runtime dispatch, preemption, and budget watchdog telemetry
 *
 * @param sched Pointer to RMS scheduler instance
 * @param stats Pointer to output statistics structure
 * @return 0 on success, negative error code on invalid parameters
 */
int edge_rms_scheduler_get_stats(const edge_rms_scheduler_t *sched, edge_rms_stats_t *stats);

/**
 * @brief Reset scheduler timeline, budgets, and task states
 *
 * @param sched Pointer to RMS scheduler instance
 * @return 0 on success, negative error code on failure
 */
int edge_rms_scheduler_reset(edge_rms_scheduler_t *sched);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_RMS_SCHEDULER_H */
