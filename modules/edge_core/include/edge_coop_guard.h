/**
 * @file edge_coop_guard.h
 * @brief Cooperative Timeslice Overrun Watchdog and Soft Circuit Breaker (AEP-719 / Issue #250)
 *
 * Enforces execution timeslice budgets for non-preemptible/third-party algorithm modules,
 * detecting execution overruns, triggering soft circuit breakers, and executing graceful
 * fallback degradation with automatic self-healing recovery cooldowns.
 * Zero dynamic memory allocation (0 malloc) using Caller-Allocated storage.
 */

#ifndef EDGE_COOP_GUARD_H
#define EDGE_COOP_GUARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_COOP_MAX_TASKS 32

/**
 * @brief Guard Operational State
 */
typedef enum {
    EDGE_COOP_STATE_HEALTHY = 0,   /**< Normal execution within budget */
    EDGE_COOP_STATE_DEGRADED = 1,  /**< Performance degraded / throttled due to repeated overruns */
    EDGE_COOP_STATE_TRIPPED = 2,   /**< Circuit breaker open / execution isolated to protect OS */
    EDGE_COOP_STATE_HALF_OPEN = 3, /**< Probe trial execution testing self-healing */
} edge_coop_state_t;

/**
 * @brief Task Timeslice Configuration
 */
typedef struct {
    uint32_t task_id;
    char name[32];
    uint64_t timeslice_budget_us;                  /**< Execution time budget per invocation (us) */
    uint32_t max_consecutive_overruns_to_degrade;  /**< Overrun count to transition to DEGRADED (e.g. 3) */
    uint32_t max_consecutive_overruns_to_trip;     /**< Overrun count to trip breaker TRIPPED (e.g. 5) */
    uint64_t recovery_cooldown_us;                 /**< Cooldown before probing HALF_OPEN (us) */
} edge_coop_task_config_t;

/**
 * @brief Guard Engine Configuration
 */
typedef struct {
    uint64_t default_budget_us;
    uint32_t default_degrade_threshold;
    uint32_t default_trip_threshold;
    uint64_t default_cooldown_us;
} edge_coop_guard_config_t;

/**
 * @brief Task Runtime Status
 */
typedef struct {
    uint32_t task_id;
    edge_coop_state_t state;
    uint64_t timeslice_budget_us;
    uint64_t last_execution_us;
    uint64_t max_execution_us;
    uint32_t consecutive_overruns;
    uint64_t total_overruns;
    uint64_t total_executions;
} edge_coop_task_status_t;

/**
 * @brief Guard Engine Statistics
 */
typedef struct {
    uint32_t registered_task_count;
    uint32_t healthy_task_count;
    uint32_t degraded_task_count;
    uint32_t tripped_task_count;
    uint64_t total_overrun_events;
    uint64_t total_tripped_events;
    uint64_t total_recovery_events;
} edge_coop_guard_stats_t;

/**
 * @brief Consumer-Defined Event Port for Cooperative Guard
 */
typedef struct {
    void (*on_timeslice_overrun)(void *ctx, uint32_t task_id, uint64_t actual_us, uint64_t budget_us);
    void (*on_state_transition)(void *ctx, uint32_t task_id, edge_coop_state_t prev_state, edge_coop_state_t new_state, const char *reason);
    void *ctx;
} edge_coop_guard_port_t;

/**
 * @brief Caller-Allocated Storage for Cooperative Guard (4096 bytes, 64-bit aligned)
 */
typedef struct {
    uint64_t raw[512];
} edge_coop_guard_storage_t;

typedef struct edge_coop_guard edge_coop_guard_t;

/**
 * @brief In-place placement construct Cooperative Guard Engine
 */
edge_coop_guard_t *edge_coop_guard_create_in(edge_coop_guard_storage_t *storage,
                                             const edge_coop_guard_config_t *cfg,
                                             const edge_coop_guard_port_t *port);

/**
 * @brief Destroy guard instance
 */
int edge_coop_guard_destroy(edge_coop_guard_t *guard);

/**
 * @brief Register task to be monitored
 */
int edge_coop_guard_register_task(edge_coop_guard_t *guard, const edge_coop_task_config_t *tcfg);

/**
 * @brief Check if task is permitted to execute or if fallback should be used
 */
bool edge_coop_guard_can_execute(edge_coop_guard_t *guard, uint32_t task_id, uint64_t now_us);

/**
 * @brief Record timeslice start boundary
 */
int edge_coop_guard_enter_slice(edge_coop_guard_t *guard, uint32_t task_id, uint64_t start_time_us);

/**
 * @brief Record timeslice completion boundary and evaluate overrun / state machine
 */
int edge_coop_guard_exit_slice(edge_coop_guard_t *guard, uint32_t task_id, uint64_t end_time_us);

/**
 * @brief Query status of a specific task
 */
int edge_coop_guard_get_task_status(const edge_coop_guard_t *guard, uint32_t task_id, edge_coop_task_status_t *status);

/**
 * @brief Manually reset task back to healthy state
 */
int edge_coop_guard_reset_task(edge_coop_guard_t *guard, uint32_t task_id);

/**
 * @brief Query guard engine runtime statistics
 */
int edge_coop_guard_get_stats(const edge_coop_guard_t *guard, edge_coop_guard_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_COOP_GUARD_H */
