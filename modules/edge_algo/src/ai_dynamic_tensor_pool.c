/**
 * @file ai_dynamic_tensor_pool.c
 * @brief Dynamic Tensor Shape Static Memory Pool Implementation (AEP-706 / Issue #239)
 */

#include "datahub.h"
#include "ai_dynamic_tensor_pool.h"
#include <string.h>

/* Internal block metadata */
typedef struct {
    size_t offset;
    size_t size;
    bool is_free;
    ai_pool_scope_t scope;
    ai_tensor_desc_t desc;
} mem_block_t;

struct ai_dyn_pool {
    size_t arena_capacity;
    size_t current_allocated;
    size_t peak_watermark;
    uint32_t active_allocations;
    uint32_t total_alloc_count;
    uint32_t total_free_count;

    uint32_t num_blocks;
    mem_block_t blocks[AI_DYN_POOL_MAX_BLOCKS];
    ai_dyn_tensor_t tensors[AI_DYN_POOL_MAX_BLOCKS];

    uint8_t arena[AI_DYN_POOL_DEFAULT_CAPACITY];
};

_Static_assert(sizeof(struct ai_dyn_pool) <= sizeof(ai_dyn_pool_storage_t),
               "ai_dyn_pool_storage_t size is smaller than internal struct ai_dyn_pool");

size_t ai_tensor_desc_calc_bytes(const ai_tensor_desc_t *desc) {
    if (!desc || desc->ndim == 0 || desc->ndim > AI_MAX_TENSOR_DIMS) {
        return 0;
    }
    size_t num_elements = 1;
    for (uint8_t i = 0; i < desc->ndim; ++i) {
        if (desc->dims[i] == 0) return 0;
        num_elements *= desc->dims[i];
    }

    size_t elem_bits = 32;
    switch (desc->dtype) {
        case AI_TENSOR_DTYPE_FLOAT32:
        case AI_TENSOR_DTYPE_INT32:
            elem_bits = 32;
            break;
        case AI_TENSOR_DTYPE_FLOAT16:
            elem_bits = 16;
            break;
        case AI_TENSOR_DTYPE_INT8:
        case AI_TENSOR_DTYPE_UINT8:
            elem_bits = 8;
            break;
        case AI_TENSOR_DTYPE_INT4:
            elem_bits = 4;
            break;
        default:
            return 0;
    }

    size_t total_bits = num_elements * elem_bits;
    size_t total_bytes = (total_bits + 7) / 8;
    /* Round up to AI_DYN_POOL_ALIGNMENT */
    size_t aligned_bytes = (total_bytes + ((size_t)AI_DYN_POOL_ALIGNMENT - 1)) & ~((size_t)AI_DYN_POOL_ALIGNMENT - 1);
    return (aligned_bytes == 0) ? AI_DYN_POOL_ALIGNMENT : aligned_bytes;
}

static void coalesce_blocks(struct ai_dyn_pool *pool) {
    if (!pool || pool->num_blocks <= 1) return;

    for (uint32_t i = 0; i < pool->num_blocks - 1; ) {
        if (pool->blocks[i].is_free && pool->blocks[i + 1].is_free &&
            (pool->blocks[i].offset + pool->blocks[i].size == pool->blocks[i + 1].offset)) {
            pool->blocks[i].size += pool->blocks[i + 1].size;
            /* Shift remaining blocks */
            for (uint32_t j = i + 1; j < pool->num_blocks - 1; ++j) {
                pool->blocks[j] = pool->blocks[j + 1];
            }
            pool->num_blocks--;
        } else {
            i++;
        }
    }
}

ai_dyn_pool_t *ai_dyn_pool_create_in(ai_dyn_pool_storage_t *storage, size_t arena_capacity) {
    if (!storage) return NULL;

    struct ai_dyn_pool *pool = (struct ai_dyn_pool *)storage->raw;
    memset(pool, 0, sizeof(*pool));

    size_t cap = arena_capacity;
    if (cap == 0 || cap > AI_DYN_POOL_DEFAULT_CAPACITY) {
        cap = AI_DYN_POOL_DEFAULT_CAPACITY;
    }
    pool->arena_capacity = cap;
    pool->num_blocks = 1;
    pool->blocks[0].offset = 0;
    pool->blocks[0].size = cap;
    pool->blocks[0].is_free = true;

    return pool;
}

ai_dyn_tensor_t *ai_dyn_pool_alloc(ai_dyn_pool_t *pool,
                                   const ai_tensor_desc_t *desc,
                                   ai_pool_scope_t scope) {
    if (!pool || !desc) return NULL;

    size_t req_bytes = ai_tensor_desc_calc_bytes(desc);
    if (req_bytes == 0 || req_bytes > pool->arena_capacity) return NULL;

    /* Best-fit search */
    int32_t best_idx = -1;
    size_t min_surplus = (size_t)-1;

    for (uint32_t i = 0; i < pool->num_blocks; ++i) {
        if (pool->blocks[i].is_free && pool->blocks[i].size >= req_bytes) {
            size_t surplus = pool->blocks[i].size - req_bytes;
            if (surplus < min_surplus) {
                min_surplus = surplus;
                best_idx = (int32_t)i;
            }
        }
    }

    if (best_idx < 0) {
        return NULL; /* Out of memory in pool */
    }

    /* Split block if surplus is significant */
    if (min_surplus >= AI_DYN_POOL_ALIGNMENT && pool->num_blocks < AI_DYN_POOL_MAX_BLOCKS) {
        /* Shift blocks to make room for new block */
        for (uint32_t j = pool->num_blocks; j > (uint32_t)best_idx + 1; --j) {
            pool->blocks[j] = pool->blocks[j - 1];
        }
        pool->blocks[best_idx + 1].offset = pool->blocks[best_idx].offset + req_bytes;
        pool->blocks[best_idx + 1].size = min_surplus;
        pool->blocks[best_idx + 1].is_free = true;
        pool->blocks[best_idx].size = req_bytes;
        pool->num_blocks++;
    }

    pool->blocks[best_idx].is_free = false;
    pool->blocks[best_idx].scope = scope;
    pool->blocks[best_idx].desc = *desc;

    /* Find free tensor handle slot */
    ai_dyn_tensor_t *tensor = NULL;
    for (uint32_t t = 0; t < AI_DYN_POOL_MAX_BLOCKS; ++t) {
        if (!pool->tensors[t].in_use) {
            tensor = &pool->tensors[t];
            tensor->block_id = (uint32_t)best_idx;
            tensor->data = &pool->arena[pool->blocks[best_idx].offset];
            tensor->size_bytes = req_bytes;
            tensor->desc = *desc;
            tensor->scope = scope;
            tensor->in_use = true;
            break;
        }
    }

    if (!tensor) {
        /* Rollback block allocation */
        pool->blocks[best_idx].is_free = true;
        coalesce_blocks(pool);
        return NULL;
    }

    pool->current_allocated += req_bytes;
    if (pool->current_allocated > pool->peak_watermark) {
        pool->peak_watermark = pool->current_allocated;
    }
    pool->active_allocations++;
    pool->total_alloc_count++;

    return tensor;
}

bool ai_dyn_pool_free(ai_dyn_pool_t *pool, ai_dyn_tensor_t *tensor) {
    if (!pool || !tensor || !tensor->in_use) return false;

    /* Verify tensor data pointer is inside pool arena */
    if ((uint8_t *)tensor->data < pool->arena ||
        (uint8_t *)tensor->data >= pool->arena + pool->arena_capacity) {
        return false;
    }

    size_t offset = (size_t)((uint8_t *)tensor->data - pool->arena);
    int32_t matched_idx = -1;
    for (uint32_t i = 0; i < pool->num_blocks; ++i) {
        if (!pool->blocks[i].is_free && pool->blocks[i].offset == offset) {
            matched_idx = (int32_t)i;
            break;
        }
    }

    if (matched_idx < 0) return false;

    pool->blocks[matched_idx].is_free = true;
    if (pool->current_allocated >= pool->blocks[matched_idx].size) {
        pool->current_allocated -= pool->blocks[matched_idx].size;
    } else {
        pool->current_allocated = 0;
    }

    tensor->in_use = false;
    tensor->data = NULL;
    if (pool->active_allocations > 0) pool->active_allocations--;
    pool->total_free_count++;

    coalesce_blocks(pool);
    return true;
}

size_t ai_dyn_pool_free_scope(ai_dyn_pool_t *pool, ai_pool_scope_t scope) {
    if (!pool) return 0;

    size_t freed_count = 0;
    for (uint32_t t = 0; t < AI_DYN_POOL_MAX_BLOCKS; ++t) {
        if (pool->tensors[t].in_use && pool->tensors[t].scope == scope) {
            if (ai_dyn_pool_free(pool, &pool->tensors[t])) {
                freed_count++;
            }
        }
    }
    return freed_count;
}

void ai_dyn_pool_reset(ai_dyn_pool_t *pool) {
    if (!pool) return;
    pool->num_blocks = 1;
    pool->blocks[0].offset = 0;
    pool->blocks[0].size = pool->arena_capacity;
    pool->blocks[0].is_free = true;
    pool->current_allocated = 0;
    pool->active_allocations = 0;

    for (uint32_t t = 0; t < AI_DYN_POOL_MAX_BLOCKS; ++t) {
        pool->tensors[t].in_use = false;
        pool->tensors[t].data = NULL;
    }
}

bool ai_dyn_pool_get_stats(const ai_dyn_pool_t *pool, ai_dyn_pool_stats_t *stats) {
    if (!pool || !stats) return false;

    stats->total_capacity_bytes = pool->arena_capacity;
    stats->current_allocated_bytes = pool->current_allocated;
    stats->peak_watermark_bytes = pool->peak_watermark;
    stats->active_allocations = pool->active_allocations;
    stats->total_alloc_count = pool->total_alloc_count;
    stats->total_free_count = pool->total_free_count;

    size_t total_free = 0;
    size_t largest_free = 0;

    for (uint32_t i = 0; i < pool->num_blocks; ++i) {
        if (pool->blocks[i].is_free) {
            total_free += pool->blocks[i].size;
            if (pool->blocks[i].size > largest_free) {
                largest_free = pool->blocks[i].size;
            }
        }
    }

    stats->total_free_bytes = total_free;
    stats->largest_free_block_bytes = largest_free;

    if (total_free > 0) {
        stats->fragmentation_pct = 100.0f * (1.0f - ((float)largest_free / (float)total_free));
    } else {
        stats->fragmentation_pct = 0.0f;
    }

    return true;
}

int ai_dyn_pool_sync_datahub(const ai_dyn_pool_t *pool, datahub_t *hub) {
    if (!pool || !hub) return -1;

    ai_dyn_pool_stats_t stats;
    if (!ai_dyn_pool_get_stats(pool, &stats)) return -1;

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 262168, (double)stats.peak_watermark_bytes, 0);
    datahub_write_shadow(hub, &msg);

    pipe_msg_init_scalar(&msg, 262169, (double)stats.fragmentation_pct, 0);
    datahub_write_shadow(hub, &msg);

    return 0;
}
