#ifndef DATAHUB_H
#define DATAHUB_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DATAHUB_MAX_SUBSCRIBERS 8

typedef struct datahub_s datahub_t;

/**
 * @brief 数据到达回调函数原型
 */
typedef void (*datahub_on_data_cb)(datahub_t *hub, void *user_data);

typedef struct {
    PipeMsg msg;
    bool valid;
} tag_slot_t;

// 订阅者上下文
typedef struct {
    datahub_on_data_cb cb;
    void *user_data;
    bool active;
} datahub_sub_t;

struct datahub_s {
    shm_ring_t ring;
    uv_loop_t *loop;

    // 多播订阅支持
    datahub_sub_t subs[DATAHUB_MAX_SUBSCRIBERS];
    uv_mutex_t sub_lock;
    uv_async_t async_handle;

    // 影子缓存 (L1 数据平面)
    uv_rwlock_t shadow_lock;
    tag_slot_t *shadow_cache;
};

/**
 * @brief 初始化数据中枢
 * @param slots SHM 环的大小
 */
int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots);

/**
 * @brief 注册订阅者
 * @return 订阅 ID (>=0), 失败返回 -1
 */
int datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data);

/**
 * @brief 取消订阅
 */
void datahub_unsubscribe(datahub_t *hub, int sub_id);

/**
 * @brief 发布数据
 */
int datahub_publish(datahub_t *hub, const pipe_msg_t *msg);

/**
 * @brief 读取影子缓存
 */
int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, PipeMsg *out);

/**
 * @brief 销毁
 */
void datahub_destroy(datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif
