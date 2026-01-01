#include "datahub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TAG_ID 4096

// 异步桥接：在主循环中分发通知给所有订阅者
static void datahub_async_bridge(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;

    uv_mutex_lock(&hub->sub_lock);
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) {
        if (hub->subs[i].active && hub->subs[i].cb) {
            hub->subs[i].cb(hub, hub->subs[i].user_data);
        }
    }
    uv_mutex_unlock(&hub->sub_lock);
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots) {
    if (!hub || !loop) return -1;

    int ret = shmring_create(&hub->ring, name, slots);
    if (ret != 0) return ret;

    hub->loop = loop;
    uv_mutex_init(&hub->sub_lock);
    memset(hub->subs, 0, sizeof(hub->subs));

    hub->shadow_cache = calloc(MAX_TAG_ID, sizeof(tag_slot_t));
    uv_rwlock_init(&hub->shadow_lock);

    uv_async_init(loop, &hub->async_handle, datahub_async_bridge);
    hub->async_handle.data = hub;

    return 0;
}

int datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub || !cb) return -1;

    uv_mutex_lock(&hub->sub_lock);
    int sub_id = -1;
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) {
        if (!hub->subs[i].active) {
            hub->subs[i].cb = cb;
            hub->subs[i].user_data = user_data;
            hub->subs[i].active = true;
            sub_id = i;
            break;
        }
    }
    uv_mutex_unlock(&hub->sub_lock);
    return sub_id;
}

void datahub_unsubscribe(datahub_t *hub, int sub_id) {
    if (!hub || sub_id < 0 || sub_id >= DATAHUB_MAX_SUBSCRIBERS) return;
    uv_mutex_lock(&hub->sub_lock);
    hub->subs[sub_id].active = false;
    hub->subs[sub_id].cb = NULL;
    uv_mutex_unlock(&hub->sub_lock);
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;

    // 1. 更新影子缓存 (L1 读平面)
    if (msg->tag_id < MAX_TAG_ID) {
        uv_rwlock_wrlock(&hub->shadow_lock);
        hub->shadow_cache[msg->tag_id].msg = *msg;
        hub->shadow_cache[msg->tag_id].valid = true;
        uv_rwlock_wrunlock(&hub->shadow_lock);
    }

    // 2. 写入物理总线
    shm_ring_write(&hub->ring, msg);

    // 3. 异步唤醒订阅者
    return uv_async_send(&hub->async_handle);
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, PipeMsg *out) {
    if (!hub || tag_id >= MAX_TAG_ID) return -1;

    uv_rwlock_rdlock(&hub->shadow_lock);
    if (!hub->shadow_cache[tag_id].valid) {
        uv_rwlock_rdunlock(&hub->shadow_lock);
        return -1;
    }
    *out = hub->shadow_cache[tag_id].msg;
    uv_rwlock_rdunlock(&hub->shadow_lock);
    return 0;
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;

    if (!uv_is_closing((uv_handle_t *)&hub->async_handle)) {
        uv_close((uv_handle_t *)&hub->async_handle, NULL);
    }

    uv_rwlock_destroy(&hub->shadow_lock);
    uv_mutex_destroy(&hub->sub_lock);

    if (hub->shadow_cache) free(hub->shadow_cache);
    shmring_destroy(&hub->ring);
}
