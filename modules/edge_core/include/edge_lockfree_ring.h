/**
 * @file edge_lockfree_ring.h
 * @brief Lock-Free Zero-Copy RingBuffer Channel for Real-Time Producer-Consumer (AEP-717 / Issue #248)
 *
 * Provides wait-free SPSC and lock-free MPMC ringbuffer queues with zero heap allocation,
 * in-place zero-copy slot reservation/commit semantics, and atomic memory ordering.
 */

#ifndef EDGE_LOCKFREE_RING_H
#define EDGE_LOCKFREE_RING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_LOCKFREE_RING_MAX_CAPACITY 256
#define EDGE_LOCKFREE_RING_MAX_ELEM_SIZE 256

/**
 * @brief Ring Queue Concurrency Mode
 */
typedef enum {
    EDGE_RING_MODE_SPSC = 0, /**< Single Producer Single Consumer (Wait-Free fast track) */
    EDGE_RING_MODE_MPMC = 1, /**< Multi Producer Multi Consumer (Lock-Free CAS) */
} edge_ring_mode_t;

/**
 * @brief Overflow Behavior
 */
typedef enum {
    EDGE_RING_OVERFLOW_DROP = 0,      /**< Return -ENOSPC on full */
    EDGE_RING_OVERFLOW_OVERWRITE = 1, /**< Overwrite oldest unread element (SPSC only) */
} edge_ring_overflow_policy_t;

/**
 * @brief Ring Configuration
 */
typedef struct {
    edge_ring_mode_t mode;
    edge_ring_overflow_policy_t overflow_policy;
    uint32_t capacity;                /**< Queue capacity (must be power of 2, <= 256) */
    uint32_t elem_size_bytes;         /**< Size in bytes of each element (<= 256) */
    uint32_t high_watermark_percent;  /**< Threshold percent to fire watermark warning (e.g. 80) */
} edge_lockfree_ring_config_t;

/**
 * @brief Ring Diagnostics & Metrics
 */
typedef struct {
    uint32_t capacity;
    uint32_t elem_size_bytes;
    uint32_t current_depth;
    uint32_t watermark_peak;
    uint64_t total_enqueued;
    uint64_t total_dequeued;
    uint64_t total_dropped;
    edge_ring_mode_t mode;
} edge_lockfree_ring_stats_t;

/**
 * @brief Consumer-Defined Event Port for Lock-Free RingBuffer
 */
typedef struct {
    void (*on_watermark_high)(void *ctx, uint32_t current_depth, uint32_t capacity);
    void (*on_overflow)(void *ctx, uint64_t total_drops, const char *reason);
    void *ctx;
} edge_lockfree_ring_port_t;

/**
 * @brief Caller-Allocated Storage for Lock-Free RingBuffer (81920 bytes / 80KB, 64-bit aligned)
 */
typedef struct {
    uint64_t raw[10240];
} edge_lockfree_ring_storage_t;

typedef struct edge_lockfree_ring edge_lockfree_ring_t;

/**
 * @brief In-place placement construct Lock-Free RingBuffer
 *
 * @param storage Caller-allocated memory buffer
 * @param cfg Ring configuration parameters
 * @param port Event callbacks (nullable)
 * @return edge_lockfree_ring_t* Pointer to initialized ring or NULL
 */
edge_lockfree_ring_t *edge_lockfree_ring_create_in(edge_lockfree_ring_storage_t *storage,
                                                   const edge_lockfree_ring_config_t *cfg,
                                                   const edge_lockfree_ring_port_t *port);

/**
 * @brief Destroy ring instance
 */
int edge_lockfree_ring_destroy(edge_lockfree_ring_t *ring);

/**
 * @brief Enqueue element by copying value into ring
 */
int edge_lockfree_ring_enqueue(edge_lockfree_ring_t *ring, const void *elem);

/**
 * @brief Dequeue element by copying value out of ring
 */
int edge_lockfree_ring_dequeue(edge_lockfree_ring_t *ring, void *out_elem);

/**
 * @brief Reserve producer slot for zero-copy write
 *
 * @param ring Ring instance
 * @param out_slot_ptr Pointer to receive pointer to internal buffer slot
 * @param out_ticket Unique commit ticket for MPMC / SPSC tracking
 * @return 0 on success, -ENOSPC if full
 */
int edge_lockfree_ring_reserve(edge_lockfree_ring_t *ring, void **out_slot_ptr, uint64_t *out_ticket);

/**
 * @brief Commit previously reserved producer slot to make it visible to consumers
 *
 * @param ring Ring instance
 * @param ticket Ticket returned by reserve
 * @return 0 on success
 */
int edge_lockfree_ring_commit(edge_lockfree_ring_t *ring, uint64_t ticket);

/**
 * @brief Peek next readable slot for zero-copy read without consuming
 *
 * @param ring Ring instance
 * @param out_slot_ptr Pointer to receive read-only pointer to slot
 * @return 0 on success, -ENOENT if empty
 */
int edge_lockfree_ring_peek(edge_lockfree_ring_t *ring, const void **out_slot_ptr);

/**
 * @brief Advance read cursor to consume previously peeked element
 *
 * @param ring Ring instance
 * @return 0 on success, -ENOENT if empty
 */
int edge_lockfree_ring_advance(edge_lockfree_ring_t *ring);

/**
 * @brief Query current queue depth
 */
uint32_t edge_lockfree_ring_depth(const edge_lockfree_ring_t *ring);

/**
 * @brief Query current queue capacity
 */
uint32_t edge_lockfree_ring_capacity(const edge_lockfree_ring_t *ring);

/**
 * @brief Query runtime statistics
 */
int edge_lockfree_ring_get_stats(const edge_lockfree_ring_t *ring, edge_lockfree_ring_stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_LOCKFREE_RING_H */
