#include "core_mem.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uv.h>

// 内存池的内部定义
typedef struct {
    net_buf_t **free_list; 
    size_t free_count;
    uv_mutex_t lock; // 使用 libuv 互斥锁
    char name[16];
    size_t total_bufs;
} slab_pool_t;

// --- 全局内存池定义 ---
#define POOL_SMALL_BUF_SIZE     256
#define POOL_SMALL_BUF_COUNT    1024
#define POOL_LARGE_BUF_SIZE     4096
#define POOL_LARGE_BUF_COUNT    128

static uint8_t g_pool_small_storage[POOL_SMALL_BUF_COUNT][POOL_SMALL_BUF_SIZE];
static uint8_t g_pool_large_storage[POOL_LARGE_BUF_COUNT][POOL_LARGE_BUF_SIZE];
static net_buf_t g_pool_small_bufs[POOL_SMALL_BUF_COUNT];
static net_buf_t g_pool_large_bufs[POOL_LARGE_BUF_COUNT];
static net_buf_t* g_pool_small_freelist[POOL_SMALL_BUF_COUNT];
static net_buf_t* g_pool_large_freelist[POOL_LARGE_BUF_COUNT];

static slab_pool_t g_pools[] = {
    [0] = { .name = "small", .total_bufs = POOL_SMALL_BUF_COUNT },
    [1] = { .name = "large", .total_bufs = POOL_LARGE_BUF_COUNT },
};
#define NUM_POOLS (sizeof(g_pools)/sizeof(slab_pool_t))

int core_mem_init(void) {
    // 初始化小内存池
    g_pools[0].free_list = g_pool_small_freelist;
    g_pools[0].free_count = POOL_SMALL_BUF_COUNT;
    uv_mutex_init(&g_pools[0].lock);
    for (size_t i = 0; i < POOL_SMALL_BUF_COUNT; i++) {
        g_pool_small_bufs[i].buffer_start = g_pool_small_storage[i];
        g_pool_small_bufs[i].size = POOL_SMALL_BUF_SIZE;
        g_pool_small_bufs[i].pool_id = 0;
        g_pools[0].free_list[i] = &g_pool_small_bufs[i];
    }

    // 初始化大内存池
    g_pools[1].free_list = g_pool_large_freelist;
    g_pools[1].free_count = POOL_LARGE_BUF_COUNT;
    uv_mutex_init(&g_pools[1].lock);
    for (size_t i = 0; i < POOL_LARGE_BUF_COUNT; i++) {
        g_pool_large_bufs[i].buffer_start = g_pool_large_storage[i];
        g_pool_large_bufs[i].size = POOL_LARGE_BUF_SIZE;
        g_pool_large_bufs[i].pool_id = 1;
        g_pools[1].free_list[i] = &g_pool_large_bufs[i];
    }

    printf("[Mem] Core memory initialized with %zu pools using libuv mutexes.\n", NUM_POOLS);
    return 0;
}

net_buf_t* core_net_buf_alloc(uint8_t pool_id) {
    if (pool_id >= NUM_POOLS) return NULL;

    slab_pool_t *pool = &g_pools[pool_id];
    net_buf_t *buf = NULL;

    uv_mutex_lock(&pool->lock);
    if (pool->free_count > 0) {
        buf = pool->free_list[--pool->free_count];
    }
    uv_mutex_unlock(&pool->lock);

    if (buf) {
        buf->next = NULL;
        atomic_init(&buf->ref, 1);
        buf->data = buf->buffer_start;
        buf->len = 0;
    }

    return buf;
}

void core_net_buf_unref(net_buf_t *buf) {
    if (!buf) return;

    if (atomic_fetch_sub(&buf->ref, 1) == 1) {
        slab_pool_t *pool = &g_pools[buf->pool_id];
        uv_mutex_lock(&pool->lock);
        pool->free_list[pool->free_count++] = buf;
        uv_mutex_unlock(&pool->lock);
    }
}

net_buf_t* core_net_buf_ref(net_buf_t *buf) {
    if (buf) {
        atomic_fetch_add(&buf->ref, 1);
    }
    return buf;
}

void* core_net_buf_push(net_buf_t *buf, size_t len) {
    if (!buf || buf->data - buf->buffer_start < (ptrdiff_t)len) {
        return NULL;
    }
    buf->data -= len;
    buf->len = (uint16_t)(buf->len + len);
    return buf->data;
}

void* core_net_buf_pull(net_buf_t *buf, size_t len) {
    if (!buf || buf->len < len) {
        return NULL;
    }
    void *old_data = buf->data;
    buf->data += len;
    buf->len = (uint16_t)(buf->len - len);
    return old_data;
}

void* core_net_buf_add(net_buf_t *buf, size_t len) {
    if (!buf) return NULL;
    uint8_t *tail = buf->data + buf->len;
    if (tail + len > buf->buffer_start + buf->size) {
        return NULL;
    }
    buf->len = (uint16_t)(buf->len + len);
    return tail;
}

int core_net_buf_append(net_buf_t *buf, const void *data, size_t len) {
    void *ptr = core_net_buf_add(buf, len);
    if (!ptr) return -1;
    memcpy(ptr, data, len);
    return 0;
}
