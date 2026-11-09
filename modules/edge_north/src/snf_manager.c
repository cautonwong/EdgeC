/**
 * @file snf_manager.c
 * @brief Store-and-Forward 管理器实现
 */

#include "snf_manager.h"
#include "edge_log.h"
#include <sqlite3.h>
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

    /* 整批确认用的待标记主键。telemetry 是 WITHOUT ROWID 且 PRIMARY KEY(ts, tag_id)，
     * 所以只能按 (ts, tag_id) 对标记。定长内嵌，避免为核心路径引入动态内存。 */
    struct {
        uint64_t ts;
        uint32_t tag_id;
    } batch[SNF_MAX_BATCH];
};

_Static_assert(sizeof(struct snf_manager_s) <= sizeof(snf_manager_storage_t),
               "snf_manager_storage_t is too small for struct snf_manager_s");

static snf_manager_storage_t g_default_snf_storage;

static void _on_sync_timer(uv_timer_t *handle) {
    snf_manager_t *mgr = (snf_manager_t *)handle->data;
    if (!mgr->is_online || mgr->is_syncing) return;

    mgr->is_syncing = true;
    
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(mgr->config.db);
    if (!db) {
        mgr->is_syncing = false;
        return;
    }

    // [Nexus] Count unsynced records for diagnostic visibility
    sqlite3_stmt *count_stmt;
    if (sqlite3_prepare_v2(db, "SELECT count(*) FROM telemetry WHERE synced = 0;", -1, &count_stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(count_stmt) == SQLITE_ROW) {
            int total = sqlite3_column_int(count_stmt, 0);
            if (total > 0) LOG_INFO(LOG_CAT_NB, "SnF: Detected %d unsynced records.", total);
        }
        sqlite3_finalize(count_stmt);
    }

    // 1. Fetch a batch of unsynced data
    sqlite3_stmt *stmt;
    const char *query = "SELECT ts, tag_id, val, q FROM telemetry WHERE synced = 0 ORDER BY ts ASC LIMIT ?;";
    
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_NB, "SnF: Failed to prepare sync query: %s", sqlite3_errmsg(db));
        mgr->is_syncing = false;
        return;
    }

    sqlite3_bind_int(stmt, 1, (int)mgr->config.sync_batch_size);

    uint32_t queued = 0; /* 成功入队的条数（不是"已送达"） */
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PipeMsg msg = {0};
        msg.ts = (uint64_t)sqlite3_column_int64(stmt, 0);
        msg.tag_id = (uint32_t)sqlite3_column_int(stmt, 1);
        msg.payload.val.f = sqlite3_column_double(stmt, 2);
        msg.quality = (uint16_t)sqlite3_column_int(stmt, 3);
        msg.type = PIPEMSG_TYPE_SCALAR;

        LOG_DEBUG(LOG_CAT_NB, "SnF: Syncing record Tag=%u TS=%llu", msg.tag_id, (unsigned long long)msg.ts);

        // 2. Try to enqueue. 返回 0 只代表"交给传输了"，**不代表对端收到**。
        if (mgr->config.publish_fn(mgr->config.user_data, &msg) != 0) {
            LOG_WARN(LOG_CAT_NB, "SnF: Publish failed for Tag=%u TS=%llu", msg.tag_id, (unsigned long long)msg.ts);
            break;
        }
        if (queued < SNF_MAX_BATCH) {
            mgr->batch[queued].ts = msg.ts;
            mgr->batch[queued].tag_id = msg.tag_id;
        }
        queued++;
    }

    sqlite3_finalize(stmt);

    if (queued == 0) {
        mgr->is_syncing = false;
        return;
    }

    // 3. 整批确认：只有"没有在途消息"才认为这一批真的送达。
    //    否则不标 synced，下一轮重发 —— at-least-once，允许重复，不允许丢。
    uint32_t inflight = mgr->config.pending_fn ? mgr->config.pending_fn(mgr->config.user_data) : 0;
    if (inflight > 0) {
        LOG_INFO(LOG_CAT_NB,
                 "SnF: %u record(s) enqueued, %u still in flight — will confirm next cycle.",
                 queued, inflight);
        mgr->is_syncing = false;
        return;
    }

    // 4. 标记已确认。按 (ts, tag_id) 主键对更新，不依赖 rowid（表是 WITHOUT ROWID）。
    sqlite3_stmt *upd;
    if (sqlite3_prepare_v2(db, "UPDATE telemetry SET synced = 1 WHERE ts = ? AND tag_id = ?;", -1,
                           &upd, NULL) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_NB, "SnF: Failed to prepare confirm statement: %s", sqlite3_errmsg(db));
        mgr->is_syncing = false;
        return;
    }

    uint32_t confirmed = 0;
    uint32_t limit = (queued < SNF_MAX_BATCH) ? queued : SNF_MAX_BATCH;
    for (uint32_t i = 0; i < limit; i++) {
        sqlite3_bind_int64(upd, 1, (sqlite3_int64)mgr->batch[i].ts);
        sqlite3_bind_int(upd, 2, (int)mgr->batch[i].tag_id);
        if (sqlite3_step(upd) == SQLITE_DONE) confirmed++;
        sqlite3_reset(upd);
    }
    sqlite3_finalize(upd);

    if (confirmed > 0) {
        LOG_INFO(LOG_CAT_NB, "SnF: Confirmed %u record(s) delivered.", confirmed);
    }
    if (queued > limit) {
        /* 超出定长缓冲的部分本轮无法确认，留到下一轮；不能静默丢掉标记。 */
        LOG_WARN(LOG_CAT_NB,
                 "SnF: %u record(s) enqueued but only %u confirmable this cycle (SNF_MAX_BATCH).",
                 queued, limit);
    }

    mgr->is_syncing = false;
}

snf_manager_t *snf_manager_create_in(snf_manager_storage_t *storage, const snf_config_t *cfg) {
    if (!storage || !cfg || !cfg->db || !cfg->loop || !cfg->publish_fn) return NULL;

    memset(storage, 0, sizeof(*storage));
    snf_manager_t *mgr = (snf_manager_t *)storage;
    mgr->is_caller_allocated = true;
    mgr->config = *cfg;
    if (mgr->config.sync_batch_size == 0) mgr->config.sync_batch_size = SNF_MAX_BATCH;
    /* 批大小受定长确认缓冲约束：超了没法标记，只会每轮重复发送。 */
    if (mgr->config.sync_batch_size > SNF_MAX_BATCH) mgr->config.sync_batch_size = SNF_MAX_BATCH;
    if (mgr->config.sync_interval_ms == 0) mgr->config.sync_interval_ms = 5000;

    // Ensure 'synced' column exists (migration)
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(mgr->config.db);
    // Check if column exists
    sqlite3_stmt *check_stmt;
    bool needs_alter = true;
    if (sqlite3_prepare_v2(db, "PRAGMA table_info(telemetry);", -1, &check_stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(check_stmt) == SQLITE_ROW) {
            const char *col_name = (const char *)sqlite3_column_text(check_stmt, 1);
            if (col_name && strcmp(col_name, "synced") == 0) {
                needs_alter = false;
                break;
            }
        }
        sqlite3_finalize(check_stmt);
    }
    
    if (needs_alter) {
        if (sqlite3_exec(db, "ALTER TABLE telemetry ADD COLUMN synced INT DEFAULT 0;", NULL, NULL, NULL) != SQLITE_OK) {
            LOG_ERROR(LOG_CAT_NB, "SnF: Failed to ALTER TABLE telemetry: %s", sqlite3_errmsg(db));
        } else {
            LOG_INFO(LOG_CAT_NB, "SnF: Successfully added 'synced' column to telemetry table.");
        }
    }
    
    // Standardize index for performance
    sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_telemetry_synced ON telemetry(synced) WHERE synced = 0;", NULL, NULL, NULL);

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
    // Actually, we just use the existing sqlite_svc_insert
    return sqlite_svc_insert(mgr->config.db, msg);
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
