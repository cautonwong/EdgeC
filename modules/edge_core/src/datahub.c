#define _GNU_SOURCE
#include "datahub.h"
#include "datahub_shm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

    datahub_config_t config;
    memset(&config, 0, sizeof(config));
    config.backend_type = DATAHUB_BACKEND_SHM;
    config.ring_slots = slots;
    config.shadow_capacity = shadow_capacity;
    if (name) {
        strncpy(config.shm_name, name, sizeof(config.shm_name) - 1);
    }

    hub->ops = &g_datahub_shm_ops;
    return hub->ops->init(hub, &config);
}

int datahub_init_with_config(datahub_t *hub, uv_loop_t *loop, const datahub_config_t *config,
                             const edge_tag_meta_t *metadata, uint32_t metadata_len,
                             struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                             datahub_wakeup_filter_cb wakeup_fn) {
    if (!hub || !config) return -1;
    memset(hub, 0, sizeof(datahub_t));
    hub->wal_fd = -1;
    hub->compaction_active = 0;
    hub->loop = loop;
    hub->io_pool = io_pool;
    hub->metadata_table = metadata;
    hub->metadata_count = metadata_len;
    hub->map_fn = map_fn;
    hub->wakeup_fn = wakeup_fn;

    switch (config->backend_type) {
        case DATAHUB_BACKEND_SHM:
            hub->ops = &g_datahub_shm_ops;
            break;
        default:
            hub->ops = &g_datahub_shm_ops;
            break;
    }

    return hub->ops->init(hub, config);
}

void datahub_destroy(datahub_t *hub) {
    if (!hub) return;
    if (hub->ops && hub->ops->destroy) {
        hub->ops->destroy(hub);
    } else {
        g_datahub_shm_ops.destroy(hub);
    }
}

int datahub_write_shadow_idx(datahub_t *hub, int idx, const pipe_msg_t *msg) {
    if (!hub || idx < 0 || idx >= (int)hub->shadow_capacity || !msg) return -1;

    // [ADR-0001] 计量 Shadow 写入
    atomic_fetch_add_explicit(&hub->metrics_shadow_writes, 1, memory_order_relaxed);

    uv_mutex_lock(&hub->shadow_lock);
    if (!hub->shadow_table) {
        uv_mutex_unlock(&hub->shadow_lock);
        return -1;
    }
    hub->shadow_table[idx] = *msg;
    
    // 💡 [WAL] Write to Write-Ahead Log if enabled
    if (hub->wal_fd >= 0) {
        // Error ignored intentionally in fast path; robust systems might log once
        if (write(hub->wal_fd, msg, sizeof(pipe_msg_t)) != sizeof(pipe_msg_t)) {
            // Write failed, but we shouldn't block memory update
        }
    }
    uv_mutex_unlock(&hub->shadow_lock);

    // --- [ARCH-DEEP-01] Fine-Grained Subscription Zero-Copy Dispatch ---
    uv_mutex_lock(&hub->sub_lock);
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) {
        if (!hub->subscribers[i].active) continue;
        datahub_subscriber_entry_t *sub = &hub->subscribers[i];
        const datahub_sub_filter_t *f = &sub->filter;

        // 1. Tag ID filter
        if (f->tag_id != 0 && msg->tag_id != f->tag_id) continue;
        if (f->tag_mask != 0 && (msg->tag_id & f->tag_mask) != (f->tag_id & f->tag_mask)) continue;

        // 2. Domain filter (support matching upper 16 bits or direct mask)
        if (f->domain_mask != 0) {
            uint32_t tag_domain = (msg->tag_id >> 16);
            if ((tag_domain & f->domain_mask) == 0 && (msg->tag_id & f->domain_mask) == 0) continue;
        }

        // 3. Device ID filter
        if (f->device_id != EDGE_DEVICE_ANY && msg->device_id != f->device_id) continue;

        // 4. Deadband / Report-by-Exception (RBE)
        if (f->notify_on_change_only && sub->has_last_msg) {
            bool changed = false;
            if (msg->type == PIPEMSG_TYPE_SCALAR) {
                if (fabs(msg->payload.val.f - sub->last_notified_msg.payload.val.f) > f->deadband ||
                    (f->deadband == 0.0 && msg->payload.val.i != sub->last_notified_msg.payload.val.i)) {
                    changed = true;
                }
            } else if (msg->type == PIPEMSG_TYPE_EVENT) {
                if (msg->payload.event.event_id != sub->last_notified_msg.payload.event.event_id ||
                    msg->payload.event.param != sub->last_notified_msg.payload.event.param) {
                    changed = true;
                }
            } else {
                if (memcmp(&msg->payload, &sub->last_notified_msg.payload, sizeof(msg->payload)) != 0) {
                    changed = true;
                }
            }
            if (!changed) continue;
        }

        // 5. Direct typed zero-copy callback invocation
        if (sub->callback) {
            sub->callback(hub, msg, sub->user_ctx);
        }
        sub->last_notified_msg = *msg;
        sub->has_last_msg = true;
    }
    uv_mutex_unlock(&hub->sub_lock);
    
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
    if (!hub || !msg) return -1;
    if (hub->ops && hub->ops->publish) {
        return hub->ops->publish(hub, msg);
    }
    return g_datahub_shm_ops.publish(hub, msg);
}

int datahub_write_shadow(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    if (hub->ops && hub->ops->write_shadow) {
        return hub->ops->write_shadow(hub, msg);
    }
    return g_datahub_shm_ops.write_shadow(hub, msg);
}

int datahub_read_shadow(datahub_t *hub, uint32_t tag_id, pipe_msg_t *out) {
    if (!hub || !out) return -1;
    if (hub->ops && hub->ops->read_shadow) {
        return hub->ops->read_shadow(hub, tag_id, out);
    }
    return g_datahub_shm_ops.read_shadow(hub, tag_id, out);
}

int datahub_read_shadow_idx(datahub_t *hub, uint32_t idx, pipe_msg_t *out) {
    if (!hub || !out || idx >= hub->shadow_capacity) return -1;
    
    uv_mutex_lock(&hub->shadow_lock);
    if (!hub->shadow_table) {
        uv_mutex_unlock(&hub->shadow_lock);
        return -1;
    }
    *out = hub->shadow_table[idx];
    uv_mutex_unlock(&hub->shadow_lock);
    return 0;
}

void datahub_subscribe(datahub_t *hub, datahub_on_data_cb cb, void *user_data) {
    if (!hub) return;
    if (hub->ops && hub->ops->subscribe) {
        hub->ops->subscribe(hub, cb, user_data);
    } else {
        g_datahub_shm_ops.subscribe(hub, cb, user_data);
    }
}

int datahub_subscribe_filtered(datahub_t *hub, const datahub_sub_filter_t *filter,
                               datahub_event_cb cb, void *user_ctx) {
    if (!hub || !cb) return -1;

    uv_mutex_lock(&hub->sub_lock);
    int slot = -1;
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) {
        if (!hub->subscribers[i].active) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        uv_mutex_unlock(&hub->sub_lock);
        return -1; // Max subscribers reached
    }

    int sub_id = hub->next_sub_id++;
    if (hub->next_sub_id <= 0) hub->next_sub_id = 1; // Overflow wrap protection

    datahub_subscriber_entry_t *sub = &hub->subscribers[slot];
    memset(sub, 0, sizeof(datahub_subscriber_entry_t));
    sub->sub_id = sub_id;
    sub->active = true;
    if (filter) {
        sub->filter = *filter;
    } else {
        sub->filter.device_id = EDGE_DEVICE_ANY;
    }
    sub->callback = cb;
    sub->user_ctx = user_ctx;
    sub->has_last_msg = false;

    uv_mutex_unlock(&hub->sub_lock);
    return sub_id;
}

int datahub_subscribe_tag(datahub_t *hub, uint32_t tag_id, datahub_event_cb cb, void *user_ctx) {
    datahub_sub_filter_t filter = {
        .tag_id = tag_id,
        .tag_mask = 0,
        .domain_mask = 0,
        .device_id = EDGE_DEVICE_ANY,
        .notify_on_change_only = false,
        .deadband = 0.0,
    };
    return datahub_subscribe_filtered(hub, &filter, cb, user_ctx);
}

int datahub_subscribe_domain(datahub_t *hub, uint32_t domain_mask, datahub_event_cb cb, void *user_ctx) {
    datahub_sub_filter_t filter = {
        .tag_id = 0,
        .tag_mask = 0,
        .domain_mask = domain_mask,
        .device_id = EDGE_DEVICE_ANY,
        .notify_on_change_only = false,
        .deadband = 0.0,
    };
    return datahub_subscribe_filtered(hub, &filter, cb, user_ctx);
}

int datahub_unsubscribe(datahub_t *hub, int sub_id) {
    if (!hub || sub_id <= 0) return -1;

    uv_mutex_lock(&hub->sub_lock);
    for (int i = 0; i < DATAHUB_MAX_SUBSCRIBERS; i++) {
        if (hub->subscribers[i].active && hub->subscribers[i].sub_id == sub_id) {
            hub->subscribers[i].active = false;
            memset(&hub->subscribers[i], 0, sizeof(datahub_subscriber_entry_t));
            uv_mutex_unlock(&hub->sub_lock);
            return 0;
        }
    }
    uv_mutex_unlock(&hub->sub_lock);
    return -1; // Not found
}


int datahub_get_stats(datahub_t *hub, datahub_backend_stats_t *stats) {
    if (!hub || !stats) return -1;
    if (hub->ops && hub->ops->get_stats) {
        return hub->ops->get_stats(hub, stats);
    }
    return g_datahub_shm_ops.get_stats(hub, stats);
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
    if (!hub || !path) return -1;

    uv_mutex_lock(&hub->shadow_lock);
    if (!hub->shadow_table || hub->shadow_capacity == 0) {
        uv_mutex_unlock(&hub->shadow_lock);
        return -1;
    }

    uint32_t capacity = hub->shadow_capacity;
    size_t bytes = capacity * sizeof(pipe_msg_t);
    pipe_msg_t *buf = malloc(bytes);
    if (!buf) {
        uv_mutex_unlock(&hub->shadow_lock);
        return -1;
    }

    memcpy(buf, hub->shadow_table, bytes);
    uv_mutex_unlock(&hub->shadow_lock);

    char tmp_path[512];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *fp = fopen(tmp_path, "wb");
    if (!fp) {
        free(buf);
        return -1;
    }

    size_t written = fwrite(buf, sizeof(pipe_msg_t), capacity, fp);
    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);
    free(buf);

    if (written != capacity) {
        unlink(tmp_path);
        return -1;
    }

    if (rename(tmp_path, path) != 0) {
        unlink(tmp_path);
        return -1;
    }

    return 0;
}

int datahub_load_snapshot(datahub_t *hub, const char *path) {
    if (!hub || !path) return -1;
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    uv_mutex_lock(&hub->shadow_lock);
    if (!hub->shadow_table) {
        uv_mutex_unlock(&hub->shadow_lock);
        fclose(fp);
        return -1;
    }
    size_t nread = fread(hub->shadow_table, sizeof(pipe_msg_t), hub->shadow_capacity, fp);
    uv_mutex_unlock(&hub->shadow_lock);
    fclose(fp);
    return (nread == hub->shadow_capacity) ? 0 : -1;
}

int datahub_enable_wal(datahub_t *hub, const char *path) {
    if (!hub || !path) return -1;
    uv_mutex_lock(&hub->shadow_lock);
    if (hub->wal_fd >= 0) {
        close(hub->wal_fd);
        hub->wal_fd = -1;
    }
    strncpy(hub->wal_path, path, sizeof(hub->wal_path) - 1);
    hub->wal_path[sizeof(hub->wal_path) - 1] = '\0';
    
    // Open for append, create if not exist
    hub->wal_fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (hub->wal_fd < 0) {
        uv_mutex_unlock(&hub->shadow_lock);
        printf("[DataHub] Failed to open WAL file: %s\n", path);
        return -1;
    }
    uv_mutex_unlock(&hub->shadow_lock);
    printf("[DataHub] WAL enabled: %s\n", path);
    return 0;
}

int datahub_replay_wal(datahub_t *hub) {
    if (!hub || hub->wal_path[0] == '\0') return -1;
    
    uv_mutex_lock(&hub->shadow_lock);
    int fd = open(hub->wal_path, O_RDONLY);
    if (fd < 0) {
        uv_mutex_unlock(&hub->shadow_lock);
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
        
        if (idx >= 0 && idx < (int)hub->shadow_capacity && hub->shadow_table) {
            hub->shadow_table[idx] = msg;
            replayed_count++;
        }
    }
    
    close(fd);
    uv_mutex_unlock(&hub->shadow_lock);
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
    uv_mutex_lock(&hub->shadow_lock);
    if (hub->wal_fd >= 0 && hub->wal_path[0] != '\0') {
        close(hub->wal_fd);
        // O_TRUNC will wipe the file, rolling it over safely since we have the snapshot
        hub->wal_fd = open(hub->wal_path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    }
    uv_mutex_unlock(&hub->shadow_lock);
    printf("[DataHub] WAL compacted and truncated.\n");
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

int datahub_read_ring(datahub_t *hub, uint64_t *inout_seq, pipe_msg_t *out_msg) {
    if (!hub || !inout_seq || !out_msg) return -1;
    return shm_ring_read(&hub->ring, inout_seq, out_msg);
}

int datahub_write_ring(datahub_t *hub, const pipe_msg_t *msg) {
    if (!hub || !msg) return -1;
    shm_ring_write(&hub->ring, msg);
    return 0;
}

int datahub_get_ring_fd(datahub_t *hub) {
    if (!hub) return -1;
    return shmring_get_fd(&hub->ring);
}

uint64_t datahub_get_ring_total_writes(const datahub_t *hub) {
    if (!hub || !hub->ring.hdr) return 0;
    return hub->ring.hdr->total_writes;
}

uint64_t datahub_get_ring_seq(const datahub_t *hub) {
    if (!hub || !hub->ring.hdr) return 0;
    return (uint64_t)atomic_load_explicit(&hub->ring.hdr->seq, memory_order_acquire);
}

int datahub_bind_io_pool(datahub_t *hub, struct net_buf_pool *pool) {
    if (!hub) return -1;
    hub->io_pool = pool;
    return 0;
}

struct net_buf_pool *datahub_get_io_pool(const datahub_t *hub) {
    if (!hub) return NULL;
    return hub->io_pool;
}

uint32_t datahub_get_listener_count(const datahub_t *hub) {
    if (!hub) return 0;
    int count = atomic_load(&hub->listener_count);
    return count < 0 ? 0 : (uint32_t)count;
}



