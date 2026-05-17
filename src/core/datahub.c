#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "datahub.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "edge_log.h"

// --- 外部路由接口声明 (Weak Symbol) ---
__attribute__((weak)) bool edge_router_needs_wakeup(int index) {
    (void)index;
    return true;
}

static void _on_legacy_async(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (hub && hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
    }
}

// --- [Batch Wakeup] 核心逻辑 ---
static void _on_batch_check(uv_check_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (!atomic_exchange_explicit(&hub->batch_pending, false, memory_order_acquire)) {
        return;
    }

    // 触发所有注册的监听器 (StreamEngine 等)
    int l_count = atomic_load_explicit(&hub->listener_count, memory_order_acquire);
    if (l_count > 16) l_count = 16;

    for (int i = 0; i < l_count; i++) {
        uv_async_t *l = hub->listeners[i];
        if (l && !uv_is_closing((uv_handle_t *)l)) {
            uv_async_send(l);
        }
    }

    // 兼容旧版回调
    if (hub->legacy_cb) hub->legacy_cb(hub, hub->legacy_user_data);
    if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_async_send(&hub->legacy_async);
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

    atomic_init(&hub->ref_count, 0);
    atomic_init(&hub->listener_count, 0);
    atomic_init(&hub->batch_pending, false);

    uv_async_init(loop, &hub->legacy_async, _on_legacy_async);
    hub->legacy_async.data = hub;
    uv_mutex_init(&hub->shadow_lock);

    // 初始化 Batch Check 句柄
    uv_check_init(loop, &hub->batch_check);
    hub->batch_check.data = hub;
    uv_check_start(&hub->batch_check, _on_batch_check);

    if (hub->shadow_table && metadata && metadata_len > 0) {
        for (uint32_t i = 0; i < metadata_len; i++) {
            const edge_tag_meta_t *m = &metadata[i];
            int idx = (map_fn) ? map_fn(m->id) : (int)m->id;
            if (idx >= 0 && (uint32_t)idx < shadow_capacity) {
                pipe_msg_t *slot = &hub->shadow_table[idx];
                if (slot->tag_id == 0) {
                    pipe_msg_init_scalar(slot, m->id, m->default_value, uv_hrtime());
                }
            }
        }
    }

    return 0;
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 uint32_t shadow_capacity, const edge_tag_meta_t *metadata, uint32_t metadata_len,
                 struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                 datahub_wakeup_filter_cb wakeup_fn) {
    memset(hub, 0, sizeof(*hub));
    LOG_INFO(LOG_CAT_BUS, "DataHub: Initializing instance %p (Name: %s, Cap: %u)", (void *)hub, name, shadow_capacity);

    if (shmring_create(&hub->ring, name, slots) != 0) return -1;

    datahub_wakeup_filter_cb actual_wakeup = NULL;
    if (wakeup_fn != (void *)0) {
        actual_wakeup = wakeup_fn;
    }

    if (shadow_capacity > 0) {
        size_t size = shadow_capacity * sizeof(pipe_msg_t);
        int fd = (int)syscall(SYS_memfd_create, "shadow_table", 0x0001U | 0x0002U);
        if (fd >= 0) {
            ftruncate(fd, (off_t)size);
            fcntl(fd, 1033 /* F_ADD_SEALS */, 0x0002 | 0x0004 | 0x0001);
            hub->shadow_table = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            hub->shadow_fd = fd;
            hub->shadow_size = size;
            memset(hub->shadow_table, 0, size);
            hub->shadow_seq = calloc(shadow_capacity, sizeof(atomic_uint));
        }
    }

    return _datahub_post_init(hub, loop, shadow_capacity, metadata, metadata_len, io_pool, map_fn,
                              actual_wakeup);
}

int datahub_init_from_fd(datahub_t *hub, uv_loop_t *loop, int ring_fd, size_t ring_size,
                         int shadow_fd, size_t shadow_size, uint32_t compact_capacity,
                         const edge_tag_meta_t *metadata, uint32_t metadata_len,
                         struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                         datahub_wakeup_filter_cb wakeup_fn) {
    memset(hub, 0, sizeof(*hub));
    if (shmring_init_from_fd(&hub->ring, ring_fd, ring_size) != 0) return -1;

    if (shadow_fd >= 0 && compact_capacity > 0) {
        // [Nexus] CLI 工具只读映射影子表
        hub->shadow_table = mmap(NULL, shadow_size, PROT_READ, MAP_SHARED, shadow_fd, 0);
        if (hub->shadow_table != MAP_FAILED) {
            hub->shadow_fd = shadow_fd;
            hub->shadow_size = shadow_size;
            hub->shadow_capacity = compact_capacity;
            hub->shadow_seq = calloc(compact_capacity, sizeof(atomic_uint));
        } else {
            hub->shadow_table = NULL;
        }
    }

    return _datahub_post_init(hub, loop, compact_capacity, metadata, metadata_len, io_pool, map_fn, wakeup_fn);
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;
    if (!uv_is_closing((uv_handle_t *)&hub->batch_check)) {
        uv_check_stop(&hub->batch_check);
        uv_close((uv_handle_t *)&hub->batch_check, NULL);
    }
    if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_close((uv_handle_t *)&hub->legacy_async, NULL);
    }
    shmring_close(&hub->ring);
    if (hub->shadow_table) {
        munmap(hub->shadow_table, hub->shadow_size);
        if (hub->shadow_fd >= 0) close(hub->shadow_fd);
    }
    if (hub->shadow_seq) free(hub->shadow_seq);
    uv_mutex_destroy(&hub->shadow_lock);
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg || !hub->ring.hdr) return -1;

    pipe_msg_t local_msg = *msg;
    if (local_msg.type == PIPEMSG_TYPE_REF) {
        int r_count = atomic_load_explicit(&hub->ref_count, memory_order_relaxed);
        for (int i = 0; i < r_count; i++) {
            if (hub->refs[i].tag_id == local_msg.tag_id) {
                local_msg.payload.ref.ptr = hub->refs[i].ptr;
                local_msg.payload.ref.fd = hub->refs[i].fd;
                break;
            }
        }
    }

    shm_ring_write(&hub->ring, &local_msg);

    if (hub->shadow_table && hub->shadow_seq) {
        int idx = (hub->map_fn) ? hub->map_fn(local_msg.tag_id) : (int)local_msg.tag_id;
        LOG_INFO(LOG_CAT_BUS, "DataHub: Publish Tag %u -> Index %d on Hub %p", local_msg.tag_id, idx, (void *)hub);
        if (idx >= 0 && (uint32_t)idx < hub->shadow_capacity) {
            LOG_INFO(LOG_CAT_BUS, "DataHub: Updating Shadow Index %d with Val %.2f", idx, local_msg.payload.val.f);
            atomic_fetch_add_explicit(&hub->shadow_seq[idx], 1, memory_order_relaxed);
            atomic_thread_fence(memory_order_release);
            hub->shadow_table[idx] = local_msg;
            atomic_fetch_add_explicit(&hub->shadow_seq[idx], 1, memory_order_release);
        } else {
            LOG_WARN(LOG_CAT_BUS, "DataHub: Shadow update failed - Invalid Index %d for Tag %u on Hub %p", idx, local_msg.tag_id, (void *)hub);
        }
    }

    bool needs_wakeup = true;
    if (hub->wakeup_fn != (void *)0) {
        int idx = (hub->map_fn) ? hub->map_fn(local_msg.tag_id) : (int)local_msg.tag_id;
        needs_wakeup = hub->wakeup_fn(idx);
    }

    if (needs_wakeup) {
        atomic_store_explicit(&hub->batch_pending, true, memory_order_release);
    }
    return 0;
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !hub->shadow_table || !out) return -1;
    int idx = hub->map_fn ? hub->map_fn(tag_id) : (int)tag_id;
    if (idx < 0 || (uint32_t)idx >= hub->shadow_capacity) return -1;

    unsigned int s1, s2;
    do {
        s1 = atomic_load_explicit(&hub->shadow_seq[idx], memory_order_acquire);
        *out = hub->shadow_table[idx];
        s2 = atomic_load_explicit(&hub->shadow_seq[idx], memory_order_acquire);
    } while (s1 != s2 || (s1 & 1));

    if (out->tag_id == tag_id) {
        int r_count = atomic_load_explicit(&hub->ref_count, memory_order_relaxed);
        for (int i = 0; i < r_count; i++) {
            if (hub->refs[i].tag_id == out->tag_id) {
                out->payload.ref.ptr = hub->refs[i].ptr;
                out->payload.ref.fd = hub->refs[i].fd;
                break;
            }
        }
    }
    return (out->tag_id == tag_id) ? 0 : -1;
}

int datahub_register_ref(datahub_t *hub, uint32_t tag_id, int fd, size_t len) {
    if (!hub) return -1;
    int count = atomic_load_explicit(&hub->ref_count, memory_order_acquire);
    if (count >= 32) return -1;
    
    hub->refs[count].tag_id = tag_id;
    hub->refs[count].fd = fd;
    hub->refs[count].len = len;
    hub->refs[count].ptr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    atomic_fetch_add_explicit(&hub->ref_count, 1, memory_order_release);
    return 0;
}

void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub) return;
    hub->legacy_cb = cb;
    hub->legacy_user_data = user_data;
}

int datahub_save_snapshot(datahub_t *hub, const char *path) {
    if (!hub || !hub->shadow_table || !path) return -1;
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    uv_mutex_lock(&hub->shadow_lock);
    fwrite(hub->shadow_table, sizeof(pipe_msg_t), hub->shadow_capacity, f);
    uv_mutex_unlock(&hub->shadow_lock);
    fclose(f);
    return 0;
}

int datahub_load_snapshot(datahub_t *hub, const char *path) {
    if (!hub || !hub->shadow_table || !path) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    uv_mutex_lock(&hub->shadow_lock);
    fread(hub->shadow_table, sizeof(pipe_msg_t), hub->shadow_capacity, f);
    uv_mutex_unlock(&hub->shadow_lock);
    fclose(f);
    return 0;
}

uint32_t datahub_get_metadata_count(datahub_t *hub) {
    return (hub ? hub->metadata_count : 0);
}

int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle) {
    uv_mutex_lock(&hub->shadow_lock);
    int count = atomic_load_explicit(&hub->listener_count, memory_order_relaxed);
    if (count >= 16) {
        uv_mutex_unlock(&hub->shadow_lock);
        return -1;
    }
    hub->listeners[count] = async_handle;
    atomic_fetch_add_explicit(&hub->listener_count, 1, memory_order_release);
    uv_mutex_unlock(&hub->shadow_lock);
    return 0;
}

int datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle) {
    uv_mutex_lock(&hub->shadow_lock);
    int count = atomic_load_explicit(&hub->listener_count, memory_order_relaxed);
    for (int i = 0; i < count; i++) {
        if (hub->listeners[i] == async_handle) {
            hub->listeners[i] = hub->listeners[count - 1];
            atomic_fetch_sub_explicit(&hub->listener_count, 1, memory_order_release);
            uv_mutex_unlock(&hub->shadow_lock);
            return 0;
        }
    }
    uv_mutex_unlock(&hub->shadow_lock);
    return -1;
}
