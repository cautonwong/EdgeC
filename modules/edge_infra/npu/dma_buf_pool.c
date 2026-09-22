/**
 * @file dma_buf_pool.c
 * @brief EdgeOS Linux DMA-BUF Memory Lifecycle Manager & Buffer Pool Implementation (AEP-746 / Issue #276)
 */

#include "dma_buf_pool.h"
#include "npu_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/time.h>
#include <unistd.h>
#endif

/* -------------------------------------------------------------------------- */
/* Internal Pool Structure                                                    */
/* -------------------------------------------------------------------------- */

struct dma_buf_pool {
    dma_buf_pool_config_t config;
    dma_buf_desc_t *buffers;
    uint32_t capacity;
    uint32_t active_count;
    uint32_t peak_count;
    uint64_t total_allocs;
    uint64_t total_frees;
    uint64_t sync_ops;
    pthread_mutex_t lock;
    bool initialized;
};

static uint64_t dma_get_time_us(void) {
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
#endif
}

/* -------------------------------------------------------------------------- */
/* Public APIs Implementation                                                 */
/* -------------------------------------------------------------------------- */

int dma_buf_pool_init(dma_buf_pool_t **out_pool, const dma_buf_pool_config_t *cfg) {
    if (!out_pool || !cfg) return -1;
    if (cfg->buffer_size == 0 || cfg->pool_capacity == 0) return -2;

    uint32_t capacity = cfg->pool_capacity;
    if (capacity > DMA_BUF_MAX_POOL_SIZE) {
        capacity = DMA_BUF_MAX_POOL_SIZE;
    }

    uint32_t align = cfg->alignment ? cfg->alignment : DMA_BUF_DEFAULT_ALIGNMENT;

    dma_buf_pool_t *pool = (dma_buf_pool_t *)calloc(1, sizeof(dma_buf_pool_t));
    if (!pool) return -3;

    pool->buffers = (dma_buf_desc_t *)calloc(capacity, sizeof(dma_buf_desc_t));
    if (!pool->buffers) {
        free(pool);
        return -4;
    }

    pool->config = *cfg;
    pool->config.pool_capacity = capacity;
    pool->config.alignment = align;
    pool->capacity = capacity;
    pool->active_count = 0;
    pool->peak_count = 0;
    pool->total_allocs = 0;
    pool->total_frees = 0;
    pool->sync_ops = 0;
    pthread_mutex_init(&pool->lock, NULL);

    /* Pre-allocate aligned memory blocks and assign synthetic DMA-BUF file descriptors */
    for (uint32_t i = 0; i < capacity; i++) {
        void *mem_ptr = NULL;
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
        if (posix_memalign(&mem_ptr, align, cfg->buffer_size) != 0) {
            mem_ptr = malloc(cfg->buffer_size);
        }
#else
        mem_ptr = malloc(cfg->buffer_size);
#endif
        if (!mem_ptr) {
            /* Cleanup on allocation failure */
            for (uint32_t j = 0; j < i; j++) {
                if (pool->buffers[j].priv_mem) free(pool->buffers[j].priv_mem);
            }
            free(pool->buffers);
            pthread_mutex_destroy(&pool->lock);
            free(pool);
            return -5;
        }

        memset(mem_ptr, 0, cfg->buffer_size);

        pool->buffers[i].fd = (int)(100 + i); /* Synthetic Linux DMA-BUF fd */
        pool->buffers[i].vaddr = mem_ptr;
        pool->buffers[i].priv_mem = mem_ptr;
        pool->buffers[i].size = cfg->buffer_size;
        pool->buffers[i].ref_count = 0;
        pool->buffers[i].buffer_id = i;
        pool->buffers[i].is_in_use = false;
        pool->buffers[i].created_at_us = dma_get_time_us();
        pool->buffers[i].last_sync_us = pool->buffers[i].created_at_us;
    }

    pool->initialized = true;
    *out_pool = pool;
    return 0;
}

int dma_buf_pool_destroy(dma_buf_pool_t *pool) {
    if (!pool) return -1;

    pthread_mutex_lock(&pool->lock);
    if (pool->buffers) {
        for (uint32_t i = 0; i < pool->capacity; i++) {
            if (pool->buffers[i].priv_mem) {
                free(pool->buffers[i].priv_mem);
                pool->buffers[i].priv_mem = NULL;
                pool->buffers[i].vaddr = NULL;
            }
        }
        free(pool->buffers);
        pool->buffers = NULL;
    }
    pool->initialized = false;
    pthread_mutex_unlock(&pool->lock);
    pthread_mutex_destroy(&pool->lock);
    free(pool);
    return 0;
}

int dma_buf_pool_acquire(dma_buf_pool_t *pool, dma_buf_desc_t **out_buf) {
    if (!pool || !out_buf) return -1;

    pthread_mutex_lock(&pool->lock);
    if (!pool->initialized) {
        pthread_mutex_unlock(&pool->lock);
        return -2;
    }

    dma_buf_desc_t *found = NULL;
    for (uint32_t i = 0; i < pool->capacity; i++) {
        if (!pool->buffers[i].is_in_use) {
            pool->buffers[i].is_in_use = true;
            pool->buffers[i].ref_count = 1;
            pool->buffers[i].last_sync_us = dma_get_time_us();
            found = &pool->buffers[i];
            break;
        }
    }

    if (!found) {
        pthread_mutex_unlock(&pool->lock);
        return -3; /* Pool exhausted */
    }

    pool->active_count++;
    if (pool->active_count > pool->peak_count) {
        pool->peak_count = pool->active_count;
    }
    pool->total_allocs++;

    pthread_mutex_unlock(&pool->lock);
    *out_buf = found;
    return 0;
}

int dma_buf_ref(dma_buf_desc_t *buf) {
    if (!buf || !buf->is_in_use) return -1;
    buf->ref_count++;
    return 0;
}

int dma_buf_unref(dma_buf_pool_t *pool, dma_buf_desc_t *buf) {
    if (!pool || !buf || !buf->is_in_use) return -1;

    pthread_mutex_lock(&pool->lock);
    if (buf->ref_count > 0) {
        buf->ref_count--;
    }

    if (buf->ref_count == 0) {
        buf->is_in_use = false;
        if (pool->active_count > 0) {
            pool->active_count--;
        }
        pool->total_frees++;
    }

    pthread_mutex_unlock(&pool->lock);
    return 0;
}

int dma_buf_sync_start(dma_buf_desc_t *buf, dma_buf_sync_flags_t flags) {
    if (!buf || !buf->is_in_use) return -1;
    (void)flags;
    buf->last_sync_us = dma_get_time_us();
    return 0;
}

int dma_buf_sync_end(dma_buf_desc_t *buf, dma_buf_sync_flags_t flags) {
    if (!buf || !buf->is_in_use) return -1;
    (void)flags;
    buf->last_sync_us = dma_get_time_us();
    return 0;
}

int dma_buf_pool_get_stats(dma_buf_pool_t *pool, dma_buf_pool_stats_t *stats) {
    if (!pool || !stats) return -1;

    pthread_mutex_lock(&pool->lock);
    stats->total_buffers = pool->capacity;
    stats->active_buffers = pool->active_count;
    stats->peak_buffers = pool->peak_count;
    stats->total_allocations = pool->total_allocs;
    stats->total_releases = pool->total_frees;
    stats->sync_ops_count = pool->sync_ops;
    stats->total_memory_bytes = (size_t)pool->capacity * pool->config.buffer_size;
    pthread_mutex_unlock(&pool->lock);

    return 0;
}

int dma_buf_to_npu_tensor(const dma_buf_desc_t *buf, struct npu_tensor_buf *out_tensor) {
    if (!buf || !out_tensor) return -1;
    if (!buf->is_in_use || !buf->vaddr) return -2;

    memset(out_tensor, 0, sizeof(npu_tensor_buf_t));
    out_tensor->host_ptr = buf->vaddr;
    out_tensor->device_ptr = buf->vaddr;
    out_tensor->size = buf->size;
    out_tensor->dma_fd = buf->fd;
    out_tensor->flags = NPU_MEM_FLAG_ZERO_COPY_SHM | NPU_MEM_FLAG_UNIFIED;
    out_tensor->is_zero_copy = true;
    out_tensor->internal_handle = (void *)((uintptr_t)buf->vaddr ^ 0xDF000000);

    return 0;
}
