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

struct snf_manager_s {
    snf_config_t config;
    bool is_online;
    uv_timer_t sync_timer;
    bool is_syncing;
    bool is_caller_allocated;
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

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PipeMsg msg = {0};
        msg.ts = (uint64_t)sqlite3_column_int64(stmt, 0);
        msg.tag_id = (uint32_t)sqlite3_column_int(stmt, 1);
        msg.payload.val.f = sqlite3_column_double(stmt, 2);
        msg.quality = (uint16_t)sqlite3_column_int(stmt, 3);
        msg.type = PIPEMSG_TYPE_SCALAR;

        LOG_DEBUG(LOG_CAT_NB, "SnF: Syncing record Tag=%u TS=%llu", msg.tag_id, (unsigned long long)msg.ts);

        // 2. Try to publish
        if (mgr->config.publish_fn(mgr->config.user_data, &msg) == 0) {
            // Success, mark as synced
            char update_sql[256];
            snprintf(update_sql, sizeof(update_sql), "UPDATE telemetry SET synced = 1 WHERE ts = %llu AND tag_id = %u;",
                     (unsigned long long)msg.ts, msg.tag_id);
            if (sqlite3_exec(db, update_sql, NULL, NULL, NULL) != SQLITE_OK) {
                LOG_ERROR(LOG_CAT_NB, "SnF: Failed to update synced status: %s", sqlite3_errmsg(db));
            }
            count++;
        } else {
            LOG_WARN(LOG_CAT_NB, "SnF: Publish failed for Tag=%u TS=%llu", msg.tag_id, (unsigned long long)msg.ts);
            break;
        }
    }

    sqlite3_finalize(stmt);
    if (count > 0) {
        LOG_INFO(LOG_CAT_NB, "SnF: Forwarded %d records to cloud.", count);
    }
    
    mgr->is_syncing = false;
}

snf_manager_t *snf_manager_create_in(snf_manager_storage_t *storage, const snf_config_t *cfg) {
    if (!storage || !cfg || !cfg->db || !cfg->loop || !cfg->publish_fn) return NULL;

    memset(storage, 0, sizeof(*storage));
    snf_manager_t *mgr = (snf_manager_t *)storage;
    mgr->is_caller_allocated = true;
    mgr->config = *cfg;
    if (mgr->config.sync_batch_size == 0) mgr->config.sync_batch_size = 100;
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
