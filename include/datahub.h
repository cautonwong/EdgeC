#ifndef EDGE_DATAHUB_H
#define EDGE_DATAHUB_H

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
typedef bool (*datahub_wakeup_filter_cb)(uint32_t tag_id);

struct datahub_s {
    uv_loop_t *loop;
    shm_ring_t ring;
    struct net_buf_pool *io_pool;  // [Phase 4] 注入的 IO 内存池引用

    const edge_tag_meta_t *metadata_table;
    uint32_t metadata_count;

    pipe_msg_t *shadow_table;
    int shadow_fd;
    size_t shadow_size;
    uint32_t shadow_capacity;
    datahub_tag_map_cb map_fn;
    datahub_wakeup_filter_cb wakeup_fn;  // [注入] 唤醒过滤器

    datahub_on_data_cb legacy_cb;
    void *legacy_user_data;
    uv_async_t legacy_async;

    uv_async_t *listeners[16];
    int listener_count;
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
int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out);
void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data);

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
