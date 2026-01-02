#ifndef EDGE_DATAHUB_H
#define EDGE_DATAHUB_H

#include <uv.h>

#include "shmring.h"

#define DATAHUB_MAX_TAGS 16384
#define DATAHUB_MAX_LISTENERS 16

typedef struct datahub_s datahub_t;

typedef void (*datahub_on_data_cb)(datahub_t *hub, void *user_data);

struct datahub_s {
    uv_loop_t *loop;
    shm_ring_t ring;

    // S8: 多路通知机制 (支持 L1/L2/Northbound 并行消费)
    uv_async_t *listeners[DATAHUB_MAX_LISTENERS];
    int listener_count;

    // S13: 影子表物理空间
    pipe_msg_t *shadow_table;
    uv_mutex_t shadow_lock;

    // [兼容性] 旧版单回调句柄
    uv_async_t legacy_async;
    datahub_on_data_cb legacy_cb;
    void *legacy_user_data;
};

/**
 * @brief 初始化 DataHub
 * @param slots RingBuffer 的槽位数
 */
int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots);

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