/**
 * @file sqlite_storage_driver.c
 * @brief SQLite implementation of abstract storage_ops_t interface
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 * 
 * [AEP-202] Port / Adapter Architecture:
 * Implements storage_ops_t VTable wrapping the battle-tested sqlite_svc_t engine.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "storage_ops.h"
#include "sqlite_service.h"
#include "shmring.h"

/* ---------------- Storage Ops Implementation for SQLite ---------------- */

static int _sqlite_driver_init(storage_driver_t *drv, uv_loop_t *loop, const storage_config_t *cfg) {
    if (!drv || !cfg) return -1;

    sqlite_config_t sql_cfg = {
        .strategy = STRATEGY_SERIES,
        .db_path = cfg->db_path,
        .batch_size = (int)cfg->batch_size,
        .flush_interval_ms = (int)cfg->flush_interval_ms,
        .cache_size_kb = 4096,
        .max_db_size_mb = cfg->max_db_size_mb ? cfg->max_db_size_mb : 100,
    };

    sqlite_svc_t *svc = sqlite_svc_create(loop, &sql_cfg);
    if (!svc) return -1;

    drv->priv_ctx = svc;
    strncpy(drv->name, "sqlite-wal", sizeof(drv->name) - 1);
    return 0;
}

static void _sqlite_driver_destroy(storage_driver_t *drv) {
    if (!drv || !drv->priv_ctx) return;
    sqlite_svc_destroy((sqlite_svc_t *)drv->priv_ctx);
    drv->priv_ctx = NULL;
}

static int _sqlite_driver_insert(storage_driver_t *drv, const void *msg_ptr) {
    if (!drv || !drv->priv_ctx || !msg_ptr) return -1;
    return sqlite_svc_insert((sqlite_svc_t *)drv->priv_ctx, (const PipeMsg *)msg_ptr);
}

typedef struct {
    storage_query_cb cb;
    void *user_data;
} query_bridge_ctx_t;

static int _query_bridge_cb(void *user_data, PipeMsg *msg) {
    query_bridge_ctx_t *ctx = (query_bridge_ctx_t *)user_data;
    if (ctx && ctx->cb) {
        return ctx->cb(ctx->user_data, msg);
    }
    return 0;
}

static int _sqlite_driver_query(storage_driver_t *drv, uint64_t start_ts, uint64_t end_ts, 
                                storage_query_cb cb, void *user_data) {
    if (!drv || !drv->priv_ctx) return -1;
    query_bridge_ctx_t bridge = { .cb = cb, .user_data = user_data };
    return sqlite_svc_query((sqlite_svc_t *)drv->priv_ctx, start_ts, end_ts, _query_bridge_cb, &bridge);
}

static int _sqlite_driver_begin_trans(storage_driver_t *drv) {
    if (!drv || !drv->priv_ctx) return -1;
    return sqlite_svc_begin_trans((sqlite_svc_t *)drv->priv_ctx);
}

static int _sqlite_driver_commit_trans(storage_driver_t *drv) {
    if (!drv || !drv->priv_ctx) return -1;
    return sqlite_svc_commit_trans((sqlite_svc_t *)drv->priv_ctx);
}

static int _sqlite_driver_rollback_trans(storage_driver_t *drv) {
    if (!drv || !drv->priv_ctx) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;
    return (sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL) == SQLITE_OK) ? 0 : -1;
}

static int _sqlite_driver_exec(storage_driver_t *drv, const char *stmt, storage_exec_cb cb, void *user_data) {
    if (!drv || !drv->priv_ctx || !stmt) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, stmt, cb, user_data, &err_msg);
    if (err_msg) {
        sqlite3_free(err_msg);
    }
    return (rc == SQLITE_OK) ? 0 : -1;
}

static int _sqlite_driver_checkpoint(storage_driver_t *drv, int mode) {
    if (!drv || !drv->priv_ctx) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;
    int eMode = (mode > 0) ? mode : SQLITE_CHECKPOINT_PASSIVE;
    int pnLog = 0, pnCkpt = 0;
    return (sqlite3_wal_checkpoint_v2(db, NULL, eMode, &pnLog, &pnCkpt) == SQLITE_OK) ? 0 : -1;
}

static int _sqlite_driver_flush(storage_driver_t *drv) {
    if (!drv || !drv->priv_ctx) return -1;
    return _sqlite_driver_checkpoint(drv, SQLITE_CHECKPOINT_PASSIVE);
}

static int _sqlite_driver_compact(storage_driver_t *drv) {
    if (!drv || !drv->priv_ctx) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;
    return (sqlite3_exec(db, "VACUUM;", NULL, NULL, NULL) == SQLITE_OK) ? 0 : -1;
}

#include <sys/stat.h>

static int _sqlite_driver_get_wal_size(storage_driver_t *drv, uint64_t *out_bytes) {
    if (!drv || !drv->priv_ctx || !out_bytes) return -1;
    *out_bytes = 0;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;

    const char *db_filename = sqlite3_db_filename(db, "main");
    if (db_filename && strlen(db_filename) > 0 && strcmp(db_filename, ":memory:") != 0) {
        char wal_path[512];
        snprintf(wal_path, sizeof(wal_path), "%s-wal", db_filename);
        struct stat st;
        if (stat(wal_path, &st) == 0 && st.st_size > 0) {
            *out_bytes = (uint64_t)st.st_size;
            return 0;
        }
    }

    int pnLog = 0, pnCkpt = 0;
    if (sqlite3_wal_checkpoint_v2(db, NULL, SQLITE_CHECKPOINT_PASSIVE, &pnLog, &pnCkpt) == SQLITE_OK) {
        *out_bytes = (uint64_t)pnLog * 4096;
        return 0;
    }
    return 0;
}

static int _sqlite_driver_partition_gc(storage_driver_t *drv, uint64_t cutoff_ts) {
    if (!drv || !drv->priv_ctx) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;
    char sql[128];
    snprintf(sql, sizeof(sql), "DELETE FROM tag_history WHERE timestamp < %llu;", (unsigned long long)cutoff_ts);
    return (sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK) ? 0 : -1;
}

static int _sqlite_driver_purge_old_data(storage_driver_t *drv, uint32_t max_size_mb) {
    if (!drv || !drv->priv_ctx) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;

    sqlite3_stmt *stmt = NULL;
    int total_rows = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tag_history;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            total_rows = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    if (total_rows > 0) {
        int purge_count = (max_size_mb > 0 && total_rows > 2) ? (total_rows / 2) : 1;
        char purge_sql[128];
        snprintf(purge_sql, sizeof(purge_sql),
                 "DELETE FROM tag_history WHERE id IN (SELECT id FROM tag_history ORDER BY timestamp ASC LIMIT %d);",
                 purge_count);
        sqlite3_exec(db, purge_sql, NULL, NULL, NULL);
    }
    return _sqlite_driver_compact(drv);
}

static int _sqlite_driver_snf_enqueue(storage_driver_t *drv, const void *msg_ptr) {
    return _sqlite_driver_insert(drv, msg_ptr);
}

static int _sqlite_driver_snf_dequeue(storage_driver_t *drv, void *out_msg_ptr) {
    if (!drv || !drv->priv_ctx || !out_msg_ptr) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;

    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, timestamp, tag_id, value_f, quality, type FROM tag_history WHERE status = 0 ORDER BY timestamp ASC LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t row_id = sqlite3_column_int64(stmt, 0);
        PipeMsg *msg = (PipeMsg *)out_msg_ptr;
        memset(msg, 0, sizeof(PipeMsg));
        msg->ts = (uint64_t)sqlite3_column_int64(stmt, 1);
        msg->tag_id = (uint32_t)sqlite3_column_int64(stmt, 2);
        msg->payload.val.f = (float)sqlite3_column_double(stmt, 3);
        msg->quality = (uint8_t)sqlite3_column_int(stmt, 4);
        msg->type = (uint8_t)sqlite3_column_int(stmt, 5);
        sqlite3_finalize(stmt);

        char update_sql[128];
        snprintf(update_sql, sizeof(update_sql), "UPDATE tag_history SET status = 1 WHERE id = %lld;", (long long)row_id);
        sqlite3_exec(db, update_sql, NULL, NULL, NULL);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

static int _sqlite_driver_snf_get_pending_count(storage_driver_t *drv, uint32_t *out_count) {
    if (!drv || !drv->priv_ctx || !out_count) return -1;
    *out_count = 0;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
    if (!db) return -1;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM tag_history WHERE status = 0;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            *out_count = (uint32_t)sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        return 0;
    }
    return -1;
}

static int _sqlite_driver_snf_flush(storage_driver_t *drv) {
    return _sqlite_driver_flush(drv);
}

static int _sqlite_driver_set_param(storage_driver_t *drv, const char *product, uint32_t tag_id, const void *val, size_t val_len) {
    if (!drv || !drv->priv_ctx) return -1;
    return sqlite_svc_set_param((sqlite_svc_t *)drv->priv_ctx, product, tag_id, val, val_len);
}

static int _sqlite_driver_get_param(storage_driver_t *drv, const char *product, uint32_t tag_id, void *buf, size_t buf_len) {
    if (!drv || !drv->priv_ctx) return -1;
    return sqlite_svc_get_param((sqlite_svc_t *)drv->priv_ctx, product, tag_id, buf, buf_len);
}

static int _sqlite_driver_save_app(storage_driver_t *drv, const void *manifest_ptr) {
    if (!drv || !drv->priv_ctx) return -1;
    return sqlite_svc_save_app((sqlite_svc_t *)drv->priv_ctx, manifest_ptr);
}

static int _sqlite_driver_get_app(storage_driver_t *drv, const char *app_id, void *out_manifest_ptr) {
    if (!drv || !drv->priv_ctx) return -1;
    return sqlite_svc_get_app((sqlite_svc_t *)drv->priv_ctx, app_id, out_manifest_ptr);
}

static int _sqlite_driver_delete_app(storage_driver_t *drv, const char *app_id) {
    if (!drv || !drv->priv_ctx) return -1;
    return sqlite_svc_delete_app((sqlite_svc_t *)drv->priv_ctx, app_id);
}

static int _sqlite_driver_list_apps(storage_driver_t *drv, void *manifests_array_ptr, int max_count) {
    if (!drv || !drv->priv_ctx) return 0;
    return sqlite_svc_list_apps((sqlite_svc_t *)drv->priv_ctx, manifests_array_ptr, max_count);
}

static void *_sqlite_driver_get_native(storage_driver_t *drv) {
    if (!drv || !drv->priv_ctx) return NULL;
    return sqlite_svc_get_db_handle((sqlite_svc_t *)drv->priv_ctx);
}

const storage_ops_t g_sqlite_storage_ops = {
    .init = _sqlite_driver_init,
    .destroy = _sqlite_driver_destroy,
    .insert = _sqlite_driver_insert,
    .query = _sqlite_driver_query,
    .begin_trans = _sqlite_driver_begin_trans,
    .commit_trans = _sqlite_driver_commit_trans,
    .rollback_trans = _sqlite_driver_rollback_trans,
    .exec = _sqlite_driver_exec,
    .checkpoint = _sqlite_driver_checkpoint,
    .flush = _sqlite_driver_flush,
    .compact = _sqlite_driver_compact,
    .get_wal_size = _sqlite_driver_get_wal_size,
    .partition_gc = _sqlite_driver_partition_gc,
    .purge_old_data = _sqlite_driver_purge_old_data,
    .snf_enqueue = _sqlite_driver_snf_enqueue,
    .snf_dequeue = _sqlite_driver_snf_dequeue,
    .snf_get_pending_count = _sqlite_driver_snf_get_pending_count,
    .snf_flush = _sqlite_driver_snf_flush,
    .set_param = _sqlite_driver_set_param,
    .get_param = _sqlite_driver_get_param,
    .save_app = _sqlite_driver_save_app,
    .get_app = _sqlite_driver_get_app,
    .delete_app = _sqlite_driver_delete_app,
    .list_apps = _sqlite_driver_list_apps,
    .get_native_handle = _sqlite_driver_get_native,
};

storage_driver_t *storage_driver_sqlite_create(uv_loop_t *loop, const storage_config_t *cfg) {
    storage_driver_t *drv = calloc(1, sizeof(storage_driver_t));
    if (!drv) return NULL;
    drv->ops = &g_sqlite_storage_ops;
    if (drv->ops->init(drv, loop, cfg) != 0) {
        free(drv);
        return NULL;
    }
    return drv;
}
