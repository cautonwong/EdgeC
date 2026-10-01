/**
 * @file edge_pcp_mutex.h
 * @brief PREEMPT_RT Priority Ceiling Protocol (PCP) Mutex & Real-Time Lock Audit (AEP-711 / Issue #244)
 * @details Implements Priority Ceiling Protocol (PCP / PTHREAD_PRIO_PROTECT) and Priority
 *          Inheritance Protocol (PIP / PTHREAD_PRIO_INHERIT) with lock holding time audit,
 *          contention metrics, priority inversion prevention counters, and consumer-defined ports.
 */

#ifndef EDGE_PCP_MUTEX_H
#define EDGE_PCP_MUTEX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_PCP_MUTEX_NAME_LEN 32

/**
 * @brief Real-Time Mutex Protocol Mode
 */
typedef enum {
    EDGE_PCP_PROTOCOL_PROTECT = 0,   /**< Priority Ceiling Protocol (PCP - PTHREAD_PRIO_PROTECT) */
    EDGE_PCP_PROTOCOL_INHERIT,       /**< Priority Inheritance Protocol (PIP - PTHREAD_PRIO_INHERIT) */
    EDGE_PCP_PROTOCOL_NORMAL         /**< Standard POSIX Mutex (PTHREAD_PRIO_NONE) */
} edge_pcp_protocol_t;

/**
 * @brief Real-Time PCP Mutex Configuration
 */
typedef struct {
    uint32_t mutex_id;                        /**< Global unique mutex identifier */
    char name[EDGE_PCP_MUTEX_NAME_LEN];       /**< Human-readable diagnostic mutex name */
    edge_pcp_protocol_t protocol;             /**< Lock protocol (PCP / PIP / NORMAL) */
    int prio_ceiling;                         /**< Priority ceiling value (1 ~ 99 for SCHED_FIFO/RR) */
    uint64_t max_expected_hold_us;            /**< Maximum expected holding budget (us) before triggering audit warning */
    bool recursive;                           /**< Enable recursive locking support */
} edge_pcp_mutex_config_t;

/**
 * @brief Lock Audit and Contention Telemetry
 */
typedef struct {
    uint32_t mutex_id;
    char name[EDGE_PCP_MUTEX_NAME_LEN];
    edge_pcp_protocol_t protocol;
    int prio_ceiling;
    uint64_t total_lock_count;                /**< Total successful lock operations */
    uint64_t contention_count;                /**< Total times a caller had to wait/block */
    uint64_t overrun_count;                   /**< Total times lock holding duration exceeded budget */
    uint64_t inversions_prevented;            /**< Priority inversion events neutralized */
    uint64_t last_wait_duration_ns;           /**< Last acquire wait duration (ns) */
    uint64_t max_wait_duration_ns;            /**< Peak acquire wait duration (ns) */
    uint64_t last_hold_duration_ns;           /**< Last critical section holding duration (ns) */
    uint64_t max_hold_duration_ns;            /**< Peak WCET critical section holding duration (ns) */
    double   avg_hold_duration_ns;            /**< Exponential moving average holding duration (ns) */
    bool is_locked;                           /**< Current lock state */
    pthread_t owner_thread;                   /**< Current owning thread handle */
} edge_pcp_mutex_stats_t;

/**
 * @brief Consumer-Defined Port for Lock Overrun Audit Notification (AEP-751)
 */
typedef struct {
    /**
     * @brief Callback invoked when a critical section exceeds max_expected_hold_us
     * @param ctx User context
     * @param mutex_id Mutex ID
     * @param hold_duration_us Actual duration held (us)
     * @param max_expected_us Budget expected (us)
     */
    void (*on_hold_overrun)(void *ctx, uint32_t mutex_id, uint64_t hold_duration_us, uint64_t max_expected_us);
    void *ctx;
} edge_pcp_mutex_port_t;

/**
 * @brief Opaque PCP Mutex Handle
 */
typedef struct edge_pcp_mutex edge_pcp_mutex_t;

/**
 * @brief Caller-Allocated Storage for PCP Mutex (256 bytes, zero dynamic allocation)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[384];
} edge_pcp_mutex_storage_t;

/**
 * @brief In-place placement constructor for Real-Time PCP Mutex
 *
 * @param store Caller-provided static/stack storage buffer
 * @param cfg Mutex configuration (priority ceiling, protocol, budget)
 * @param port Injected audit port callback (optional, may be NULL)
 * @return edge_pcp_mutex_t* handle pointing into store, or NULL on error
 */
edge_pcp_mutex_t *edge_pcp_mutex_create_in(edge_pcp_mutex_storage_t *store,
                                           const edge_pcp_mutex_config_t *cfg,
                                           const edge_pcp_mutex_port_t *port);

/**
 * @brief Acquire PCP real-time mutex with priority ceiling / inheritance elevation
 *
 * @param m Pointer to PCP mutex instance
 * @return 0 on success, negative error code on failure
 */
int edge_pcp_mutex_lock(edge_pcp_mutex_t *m);

/**
 * @brief Try to acquire PCP mutex without blocking
 *
 * @param m Pointer to PCP mutex instance
 * @return 0 on success, EBUSY if locked, negative on error
 */
int edge_pcp_mutex_trylock(edge_pcp_mutex_t *m);

/**
 * @brief Acquire PCP mutex with timeout deadline
 *
 * @param m Pointer to PCP mutex instance
 * @param timeout_us Timeout in microseconds
 * @return 0 on success, ETIMEDOUT on expiry, negative on error
 */
int edge_pcp_mutex_timedlock(edge_pcp_mutex_t *m, uint64_t timeout_us);

/**
 * @brief Release PCP real-time mutex, restores original task priority and records audit metrics
 *
 * @param m Pointer to PCP mutex instance
 * @return 0 on success, negative error code on failure
 */
int edge_pcp_mutex_unlock(edge_pcp_mutex_t *m);

/**
 * @brief Query runtime lock telemetry and contention statistics
 *
 * @param m Pointer to PCP mutex instance
 * @param stats Pointer to output statistics structure
 * @return 0 on success, negative error code on invalid parameters
 */
int edge_pcp_mutex_get_stats(const edge_pcp_mutex_t *m, edge_pcp_mutex_stats_t *stats);

/**
 * @brief Dynamically adjust priority ceiling (for PCP protocol)
 *
 * @param m Pointer to PCP mutex instance
 * @param new_ceiling New priority ceiling value (1 ~ 99)
 * @param old_ceiling Optional pointer to receive old priority ceiling
 * @return 0 on success, negative error code on failure
 */
int edge_pcp_mutex_set_prioceiling(edge_pcp_mutex_t *m, int new_ceiling, int *old_ceiling);

/**
 * @brief Destroy PCP mutex resources
 *
 * @param m Pointer to PCP mutex instance
 * @return 0 on success, negative error code on failure
 */
int edge_pcp_mutex_destroy(edge_pcp_mutex_t *m);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_PCP_MUTEX_H */
