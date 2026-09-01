#define _GNU_SOURCE
#include "datahub_shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "edge_log.h"
#include "edge_metrics.h"

static void _shm_legacy_async_cb(uv_async_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (hub && hub->legacy_cb) {
        hub->legacy_cb(hub, hub->legacy_user_data);
    }
}

int datahub_shm_init(datahub_t *hub, const datahub_config_t *config) {
    if (!hub) return -1;
    hub->backend_type = DATAHUB_BACKEND_SHM;
    hub->ops = &g_datahub_shm_ops;

    uint32_t slots = (config && config->ring_slots > 0) ? config->ring_slots : 1024;
    uint32_t shadow_capacity = config ? config->shadow_capacity : 0;
    const char *name = (config && config->shm_name[0]) ? config->shm_name : "default";

    // 💡 [Omnis Auto-Capacity]
    uint32_t max_id = shadow_capacity;
    if (hub->metadata_table) {
        for (uint32_t i = 0; i < hub->metadata_count; i++) {
            if (hub->metadata_table[i].id > max_id) max_id = hub->metadata_table[i].id;
        }
    }
    uint32_t capacity = (max_id > shadow_capacity) ? max_id + 1 : shadow_capacity;
    if (capacity == 0) capacity = 1; // 最小分配 1 槽位，避免 mmap(0) 失败
    hub->shadow_capacity = capacity;

    int ring_fd = shmring_create(&hub->ring, name, slots);
    if (ring_fd < 0) return -1;

    // 💡 [Nexus-Go] 使用 POSIX SHM 映射影子表，支持 Golang 零拷贝挂载
    char shm_path[256];
    const char *safe_name = name;
    if (safe_name[0] == '/') safe_name++; // 跳过开头的斜杠，避免 // 路径
    
    snprintf(shm_path, sizeof(shm_path), "/edgeos_shadow_%s", safe_name);
    
    // 进一步清理路径中的非法斜杠
    for (char *p = shm_path + 1; *p; p++) {
        if (*p == '/') *p = '_';
    }
    
    hub->shadow_size = capacity * sizeof(pipe_msg_t);
    hub->shadow_fd = shm_open(shm_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (hub->shadow_fd < 0) {
        LOG_ERROR(LOG_CAT_BUS, "Failed to create SHM shadow table: %s", strerror(errno));
        shmring_destroy(&hub->ring);
        return -1;
    }

    if (ftruncate(hub->shadow_fd, (off_t)hub->shadow_size) < 0) {
        close(hub->shadow_fd);
        shm_unlink(shm_path);
        shmring_destroy(&hub->ring);
        return -1;
    }

    hub->shadow_table = mmap(NULL, hub->shadow_size, PROT_READ | PROT_WRITE, MAP_SHARED, hub->shadow_fd, 0);
    if (hub->shadow_table == MAP_FAILED) {
        close(hub->shadow_fd);
        shm_unlink(shm_path);
        shmring_destroy(&hub->ring);
        hub->shadow_table = NULL;
        return -1;
    }
    
    // 初始化影子锁与细粒度订阅锁
    uv_mutex_init(&hub->shadow_lock);
    uv_mutex_init(&hub->sub_lock);
    hub->next_sub_id = 1;
    memset(hub->subscribers, 0, sizeof(hub->subscribers));

    // [ADR-0001] 初始化 Metrics
    atomic_store_explicit(&hub->metrics_shadow_writes, 0, memory_order_relaxed);
    hub->metrics_active = 0;
    edge_metrics_register(12001, "datahub.ring.watermark",
                          "Ring buffer write_idx / slot_count ratio (0.0-1.0)");
    edge_metrics_register(12002, "datahub.ring.drop_count",
                          "Cumulative dropped message count");
    edge_metrics_register(12003, "datahub.ring.read_latency",
                          "Last shm_ring_read latency (ms)");
    edge_metrics_register(12004, "datahub.shadow.write_count",
                          "Total shadow table writes");

    if (hub->loop) {
        hub->legacy_async.data = hub;
        uv_async_init(hub->loop, &hub->legacy_async, _shm_legacy_async_cb);
    }
    return 0;
}

int datahub_shm_publish(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg || !hub->ring.hdr) return -1;
    
    atomic_fetch_add_explicit(&hub->total_publishes, 1, memory_order_relaxed);

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

int datahub_shm_write_shadow(datahub_t *hub, const pipe_msg_t *msg) {
    return datahub_shm_publish(hub, msg);
}

int datahub_shm_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (!hub || !out_msg) return -1;
    atomic_fetch_add_explicit(&hub->total_shadow_reads, 1, memory_order_relaxed);
    int idx = -1;
    if (hub->map_fn_v2) {
        idx = hub->map_fn_v2(tag_id, 0); // Default device_id to 0
    } else if (hub->map_fn) {
        idx = hub->map_fn(tag_id);
    }
    if (idx < 0) idx = (int)tag_id;
    return datahub_read_shadow_idx(hub, (uint32_t)idx, out_msg);
}

int datahub_shm_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub) return -1;
    hub->legacy_cb = cb;
    hub->legacy_user_data = user_data;
    return 0;
}

int datahub_shm_query_snapshot(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (!hub || !out_msg) return -1;
    return datahub_shm_read_shadow(hub, tag_id, out_msg);
}

int datahub_shm_get_stats(datahub_t *hub, datahub_backend_stats_t *stats) {
    if (!hub || !stats) return -1;
    stats->total_publishes = atomic_load_explicit(&hub->total_publishes, memory_order_relaxed);
    stats->total_shadow_writes = atomic_load_explicit(&hub->metrics_shadow_writes, memory_order_relaxed);
    stats->total_shadow_reads = atomic_load_explicit(&hub->total_shadow_reads, memory_order_relaxed);
    stats->network_packets_tx = atomic_load_explicit(&hub->network_packets_tx, memory_order_relaxed);
    stats->network_packets_rx = atomic_load_explicit(&hub->network_packets_rx, memory_order_relaxed);
    stats->drops = atomic_load_explicit(&hub->backend_drops, memory_order_relaxed);
    return 0;
}

void datahub_shm_destroy(datahub_t *hub) {
    if (!hub) return;
    datahub_stop_metrics(hub);
    shmring_destroy(&hub->ring);
    uv_mutex_lock(&hub->shadow_lock);
    if (hub->shadow_table) {
        munmap(hub->shadow_table, hub->shadow_size);
        hub->shadow_table = NULL;
        if (hub->shadow_fd >= 0) {
            close(hub->shadow_fd);
            hub->shadow_fd = -1;
        }
    }
    if (hub->wal_fd >= 0) {
        close(hub->wal_fd);
        hub->wal_fd = -1;
    }
    uv_mutex_unlock(&hub->shadow_lock);
    uv_mutex_destroy(&hub->shadow_lock);
    uv_mutex_destroy(&hub->sub_lock);
    if (hub->loop && !uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_close((uv_handle_t *)&hub->legacy_async, NULL);
    }
    if (hub->compaction_active && !uv_is_closing((uv_handle_t *)&hub->compaction_timer)) {
        uv_close((uv_handle_t *)&hub->compaction_timer, NULL);
    }
}

const datahub_ops_t g_datahub_shm_ops = {
    .init = datahub_shm_init,
    .publish = datahub_shm_publish,
    .write_shadow = datahub_shm_write_shadow,
    .read_shadow = datahub_shm_read_shadow,
    .subscribe = datahub_shm_subscribe,
    .query_snapshot = datahub_shm_query_snapshot,
    .get_stats = datahub_shm_get_stats,
    .destroy = datahub_shm_destroy,
};
