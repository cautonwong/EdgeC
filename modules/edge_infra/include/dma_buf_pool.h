/**
 * @file dma_buf_pool.h
 * @brief EdgeOS Linux DMA-BUF Memory Lifecycle Manager & Pre-allocated Buffer Pool (AEP-746 / Issue #276)
 *
 * Provides standardized DMA-BUF lifecycle management, pre-allocated memory pool,
 * reference counting, CPU/NPU cache synchronization, and zero-copy NPU tensor mapping.
 */

#ifndef EDGE_DMA_BUF_POOL_H
#define EDGE_DMA_BUF_POOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct npu_tensor_buf;

/* -------------------------------------------------------------------------- */
/* Constants and Enums                                                        */
/* -------------------------------------------------------------------------- */

#define DMA_BUF_DEFAULT_ALIGNMENT    64
#define DMA_BUF_MAX_POOL_SIZE        64

typedef enum {
    DMA_BUF_SYNC_READ  = (1 << 0),
    DMA_BUF_SYNC_WRITE = (1 << 1),
    DMA_BUF_SYNC_RW    = (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
} dma_buf_sync_flags_t;

/* -------------------------------------------------------------------------- */
/* Structures                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief DMA-BUF Descriptor representing an active or pooled DMA-BUF
 */
typedef struct dma_buf_desc {
    int fd;                         /**< Linux DMA-BUF file descriptor (simulated or real) */
    void *vaddr;                    /**< Mapped virtual memory pointer */
    size_t size;                    /**< Buffer byte size */
    uint32_t ref_count;             /**< Active reference count across drivers/NPU */
    uint32_t buffer_id;             /**< Sequential buffer ID inside pool */
    bool is_in_use;                 /**< True if checked out from pool */
    uint64_t created_at_us;         /**< Creation timestamp in us */
    uint64_t last_sync_us;          /**< Last cache sync timestamp in us */
    void *priv_mem;                 /**< Internal backing memory buffer */
} dma_buf_desc_t;

/**
 * @brief DMA-BUF Pool Configuration
 */
typedef struct {
    size_t buffer_size;             /**< Byte size of each buffer in pool */
    uint32_t pool_capacity;         /**< Number of pre-allocated buffers */
    bool enable_auto_flush;         /**< Automatically flush cache on start/end sync */
    uint32_t alignment;             /**< Memory byte alignment (default: 64) */
} dma_buf_pool_config_t;

/**
 * @brief DMA-BUF Pool Statistics & Monitoring
 */
typedef struct {
    uint32_t total_buffers;         /**< Total buffers in pool */
    uint32_t active_buffers;        /**< Currently acquired buffers */
    uint32_t peak_buffers;          /**< Peak concurrent buffer usage */
    uint64_t total_allocations;     /**< Cumulative acquire count */
    uint64_t total_releases;        /**< Cumulative release count */
    uint64_t sync_ops_count;        /**< Cumulative sync operations */
    size_t total_memory_bytes;      /**< Total memory managed by pool */
} dma_buf_pool_stats_t;

typedef struct dma_buf_pool dma_buf_pool_t;

/* -------------------------------------------------------------------------- */
/* Public APIs                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize a pre-allocated DMA-BUF buffer pool
 */
int dma_buf_pool_init(dma_buf_pool_t **out_pool, const dma_buf_pool_config_t *cfg);

/**
 * @brief Destroy a DMA-BUF pool and free all underlying buffers
 */
int dma_buf_pool_destroy(dma_buf_pool_t *pool);

/**
 * @brief Acquire a pre-allocated buffer from the pool (ref_count starts at 1)
 */
int dma_buf_pool_acquire(dma_buf_pool_t *pool, dma_buf_desc_t **out_buf);

/**
 * @brief Increment the reference count of a DMA-BUF descriptor
 */
int dma_buf_ref(dma_buf_desc_t *buf);

/**
 * @brief Decrement reference count; if it drops to 0, returns buffer to pool
 */
int dma_buf_unref(dma_buf_pool_t *pool, dma_buf_desc_t *buf);

/**
 * @brief Start CPU/Device synchronization before read/write access
 */
int dma_buf_sync_start(dma_buf_desc_t *buf, dma_buf_sync_flags_t flags);

/**
 * @brief End CPU/Device synchronization after access
 */
int dma_buf_sync_end(dma_buf_desc_t *buf, dma_buf_sync_flags_t flags);

/**
 * @brief Get telemetry statistics of the DMA-BUF pool
 */
int dma_buf_pool_get_stats(dma_buf_pool_t *pool, dma_buf_pool_stats_t *stats);

/**
 * @brief Convert DMA-BUF descriptor to NPU HAL tensor buffer structure
 */
int dma_buf_to_npu_tensor(const dma_buf_desc_t *buf, struct npu_tensor_buf *out_tensor);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_DMA_BUF_POOL_H */
