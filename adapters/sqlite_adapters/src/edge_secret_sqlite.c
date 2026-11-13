/**
 * @file edge_secret_sqlite.c
 * @brief SQLite adapter for the edge_mgmt secret store port.
 *
 * The value column holds caller-encrypted ciphertext. This adapter copies
 * bytes and never inspects or logs them.
 */

#include "edge_secret_sqlite.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "sqlite_service.h"

static sqlite3 *_db(void *ctx) {
    return (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)ctx);
}

static int _get(void *ctx, const char *app_id, const char *name, void *out, size_t cap,
                size_t *len) {
    sqlite3 *db = _db(ctx);
    if (!db || !app_id || !name) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
                           "SELECT value FROM app_secret WHERE app_id = ? AND name = ? LIMIT 1;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    int found = 0;
    if (rc == SQLITE_ROW) {
        const void *blob = sqlite3_column_blob(stmt, 0);
        size_t blob_len = (size_t)sqlite3_column_bytes(stmt, 0);
        if (len) *len = blob_len;
        if (out && cap > 0 && blob) memcpy(out, blob, blob_len < cap ? blob_len : cap);
        found = 1;
    } else if (rc != SQLITE_DONE) {
        found = -1;
    }
    sqlite3_finalize(stmt);
    return found;
}

static int _put(void *ctx, const char *app_id, const char *name, const void *value, size_t len) {
    sqlite3 *db = _db(ctx);
    if (!db || !app_id || !name || !value) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(
            db, "INSERT OR REPLACE INTO app_secret (app_id, name, value) VALUES (?, ?, ?);", -1,
            &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 3, value, (int)len, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

static int _remove(void *ctx, const char *app_id, const char *name) {
    sqlite3 *db = _db(ctx);
    if (!db || !app_id || !name) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM app_secret WHERE app_id = ? AND name = ?;", -1, &stmt,
                           NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

static int _list_names(void *ctx, const char *app_id, char names[][EDGE_SECRET_NAME_SIZE],
                       size_t max, size_t *count) {
    sqlite3 *db = _db(ctx);
    if (!db || !app_id || !names || !count || max == 0) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
                           "SELECT name FROM app_secret WHERE app_id = ? ORDER BY name LIMIT ?;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)max);

    size_t copied = 0;
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && copied < max) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        snprintf(names[copied], EDGE_SECRET_NAME_SIZE, "%s", text ? (const char *)text : "");
        copied++;
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) return -1; /* ROW means truncated at max */
    *count = copied;
    return 0;
}

static int _count(void *ctx, const char *app_id, size_t *count) {
    sqlite3 *db = _db(ctx);
    if (!db || !app_id || !count) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM app_secret WHERE app_id = ?;", -1, &stmt,
                           NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) *count = (size_t)sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return rc == SQLITE_ROW ? 0 : -1;
}

edge_secret_store_t edge_secret_sqlite(sqlite_svc_t *db) {
    edge_secret_store_t store = {
        .ctx = db,
        .get = _get,
        .put = _put,
        .remove = _remove,
        .list_names = _list_names,
        .count = _count,
    };
    return store;
}
