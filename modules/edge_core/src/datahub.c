#define _GNU_SOURCE
#include "datahub.h"
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
    hub->compaction_active = 0;
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
    if (capacity == 0) capacity = 1; // 最小分配 1 槽位，避免 mmap(0) 失败
    hub->shadow_capacity = capacity;

    int ring_fd = shmring_create(&hub->ring, name, slots);
    if (ring_fd < 0) return -1;

    // 💡 [Nexus-Go] 使用 POSIX SHM 映射影子表，支持 Golang 零拷贝挂载
    char shm_path[256];
    const char *safe_name = name ? name : "default";
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
        return -1;
    }

    if (ftruncate(hub->shadow_fd, (off_t)hub->shadow_size) < 0) {
        close(hub->shadow_fd);
        shm_unlink(shm_path);
        return -1;
    }

    hub->shadow_table = mmap(NULL, hub->shadow_size, PROT_READ | PROT_WRITE, MAP_SHARED, hub->shadow_fd, 0);
    if (hub->shadow_table == MAP_FAILED) {
        close(hub->shadow_fd);
        shm_unlink(shm_path);
        hub->shadow_table = NULL;
        return -1;
    }
    
    // 初始化影子锁
    uv_mutex_init(&hub->shadow_lock);

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

    hub->legacy_async.data = hub;
    uv_async_init(loop, &hub->legacy_async, _legacy_async_cb);
    return 0;
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;
    datahub_stop_metrics(hub);
    shmring_destroy(&hub->ring);
    if (hub->shadow_table) {
        munmap(hub->shadow_table, hub->shadow_size);
        if (hub->shadow_fd >= 0) {
            close(hub->shadow_fd);
        }
    }
    uv_mutex_destroy(&hub->shadow_lock);
    if (!uv_is_closing((uv_handle_t *)&hub->legacy_async)) {
        uv_close((uv_handle_t *)&hub->legacy_async, NULL);
    }
    if (hub->wal_fd >= 0) {
        close(hub->wal_fd);
        hub->wal_fd = -1;
    }
    if (hub->compaction_active && !uv_is_closing((uv_handle_t *)&hub->compaction_timer)) {
        uv_close((uv_handle_t *)&hub->compaction_timer, NULL);
    }
}

int datahub_write_shadow_idx(datahub_t *hub, int idx, const pipe_msg_t *msg) {
    if (!hub || idx < 0 || idx >= (int)hub->shadow_capacity || !msg || !hub->shadow_table) return -1;

    // [ADR-0001] 计量 Shadow 写入
    atomic_fetch_add_explicit(&hub->metrics_shadow_writes, 1, memory_order_relaxed);

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

int datahub_write_shadow(datahub_t *hub, const pipe_msg_t *msg) {
    return datahub_publish(hub, msg);
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

int datahub_compact_now(datahub_t *hub) {
    if (!hub || hub->snapshot_path[0] == '\0') return -1;
    
    // 1. Save memory table to full snapshot
    if (datahub_save_snapshot(hub, hub->snapshot_path) != 0) {
        printf("[DataHub] Compaction failed to save snapshot.\n");
        return -1;
    }
    
    // 2. Truncate WAL file
    if (hub->wal_fd >= 0 && hub->wal_path[0] != '\0') {
        uv_mutex_lock(&hub->shadow_lock);
        close(hub->wal_fd);
        // O_TRUNC will wipe the file, rolling it over safely since we have the snapshot
        hub->wal_fd = open(hub->wal_path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        uv_mutex_unlock(&hub->shadow_lock);
        printf("[DataHub] WAL compacted and truncated.\n");
    }
    return 0;
}

static void _compaction_timer_cb(uv_timer_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    datahub_compact_now(hub);
}

int datahub_start_compaction(datahub_t *hub, uint64_t interval_ms, const char *snapshot_path) {
    if (!hub || !snapshot_path || !hub->loop) return -1;
    
    strncpy(hub->snapshot_path, snapshot_path, sizeof(hub->snapshot_path) - 1);
    
    uv_timer_init(hub->loop, &hub->compaction_timer);
    hub->compaction_timer.data = hub;
    hub->compaction_active = 1;
    
    uv_timer_start(&hub->compaction_timer, _compaction_timer_cb, interval_ms, interval_ms);
    printf("[DataHub] Compaction scheduled every %llu ms (Snapshot Path: %s)\n", 
           (unsigned long long)interval_ms, snapshot_path);
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
    
    if (shadow_fd >= 0) {
        hub->shadow_table = mmap(NULL, shadow_size, PROT_READ | PROT_WRITE, MAP_SHARED, shadow_fd, 0);
        if (hub->shadow_table == MAP_FAILED) {
            hub->shadow_table = NULL;
            return -1;
        }
    } else {
        hub->shadow_table = calloc(compact_capacity, sizeof(pipe_msg_t));
        if (!hub->shadow_table) return -1;
    }

    uv_mutex_init(&hub->shadow_lock);
    hub->legacy_async.data = hub;
    uv_async_init(loop, &hub->legacy_async, _legacy_async_cb);
    return 0;
}

// --- [ADR-0001] DataHub Metrics Timer ---

static void _metrics_timer_cb(uv_timer_t *handle) {
    datahub_t *hub = (datahub_t *)handle->data;
    if (!hub || !hub->ring.hdr) return;

    ShmRingHeader *hdr = hub->ring.hdr;
    uint32_t w_idx = atomic_load_explicit(&hdr->write_idx, memory_order_relaxed);
    uint32_t slots = hdr->slot_count;
    double watermark = (slots > 0) ? (double)(w_idx % slots) / (double)slots : 0.0;
    double drops = (double)atomic_load_explicit(&hdr->drop_count, memory_order_relaxed);
    double shadow_writes = (double)atomic_load_explicit(&hub->metrics_shadow_writes,
                                                        memory_order_relaxed);

    edge_metrics_report(hub, 12001, watermark, 0);
    edge_metrics_report(hub, 12002, drops, 0);
    edge_metrics_report(hub, 12004, shadow_writes, 0);
}

int datahub_start_metrics(datahub_t *hub) {
    if (!hub || !hub->loop) return -1;
    if (hub->metrics_active) return 0;

    uv_timer_init(hub->loop, &hub->metrics_timer);
    hub->metrics_timer.data = hub;
    hub->metrics_active = 1;
    uv_timer_start(&hub->metrics_timer, _metrics_timer_cb, 100, 100);
    return 0;
}

void datahub_stop_metrics(datahub_t *hub) {
    if (!hub || !hub->metrics_active) return;
    uv_timer_stop(&hub->metrics_timer);
    if (!uv_is_closing((uv_handle_t *)&hub->metrics_timer)) {
        uv_close((uv_handle_t *)&hub->metrics_timer, NULL);
    }
    hub->metrics_active = 0;
}

int datahub_allocate_chunk(datahub_t *hub, uint32_t additional_capacity, datahub_chunk_t *out_chunk) {
    if (!hub || additional_capacity == 0 || !out_chunk) return -1;
    if (hub->chunk_count >= 16) return -1;

    size_t chunk_size = (additional_capacity * sizeof(pipe_msg_t) + 4095) & ~4095UL;
    char shm_name[128];
    snprintf(shm_name, sizeof(shm_name), "/edgeos_chunk_%u_%d", hub->chunk_count, (int)getpid());

    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd < 0) return -1;
    shm_unlink(shm_name);

    if (ftruncate(fd, (off_t)chunk_size) != 0) {
        close(fd);
        return -1;
    }

    pipe_msg_t *ptr = mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        close(fd);
        return -1;
    }

    memset(ptr, 0, chunk_size);
    out_chunk->chunk_id = hub->chunk_count;
    out_chunk->fd = fd;
    out_chunk->ptr = ptr;
    out_chunk->size = chunk_size;
    out_chunk->capacity = additional_capacity;

    hub->chunks[hub->chunk_count] = *out_chunk;
    hub->chunk_count++;
    return 0;
}

int datahub_switch_schema(datahub_t *hub, schema_version_t *new_schema) {
    if (!hub || !new_schema) return -1;
    atomic_store_explicit(&hub->active_schema, (uintptr_t)new_schema, memory_order_release);
    LOG_INFO(LOG_CAT_CORE, "[DataHub] Dynamic Schema CAS hot-switched to version v%u (Tag count: %u)",
             new_schema->version, new_schema->tag_count);
    return 0;
}

int datahub_devicetwin_reconcile(datahub_t *hub, uint32_t desired_tag_id, uint32_t reported_tag_id, double tolerance) {
    if (!hub) return -1;
    pipe_msg_t msg_desired, msg_reported;
    if (datahub_read_shadow(hub, desired_tag_id, &msg_desired) != 0) return -1;
    if (datahub_read_shadow(hub, reported_tag_id, &msg_reported) != 0) return -1;
    if (msg_desired.type != PIPEMSG_TYPE_SCALAR || msg_reported.type != PIPEMSG_TYPE_SCALAR) return -1;

    double diff = msg_desired.payload.val.f - msg_reported.payload.val.f;
    if (diff < 0) diff = -diff;
    return (diff <= tolerance) ? 0 : 1;
}

int datahub_query_snapshot(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (!hub || !out_msg) return -1;
    return datahub_read_shadow(hub, tag_id, out_msg);
}


