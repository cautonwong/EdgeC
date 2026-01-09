#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "datahub.h"
#include "edge_log.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

__attribute__((weak)) bool edge_router_needs_wakeup(uint32_t tag_id) {
    (void)tag_id;
    return true;
}

static void _on_legacy_async(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (hub && hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
    }
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 uint32_t shadow_capacity, const edge_tag_meta_t *metadata, uint32_t metadata_len,
                 datahub_tag_map_cb map_fn) {
    memset(hub, 0, sizeof(*hub));
    hub->loop = loop;
    hub->shadow_capacity = shadow_capacity;
    hub->metadata_table = metadata;
    hub->metadata_count = metadata_len;
    hub->map_fn = map_fn;

    if (shmring_create(&hub->ring, name, slots) != 0) return -1;

    if (shadow_capacity > 0) {
        size_t size = shadow_capacity * sizeof(pipe_msg_t);
        int fd = (int)syscall(SYS_memfd_create, "shadow_table", 0x0001U | 0x0002U);
        if (fd >= 0) {
            ftruncate(fd, (off_t)size);
            hub->shadow_table = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            hub->shadow_fd = fd;
            hub->shadow_size = size;
        }
    }

    uv_async_init(loop, &hub->legacy_async, _on_legacy_async);
    hub->legacy_async.data = hub;
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
        munmap(hub->shadow_table, hub->shadow_size);
        if (hub->shadow_fd >= 0) close(hub->shadow_fd);
    }
    uv_mutex_destroy(&hub->shadow_lock);
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    shm_ring_write(&hub->ring, msg);

    if (hub->shadow_table) {
        int idx = hub->map_fn ? hub->map_fn(msg->tag_id) : (int)msg->tag_id;
        if (idx >= 0 && (uint32_t)idx < hub->shadow_capacity) {
            uv_mutex_lock(&hub->shadow_lock);
            hub->shadow_table[idx] = *msg;
            uv_mutex_unlock(&hub->shadow_lock);
        }
    }

    if (edge_router_needs_wakeup(msg->tag_id)) {
        for (int i = 0; i < hub->listener_count; i++) {
            if (hub->listeners[i]) uv_async_send(hub->listeners[i]);
        }
        if (hub->legacy_cb) uv_async_send(&hub->legacy_async);
    }
    return 0;
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !hub->shadow_table || !out) return -1;
    int idx = hub->map_fn ? hub->map_fn(tag_id) : (int)tag_id;
    if (idx < 0 || (uint32_t)idx >= hub->shadow_capacity) return -1;
    uv_mutex_lock(&hub->shadow_lock);
    *out = hub->shadow_table[idx];
    uv_mutex_unlock(&hub->shadow_lock);
    return (out->tag_id == tag_id) ? 0 : -1;
}

void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub) return;
    hub->legacy_cb = cb;
    hub->legacy_user_data = user_data;
}

int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle) {
    if (hub->listener_count >= 16) return -1;
    hub->listeners[hub->listener_count++] = async_handle;
    return 0;
}

int datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle) {
    for (int i = 0; i < hub->listener_count; i++) {
        if (hub->listeners[i] == async_handle) {
            hub->listeners[i] = hub->listeners[--hub->listener_count];
            return 0;
        }
    }
    return -1;
}