#include "datahub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TAG_ID 4096

static void datahub_async_bridge(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (hub->on_data) hub->on_data(hub, hub->user_data);
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 datahub_on_data_cb on_data, void *user_data) {
    if (!hub || !loop) return -1;

    int ret = shmring_create(&hub->ring, name, slots);
    if (ret != 0) return ret;

    hub->loop = loop;
    hub->on_data = on_data;
    hub->user_data = user_data;

    hub->shadow_cache = calloc(MAX_TAG_ID, sizeof(tag_slot_t));
    uv_rwlock_init(&hub->shadow_lock);

    uv_async_init(loop, &hub->async_handle, datahub_async_bridge);
    hub->async_handle.data = hub;

    return 0;
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    if (msg->tag_id < MAX_TAG_ID) {
        uv_rwlock_wrlock(&hub->shadow_lock);
        hub->shadow_cache[msg->tag_id].msg = *msg;
        hub->shadow_cache[msg->tag_id].valid = true;
        uv_rwlock_wrunlock(&hub->shadow_lock);
    }
    shm_ring_write(&hub->ring, msg);
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
    if (hub->shadow_cache) free(hub->shadow_cache);
    shmring_destroy(&hub->ring);
}