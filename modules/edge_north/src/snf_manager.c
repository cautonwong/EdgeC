/**
 * @file snf_manager.c
 * @brief Store-and-Forward 管理器实现
 */

#include "snf_manager.h"
#include "edge_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

struct snf_manager_s {
    snf_config_t config;
    bool is_online;
    uv_timer_t sync_timer;
    bool is_syncing;
    bool is_caller_allocated;
    uint32_t batch_count; /* records published but not yet confirmed */

    /* 整批确认用的待标记主键。telemetry 是 WITHOUT ROWID 且 PRIMARY KEY(ts, tag_id)，
     * 所以只能按 (ts, tag_id) 对标记。定长内嵌，避免为核心路径引入动态内存。 */
    snf_record_t batch[SNF_MAX_BATCH];
};

_Static_assert(sizeof(struct snf_manager_s) <= sizeof(snf_manager_storage_t),
               "snf_manager_storage_t is too small for struct snf_manager_s");

static snf_manager_storage_t g_default_snf_storage;

static void _on_sync_timer(uv_timer_t *handle) {
    snf_manager_t *mgr = (snf_manager_t *)handle->data;
    if (!mgr->is_online || mgr->is_syncing) return;

    mgr->is_syncing = true;
    const snf_store_port_t *store = mgr->config.store;
    if (!store || !store->read_unsynced) {
        mgr->is_syncing = false;
        return;
    }

    uint32_t inflight =
        mgr->config.pending_fn ? mgr->config.pending_fn(mgr->config.user_data) : 0;
    if (mgr->batch_count > 0) {
        if (inflight > 0) {
            LOG_INFO(LOG_CAT_NB,
                     "SnF: %u record(s) still in flight — keep the batch, do not republish.",
                     inflight);
            mgr->is_syncing = false;
            return;
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
        msg.quality = records[i].quality;
        msg.type = PIPEMSG_TYPE_SCALAR;

        LOG_DEBUG(LOG_CAT_NB, "SnF: Syncing record Tag=%u TS=%llu", msg.tag_id,
                  (unsigned long long)msg.ts);

        if (mgr->config.publish_fn(mgr->config.user_data, &msg) != 0) {
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
    uint32_t failed =
        mgr->config.failures_fn ? mgr->config.failures_fn(mgr->config.user_data) : 0;
    if (failed > 0) {
        LOG_WARN(LOG_CAT_NB, "SnF: %u publish(es) failed — retry the batch next cycle.", failed);
        mgr->batch_count = 0;
        mgr->is_syncing = false;
        return;
    }
    uint32_t inflight_after =
        mgr->config.pending_fn ? mgr->config.pending_fn(mgr->config.user_data) : 0;
    if (inflight_after > 0) {
        LOG_INFO(LOG_CAT_NB,
                 "SnF: %u record(s) enqueued, %u still in flight — confirm after ACK.",
                 queued, inflight_after);
        mgr->is_syncing = false;
        return;
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
    return store->insert(store->ctx, msg);
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
