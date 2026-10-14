/**
 * @file ai_dynamic_tensor_pool.h
 * @brief Dynamic Tensor Shape Static Memory Pool (AEP-706 / Issue #239)
 *
 * Implements industrial zero-heap static memory pool for dynamic tensor shapes:
 * - Fixed-capacity arena / multi-slab static pool with 0 malloc runtime guarantee;
 * - Multi-dimensional tensor descriptors (FP32, FP16, INT8, INT4, INT32, UINT8) and shape calculation;
 * - Scoped lifetime tags (EPHEMERAL, ACTIVATION, PERSISTENT) for instant batch deallocation;
 * - 64-byte cacheline / NPU DMA alignment;
 * - Real-time watermark tracking & external fragmentation rate calculation;
 * - DataHub telemetry synchronization.
 */

#ifndef EDGE_AI_DYNAMIC_TENSOR_POOL_H
#define EDGE_AI_DYNAMIC_TENSOR_POOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

/* -------------------------------------------------------------------------- */
/* Constants & Enumerations                                                   */
/* -------------------------------------------------------------------------- */

#define AI_DYN_POOL_MAX_BLOCKS        128
#define AI_DYN_POOL_DEFAULT_CAPACITY  65536
#define AI_DYN_POOL_ALIGNMENT         64
#define AI_MAX_TENSOR_DIMS            4

typedef enum {
    AI_TENSOR_DTYPE_FLOAT32 = 0,
    AI_TENSOR_DTYPE_FLOAT16 = 1,
    AI_TENSOR_DTYPE_INT8    = 2,
    AI_TENSOR_DTYPE_INT4    = 3,
    AI_TENSOR_DTYPE_INT32   = 4,
    AI_TENSOR_DTYPE_UINT8   = 5
} ai_tensor_dtype_t;

typedef enum {
    AI_POOL_SCOPE_EPHEMERAL  = 0,  /**< Intermediate scratchpad, freed after single op */
    AI_POOL_SCOPE_ACTIVATION = 1,  /**< Layer activation, freed after inference step */
    AI_POOL_SCOPE_PERSISTENT = 2   /**< Weights / persistent states */
} ai_pool_scope_t;

/**
 * @brief Tensor Shape and Descriptor
 */
typedef struct {
    ai_tensor_dtype_t dtype;
    uint8_t ndim;
    uint32_t dims[AI_MAX_TENSOR_DIMS];
} ai_tensor_desc_t;

/**
 * @brief Allocated Tensor Handle
 */
typedef struct {
    void *data;
    size_t size_bytes;
    ai_tensor_desc_t desc;
    ai_pool_scope_t scope;
    uint32_t block_id;
    bool in_use;
} ai_dyn_tensor_t;

/**
 * @brief Pool Statistics
 */
typedef struct {
    size_t total_capacity_bytes;
    size_t current_allocated_bytes;
    size_t peak_watermark_bytes;
    size_t largest_free_block_bytes;
    size_t total_free_bytes;
    float fragmentation_pct;
    uint32_t active_allocations;
    uint32_t total_alloc_count;
    uint32_t total_free_count;
} ai_dyn_pool_stats_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_DYN_POOL_STORAGE_SIZE       (AI_DYN_POOL_DEFAULT_CAPACITY + 16384)

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_DYN_POOL_STORAGE_SIZE];
} ai_dyn_pool_storage_t;

typedef struct ai_dyn_pool ai_dyn_pool_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Calculate byte size required for a given tensor descriptor
 */
size_t ai_tensor_desc_calc_bytes(const ai_tensor_desc_t *desc);

/**
 * @brief Initialize dynamic tensor memory pool in caller-allocated buffer
 */
ai_dyn_pool_t *ai_dyn_pool_create_in(ai_dyn_pool_storage_t *storage,
                                     size_t arena_capacity);

/**
 * @brief Allocate a dynamic tensor from the pool
 */
ai_dyn_tensor_t *ai_dyn_pool_alloc(ai_dyn_pool_t *pool,
                                   const ai_tensor_desc_t *desc,
                                   ai_pool_scope_t scope);

/**
 * @brief Free an individual allocated tensor
 */
bool ai_dyn_pool_free(ai_dyn_pool_t *pool, ai_dyn_tensor_t *tensor);

/**
 * @brief Batch free all allocations associated with a given scope
 */
size_t ai_dyn_pool_free_scope(ai_dyn_pool_t *pool, ai_pool_scope_t scope);

/**
 * @brief Reset pool completely, marking all blocks free
 */
void ai_dyn_pool_reset(ai_dyn_pool_t *pool);

/**
 * @brief Query current memory pool statistics and fragmentation
 */
bool ai_dyn_pool_get_stats(const ai_dyn_pool_t *pool, ai_dyn_pool_stats_t *stats);

#include "edge_algo_port.h"

/**
 * @brief Emit pool telemetry via Consumer-Defined Algo Sink
 */
int ai_dyn_pool_emit_metrics(const ai_dyn_pool_t *pool, const edge_algo_sink_t *sink);

/**
 * @brief Synchronize pool telemetry to DataHub (Adapter bridge)
 */
int ai_dyn_pool_sync_datahub(const ai_dyn_pool_t *pool, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_DYNAMIC_TENSOR_POOL_H */
