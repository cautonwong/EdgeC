#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "datahub.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "edge_log.h"

// 兼容旧内核定义 (同 shmring.c)
#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
#ifndef MFD_ALLOW_SEALING
#define MFD_ALLOW_SEALING 0x0002U
#endif
#ifndef F_SEAL_SEAL
#define F_SEAL_SEAL 0x0001
#endif
#ifndef F_SEAL_SHRINK
#define F_SEAL_SHRINK 0x0002
#endif
#ifndef F_SEAL_GROW
#define F_SEAL_GROW 0x0004
#endif

static void _on_legacy_async(uv_async_t *handle) {
    datahub_t *hub = handle->data;
    if (hub && hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
    }
}

static int _datahub_post_init(datahub_t *hub, uv_loop_t *loop, uint32_t compact_capacity,
                              datahub_tag_map_cb map_fn) {
    hub->loop = loop;
    hub->shadow_capacity = compact_capacity;
    hub->map_fn = map_fn;

    uv_async_init(loop, &hub->legacy_async, _on_legacy_async);
    hub->legacy_async.data = hub;

    uv_mutex_init(&hub->shadow_lock);
    return 0;
}

int datahub_init(datahub_t *hub, uv_loop_t *loop, const char *name, uint32_t slots,
                 uint32_t compact_capacity, datahub_tag_map_cb map_fn) {
    memset(hub, 0, sizeof(*hub));
    hub->shadow_fd = -1;

    // 1. 初始化 RingBuffer (内部创建 memfd)
    if (shmring_create(&hub->ring, name, slots) != 0) {
        LOG_ERROR(LOG_CAT_BUS, "Failed to create shmring '%s'", name ? name : "anon");
        return -1;
    }

    // 2. 初始化 ShadowTable (创建第二个 memfd)
    if (compact_capacity > 0) {
        size_t size = compact_capacity * sizeof(pipe_msg_t);
        size = (size + 4095) & ~4095UL;  // 对齐

        const char *shm_name = name ? name : "edge_shadow";
        int fd = (int)syscall(SYS_memfd_create, shm_name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
        if (fd < 0) {
            LOG_ERROR(LOG_CAT_BUS, "Shadow memfd_create failed: %s", strerror(errno));
            shmring_close(&hub->ring);
            return -1;
        }

        if (ftruncate(fd, (off_t)size) < 0) {
            close(fd);
            shmring_close(&hub->ring);
            return -1;
        }

        // 密封 ShadowTable：禁止改变大小
        fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_SEAL);

        void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            close(fd);
            shmring_close(&hub->ring);
            return -1;
        }

        memset(ptr, 0, size);
        hub->shadow_table = (pipe_msg_t *)ptr;
        hub->shadow_fd = fd;
        hub->shadow_size = size;
    }

    return _datahub_post_init(hub, loop, compact_capacity, map_fn);
}

int datahub_init_from_fd(datahub_t *hub, uv_loop_t *loop, int ring_fd, size_t ring_size,
                         int shadow_fd, size_t shadow_size, uint32_t compact_capacity,
                         datahub_tag_map_cb map_fn) {
    memset(hub, 0, sizeof(*hub));

    // 1. 映射 RingBuffer
    if (shmring_init_from_fd(&hub->ring, ring_fd, ring_size) != 0) {
        LOG_ERROR(LOG_CAT_BUS, "Failed to init shmring from FD %d", ring_fd);
        return -1;
    }

    // 2. 映射 ShadowTable
    if (shadow_fd >= 0 && compact_capacity > 0) {
        void *ptr = mmap(NULL, shadow_size, PROT_READ | PROT_WRITE, MAP_SHARED, shadow_fd, 0);
        if (ptr == MAP_FAILED) {
            LOG_ERROR(LOG_CAT_BUS, "Shadow mmap from FD %d failed: %s", shadow_fd, strerror(errno));
            shmring_close(&hub->ring);
            return -1;
        }
        hub->shadow_table = (pipe_msg_t *)ptr;
        hub->shadow_fd = shadow_fd;  // 注意：子进程通常不关闭 FD 除非 destroy
        hub->shadow_size = shadow_size;
    }

    return _datahub_post_init(hub, loop, compact_capacity, map_fn);
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;

    if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_close((uv_handle_t *)&hub->legacy_async, NULL);
    }
    shmring_close(&hub->ring);

    if (hub->shadow_table) {
        munmap(hub->shadow_table, hub->shadow_size);
        if (hub->shadow_fd >= 0) {
            close(hub->shadow_fd);
        }
        hub->shadow_table = NULL;
    }
    uv_mutex_destroy(&hub->shadow_lock);
}

int datahub_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;

    // 1. 物理写入共享内存 RingBuffer
    shm_ring_write(&hub->ring, msg);

    // 2. 更新影子表
    if (hub->shadow_table) {
        int idx = -1;
        if (hub->map_fn) {
            idx = hub->map_fn(msg->tag_id);
        } else {
            // 回退逻辑：直接索引 (用于测试或未定义契约的情况)
            idx = (int)msg->tag_id;
        }

        if (idx >= 0 && (uint32_t)idx < hub->shadow_capacity) {
            uv_mutex_lock(&hub->shadow_lock);
            hub->shadow_table[idx] = *msg;
            uv_mutex_unlock(&hub->shadow_lock);
        }
    }

    // 3. 多路异步通知 (S8)
    for (int i = 0; i < hub->listener_count; i++) {
        if (hub->listeners[i]) {
            uv_async_send(hub->listeners[i]);
        }
    }

    // 4. 同步触发
    if (hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
    }

    if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_async_send(&hub->legacy_async);
    }

    return 0;
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !out || !hub->shadow_table) return -1;

    int idx = hub->map_fn ? hub->map_fn(tag_id) : (int)tag_id;
    if (idx < 0 || (uint32_t)idx >= hub->shadow_capacity) return -1;

    uv_mutex_lock(&hub->shadow_lock);
    *out = hub->shadow_table[idx];
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
