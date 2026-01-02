/**
 * @file datahub.c
 * @brief 核心数据中心实现 (支持 S8 多路订阅与 S13 影子模式)
 */

#include "datahub.h"

#include <stdlib.h>
#include <string.h>

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots) {
    memset(hub, 0, sizeof(*hub));
    hub->loop = loop;

    if (shmring_create(&hub->ring, name, slots) != 0) {
        if (shmring_open(&hub->ring, name) != 0) {
            return -1;
        }
    }

    // S13: 分配影子表内存
    hub->shadow_table = calloc(DATAHUB_MAX_TAGS, sizeof(pipe_msg_t));
    uv_mutex_init(&hub->shadow_lock);

    hub->listener_count = 0;
    return 0;
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;

    shmring_close(&hub->ring);

    if (hub->shadow_table) {
        free(hub->shadow_table);
        hub->shadow_table = NULL;
    }
    uv_mutex_destroy(&hub->shadow_lock);
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;

    // 1. 物理写入共享内存 RingBuffer (供计算平面消费)
    shm_ring_write(&hub->ring, msg);

    // 2. S13: 同步更新影子表 (供北向点抄)
    if (msg->tag_id < DATAHUB_MAX_TAGS) {
        uv_mutex_lock(&hub->shadow_lock);
        hub->shadow_table[msg->tag_id] = *msg;
        uv_mutex_unlock(&hub->shadow_lock);
    }

    // 3. S8: 多路异步通知 (扇出)
    for (int i = 0; i < DATAHUB_MAX_LISTENERS; i++) {
        if (hub->listeners[i]) {
            uv_async_send(hub->listeners[i]);
        }
    }
    return 0;
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !out || tag_id >= DATAHUB_MAX_TAGS) return -1;

    uv_mutex_lock(&hub->shadow_lock);
    *out = hub->shadow_table[tag_id];
    uv_mutex_unlock(&hub->shadow_lock);

    if (out->ts == 0) return -1;
    return 0;
}

int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle) {
    if (!hub || !async_handle) return -1;
    
    for (int i = 0; i < DATAHUB_MAX_LISTENERS; i++) {
        if (hub->listeners[i] == NULL) {
            hub->listeners[i] = async_handle;
            hub->listener_count++;
            return 0;
        }
    }
    return -1; // 满额
}

void datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle) {
    if (!hub || !async_handle) return;
    
    for (int i = 0; i < DATAHUB_MAX_LISTENERS; i++) {
        if (hub->listeners[i] == async_handle) {
            hub->listeners[i] = NULL;
            hub->listener_count--;
            break;
        }
    }
}

// 兼容性接口：目前这些接口可能被旧测试使用，我们将其标记为过时或暂时空实现
// 建议全面转向 datahub_add_listener
void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    (void)hub; (void)cb; (void)user_data;
}

void datahub_unsubscribe(datahub_t *hub) {
    (void)hub;
}