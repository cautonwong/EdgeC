#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "datahub.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "edge_log.h"

static void _on_legacy_async(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (hub && hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
    }
}

static int _datahub_post_init(datahub_t *hub, uv_loop_t *loop, uint32_t shadow_capacity,
                              const edge_tag_meta_t *metadata, uint32_t metadata_len,
                              struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                              datahub_wakeup_filter_cb wakeup_fn) {
    hub->loop = loop;
    hub->shadow_capacity = shadow_capacity;
    hub->metadata_table = metadata;
    hub->metadata_count = metadata_len;
    hub->io_pool = io_pool;
    hub->map_fn = map_fn;
    hub->wakeup_fn = wakeup_fn;

    uv_async_init(loop, &hub->legacy_async, _on_legacy_async);
    hub->legacy_async.data = hub;
    uv_mutex_init(&hub->shadow_lock);
    return 0;
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 uint32_t shadow_capacity, const edge_tag_meta_t *metadata, uint32_t metadata_len,
                 struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                 datahub_wakeup_filter_cb wakeup_fn) {
    memset(hub, 0, sizeof(*hub));

    if (shmring_create(&hub->ring, name, slots) != 0) return -1;

    if (shadow_capacity > 0) {
        size_t size = shadow_capacity * sizeof(pipe_msg_t);
        int fd = (int)syscall(SYS_memfd_create, "shadow_table", 0x0001U | 0x0002U);
        if (fd >= 0) {
            ftruncate(fd, (off_t)size);
            // 应用密封位 (F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL)
            fcntl(fd, 1033 /* F_ADD_SEALS */, 0x0002 | 0x0004 | 0x0001);
            hub->shadow_table = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            hub->shadow_fd = fd;
            hub->shadow_size = size;
        }
    }

    return _datahub_post_init(hub, loop, shadow_capacity, metadata, metadata_len, io_pool, map_fn,
                              wakeup_fn);
}

int datahub_init_from_fd(datahub_t *hub, uv_loop_t *loop, int ring_fd, size_t ring_size,
                         int shadow_fd, size_t shadow_size, uint32_t compact_capacity,
                         const edge_tag_meta_t *metadata, uint32_t metadata_len,
                         struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                         datahub_wakeup_filter_cb wakeup_fn) {
    memset(hub, 0, sizeof(*hub));

    if (shmring_init_from_fd(&hub->ring, ring_fd, ring_size) != 0) {
        return -1;
    }

    if (shadow_fd >= 0 && compact_capacity > 0) {
        hub->shadow_table =
            mmap(NULL, shadow_size, PROT_READ | PROT_WRITE, MAP_SHARED, shadow_fd, 0);
        hub->shadow_fd = shadow_fd;
        hub->shadow_size = shadow_size;
    }

    return _datahub_post_init(hub, loop, compact_capacity, metadata, metadata_len, io_pool, map_fn,
                              wakeup_fn);
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
            // fprintf(stderr, "[DEBUG][DataHub] Shadow updated for Tag %u at Index %d\n",
            // msg->tag_id, idx);
        } else {
            fprintf(stderr,
                    "[ERROR][DataHub] Shadow Index OUT OF BOUNDS for Tag %u (idx: %d, cap: %u)\n",
                    msg->tag_id, idx, hub->shadow_capacity);
        }
    }

    bool needs_wakeup = true;
    if (hub->wakeup_fn) {
        needs_wakeup = hub->wakeup_fn(msg->tag_id);
    }

    if (needs_wakeup) {
        fprintf(stderr, "[DEBUG][DataHub] Wakeup triggered for Tag %u. Listener count: %d\n",
                msg->tag_id, hub->listener_count);
        for (int i = 0; i < hub->listener_count; i++) {
            if (hub->listeners[i]) {
                fprintf(stderr, "[DEBUG][DataHub] Sending async signal to listener %d (%p)\n", i,
                        (void *)hub->listeners[i]);
                uv_async_send(hub->listeners[i]);
            }
        }
        if (hub->legacy_cb) hub->legacy_cb(hub, hub->legacy_user_data);
        if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) uv_async_send(&hub->legacy_async);
    } else {
        fprintf(stderr,
                "[DEBUG][DataHub] Wakeup SKIPPED for Tag %u (router decided no subscribers)\n",
                msg->tag_id);
    }
    return 0;
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !hub->shadow_table || !out) return -1;
    int idx = hub->map_fn ? hub->map_fn(tag_id) : (int)tag_id;
    if (idx < 0 || (uint32_t)idx >= hub->shadow_capacity) {
        fprintf(stderr, "[ERROR][DataHub] Shadow Read OUT OF BOUNDS for Tag %u (idx: %d)\n", tag_id,
                idx);
        return -1;
    }
    uv_mutex_lock(&hub->shadow_lock);
    *out = hub->shadow_table[idx];
    uv_mutex_unlock(&hub->shadow_lock);

    if (out->tag_id != tag_id) {
        // fprintf(stderr, "[DEBUG][DataHub] Shadow Read MISMATCH for Tag %u: found ID %u at Index
        // %d\n",
        //         tag_id, out->tag_id, idx);
        return -1;
    }
    return 0;
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