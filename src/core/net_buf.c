/**
 * @file net_buf.c
 * @brief 工业级网络缓冲区实现 (V3.0 升维版)
 */

#include "net_buf.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- 1. net_buf_simple 实现 (保持极致轻量) --- */

void net_buf_simple_init(struct net_buf_simple *buf, size_t reserve) {
    buf->data = buf->__buf + reserve;
    buf->len = 0;
}

void net_buf_simple_init_with_data(struct net_buf_simple *buf, void *data, size_t size) {
    buf->__buf = (uint8_t *)data;
    buf->data = (uint8_t *)data;
    buf->size = (uint16_t)size;
    buf->len = (uint16_t)size;
}

void net_buf_simple_reset(struct net_buf_simple *buf) {
    buf->len = 0;
    buf->data = buf->__buf;
}

void *net_buf_simple_add(struct net_buf_simple *buf, size_t len) {
    assert(net_buf_simple_tailroom(buf) >= len);
    void *ptr = buf->data + buf->len;
    buf->len += (uint16_t)len;
    return ptr;
}

void *net_buf_simple_add_mem(struct net_buf_simple *buf, const void *mem, size_t len) {
    void *ptr = net_buf_simple_add(buf, len);
    if (mem && len > 0) memcpy(ptr, mem, len);
    return ptr;
}

void *net_buf_simple_push(struct net_buf_simple *buf, size_t len) {
    assert(net_buf_simple_headroom(buf) >= len);
    buf->data -= len;
    buf->len += (uint16_t)len;
    return buf->data;
}

void *net_buf_simple_pull(struct net_buf_simple *buf, size_t len) {
    assert(buf->len >= len);
    void *ptr = buf->data;
    buf->data += len;
    buf->len -= (uint16_t)len;
    return ptr;
}

void *net_buf_simple_pull_mem(struct net_buf_simple *buf, void *dst, size_t len) {
    void *ptr = net_buf_simple_pull(buf, len);
    if (dst && len > 0) memcpy(dst, ptr, len);
    return ptr;
}

/* --- 2. 增强型 Pool 实现 --- */

/**
 * 内存布局：[net_buf 结构体] [元数据 UserData] [物理载荷 __buf] (全 8 字节对齐)
 */
static size_t get_node_size(net_buf_pool_t *pool) {
    size_t sz = sizeof(struct net_buf);
    sz += pool->user_data_size;
    // 确保载荷起始地址对齐
    sz = (sz + 7) & (size_t)~7;
    sz += pool->buf_size;
    return (sz + 7) & (size_t)~7;
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

    size_t total_size = (size_t)count * get_node_size(pool);
    pool->storage = calloc(1, total_size);
    if (!pool->storage) return -1;

    uv_mutex_init(&pool->lock);
    uv_cond_init(&pool->cond);
    return 0;
}

void net_buf_pool_destroy(net_buf_pool_t *pool) {
    if (!pool) return;
    uv_mutex_destroy(&pool->lock);
    uv_cond_destroy(&pool->cond);
    if (pool->storage) {
        free(pool->storage);
        pool->storage = NULL;
    }
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
            // 物理缓冲区位于 UserData 之后，且需重新计算对齐后的偏移
            size_t payload_offset =
                (sizeof(struct net_buf) + pool->user_data_size + 7) & (size_t)~7;
            buf->b.__buf = (uint8_t *)buf + payload_offset;
            pool->uninit_count--;
            break;
        }
        if (timeout_ms == 0) break;
        // 注意：此处简化处理，实际应根据 timeout_ms 决定等待时间
        uv_cond_wait(&pool->cond, &pool->lock);
        if (timeout_ms > 0) break;
    }
    uv_mutex_unlock(&pool->lock);

    if (buf) {
        atomic_init(&buf->ref, 1);
        buf->next = NULL;
        buf->frags = NULL;
        buf->flags = 0;
        net_buf_simple_init(&buf->b, 0);
    }
    return buf;
}

net_buf_t *net_buf_ref(net_buf_t *buf) {
    if (buf) atomic_fetch_add_explicit(&buf->ref, 1, memory_order_relaxed);
    return buf;
}

void net_buf_unref(net_buf_t *buf) {
    if (!buf) return;

    // 返回递减前的值，如果之前是 1，说明现在是 0，需要回收
    if (atomic_fetch_sub_explicit(&buf->ref, 1, memory_order_acq_rel) != 1) return;

    // 递归释放分片
    if (buf->frags) {
        net_buf_unref(buf->frags);
        buf->frags = NULL;
    }

    net_buf_pool_t *pool = buf->pool;
    if (pool->destroy_cb) pool->destroy_cb(buf);

    uv_mutex_lock(&pool->lock);
    buf->next = pool->free_list;
    pool->free_list = buf;
    uv_cond_signal(&pool->cond);
    uv_mutex_unlock(&pool->lock);
}

net_buf_t *net_buf_clone(net_buf_t *buf, int64_t timeout_ms) {
    net_buf_t *clone = net_buf_alloc(buf->pool, timeout_ms);
    if (!clone) return NULL;

    // 拷贝数据状态
    clone->b.data = clone->b.__buf + net_buf_simple_headroom(&buf->b);
    clone->b.len = buf->b.len;
    memcpy(clone->b.data, buf->b.data, buf->b.len);

    // 拷贝元数据
    memcpy(clone->__user_data, buf->__user_data, buf->pool->user_data_size);

    return clone;
}

void net_buf_frag_add(net_buf_t *head, net_buf_t *frag) {
    if (!head || !frag) return;
    net_buf_t *last = head;
    while (last->frags) last = last->frags;
    last->frags = frag;
}

net_buf_t *net_buf_frag_del(net_buf_t *parent, net_buf_t *frag) {
    if (!parent || !frag) return NULL;
    if (parent->frags == frag) {
        parent->frags = frag->frags;
        frag->frags = NULL;
        return frag;
    }
    net_buf_t *prev = parent->frags;
    while (prev && prev->frags != frag) prev = prev->frags;
    if (prev) {
        prev->frags = frag->frags;
        frag->frags = NULL;
        return frag;
    }
    return NULL;
}

net_buf_t *net_buf_frag_last(net_buf_t *buf) {
    net_buf_t *last = buf;
    while (last->frags) last = last->frags;
    return last;
}

size_t net_buf_linearize(void *dst, size_t dst_len, net_buf_t *src, size_t offset, size_t len) {
    size_t total = 0;
    net_buf_t *frag = src;
    while (frag && offset >= frag->b.len) {
        offset -= frag->b.len;
        frag = frag->frags;
    }
    while (frag && total < len && total < dst_len) {
        size_t to_copy = frag->b.len - offset;
        if (to_copy > (len - total)) to_copy = len - total;
        if (to_copy > (dst_len - total)) to_copy = dst_len - total;
        memcpy((uint8_t *)dst + total, frag->b.data + offset, to_copy);
        total += to_copy;
        offset = 0;
        frag = frag->frags;
    }
    return total;
}