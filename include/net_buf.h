/**
 * @file net_buf.h
 * @brief Network Buffer Library (100% Faithful Zephyr RTOS Port)
 */

#ifndef EDGE_NET_BUF_H
#define EDGE_NET_BUF_H

#include <assert.h>
#include <endian.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_BUF_ALIGN __attribute__((__aligned__(sizeof(void *))))

struct net_buf_simple {
    uint8_t *data;
    uint16_t len;
    uint16_t size;
    uint8_t *__buf;
};

struct net_buf_simple_state {
    uint16_t offset;
    uint16_t len;
};

struct net_buf_pool;

struct net_buf {
    struct net_buf *frags;
    struct net_buf *next;
    struct net_buf *parent;
    struct net_buf_pool *pool;
    uint8_t ref;
    uint8_t flags;
    void *user_data;
    struct net_buf_simple b;
    uint8_t __buf_start[] NET_BUF_ALIGN;
};

typedef struct net_buf net_buf_t;

typedef struct net_buf_pool {
    uv_mutex_t lock;
    uv_cond_t cond;
    net_buf_t *free_list;
    uint16_t buf_count;
    uint16_t buf_size;
    uint16_t uninit_count;
    uint16_t user_data_size;
    void (*destroy_cb)(net_buf_t *buf);
    void *user_data;
    uint8_t *storage;
} net_buf_pool_t;

#define net_buf_user_data(buf) ((void *)((buf)->__buf_start))

/* --- 1. net_buf_simple API --- */

static inline size_t net_buf_simple_tailroom(struct net_buf_simple *buf) {
    return (size_t)(buf->size - (buf->data - buf->__buf) - buf->len);
}
static inline size_t net_buf_simple_headroom(struct net_buf_simple *buf) {
    return (size_t)(buf->data - buf->__buf);
}

void net_buf_simple_init(struct net_buf_simple *buf, size_t reserve);
void net_buf_simple_init_with_data(struct net_buf_simple *buf, void *data, size_t size);
void *net_buf_simple_add(struct net_buf_simple *buf, size_t len);
void *net_buf_simple_add_mem(struct net_buf_simple *buf, const void *mem, size_t len);
void *net_buf_simple_push(struct net_buf_simple *buf, size_t len);
void *net_buf_simple_pull(struct net_buf_simple *buf, size_t len);
void *net_buf_simple_pull_mem(struct net_buf_simple *buf, void *dst, size_t len);
void net_buf_simple_reset(struct net_buf_simple *buf);

// Endian Helpers
static inline void net_buf_simple_add_u8(struct net_buf_simple *buf, uint8_t val) {
    *(uint8_t *)net_buf_simple_add(buf, 1) = val;
}

static inline void net_buf_simple_add_be64(struct net_buf_simple *buf, uint64_t val) {
    uint64_t v = htobe64(val);
    memcpy(net_buf_simple_add(buf, 8), &v, 8);
}
static inline uint64_t net_buf_simple_pull_be64(struct net_buf_simple *buf) {
    uint64_t v;
    memcpy(&v, net_buf_simple_pull(buf, 8), 8);
    return be64toh(v);
}
static inline void net_buf_simple_add_be32(struct net_buf_simple *buf, uint32_t val) {
    uint32_t v = htobe32(val);
    memcpy(net_buf_simple_add(buf, 4), &v, 4);
}
static inline uint32_t net_buf_simple_pull_be32(struct net_buf_simple *buf) {
    uint32_t v;
    memcpy(&v, net_buf_simple_pull(buf, 4), 4);
    return be32toh(v);
}
static inline void net_buf_simple_add_le32(struct net_buf_simple *buf, uint32_t val) {
    uint32_t v = htole32(val);
    memcpy(net_buf_simple_add(buf, 4), &v, 4);
}
static inline uint32_t net_buf_simple_pull_le32(struct net_buf_simple *buf) {
    uint32_t v;
    memcpy(&v, net_buf_simple_pull(buf, 4), 4);
    return le32toh(v);
}
static inline uint8_t net_buf_simple_pull_u8(struct net_buf_simple *buf) {
    return *(uint8_t *)net_buf_simple_pull(buf, 1);
}

/* --- 2. Full net_buf API --- */

int net_buf_pool_init(net_buf_pool_t *pool, uint16_t count, uint16_t size, uint16_t user_data_size,
                      void (*destroy_cb)(net_buf_t *buf), void *user_data);
void net_buf_pool_destroy(net_buf_pool_t *pool);
net_buf_t *net_buf_alloc(net_buf_pool_t *pool, int64_t timeout_ms);
net_buf_t *net_buf_ref(net_buf_t *buf);
void net_buf_unref(net_buf_t *buf);
net_buf_t *net_buf_clone(net_buf_t *buf, int64_t timeout_ms);
uint16_t net_buf_id(net_buf_t *buf);
net_buf_t *net_buf_pool_get(net_buf_pool_t *pool, uint16_t id);

static inline void net_buf_reserve(net_buf_t *buf, size_t reserve) {
    net_buf_simple_init(&buf->b, reserve);
}
static inline void *net_buf_add(net_buf_t *buf, size_t len) {
    return net_buf_simple_add(&buf->b, len);
}
static inline void *net_buf_push(net_buf_t *buf, size_t len) {
    return net_buf_simple_push(&buf->b, len);
}
static inline void *net_buf_pull(net_buf_t *buf, size_t len) {
    return net_buf_simple_pull(&buf->b, len);
}

#define net_buf_tailroom(buf) net_buf_simple_tailroom(&(buf)->b)
#define net_buf_headroom(buf) net_buf_simple_headroom(&(buf)->b)

void net_buf_frag_add(net_buf_t *head, net_buf_t *frag);
net_buf_t *net_buf_frag_del(net_buf_t *parent, net_buf_t *frag);
net_buf_t *net_buf_frag_last(net_buf_t *buf);
size_t net_buf_linearize(void *dst, size_t dst_len, net_buf_t *src, size_t offset, size_t len);

#ifdef __cplusplus
}
#endif

#endif