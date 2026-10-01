/**
 * @file edge_edf_scheduler.h
 * @brief Earliest Deadline First (EDF) Dynamic Hard Real-Time Preemptive Scheduler (AEP-712 / Issue #245)
 * @details Implements optimal uniprocessor EDF dynamic priority scheduling, Liu & Layland /
 *          Baruah Processor Demand Criteria (DBF) exact schedulability bound tests,
 *          dynamic preemption detection, deadline overrun isolation, and consumer-defined ports.
 */

#ifndef EDGE_EDF_SCHEDULER_H
#define EDGE_EDF_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_EDF_MAX_TASKS 32
#define EDGE_EDF_TASK_NAME_LEN 32

/**
 * @brief Dynamic task lifecycle states under EDF
 */
typedef enum {
    EDGE_EDF_STATE_DORMANT = 0,   /**< Task registered but not yet released */
    EDGE_EDF_STATE_READY,         /**< Released, waiting for CPU assignment */
    EDGE_EDF_STATE_RUNNING,       /**< Currently executing on CPU */
    EDGE_EDF_STATE_COMPLETED,     /**< Current job completed within deadline */
    EDGE_EDF_STATE_MISSED         /**< Current job missed absolute deadline */
} edge_edf_task_state_t;

/**
 * @brief EDF Real-Time Task Configuration
 */
typedef struct {
    uint32_t task_id;                         /**< Unique task identifier */
    char name[EDGE_EDF_TASK_NAME_LEN];        /**< Task descriptive name */
    uint64_t period_us;                       /**< Task period T_i (us) */
    uint64_t relative_deadline_us;            /**< Relative deadline D_i (us), <= T_i (if 0, D_i = T_i) */
    uint64_t wcet_us;                         /**< Worst-case execution time C_i (us) */
    uint64_t initial_phase_us;                /**< Initial release phase offset (us) */
    int (*run)(void *task_ctx);               /**< Task job step execution callback */
    void *task_ctx;                           /**< User context pointer passed to run */
} edge_edf_task_config_t;

/**
 * @brief Schedulability Analysis Report
 */
typedef struct {
    bool is_schedulable;                      /**< Exact schedulability verdict */
    double total_utilization;                 /**< Sum(C_i / T_i) */
    uint64_t hyperperiod_us;                  /**< LCM(T_1, ..., T_n) */
    uint64_t max_busy_period_us;              /**< Peak busy period duration L* (us) */
    char failure_reason[128];                 /**< Diagnostic message if unschedulable */
} edge_edf_schedulability_report_t;

/**
 * @brief Dispatch Step Execution Result
 */
typedef struct {
    bool task_dispatched;                     /**< True if a task was assigned CPU */
    uint32_t active_task_id;                  /**< Dispatched task ID */
    char active_task_name[EDGE_EDF_TASK_NAME_LEN]; /**< Dispatched task name */
    uint64_t absolute_deadline_us;            /**< Absolute deadline d_i of the active job */
    uint64_t remaining_budget_us;             /**< Remaining WCET budget in current job */
    bool is_preemption;                       /**< True if this step preempted a running task */
    uint32_t preempted_task_id;               /**< Task ID of preempted task (if is_preemption) */
    bool is_deadline_miss;                    /**< True if a deadline miss was detected this step */
    uint32_t missed_task_id;                  /**< Task ID that missed deadline */
} edge_edf_dispatch_result_t;

/**
 * @brief EDF Scheduler Runtime Statistics
 */
typedef struct {
    uint32_t registered_task_count;           /**< Currently active registered tasks */
    double total_utilization;                 /**< Current CPU utilization (0.0 ~ 1.0) */
    uint64_t total_dispatches;                /**< Total job dispatch events */
    uint64_t total_preemptions;               /**< Total dynamic preemption events */
    uint64_t total_deadline_misses;           /**< Total deadline overruns */
    uint64_t total_completions;               /**< Total successfully completed jobs */
    uint64_t peak_response_time_us;           /**< Maximum observed response time (us) */
} edge_edf_stats_t;

/**
 * @brief Consumer-Defined Port for EDF Scheduler Events (AEP-751)
 */
typedef struct {
    /**
     * @brief Callback invoked when a newly ready task preempts current running task
     */
    void (*on_task_preempt)(void *ctx, uint32_t preempted_task_id, uint32_t preempting_task_id, uint64_t time_us);

    /**
     * @brief Callback invoked when a task job exceeds its absolute deadline
     */
    void (*on_deadline_miss)(void *ctx, uint32_t task_id, uint64_t time_us, uint64_t overdue_us);

    /**
     * @brief Callback invoked when CPU utilization or demand bound analysis detects overload
     */
    void (*on_schedulability_warning)(void *ctx, double total_utilization, const char *reason);

    void *ctx;
} edge_edf_port_t;

/**
 * @brief Global EDF Scheduler Configuration
 */
typedef struct {
    uint64_t tick_resolution_us;              /**< Step tick resolution (e.g. 1000us) */
    bool enable_dynamic_preemption;           /**< Enable preemptive task switching */
    bool drop_on_deadline_miss;               /**< Drop overdue job on deadline miss */
} edge_edf_config_t;

/**
 * @brief Opaque EDF Scheduler Handle
 */
typedef struct edge_edf_scheduler edge_edf_scheduler_t;

/**
 * @brief Caller-Allocated Storage for EDF Scheduler (4096 bytes, zero heap allocation)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[6144];
} edge_edf_scheduler_storage_t;

/**
 * @brief In-place placement constructor for EDF Scheduler
 *
 * @param store Caller-provided static/stack storage buffer
 * @param cfg Scheduler configuration (ticks, preemption mode)
 * @param port Injected audit port callbacks (optional, may be NULL)
 * @return edge_edf_scheduler_t* handle pointing into store, or NULL on error
 */
edge_edf_scheduler_t *edge_edf_scheduler_create_in(edge_edf_scheduler_storage_t *store,
                                                  const edge_edf_config_t *cfg,
                                                  const edge_edf_port_t *port);

/**
 * @brief Register a periodic real-time task into the EDF task set
 *
 * @param sched Pointer to EDF scheduler instance
 * @param task_cfg Task configuration (period, deadline, WCET)
 * @return 0 on success, negative error code on failure
 */
int edge_edf_scheduler_register_task(edge_edf_scheduler_t *sched,
                                    const edge_edf_task_config_t *task_cfg);

/**
 * @brief Unregister a task from the EDF task set
 *
 * @param sched Pointer to EDF scheduler instance
 * @param task_id Task identifier to remove
 * @return 0 on success, negative error code on failure
 */
int edge_edf_scheduler_unregister_task(edge_edf_scheduler_t *sched, uint32_t task_id);

/**
 * @brief Query current total processor utilization U = sum(C_i / T_i)
 *
 * @param sched Pointer to EDF scheduler instance
 * @return Total utilization (e.g. 0.75 for 75%), or -1.0 on error
 */
double edge_edf_scheduler_get_utilization(const edge_edf_scheduler_t *sched);

/**
 * @brief Perform exact mathematical schedulability analysis (Liu & Layland / Baruah DBF)
 *
 * @param sched Pointer to EDF scheduler instance
 * @param report Pointer to output schedulability report structure
 * @return 0 on analysis completion, negative error code on invalid parameters
 */
int edge_edf_scheduler_check_schedulability(const edge_edf_scheduler_t *sched,
                                           edge_edf_schedulability_report_t *report);

/**
 * @brief Advance the EDF scheduler by a time step, release jobs, and perform preemptive dispatch
 *
 * @param sched Pointer to EDF scheduler instance
 * @param current_time_us Current wall-clock / simulation time (us)
 * @param step_duration_us Duration of this execution slice (us)
 * @param result Pointer to receive dispatch outcome
 * @return 0 on success, negative error code on failure
 */
int edge_edf_scheduler_step(edge_edf_scheduler_t *sched,
                           uint64_t current_time_us,
                           uint64_t step_duration_us,
                           edge_edf_dispatch_result_t *result);

/**
 * @brief Explicitly signal completion of the active job for a task
 *
 * @param sched Pointer to EDF scheduler instance
 * @param task_id Task identifier completing job
 * @param finish_time_us Completion timestamp (us)
 * @return 0 on success, negative error code on failure
 */
int edge_edf_scheduler_complete_job(edge_edf_scheduler_t *sched,
                                   uint32_t task_id,
                                   uint64_t finish_time_us);

/**
 * @brief Query runtime dispatch and preemption telemetry
 *
 * @param sched Pointer to EDF scheduler instance
 * @param stats Pointer to output statistics structure
 * @return 0 on success, negative error code on invalid parameters
 */
int edge_edf_scheduler_get_stats(const edge_edf_scheduler_t *sched, edge_edf_stats_t *stats);

/**
 * @brief Reset scheduler timeline and task release states
 *
 * @param sched Pointer to EDF scheduler instance
 * @return 0 on success, negative error code on failure
 */
int edge_edf_scheduler_reset(edge_edf_scheduler_t *sched);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_EDF_SCHEDULER_H */
