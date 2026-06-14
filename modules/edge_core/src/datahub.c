#define _GNU_SOURCE
#include "datahub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "edge_log.h"

static void _legacy_async_cb(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (hub && hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
    }
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 uint32_t shadow_capacity, const edge_tag_meta_t *metadata, uint32_t metadata_len,
                 struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                 datahub_wakeup_filter_cb wakeup_fn) {
    if (!hub) return -1;
    memset(hub, 0, sizeof(datahub_t));
    hub->wal_fd = -1;
    hub->loop = loop;
    hub->io_pool = io_pool;
    hub->metadata_table = metadata;
    hub->metadata_count = metadata_len;
    hub->map_fn = map_fn;
    hub->wakeup_fn = wakeup_fn;

    // 💡 [Omnis Auto-Capacity]
    uint32_t max_id = shadow_capacity;
    if (metadata) {
        for (uint32_t i = 0; i < metadata_len; i++) {
            if (metadata[i].id > max_id) max_id = metadata[i].id;
        }
    }
    uint32_t capacity = (max_id > shadow_capacity) ? max_id + 1 : shadow_capacity;
    hub->shadow_capacity = capacity;

    int ring_fd = shmring_create(&hub->ring, name, slots);
    if (ring_fd < 0) return -1;

    // 💡 [Nexus Fix] 回归最稳健的堆内存，消除 memfd 在某些环境下的可见性问题
    hub->shadow_table = calloc(capacity, sizeof(pipe_msg_t));
    if (!hub->shadow_table) return -1;
    
    // 初始化影子锁
    uv_mutex_init(&hub->shadow_lock);
    hub->legacy_async.data = hub;
    uv_async_init(loop, &hub->legacy_async, _legacy_async_cb);
    return 0;
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;
    shmring_destroy(&hub->ring);
    if (hub->shadow_table) free(hub->shadow_table);
    uv_mutex_destroy(&hub->shadow_lock);
    if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_close((uv_handle_t *)&hub->legacy_async, NULL);
    }
    if (hub->wal_fd >= 0) {
        close(hub->wal_fd);
        hub->wal_fd = -1;
    }
}

int datahub_write_shadow_idx(datahub_t *hub, int idx, const pipe_msg_t *msg) {
    if (!hub || idx < 0 || idx >= (int)hub->shadow_capacity || !msg || !hub->shadow_table) return -1;
    
    uv_mutex_lock(&hub->shadow_lock);
    hub->shadow_table[idx] = *msg;
    
    // 💡 [WAL] Write to Write-Ahead Log if enabled
    if (hub->wal_fd >= 0) {
        // Error ignored intentionally in fast path; robust systems might log once
        if (write(hub->wal_fd, msg, sizeof(pipe_msg_t)) != sizeof(pipe_msg_t)) {
            // Write failed, but we shouldn't block memory update
        }
    }
    uv_mutex_unlock(&hub->shadow_lock);
    
    if (hub->legacy_cb) {
        uv_async_send(&hub->legacy_async);
    }
    // Notify registered listeners (e.g. StreamEngine compute thread)
    int lc = atomic_load(&hub->listener_count);
    for (int i = 0; i < lc; i++) {
        if (hub->listeners[i]) uv_async_send(hub->listeners[i]);
    }
    return 0;
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg || !hub->ring.hdr) return -1;
    
    pipe_msg_t local_msg = *msg;
    shm_ring_write(&hub->ring, &local_msg);
    
    int idx = -1;
    if (hub->map_fn_v2) {
        idx = hub->map_fn_v2(msg->tag_id, msg->device_id);
    } else if (hub->map_fn) {
        idx = hub->map_fn(msg->tag_id);
    }
    if (idx < 0) idx = (int)msg->tag_id; // Fallback
    
    return datahub_write_shadow_idx(hub, idx, &local_msg);
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !out) return -1;
    int idx = -1;
    if (hub->map_fn_v2) {
        idx = hub->map_fn_v2(tag_id, 0); // Default device_id to 0
    } else if (hub->map_fn) {
        idx = hub->map_fn(tag_id);
    }
    if (idx < 0) idx = (int)tag_id;
    return datahub_read_shadow_idx(hub, (uint32_t)idx, out);
}

int datahub_read_shadow_idx(datahub_t *hub, uint32_t idx, pipe_msg_t *out) {
    if (!hub || !out || idx >= hub->shadow_capacity || !hub->shadow_table) return -1;
    
    uv_mutex_lock(&hub->shadow_lock);
    *out = hub->shadow_table[idx];
    uv_mutex_unlock(&hub->shadow_lock);
    return 0;
}

void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub) return;
    hub->legacy_cb = cb;
    hub->legacy_user_data = user_data;
}

int datahub_register_ref(datahub_t *hub, uint32_t tag_id, int fd, size_t len) {
    if (!hub) return -1;
    int idx = atomic_fetch_add(&hub->ref_count, 1);
    if (idx >= 32) {
        atomic_fetch_sub(&hub->ref_count, 1);
        return -1;
    }
    hub->refs[idx].tag_id = tag_id;
    hub->refs[idx].fd = fd;
    hub->refs[idx].len = len;
    hub->refs[idx].ptr = NULL;
    return idx;
}

int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle) {
    if (!hub || !async_handle) return -1;
    int idx = atomic_fetch_add(&hub->listener_count, 1);
    if (idx >= 16) {
        atomic_fetch_sub(&hub->listener_count, 1);
        return -1;
    }
    hub->listeners[idx] = async_handle;
    return 0;
}

int datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle) {
    if (!hub || !async_handle) return -1;
    int lc = atomic_load(&hub->listener_count);
    for (int i = 0; i < lc; i++) {
        if (hub->listeners[i] == async_handle) {
            hub->listeners[i] = hub->listeners[lc - 1];
            hub->listeners[lc - 1] = NULL;
            atomic_fetch_sub(&hub->listener_count, 1);
            return 0;
        }
    }
    return -1;
}

uint32_t datahub_get_metadata_count(datahub_t *hub) {
    return hub ? hub->metadata_count : 0;
}

int datahub_save_snapshot(datahub_t *hub, const char *path) {
    if (!hub || !path || !hub->shadow_table) return -1;
    FILE *fp = fopen(path, "wb");
    if (!fp) return -1;
    uv_mutex_lock(&hub->shadow_lock);
    size_t written = fwrite(hub->shadow_table, sizeof(pipe_msg_t), hub->shadow_capacity, fp);
    uv_mutex_unlock(&hub->shadow_lock);
    fclose(fp);
    return (written == hub->shadow_capacity) ? 0 : -1;
}

int datahub_load_snapshot(datahub_t *hub, const char *path) {
    if (!hub || !path || !hub->shadow_table) return -1;
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    uv_mutex_lock(&hub->shadow_lock);
    size_t nread = fread(hub->shadow_table, sizeof(pipe_msg_t), hub->shadow_capacity, fp);
    uv_mutex_unlock(&hub->shadow_lock);
    fclose(fp);
    return (nread == hub->shadow_capacity) ? 0 : -1;
}

int datahub_enable_wal(datahub_t *hub, const char *path) {
    if (!hub || !path) return -1;
    if (hub->wal_fd >= 0) close(hub->wal_fd);
    strncpy(hub->wal_path, path, sizeof(hub->wal_path) - 1);
    
    // Open for append, create if not exist
    hub->wal_fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (hub->wal_fd < 0) {
        printf("[DataHub] Failed to open WAL file: %s\n", path);
        return -1;
    }
    printf("[DataHub] WAL enabled: %s\n", path);
    return 0;
}

int datahub_replay_wal(datahub_t *hub) {
    if (!hub || hub->wal_path[0] == '\0') return -1;
    
    int fd = open(hub->wal_path, O_RDONLY);
    if (fd < 0) {
        printf("[DataHub] No existing WAL file to replay: %s\n", hub->wal_path);
        return 0; // Not an error if file doesn't exist yet
    }
    
    pipe_msg_t msg;
    int replayed_count = 0;
    while (read(fd, &msg, sizeof(pipe_msg_t)) == sizeof(pipe_msg_t)) {
        int idx = -1;
        if (hub->map_fn_v2) {
            idx = hub->map_fn_v2(msg.tag_id, msg.device_id);
        } else if (hub->map_fn) {
            idx = hub->map_fn(msg.tag_id);
        }
        if (idx < 0) idx = (int)msg.tag_id;
        
        if (idx >= 0 && idx < (int)hub->shadow_capacity) {
            hub->shadow_table[idx] = msg;
            replayed_count++;
        }
    }
    
    close(fd);
    printf("[DataHub] WAL replayed successfully, loaded %d events\n", replayed_count);
    return 0;
}

int datahub_init_from_contract(datahub_t *hub, uv_loop_t *loop, const edge_contract_t *contract,
                               struct net_buf_pool *io_pool) {
    if (!hub || !contract) return -1;
    return datahub_init(hub, loop, contract->name,
                        1024,  /* default ring slots */
                        contract->tag_count,
                        contract->tags, contract->tag_count,
                        io_pool, NULL, NULL);
}

int datahub_init_from_fd(datahub_t *hub, uv_loop_t *loop, int ring_fd, size_t ring_size,
                         int shadow_fd, size_t shadow_size, uint32_t compact_capacity,
                         const edge_tag_meta_t *metadata, uint32_t metadata_len,
                         struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                         datahub_wakeup_filter_cb wakeup_fn) {
    if (!hub) return -1;
    memset(hub, 0, sizeof(datahub_t));
    hub->loop = loop;
    hub->io_pool = io_pool;
    hub->metadata_table = metadata;
    hub->metadata_count = metadata_len;
    hub->map_fn = map_fn;
    hub->wakeup_fn = wakeup_fn;
    hub->shadow_capacity = compact_capacity;

    int rc = shmring_init_from_fd(&hub->ring, ring_fd, ring_size);
    if (rc < 0) return -1;

    hub->shadow_fd = shadow_fd;
    hub->shadow_size = shadow_size;
    hub->shadow_table = calloc(compact_capacity, sizeof(pipe_msg_t));
    if (!hub->shadow_table) return -1;

    uv_mutex_init(&hub->shadow_lock);
    hub->legacy_async.data = hub;
    uv_async_init(loop, &hub->legacy_async, _legacy_async_cb);
    return 0;
}
