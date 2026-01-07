#ifndef EDGE_DATAHUB_H
#define EDGE_DATAHUB_H

#include <uv.h>

#include "shmring.h"

#define DATAHUB_MAX_LISTENERS 16

typedef struct datahub_s datahub_t;

typedef void (*datahub_on_data_cb)(datahub_t *hub, void *user_data);

typedef int (*datahub_tag_map_cb)(uint32_t tag_id);

struct datahub_s {
    uv_loop_t *loop;

    shm_ring_t ring;

    // S8: 多路通知机制
    uv_async_t *listeners[DATAHUB_MAX_LISTENERS];
    int listener_count;

    // S13: 紧凑型影子表 (基于 memfd 共享)
    pipe_msg_t *shadow_table;
    int shadow_fd;
    size_t shadow_size;
    uint32_t shadow_capacity;
    uv_mutex_t shadow_lock;
    datahub_tag_map_cb map_fn;  // 注入映射逻辑

    // [兼容性]
    uv_async_t legacy_async;
    datahub_on_data_cb legacy_cb;
    void *legacy_user_data;
};

/**
 * @brief 初始化 DataHub
 * @param slots RingBuffer 的槽位数
 * @param compact_capacity 实际激活的测点总数
 * @param map_fn TagID -> Index 的映射函数 (通常由 contracts.h 提供)
 */
int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 uint32_t compact_capacity, datahub_tag_map_cb map_fn);

/**
 * @brief 从已有文件描述符初始化 DataHub (用于子进程/插件)
 * @param ring_fd RingBuffer 的 FD
 * @param ring_size RingBuffer 的大小
 * @param shadow_fd ShadowTable 的 FD
 * @param shadow_size ShadowTable 的大小
 */
int datahub_init_from_fd(datahub_t *hub, uv_loop_t *loop, int ring_fd, size_t ring_size,
                         int shadow_fd, size_t shadow_size, uint32_t compact_capacity,
                         datahub_tag_map_cb map_fn);

/**
 * @brief 销毁 DataHub
 */
void datahub_destroy(datahub_t *hub);

/**
 * @brief 发布数据到总线，并同步更新影子库 (S13)
 */
int datahub_publish(datahub_t *hub, const pipe_msg_t *msg);

/**
 * @brief 从影子库 O(1) 读取最新快照 (S13)
 */
int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out);

/**
 * @brief 注册异步通知句柄 (用于跨线程订阅)
 */
int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle);

/**
 * @brief 移除异步通知句柄
 */
void datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle);

/**
 * @brief [兼容接口] 订阅主循环通知
 */
void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data);

/**
 * @brief 退订通知 (S8: 用于平滑热更)
 */
void datahub_unsubscribe(datahub_t *hub);

#endif