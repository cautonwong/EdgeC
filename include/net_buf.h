/**
 * @file net_buf.h
 * @brief 增强版工业网络缓冲区 (支持原子计数与内嵌元数据)
 */

#ifndef EDGE_NET_BUF_H
#define EDGE_NET_BUF_H

#include <assert.h>
#include <endian.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_BUF_ALIGN __attribute__((__aligned__(sizeof(void *))))

/**
 * @brief 基础数据容器 (POD)
 */
struct net_buf_simple {
    uint8_t *data;  /**< 指向当前数据起始 */
    uint16_t len;   /**< 当前有效数据长度 */
    uint16_t size;  /**< 物理总容量 */
    uint8_t *__buf; /**< 物理内存起始 */
};

struct net_buf_pool;

/**
 * @brief 工业级网络缓冲区结构体
 */
struct net_buf {
    struct net_buf *frags;     /**< 链式缓冲区支持 */
    struct net_buf *next;      /**< 队列挂载支持 */
    struct net_buf_pool *pool; /**< 所属内存池 */

    atomic_uint ref; /**< [补强] 原子引用计数 (C11) */
    uint8_t flags;   /**< 标志位 */

#define NET_BUF_FLAG_PERSISTENT 0x01 /**< [Nexus] 长效缓冲区，绕过审计回收 */

    uint64_t alloc_ts;      /**< [Phase 4] 分配时间戳 (ns) */
    const char *last_owner; /**< [Phase 4] 责任追踪标签 */

    struct net_buf_simple b; /**< 数据视图 */

    /**
     * @brief [补强] 用户元数据区
     * 紧跟在结构体后面，大小由 pool->user_data_size 决定
     */
    uint8_t __user_data[] NET_BUF_ALIGN;
};

typedef struct net_buf net_buf_t;

/**
 * @brief 缓冲区内存池
 */
typedef struct net_buf_pool {
    uv_mutex_t lock;
    uv_cond_t cond;
    net_buf_t *free_list;
    uint16_t buf_count;
    uint16_t buf_size;
    uint16_t uninit_count;
    uint16_t user_data_size; /**< 每个 buffer 附带的元数据大小 */
    uint16_t peak_usage;     /**< [Phase 6] 峰值使用量统计 */
    void (*destroy_cb)(net_buf_t *buf);
    void *user_data;
    uint8_t *storage;
} net_buf_pool_t;

/**
 * @brief 获取内嵌元数据指针
 */
#define net_buf_user_data(buf) ((void *)((buf)->__user_data))

/* --- 1. net_buf_simple API (零分配解析逻辑) --- */

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

// --- 补强：大端/小端双向操作接口 ---

static inline void net_buf_simple_add_u8(struct net_buf_simple *buf, uint8_t val) {
    *(uint8_t *)net_buf_simple_add(buf, 1) = val;
}
static inline void net_buf_simple_push_u8(struct net_buf_simple *buf, uint8_t val) {
    *(uint8_t *)net_buf_simple_push(buf, 1) = val;
}

static inline void net_buf_simple_add_be16(struct net_buf_simple *buf, uint16_t val) {
    uint16_t v = htobe16(val);
    memcpy(net_buf_simple_add(buf, 2), &v, 2);
}
static inline uint16_t net_buf_simple_pull_be16(struct net_buf_simple *buf) {
    uint16_t v;
    memcpy(&v, net_buf_simple_pull(buf, 2), 2);
    return be16toh(v);
}

static inline void net_buf_simple_add_le16(struct net_buf_simple *buf, uint16_t val) {
    uint16_t v = htole16(val);
    memcpy(net_buf_simple_add(buf, 2), &v, 2);
}
static inline uint16_t net_buf_simple_pull_le16(struct net_buf_simple *buf) {
    uint16_t v;
    memcpy(&v, net_buf_simple_pull(buf, 2), 2);
    return le16toh(v);
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

static inline void net_buf_simple_add_be64(struct net_buf_simple *buf, uint64_t val) {
    uint64_t v = htobe64(val);
    memcpy(net_buf_simple_add(buf, 8), &v, 8);
}
static inline uint64_t net_buf_simple_pull_be64(struct net_buf_simple *buf) {
    uint64_t v;
    memcpy(&v, net_buf_simple_pull(buf, 8), 8);
    return be64toh(v);
}

static inline uint8_t net_buf_simple_pull_u8(struct net_buf_simple *buf) {
    return *(uint8_t *)net_buf_simple_pull(buf, 1);
}

/* --- 2. 完整 net_buf 声明周期 API --- */

int net_buf_pool_init(net_buf_pool_t *pool, uint16_t count, uint16_t size, uint16_t user_data_size,
                      void (*destroy_cb)(net_buf_t *buf), void *user_data);
void net_buf_pool_destroy(net_buf_pool_t *pool);

/**
 * @brief 内存池审计统计数据
 */
typedef struct {
    uint32_t total_count;
    uint32_t free_count;
    uint32_t leaked_reclaimed;
    uint32_t peak_usage;
} net_buf_pool_stats_t;

/**
 * @brief 审计并强制回收长时间未释放的 buffer (泄漏看门狗)
 * @param age_threshold_ms 存活时间阈值 (毫秒)
 * @param out_stats 输出统计信息 (可选)
 * @return 回收的数量
 */
int net_buf_pool_audit(net_buf_pool_t *pool, uint64_t age_threshold_ms,
                       net_buf_pool_stats_t *out_stats);

net_buf_t *net_buf_alloc(net_buf_pool_t *pool, int64_t timeout_ms);
net_buf_t *net_buf_ref(net_buf_t *buf);
void net_buf_unref(net_buf_t *buf);
net_buf_t *net_buf_clone(net_buf_t *buf, int64_t timeout_ms);
void *net_buf_append_bytes(net_buf_t *buf, const void *data, size_t len);

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

static inline void net_buf_reset(net_buf_t *buf) {
    net_buf_simple_reset(&buf->b);
}

#define net_buf_tailroom(buf) net_buf_simple_tailroom(&(buf)->b)
#define net_buf_headroom(buf) net_buf_simple_headroom(&(buf)->b)

// --- 链式支持 ---
void net_buf_frag_add(net_buf_t *head, net_buf_t *frag);
net_buf_t *net_buf_frag_del(net_buf_t *parent, net_buf_t *frag);
net_buf_t *net_buf_frag_last(net_buf_t *buf);
size_t net_buf_linearize(void *dst, size_t dst_len, net_buf_t *src, size_t offset, size_t len);

#ifdef __cplusplus
}
#endif

#endif
