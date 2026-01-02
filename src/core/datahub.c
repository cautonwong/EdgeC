/**
 * @file datahub.c
 * @brief 核心数据中心实现 (支持 S13 影子模式)
 */

#include "datahub.h"

#include <stdlib.h>
#include <string.h>

static void _on_async(uv_async_t *handle) {
    datahub_t *hub = handle->data;
    if (hub->on_data) {
        hub->on_data(hub, hub->user_data);
    }
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots) {
    memset(hub, 0, sizeof(*hub));
    hub->loop = loop;

    if (shmring_create(&hub->ring, name, slots) != 0) {
        if (shmring_open(&hub->ring, name) != 0) {
            return -1;
        }
    }

    uv_async_init(loop, &hub->async, _on_async);
    hub->async.data = hub;

    // S13: 分配影子表内存
    hub->shadow_table = calloc(DATAHUB_MAX_TAGS, sizeof(pipe_msg_t));
    uv_mutex_init(&hub->shadow_lock);

    return 0;
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;

    uv_close((uv_handle_t *)&hub->async, NULL);
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

    // 3. 异步通知
    uv_async_send(&hub->async);
    return 0;
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !out || tag_id >= DATAHUB_MAX_TAGS) return -1;

    uv_mutex_lock(&hub->shadow_lock);
    *out = hub->shadow_table[tag_id];
    uv_mutex_unlock(&hub->shadow_lock);

    // 如果时间戳为 0，说明该点位从未被更新过
    if (out->ts == 0) return -1;

    return 0;
}

void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    hub->on_data = cb;

    hub->user_data = user_data;
}

void datahub_unsubscribe(datahub_t *hub) {
    if (!hub) return;

    hub->on_data = NULL;

    hub->user_data = NULL;
}
