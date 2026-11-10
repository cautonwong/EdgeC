/**
 * @file snf_sqlite_adapter.c
 * @brief SnF store port backed by the product SQLite service.
 */

#include "snf_sqlite_adapter.h"
#include "edge_log.h"

#include <sqlite3.h>
#include <string.h>

static sqlite3 *_db(void *ctx) {
    return (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)ctx);
}

static int _ensure_schema(void *ctx) {
    sqlite3 *db = _db(ctx);
    if (!db) return -1;

    sqlite3_stmt *check_stmt = NULL;
    bool needs_alter = true;
    if (sqlite3_prepare_v2(db, "PRAGMA table_info(telemetry);", -1, &check_stmt, NULL) ==
        SQLITE_OK) {
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
        if (sqlite3_exec(db, "ALTER TABLE telemetry ADD COLUMN synced INT DEFAULT 0;", NULL, NULL,
                         NULL) != SQLITE_OK) {
            LOG_ERROR(LOG_CAT_NB, "SnF: Failed to ALTER TABLE telemetry: %s", sqlite3_errmsg(db));
            return -1;
        }
        LOG_INFO(LOG_CAT_NB, "SnF: Successfully added 'synced' column to telemetry table.");
    }

    if (sqlite3_exec(db,
                     "CREATE INDEX IF NOT EXISTS idx_telemetry_synced ON telemetry(synced) "
                     "WHERE synced = 0;",
                     NULL, NULL, NULL) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_NB, "SnF: Failed to create synced index: %s", sqlite3_errmsg(db));
        return -1;
    }
    return 0;
}

static int _count_unsynced(void *ctx, int *out_count) {
    if (!out_count) return -1;
    sqlite3 *db = _db(ctx);
    if (!db) return -1;

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT count(*) FROM telemetry WHERE synced = 0;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *out_count = sqlite3_column_int(stmt, 0);
        rc = 0;
    }
    sqlite3_finalize(stmt);
    return rc;
}

static int _read_unsynced(void *ctx, uint32_t limit, snf_record_t *out, uint32_t cap,
                          uint32_t *out_count) {
    if (!out || !out_count || !cap) return -1;
    sqlite3 *db = _db(ctx);
    if (!db) return -1;

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT ts, tag_id, val, q FROM telemetry WHERE synced = 0 "
                           "ORDER BY ts ASC LIMIT ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_NB, "SnF: Failed to prepare sync query: %s", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_bind_int(stmt, 1, (int)limit);

    uint32_t count = 0;
    while (count < cap && sqlite3_step(stmt) == SQLITE_ROW) {
        out[count].ts = (uint64_t)sqlite3_column_int64(stmt, 0);
        out[count].tag_id = (uint32_t)sqlite3_column_int(stmt, 1);
        out[count].value = sqlite3_column_double(stmt, 2);
        out[count].quality = (uint16_t)sqlite3_column_int(stmt, 3);
        count++;
    }
    sqlite3_finalize(stmt);
    *out_count = count;
    return 0;
}

static int _mark_synced(void *ctx, const snf_record_t *records, uint32_t count) {
    if (!records || !count) return 0;
    sqlite3 *db = _db(ctx);
    if (!db) return -1;

    sqlite3_stmt *upd = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE telemetry SET synced = 1 WHERE ts = ? AND tag_id = ?;", -1,
                           &upd, NULL) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_NB, "SnF: Failed to prepare confirm statement: %s", sqlite3_errmsg(db));
        return -1;
    }

    int confirmed = 0;
    for (uint32_t i = 0; i < count; i++) {
        sqlite3_bind_int64(upd, 1, (sqlite3_int64)records[i].ts);
        sqlite3_bind_int(upd, 2, (int)records[i].tag_id);
        if (sqlite3_step(upd) == SQLITE_DONE) confirmed++;
        sqlite3_reset(upd);
    }
    sqlite3_finalize(upd);
    if (confirmed > 0) LOG_INFO(LOG_CAT_NB, "SnF: Confirmed %d record(s) delivered.", confirmed);
    return 0;
}

static int _insert(void *ctx, const PipeMsg *msg) {
    return sqlite_svc_insert((sqlite_svc_t *)ctx, msg);
}

snf_store_port_t snf_sqlite_store_port(sqlite_svc_t *db) {
    snf_store_port_t port = {
        .ctx = db,
        .ensure_schema = _ensure_schema,
        .count_unsynced = _count_unsynced,
        .read_unsynced = _read_unsynced,
        .mark_synced = _mark_synced,
        .insert = _insert,
    };
    return port;
}
