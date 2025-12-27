#include "datahub.h"
#include <stdlib.h>
#include <string.h>

static void on_async_notification(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (hub->on_data) {
        hub->on_data(hub, hub->user_data);
    }
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots, datahub_on_data_cb on_data, void *user_data) {
    if (!hub || !loop || !name) return -1;

    if (shmring_create(&hub->ring, name, slots) != 0) {
        // Try to open if exists
        if (shmring_open(&hub->ring, name) != 0) {
            return -1;
        }
    }

    hub->loop = loop;
    hub->on_data = on_data;
    hub->user_data = user_data;

    uv_async_init(loop, &hub->async_handle, on_async_notification);
    hub->async_handle.data = hub;

    return 0;
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;
    uv_close((uv_handle_t*)&hub->async_handle, NULL);
    shmring_close(&hub->ring);
    shmring_destroy(&hub->ring);
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    shm_ring_write(&hub->ring, msg);
    return uv_async_send(&hub->async_handle);
}
