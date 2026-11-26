/**
 * @file snf_manager.c
 * @brief Store-and-Forward 管理器实现
 */

#include "snf_manager.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <uv.h>

struct snf_manager_s {
    snf_config_t config;
    bool is_online;
    uv_timer_t sync_timer;
    bool is_syncing;
    bool is_caller_allocated;
    uint32_t batch_count; /* records published but not yet confirmed */
    uint64_t last_wall_us; /* 本实例最近一次入库的墙钟，用于严格递增钳制 */

    /* 整批确认用的待标记主键。telemetry 是 WITHOUT ROWID 且
     * PRIMARY KEY(wall_ts_us, tag_id)，所以只能按 (wall_ts_us, tag_id) 对标记。
     * 定长内嵌，避免为核心路径引入动态内存。 */
    snf_record_t batch[SNF_MAX_BATCH];
};

_Static_assert(sizeof(struct snf_manager_s) <= sizeof(snf_manager_storage_t),
               "snf_manager_storage_t is too small for struct snf_manager_s");

static snf_manager_storage_t g_default_snf_storage;

typedef enum {
    SNF_CONFIRM_READY,
    SNF_CONFIRM_INFLIGHT,
    SNF_CONFIRM_FAILED,
} snf_confirm_state_t;

/* 跨线程确认判据（契约见 snf_manager.h）：先采样 pending，为 0 再采样 failures。
 * 传输层必须在递减 inflight 之前发布失败计数；这样 pending 读到 0 时，已经发生
 * 的失败一定在 failures 里可见。反过来采样会漏掉中间态，把失败行标成已送达。 */
static snf_confirm_state_t _sample_confirm_state(snf_manager_t *mgr) {
    uint32_t inflight =
        mgr->config.pending_fn ? mgr->config.pending_fn(mgr->config.user_data) : 0;
    if (inflight > 0) return SNF_CONFIRM_INFLIGHT;

    uint32_t failed =
        mgr->config.failures_fn ? mgr->config.failures_fn(mgr->config.user_data) : 0;
    if (failed > 0) {
        LOG_WARN(LOG_CAT_NB,
                 "SnF: %u async publish failure(s) — discard the batch and replay next cycle.",
                 failed);
        return SNF_CONFIRM_FAILED;
    }
    return SNF_CONFIRM_READY;
}

static void _on_sync_timer(uv_timer_t *handle) {
    snf_manager_t *mgr = (snf_manager_t *)handle->data;
    if (!mgr->is_online || mgr->is_syncing) return;

    mgr->is_syncing = true;
    const snf_store_port_t *store = mgr->config.store;
    if (!store || !store->read_unsynced) {
        mgr->is_syncing = false;
        return;
    }

    if (mgr->batch_count > 0) {
        switch (_sample_confirm_state(mgr)) {
        case SNF_CONFIRM_INFLIGHT:
            LOG_INFO(LOG_CAT_NB,
                     "SnF: batch still in flight — keep it, do not republish.");
            mgr->is_syncing = false;
            return;
        case SNF_CONFIRM_FAILED:
            mgr->batch_count = 0;
            mgr->is_syncing = false;
            return;
        case SNF_CONFIRM_READY:
            break;
        }
        if (store->mark_synced &&
            store->mark_synced(store->ctx, mgr->batch, mgr->batch_count) != 0) {
            LOG_ERROR(LOG_CAT_NB, "SnF: Failed to mark records synced; retry next cycle");
            mgr->is_syncing = false;
            return;
        }
        mgr->batch_count = 0;
        mgr->is_syncing = false;
        return;
    }

    int total = 0;
    if (store->count_unsynced && store->count_unsynced(store->ctx, &total) == 0 && total > 0) {
        LOG_INFO(LOG_CAT_NB, "SnF: Detected %d unsynced records.", total);
    }

    snf_record_t records[SNF_MAX_BATCH];
    uint32_t read_count = 0;
    if (store->read_unsynced(store->ctx, mgr->config.sync_batch_size, records, SNF_MAX_BATCH,
                             &read_count) != 0) {
        LOG_ERROR(LOG_CAT_NB, "SnF: Failed to read unsynced records");
        mgr->is_syncing = false;
        return;
    }

    uint32_t queued = 0;
    for (uint32_t i = 0; i < read_count; i++) {
        PipeMsg msg = {0};
        msg.ts = records[i].ts;
        msg.tag_id = records[i].tag_id;
        msg.payload.val.f = records[i].value;
        /* quality 在 PipeMsg 是 8 位；上溢会把坏质量绕回好质量，饱和到 255。 */
        msg.quality = (uint8_t)(records[i].quality > 255u ? 255u : records[i].quality);
        msg.type = PIPEMSG_TYPE_SCALAR;

        LOG_DEBUG(LOG_CAT_NB, "SnF: Syncing record Tag=%u wall_us=%llu mono_ts=%llu", msg.tag_id,
                  (unsigned long long)records[i].wall_ts_us, (unsigned long long)msg.ts);

        if (mgr->config.publish_fn(mgr->config.user_data, &msg, records[i].wall_ts_us) != 0) {
            LOG_WARN(LOG_CAT_NB, "SnF: Publish failed for Tag=%u TS=%llu", msg.tag_id,
                     (unsigned long long)msg.ts);
            break;
        }
        if (queued < SNF_MAX_BATCH) mgr->batch[queued] = records[i];
        queued++;
    }

    if (queued == 0) {
        mgr->is_syncing = false;
        return;
    }

    mgr->batch_count = (queued < SNF_MAX_BATCH) ? queued : SNF_MAX_BATCH;
    /* Re-sample after publishing: pending_fn may only become non-zero once
     * the transport has accepted the queued frames. */
    switch (_sample_confirm_state(mgr)) {
    case SNF_CONFIRM_INFLIGHT:
        LOG_INFO(LOG_CAT_NB,
                 "SnF: %u record(s) enqueued, batch still in flight — confirm after ACK.",
                 queued);
        mgr->is_syncing = false;
        return;
    case SNF_CONFIRM_FAILED:
        mgr->batch_count = 0;
        mgr->is_syncing = false;
        return;
    case SNF_CONFIRM_READY:
        break;
    }

    if (store->mark_synced &&
        store->mark_synced(store->ctx, mgr->batch, mgr->batch_count) != 0) {
        LOG_ERROR(LOG_CAT_NB, "SnF: Failed to mark records synced; retry next cycle");
        mgr->is_syncing = false;
        return;
    }
    mgr->batch_count = 0;
    mgr->is_syncing = false;
}

snf_manager_t *snf_manager_create_in(snf_manager_storage_t *storage, const snf_config_t *cfg) {
    if (!storage || !cfg || !cfg->store || !cfg->loop || !cfg->publish_fn) return NULL;
    /* 只跟踪 pending、不跟踪 failures 会把异步失败当成送达；契约要求成对提供。 */
    if (cfg->pending_fn && !cfg->failures_fn) return NULL;

    memset(storage, 0, sizeof(*storage));
    snf_manager_t *mgr = (snf_manager_t *)storage;
    mgr->is_caller_allocated = true;
    mgr->config = *cfg;
    if (mgr->config.sync_batch_size == 0) mgr->config.sync_batch_size = SNF_MAX_BATCH;
    /* 批大小受定长确认缓冲约束：超了没法标记，只会每轮重复发送。 */
    if (mgr->config.sync_batch_size > SNF_MAX_BATCH) mgr->config.sync_batch_size = SNF_MAX_BATCH;
    if (mgr->config.sync_interval_ms == 0) mgr->config.sync_interval_ms = 5000;

    if (mgr->config.store->ensure_schema &&
        mgr->config.store->ensure_schema(mgr->config.store->ctx) != 0) {
        LOG_WARN(LOG_CAT_NB, "SnF: schema ensure failed; continuing with existing schema");
    }

    uv_timer_init(cfg->loop, &mgr->sync_timer);
    mgr->sync_timer.data = mgr;
    uv_timer_start(&mgr->sync_timer, _on_sync_timer, mgr->config.sync_interval_ms, mgr->config.sync_interval_ms);

    LOG_INFO(LOG_CAT_NB, "SnF: Manager initialized.");
    return mgr;
}

snf_manager_t *snf_manager_create(const snf_config_t *cfg) {
    return snf_manager_create_in(&g_default_snf_storage, cfg);
}

void snf_manager_set_online(snf_manager_t *mgr, bool online) {
    if (!mgr) return;
    if (mgr->is_online != online) {
        mgr->is_online = online;
        LOG_INFO(LOG_CAT_NB, "SnF: State changed to %s", online ? "ONLINE" : "OFFLINE");
        if (online) {
            // Immediate sync attempt
            uv_timer_again(&mgr->sync_timer);
        }
    }
}

int snf_manager_buffer_data(snf_manager_t *mgr, const PipeMsg *msg) {
    if (!mgr || !msg) return -1;
    const snf_store_port_t *store = mgr->config.store;
    if (!store || !store->insert) return -1;

    /* 持久化边界采样一次墙钟。同一微秒的突发按 +1 钳制，保证 (wall_ts_us, tag_id)
     * 主键不互相覆盖；偏差上限为突发条数微秒。 */
    struct timespec now;
    uint64_t wall_ts_us = mgr->last_wall_us + 1;
    if (clock_gettime(CLOCK_REALTIME, &now) == 0) {
        uint64_t sampled =
            (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
        if (sampled > mgr->last_wall_us) wall_ts_us = sampled;
    }
    mgr->last_wall_us = wall_ts_us;

    return store->insert(store->ctx, msg, wall_ts_us);
}

static void _on_snf_close(uv_handle_t *handle) {
    snf_manager_t *mgr = (snf_manager_t *)handle->data;
    if (mgr && !mgr->is_caller_allocated) free(mgr);
}

void snf_manager_destroy(snf_manager_t *mgr) {
    if (!mgr) return;
    uv_timer_stop(&mgr->sync_timer);
    if (!uv_is_closing((uv_handle_t *)&mgr->sync_timer)) {
        uv_close((uv_handle_t *)&mgr->sync_timer, _on_snf_close);
    } else {
        if (!mgr->is_caller_allocated) free(mgr);
    }
}
