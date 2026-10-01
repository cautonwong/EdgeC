/**
 * @file edge_cpu_affinity.h
 * @brief CPU Core Affinity Isolation, IRQ Tuning, and Memory Locking (AEP-716 / Issue #247)
 *
 * Provides deterministic CPU core pinning, SMP IRQ interrupt steering away from RT cores,
 * mlockall memory page locking, and stack pre-faulting for hard real-time tasks.
 * Zero dynamic memory allocation (0 malloc) using Caller-Allocated storage.
 */

#ifndef EDGE_CPU_AFFINITY_H
#define EDGE_CPU_AFFINITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_CPU_MAX_CORES 64
#define EDGE_MAX_IRQS_TRACKED 64
#define EDGE_MAX_PINNED_THREADS 32

/**
 * @brief CPU Topology Information
 */
typedef struct {
    uint32_t total_online_cores;      /**< Total active cores detected (sysconf / /sys) */
    uint64_t available_core_mask;     /**< Bitmask of online CPU cores */
    uint32_t numa_nodes_count;        /**< Detected NUMA nodes */
    uint64_t isolated_core_mask;      /**< Mask of kernel-isolated cores (isolcpus) */
} edge_cpu_topology_t;

/**
 * @brief Thread Core Pinning Record
 */
typedef struct {
    pthread_t thread_handle;
    pid_t tid;
    uint32_t assigned_core_id;
    char task_name[32];
    bool is_active;
} edge_pinned_thread_info_t;

/**
 * @brief CPU Affinity Configuration
 */
typedef struct {
    uint64_t rt_core_mask;            /**< Mask of dedicated Real-Time execution cores */
    uint64_t housekeeping_core_mask;  /**< Mask of OS/housekeeping cores (e.g. Core 0) */
    bool auto_isolate_irqs;           /**< Automatically steer non-RT IRQs away from RT cores */
    bool enable_mlockall;             /**< Lock process memory pages via mlockall */
    size_t pre_fault_stack_size;      /**< Stack pre-fault pool bytes (e.g. 131072 for 128KB) */
    bool set_realtime_priority;       /**< Set SCHED_FIFO/SCHED_RR real-time priority */
    int realtime_priority;            /**< Real-time priority level (1..99) */
} edge_cpu_affinity_config_t;

/**
 * @brief CPU Affinity and IRQ Statistics
 */
typedef struct {
    uint64_t rt_core_mask;
    uint64_t housekeeping_core_mask;
    uint32_t pinned_thread_count;
    uint32_t irqs_steered_count;
    bool memory_locked;
    size_t prefaulted_bytes;
    uint64_t minor_page_faults;
    uint64_t major_page_faults;
} edge_cpu_affinity_stats_t;

/**
 * @brief Consumer-Defined Event Port for CPU Affinity & IRQ Tuning
 */
typedef struct {
    void (*on_core_pinned)(void *ctx, pid_t tid, uint32_t core_id, const char *task_name);
    void (*on_irq_migrated)(void *ctx, uint32_t irq_num, uint64_t prev_mask, uint64_t new_mask);
    void (*on_memlock_applied)(void *ctx, bool success, size_t prefaulted_bytes, int err_code);
    void *ctx;
} edge_cpu_affinity_port_t;

/**
 * @brief Caller-Allocated Storage for CPU Affinity Manager (4096 bytes, 64-bit aligned)
 */
typedef struct {
    uint64_t raw[512];
} edge_cpu_affinity_storage_t;

typedef struct edge_cpu_affinity edge_cpu_affinity_t;

/**
 * @brief In-place placement construct CPU Affinity Manager
 *
 * @param storage Caller-allocated memory buffer
 * @param cfg Configuration options
 * @param port Consumer-defined event callbacks (nullable)
 * @return edge_cpu_affinity_t* Pointer to initialized manager or NULL
 */
edge_cpu_affinity_t *edge_cpu_affinity_create_in(edge_cpu_affinity_storage_t *storage,
                                                 const edge_cpu_affinity_config_t *cfg,
                                                 const edge_cpu_affinity_port_t *port);

/**
 * @brief Destroy CPU Affinity Manager and restore default masks
 */
int edge_cpu_affinity_destroy(edge_cpu_affinity_t *aff);

/**
 * @brief Query system CPU topology
 */
int edge_cpu_affinity_get_topology(const edge_cpu_affinity_t *aff, edge_cpu_topology_t *topo);

/**
 * @brief Pin the calling thread to a specific CPU core
 */
int edge_cpu_affinity_pin_current_thread(edge_cpu_affinity_t *aff, uint32_t core_id, const char *task_name);

/**
 * @brief Pin a specified pthread to a specific CPU core
 */
int edge_cpu_affinity_pin_thread(edge_cpu_affinity_t *aff, pthread_t thread, uint32_t core_id, const char *task_name);

/**
 * @brief Set CPU affinity bitmask for a pthread
 */
int edge_cpu_affinity_set_mask_for_thread(edge_cpu_affinity_t *aff, pthread_t thread, uint64_t core_mask);

/**
 * @brief Lock current and future process memory pages (mlockall) and pre-fault stack
 */
int edge_cpu_affinity_lock_memory(edge_cpu_affinity_t *aff, size_t prefault_bytes);

/**
 * @brief Steer interrupts away from RT cores to target housekeeping mask
 */
int edge_cpu_affinity_steer_irqs(edge_cpu_affinity_t *aff, uint64_t target_mask, uint32_t *migrated_count);

/**
 * @brief Get CPU Affinity runtime statistics
 */
int edge_cpu_affinity_get_stats(const edge_cpu_affinity_t *aff, edge_cpu_affinity_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_CPU_AFFINITY_H */
