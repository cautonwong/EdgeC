/**
 * @file net_buf.c
 * @brief 增强版工业网络缓冲区 (支持原子计数与内嵌元数据)
 */

#include "net_buf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "edge_log.h"

static size_t get_node_size(net_buf_pool_t *pool) {
    size_t sz = sizeof(struct net_buf);
    sz = (sz + 7) & (size_t)~7;
    sz += pool->buf_size;
    return (sz + 7) & (size_t)~7;
}

net_buf_pool_t *net_buf_pool_new(uint32_t count, uint16_t size, uint16_t user_data_size) {
    net_buf_pool_t *pool = calloc(1, sizeof(net_buf_pool_t));
    if (!pool) return NULL;
    if (net_buf_pool_init(pool, (uint16_t)count, size, user_data_size, NULL, NULL) != 0) {
        free(pool);
        return NULL;
    }
    pool->flags |= NET_BUF_POOL_HEAP;
    LOG_INFO(LOG_CAT_CORE, "NetBufPool %p: Created (Refs: 1)", (void *)pool);
    return pool;
}

int net_buf_pool_init(net_buf_pool_t *pool, uint16_t count, uint16_t size, uint16_t user_data_size,
                      void (*destroy_cb)(net_buf_t *buf), void *user_data) {
    if (!pool || count == 0 || size == 0) return -1;
    memset(pool, 0, sizeof(*pool));
    pool->buf_count = count;
    pool->buf_size = size;
    pool->uninit_count = count;
    pool->user_data_size = (user_data_size + 7) & (uint16_t)~7;
    pool->destroy_cb = destroy_cb;
    pool->user_data = user_data;
    atomic_init(&pool->ref_count, 1);

    size_t total_size = (size_t)count * get_node_size(pool);
    pool->storage = calloc(1, total_size);
    if (!pool->storage) return -1;

    uv_mutex_init(&pool->lock);
    uv_cond_init(&pool->cond);
    return 0;
}

void net_buf_pool_ref(net_buf_pool_t *pool) {
    if (pool) {
        int refs = atomic_fetch_add(&pool->ref_count, 1);
        LOG_INFO(LOG_CAT_CORE, "NetBufPool %p: Ref (%d -> %d)", (void *)pool, refs, refs + 1);
    }
}

static void _net_buf_pool_destroy_internal(net_buf_pool_t *pool) {
    if (!pool) return;
    uv_mutex_destroy(&pool->lock);
    uv_cond_destroy(&pool->cond);
    if (pool->storage) {
        free(pool->storage);
        pool->storage = NULL;
    }
}

void net_buf_pool_unref(net_buf_pool_t *pool) {
    if (!pool) return;
    int refs = atomic_fetch_sub(&pool->ref_count, 1);
    LOG_INFO(LOG_CAT_CORE, "NetBufPool %p: Unref (%d -> %d)", (void *)pool, refs, refs - 1);
    if (refs == 1) {
        bool is_heap = (pool->flags & NET_BUF_POOL_HEAP);
        _net_buf_pool_destroy_internal(pool);
        if (is_heap) free(pool);
    }
}

void net_buf_pool_destroy(net_buf_pool_t *pool) {
    net_buf_pool_unref(pool);
}

uint16_t net_buf_id(net_buf_t *buf) {
    net_buf_pool_t *pool = buf->pool;
    size_t offset = (size_t)((uint8_t *)buf - pool->storage);
    return (uint16_t)(offset / get_node_size(pool));
}

net_buf_t *net_buf_pool_get(net_buf_pool_t *pool, uint16_t id) {
    if (id >= pool->buf_count) return NULL;
    return (net_buf_t *)(pool->storage + ((size_t)id * get_node_size(pool)));
}

net_buf_t *net_buf_alloc(net_buf_pool_t *pool, int64_t timeout_ms) {
    if (!pool) return NULL;
    uv_mutex_lock(&pool->lock);
    net_buf_t *buf = NULL;
    while (true) {
        if (pool->free_list) {
            buf = pool->free_list;
            pool->free_list = buf->next;
            break;
        }
        if (pool->uninit_count > 0) {
            buf = net_buf_pool_get(pool, (uint16_t)(pool->buf_count - pool->uninit_count));
            buf->pool = pool;
            buf->b.size = pool->buf_size;
            size_t payload_offset = (sizeof(struct net_buf) + pool->user_data_size + 7) & (size_t)~7;
            buf->b.__buf = (uint8_t *)buf + payload_offset;
            pool->uninit_count--;
            break;
        }
        if (timeout_ms == 0) break;
        uv_cond_wait(&pool->cond, &pool->lock);
        if (timeout_ms > 0) break;
    }
    uv_mutex_unlock(&pool->lock);

    if (buf) {
        atomic_init(&buf->ref, 1);
        buf->next = NULL;
        buf->frags = NULL;
        buf->flags = 0;
        buf->alloc_ts = uv_hrtime();
        buf->last_owner = "unknown";
        
        // [Nexus] 为协议栈预留 64 字节头部空间，防止 net_buf_push 覆盖 net_buf 结构体
        net_buf_simple_init(&buf->b, 64);
        
        buf->pool = pool;
        net_buf_pool_ref(pool);

        uint16_t current_usage = (uint16_t)(pool->buf_count - pool->uninit_count);
        if (current_usage > pool->peak_usage) pool->peak_usage = current_usage;
    }
    return buf;
}

net_buf_t *net_buf_ref(net_buf_t *buf) {
    if (buf) atomic_fetch_add_explicit(&buf->ref, 1, memory_order_relaxed);
    return buf;
}

void net_buf_unref(net_buf_t *buf) {
    if (!buf) return;
    if (atomic_fetch_sub_explicit(&buf->ref, 1, memory_order_acq_rel) != 1) return;

    if (buf->frags) {
        net_buf_unref(buf->frags);
        buf->frags = NULL;
    }

    net_buf_pool_t *pool = buf->pool;
    if (!pool || ((uintptr_t)pool & 7)) {
        LOG_ERROR(LOG_CAT_CORE, "net_buf: Attempted to unref buffer %p with invalid pool pointer %p!", (void *)buf, (void *)pool);
        return;
    }
    
    if (pool->destroy_cb) pool->destroy_cb(buf);

    uv_mutex_lock(&pool->lock);
    buf->next = pool->free_list;
    pool->free_list = buf;
    uv_cond_signal(&pool->cond);
    uv_mutex_unlock(&pool->lock);
    net_buf_pool_unref(pool);
}

net_buf_t *net_buf_clone(net_buf_t *buf, int64_t timeout_ms) {
    net_buf_t *clone = net_buf_alloc(buf->pool, timeout_ms);
    if (!clone) return NULL;
    net_buf_simple_add_mem(&clone->b, buf->b.data, buf->b.len);
    return clone;
}

void net_buf_simple_init(struct net_buf_simple *buf, size_t reserve) {
    buf->data = buf->__buf + reserve;
    buf->len = 0;
}

void net_buf_simple_init_with_data(struct net_buf_simple *buf, void *data, size_t size) {
    buf->__buf = (uint8_t *)data;
    buf->data = (uint8_t *)data;
    buf->size = (uint16_t)size;
    buf->len = 0; // [Nexus] 初始化为空，用于写入
}

void net_buf_simple_reset(struct net_buf_simple *buf) {
    buf->data = buf->__buf;
    buf->len = 0;
}

void *net_buf_simple_add(struct net_buf_simple *buf, size_t len) {
    void *ptr = buf->data + buf->len;
    buf->len = (uint16_t)(buf->len + len);
    return ptr;
}

void *net_buf_simple_add_mem(struct net_buf_simple *buf, const void *mem, size_t len) {
    return memcpy(net_buf_simple_add(buf, len), mem, len);
}

void *net_buf_simple_pull(struct net_buf_simple *buf, size_t len) {
    if (!buf || len > buf->len) return NULL;
    void *ptr = buf->data;
    buf->data += len;
    buf->len = (uint16_t)(buf->len - len);
    return ptr;
}

void *net_buf_simple_push(struct net_buf_simple *buf, size_t len) {
    if (!buf || (buf->data - len < buf->__buf)) return NULL;
    buf->data -= len;
    buf->len = (uint16_t)(buf->len + len);
    return buf->data;
}

void *net_buf_append_bytes(net_buf_t *buf, const void *data, size_t len) {
    return net_buf_simple_add_mem(&buf->b, data, len);
}

int net_buf_pool_audit(net_buf_pool_t *pool, uint64_t threshold_ms, net_buf_pool_stats_t *out_stats) {
    if (!pool) return 0;
    int reclaimed = 0;
    uint64_t now = uv_hrtime();
    uint64_t threshold_ns = threshold_ms * 1000000;

    uv_mutex_lock(&pool->lock);
    if (out_stats) {
        out_stats->total_count = pool->buf_count;
        out_stats->peak_usage = pool->peak_usage;
        out_stats->free_count = 0;
    }

    size_t node_sz = get_node_size(pool);
    for (uint16_t i = 0; i < pool->buf_count; i++) {
        net_buf_t *buf = (net_buf_t *)(pool->storage + (size_t)i * node_sz);
        uint32_t ref = atomic_load_explicit(&buf->ref, memory_order_acquire);
        if (ref > 0) {
            if (buf->flags & 0x01 /* PERSISTENT */) continue;
            uint64_t age = now - buf->alloc_ts;
            if (age > threshold_ns) {
                LOG_WARN(LOG_CAT_CORE, "Long-lived NetBuf: ID %u, Age %lu ms, Owner %s", i, (unsigned long)(age / 1000000), buf->last_owner);
            }
        } else if (out_stats) {
            out_stats->free_count++;
        }
    }
    uv_mutex_unlock(&pool->lock);
    return reclaimed;
}
