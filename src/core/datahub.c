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
// 默认行为：如果编译器没有生成路由表，则唤醒所有订阅者（回退到旧行为）
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

    // [Aetheris Fix] 从元数据回填契约默认值，解决“睁眼全是0”的问题
    if (hub->shadow_table && metadata && metadata_len > 0) {
        for (uint32_t i = 0; i < metadata_len; i++) {
            const edge_tag_meta_t *m = &metadata[i];
            int idx = (map_fn) ? map_fn(m->id) : (int)m->id;
            if (idx >= 0 && (uint32_t)idx < shadow_capacity) {
                pipe_msg_t *slot = &hub->shadow_table[idx];
                // 仅当槽位未被占用时初始化 (防止 init_from_fd 冲突)
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

    if (shmring_create(&hub->ring, name, slots) != 0) return -1;

    // [Defense] 防止传入的弱符号地址为 NULL (未正确链接路由表)
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
            memset(hub->shadow_table, 0, size);  // 初始化清零
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

int datahub_save_snapshot(datahub_t *hub, const char *path) {
    if (!hub || !hub->shadow_table || !path) return -1;

    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    uv_mutex_lock(&hub->shadow_lock);
    size_t written = fwrite(hub->shadow_table, sizeof(pipe_msg_t), hub->shadow_capacity, f);
    uv_mutex_unlock(&hub->shadow_lock);

    fclose(f);
    LOG_INFO(LOG_CAT_BUS, "DataHub: Snapshot saved to %s (%zu slots)", path, written);
    return (written == hub->shadow_capacity) ? 0 : -1;
}

int datahub_load_snapshot(datahub_t *hub, const char *path) {
    if (!hub || !hub->shadow_table || !path) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    uv_mutex_lock(&hub->shadow_lock);
    size_t read = fread(hub->shadow_table, sizeof(pipe_msg_t), hub->shadow_capacity, f);
    uv_mutex_unlock(&hub->shadow_lock);

    fclose(f);
    LOG_INFO(LOG_CAT_BUS, "DataHub: Snapshot loaded from %s (%zu slots)", path, read);
    return (read == hub->shadow_capacity) ? 0 : -1;
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    if (hub->ring.hdr == NULL) {
        return -1;
    }

    pipe_msg_t local_msg = *msg;

    // Phase 4: 如果是引用类型，尝试校准本地指针 (针对同进程内转发)
    if (local_msg.type == PIPEMSG_TYPE_REF) {
        for (int i = 0; i < hub->ref_count; i++) {
            if (hub->refs[i].tag_id == local_msg.tag_id) {
                local_msg.payload.ref.ptr = hub->refs[i].ptr;
                local_msg.payload.ref.fd = hub->refs[i].fd;
                break;
            }
        }
    }

    shm_ring_write(&hub->ring, &local_msg);

    if (hub->shadow_table) {
        int idx = (hub->map_fn) ? hub->map_fn(local_msg.tag_id) : (int)local_msg.tag_id;

        if (idx >= 0 && (uint32_t)idx < hub->shadow_capacity) {
            uv_mutex_lock(&hub->shadow_lock);
            hub->shadow_table[idx] = local_msg;
            uv_mutex_unlock(&hub->shadow_lock);
        }
    }

    bool needs_wakeup = true;
    if (hub->wakeup_fn != (void *)0) {
        int idx = (hub->map_fn) ? hub->map_fn(local_msg.tag_id) : (int)local_msg.tag_id;
        needs_wakeup = hub->wakeup_fn(idx);
    }

    if (needs_wakeup) {
        uv_mutex_lock(&hub->shadow_lock);
        int l_count = hub->listener_count;
        if (l_count > 16) l_count = 16;
        uv_async_t *l_copy[16];
        if (l_count > 0) {
            memcpy(l_copy, hub->listeners, sizeof(uv_async_t *) * (size_t)l_count);
        }
        uv_mutex_unlock(&hub->shadow_lock);

        for (int i = 0; i < l_count; i++) {
            if (l_copy[i]) {
                if (!uv_is_closing((uv_handle_t *)l_copy[i])) {
                    uv_async_send(l_copy[i]);
                }
            }
        }
        if (hub->legacy_cb) hub->legacy_cb(hub, hub->legacy_user_data);
        if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) uv_async_send(&hub->legacy_async);
    }
    return 0;
}
int datahub_register_ref(datahub_t *hub, uint32_t tag_id, int fd, size_t len) {
    if (!hub || fd < 0) return -1;
    if (hub->ring.hdr == NULL) return -1;

    // 1. 建立本地映射
    void *ptr = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        LOG_ERROR(LOG_CAT_BUS, "Failed to mmap ref FD %d for Tag %u: %s", fd, tag_id,
                  strerror(errno));
        return -1;
    }

    // 2. 存入注册表
    uv_mutex_lock(&hub->shadow_lock);
    if (hub->ref_count < 32) {
        hub->refs[hub->ref_count++] =
            (datahub_ref_t){.tag_id = tag_id, .fd = fd, .ptr = ptr, .len = len};
    }
    uv_mutex_unlock(&hub->shadow_lock);

    LOG_INFO(LOG_CAT_BUS, "Registered local ref for Tag %u (FD %d -> Ptr %p)", tag_id, fd, ptr);
    return 0;
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !hub->shadow_table || !out) return -1;
    int idx = hub->map_fn ? hub->map_fn(tag_id) : (int)tag_id;
    if (idx < 0 || (uint32_t)idx >= hub->shadow_capacity) return -1;

    uv_mutex_lock(&hub->shadow_lock);
    *out = hub->shadow_table[idx];

    if (out->tag_id == tag_id) {
        // Phase 4: 跨进程指针修复
        if (out->type == PIPEMSG_TYPE_REF) {
            for (int i = 0; i < hub->ref_count; i++) {
                if (hub->refs[i].tag_id == out->tag_id) {
                    out->payload.ref.ptr = hub->refs[i].ptr;
                    out->payload.ref.fd = hub->refs[i].fd;
                    break;
                }
            }
        }
    }
    uv_mutex_unlock(&hub->shadow_lock);

    if (out->tag_id != tag_id) {
        return -1;
    }

    return 0;
}

void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub) return;
    uv_mutex_lock(&hub->shadow_lock);
    hub->legacy_cb = cb;
    hub->legacy_user_data = user_data;
    uv_mutex_unlock(&hub->shadow_lock);
}

int datahub_add_listener(datahub_t *hub, uv_async_t *async_handle) {
    uv_mutex_lock(&hub->shadow_lock);
    if (hub->listener_count >= 16) {
        uv_mutex_unlock(&hub->shadow_lock);
        return -1;
    }
    hub->listeners[hub->listener_count++] = async_handle;
    uv_mutex_unlock(&hub->shadow_lock);
    return 0;
}

int datahub_remove_listener(datahub_t *hub, uv_async_t *async_handle) {
    uv_mutex_lock(&hub->shadow_lock);
    for (int i = 0; i < hub->listener_count; i++) {
        if (hub->listeners[i] == async_handle) {
            hub->listeners[i] = hub->listeners[--hub->listener_count];
            uv_mutex_unlock(&hub->shadow_lock);
            return 0;
        }
    }
    uv_mutex_unlock(&hub->shadow_lock);
    return -1;
}