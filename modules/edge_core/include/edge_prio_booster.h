/**
 * @file edge_prio_booster.h
 * @brief Dynamic Priority Boosting and Annealing Decay Engine (AEP-718 / Issue #249)
 *
 * Provides instantaneous real-time priority elevation (up to POSIX SCHED_FIFO 99)
 * on emergency/fault events (e.g. BESS thermal runaway, PCS trip), with deterministic
 * time-window hold and graceful annealing decay to baseline priority to prevent starvation.
 * Zero dynamic memory allocation (0 malloc) using Caller-Allocated storage.
 */

#ifndef EDGE_PRIO_BOOSTER_H
#define EDGE_PRIO_BOOSTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_BOOSTER_MAX_TASKS 32

/**
 * @brief Annealing Decay Strategy
 */
typedef enum {
    EDGE_ANNEAL_STEP = 0,        /**< Instantaneous revert to base priority after hold window */
    EDGE_ANNEAL_LINEAR = 1,      /**< Linear step-down decay over time */
    EDGE_ANNEAL_EXPONENTIAL = 2, /**< Fast exponential decay down to baseline */
} edge_anneal_curve_t;

/**
 * @brief Task Registration Configuration
 */
typedef struct {
    uint32_t task_id;
    char name[32];
    int base_priority;          /**< Normal baseline priority (1..99) */
    pthread_t thread_handle;    /**< Optional OS thread handle to adjust via pthread_setschedparam */
} edge_boost_task_config_t;

/**
 * @brief Priority Booster Engine Configuration
 */
typedef struct {
    uint64_t tick_resolution_us;     /**< Tick interval for timer checking (e.g. 1000us) */
    uint64_t default_boost_hold_us;  /**< Default emergency hold window (e.g. 50000us) */
    int max_boosted_prio;            /**< Max priority ceiling (e.g. 99) */
    edge_anneal_curve_t anneal_curve;
} edge_prio_booster_config_t;

/**
 * @brief Priority Booster Statistics
 */
typedef struct {
    uint32_t registered_task_count;
    uint32_t active_boosted_count;
    uint64_t total_boost_events;
    uint64_t total_anneal_events;
} edge_prio_booster_stats_t;

/**
 * @brief Consumer-Defined Event Port for Priority Booster
 */
typedef struct {
    void (*on_priority_boosted)(void *ctx, uint32_t task_id, int old_prio, int boosted_prio, const char *reason);
    void (*on_priority_annealed)(void *ctx, uint32_t task_id, int current_prio, int base_prio);
    void *ctx;
} edge_prio_booster_port_t;

/**
 * @brief Caller-Allocated Storage for Priority Booster Engine (4096 bytes, 64-bit aligned)
 */
typedef struct {
    uint64_t raw[512];
} edge_prio_booster_storage_t;

typedef struct edge_prio_booster edge_prio_booster_t;

/**
 * @brief In-place placement construct Priority Booster Engine
 */
edge_prio_booster_t *edge_prio_booster_create_in(edge_prio_booster_storage_t *storage,
                                                 const edge_prio_booster_config_t *cfg,
                                                 const edge_prio_booster_port_t *port);

/**
 * @brief Destroy booster instance and restore all tasks to base priority
 */
int edge_prio_booster_destroy(edge_prio_booster_t *booster);

/**
 * @brief Register a task to be monitored and managed by the booster
 */
int edge_prio_booster_register_task(edge_prio_booster_t *booster, const edge_boost_task_config_t *tcfg);

/**
 * @brief Trigger instantaneous priority boost for an emergency event
 *
 * @param booster Engine instance
 * @param task_id Target task ID
 * @param target_boost_prio Boosted priority level (e.g. 99, 0 for max configured)
 * @param hold_duration_us Duration to hold elevated priority before annealing (0 for default)
 * @param reason Event trigger description
 * @return 0 on success, negative errno on failure
 */
int edge_prio_booster_trigger_boost(edge_prio_booster_t *booster,
                                    uint32_t task_id,
                                    int target_boost_prio,
                                    uint64_t hold_duration_us,
                                    const char *reason);

/**
 * @brief Step timer tick and perform priority annealing / decay
 *
 * @param booster Engine instance
 * @param elapsed_us Microseconds elapsed since last tick
 * @return 0 on success
 */
int edge_prio_booster_tick(edge_prio_booster_t *booster, uint64_t elapsed_us);

/**
 * @brief Query current effective priority of a task
 */
int edge_prio_booster_get_task_priority(const edge_prio_booster_t *booster, uint32_t task_id, int *current_prio);

/**
 * @brief Query runtime statistics
 */
int edge_prio_booster_get_stats(const edge_prio_booster_t *booster, edge_prio_booster_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_PRIO_BOOSTER_H */
