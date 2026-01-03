/**
 * @file datahub.c
 * @brief 核心数据中心实现 (支持 S13 影子模式与 S8 多路监听)
 */

#include "datahub.h"

#include <stdlib.h>
#include <string.h>

static void _on_legacy_async(uv_async_t *handle) {
    datahub_t *hub = handle->data;
    if (hub && hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
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

    uv_async_init(loop, &hub->legacy_async, _on_legacy_async);
    hub->legacy_async.data = hub;

    hub->shadow_table = calloc(DATAHUB_MAX_TAGS, sizeof(pipe_msg_t));
    uv_mutex_init(&hub->shadow_lock);

    return 0;
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;

    if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_close((uv_handle_t *)&hub->legacy_async, NULL);
    }
    shmring_close(&hub->ring);

    if (hub->shadow_table) {
        free(hub->shadow_table);
        hub->shadow_table = NULL;
    }
    uv_mutex_destroy(&hub->shadow_lock);
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;

    // 1. 物理写入共享内存 RingBuffer
    shm_ring_write(&hub->ring, msg);

    // 2. 更新影子表
    if (msg->tag_id < DATAHUB_MAX_TAGS) {
        uv_mutex_lock(&hub->shadow_lock);
        hub->shadow_table[msg->tag_id] = *msg;
        uv_mutex_unlock(&hub->shadow_lock);
    }

    // 3. 多路异步通知 (S8)
    for (int i = 0; i < hub->listener_count; i++) {
        if (hub->listeners[i]) {
            uv_async_send(hub->listeners[i]);
        }
    }

    // 4. [补强] 安全同步触发：必须检查回调是否存在
    // 在单线程 Loop 中，这保证了测试的确定性
    if (hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
    }

    // 异步通知
    if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_async_send(&hub->legacy_async);
    }

    return 0;
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !out || tag_id >= DATAHUB_MAX_TAGS) return -1;

    uv_mutex_lock(&hub->shadow_lock);
    *out = hub->shadow_table[tag_id];
    uv_mutex_unlock(&hub->shadow_lock);

    return (out->ts == 0) ? -1 : 0;
}

int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle) {
    if (!hub || !async_handle || hub->listener_count >= DATAHUB_MAX_LISTENERS) return -1;
    hub->listeners[hub->listener_count++] = async_handle;
    return 0;
}

void datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle) {
    if (!hub || !async_handle) return;
    for (int i = 0; i < hub->listener_count; i++) {
        if (hub->listeners[i] == async_handle) {
            hub->listeners[i] = hub->listeners[hub->listener_count - 1];
            hub->listener_count--;
            return;
        }
    }
}

void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    hub->legacy_cb = cb;
    hub->legacy_user_data = user_data;
}

void datahub_unsubscribe(datahub_t *hub) {
    if (!hub) return;
    hub->legacy_cb = NULL;
    hub->legacy_user_data = NULL;
}
