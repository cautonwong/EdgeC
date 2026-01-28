#ifndef EDGE_DATAHUB_H
#define EDGE_DATAHUB_H

#include <stdatomic.h>
#include <uv.h>

#include "contract_data_types.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

/**
 * @brief 测点映射回调
 */
typedef int (*datahub_tag_map_cb)(uint32_t tag_id);

/**
 * @brief 数据到达通知回调
 */
typedef void (*datahub_on_data_cb)(datahub_t *hub, void *user_data);

/**
 * @brief 唤醒过滤回调 (Phase 3 优化)
 */
typedef bool (*datahub_wakeup_filter_cb)(int index);

typedef struct {
    uint32_t tag_id;
    int fd;
    void *ptr;
    size_t len;
} datahub_ref_t;

struct datahub_s {
    uv_loop_t *loop;
    shm_ring_t ring;
    struct net_buf_pool *io_pool;  // [Phase 4] 注入的 IO 内存池引用

    const edge_tag_meta_t *metadata_table;
    uint32_t metadata_count;

    pipe_msg_t *shadow_table;
    atomic_uint *shadow_seq;  // [新增] 用于实现每槽位的 Sequence Lock
    int shadow_fd;
    size_t shadow_size;
    uint32_t shadow_capacity;
    datahub_tag_map_cb map_fn;
    datahub_wakeup_filter_cb wakeup_fn;  // [注入] 唤醒过滤器

    datahub_ref_t refs[32];  // [新增] 引用注册表，支持32个并发大块数据流
    atomic_int ref_count;    // [优化] 改为原子类型

    datahub_on_data_cb legacy_cb;
    void *legacy_user_data;
    uv_async_t legacy_async;

    uv_async_t *listeners[16];
    atomic_int listener_count;  // [优化] 改为原子类型
    uv_mutex_t shadow_lock;
};

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 uint32_t shadow_capacity, const edge_tag_meta_t *metadata, uint32_t metadata_len,
                 struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                 datahub_wakeup_filter_cb wakeup_fn);

int datahub_init_from_fd(datahub_t *hub, uv_loop_t *loop, int ring_fd, size_t ring_size,
                         int shadow_fd, size_t shadow_size, uint32_t compact_capacity,
                         const edge_tag_meta_t *metadata, uint32_t metadata_len,
                         struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                         datahub_wakeup_filter_cb wakeup_fn);

void datahub_destroy(datahub_t *hub);
int datahub_publish(datahub_t *hub, const pipe_msg_t *msg);
int datahub_register_ref(datahub_t *hub, uint32_t tag_id, int fd, size_t len);
int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out);
void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data);

/**
 * @brief [Aetheris] 持久化快照：将影子表存入磁盘
 */
int datahub_save_snapshot(datahub_t *hub, const char *path);

/**
 * @brief [Aetheris] 加载快照：从磁盘恢复影子表状态
 */
int datahub_load_snapshot(datahub_t *hub, const char *path);

// 监听器管理 (由 StreamEngine 等使用)
int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle);
int datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle);

static inline uint32_t datahub_get_metadata_count(datahub_t *hub) {
    return hub ? hub->metadata_count : 0;
}

static inline const edge_tag_meta_t *datahub_get_metadata_by_index(datahub_t *hub, uint32_t index) {
    if (hub && index < hub->metadata_count) return &hub->metadata_table[index];
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif
