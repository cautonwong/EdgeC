/**
 * @file edge_app_task_sqlite.c
 * @brief SQLite adapter for the edge_mgmt app task store port.
 */

#include "edge_app_task_sqlite.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "sqlite_service.h"

#define SELECT_COLUMNS                                                              \
    "SELECT id, kind, idempotency_key, state, created_at, updated_at, result_json " \
    "FROM app_task "

static sqlite3 *_db(void *ctx) {
    return (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)ctx);
}

static void _read_task(sqlite3_stmt *stmt, edge_app_task_t *out) {
    const unsigned char *text;
    memset(out, 0, sizeof(*out));

    text = sqlite3_column_text(stmt, 0);
    snprintf(out->id, sizeof(out->id), "%s", text ? (const char *)text : "");
    out->kind = (edge_app_task_kind_t)sqlite3_column_int(stmt, 1);
    text = sqlite3_column_text(stmt, 2);
    snprintf(out->idempotency_key, sizeof(out->idempotency_key), "%s",
             text ? (const char *)text : "");
    out->state = (edge_app_task_state_t)sqlite3_column_int(stmt, 3);
    out->created_at = sqlite3_column_int64(stmt, 4);
    out->updated_at = sqlite3_column_int64(stmt, 5);
    text = sqlite3_column_text(stmt, 6);
    snprintf(out->result_json, sizeof(out->result_json), "%s", text ? (const char *)text : "");
}

static int _select(void *ctx, const char *sql, const char *key, edge_app_task_t *out) {
    sqlite3 *db = _db(ctx);
    if (!db || !key || !out) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

    int found = 0;
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        _read_task(stmt, out);
        found = 1;
    } else if (rc != SQLITE_DONE) {
        found = -1;
    }
    sqlite3_finalize(stmt);
    return found;
}

static int _get(void *ctx, const char *id, edge_app_task_t *out) {
    return _select(ctx, SELECT_COLUMNS "WHERE id = ?;", id, out);
}

static int _find_by_key(void *ctx, const char *idempotency_key, edge_app_task_t *out) {
    return _select(ctx, SELECT_COLUMNS "WHERE idempotency_key = ?;", idempotency_key, out);
}

static void _bind_task(sqlite3_stmt *stmt, const edge_app_task_t *task) {
    sqlite3_bind_text(stmt, 1, task->id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)task->kind);
    sqlite3_bind_text(stmt, 3, task->idempotency_key, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, (int)task->state);
    sqlite3_bind_int64(stmt, 5, task->created_at);
    sqlite3_bind_int64(stmt, 6, task->updated_at);
    if (task->result_json[0]) {
        sqlite3_bind_text(stmt, 7, task->result_json, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 7);
    }
}

static int _insert(void *ctx, const edge_app_task_t *task) {
    sqlite3 *db = _db(ctx);
    if (!db || !task) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
                           "INSERT OR REPLACE INTO app_task "
                           "(id, kind, idempotency_key, state, created_at, updated_at, "
                           "result_json) VALUES (?, ?, ?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    _bind_task(stmt, task);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

static int _update(void *ctx, const edge_app_task_t *task) {
    sqlite3 *db = _db(ctx);
    if (!db || !task) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
                           "UPDATE app_task SET state = ?, updated_at = ?, result_json = ? "
                           "WHERE id = ?;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt, 1, (int)task->state);
    sqlite3_bind_int64(stmt, 2, task->updated_at);
    if (task->result_json[0]) {
        sqlite3_bind_text(stmt, 3, task->result_json, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 3);
    }
    sqlite3_bind_text(stmt, 4, task->id, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    int changed = sqlite3_changes(db);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return -1;
    return changed > 0 ? 0 : 1;
}

static int _purge_expired(void *ctx, int64_t cutoff_ms, size_t *removed) {
    sqlite3 *db = _db(ctx);
    if (!db) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM app_task WHERE state IN (?, ?) AND updated_at < ?;", -1,
                           &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt, 1, (int)EDGE_APP_TASK_SUCCEEDED);
    sqlite3_bind_int(stmt, 2, (int)EDGE_APP_TASK_FAILED);
    sqlite3_bind_int64(stmt, 3, cutoff_ms);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE && removed) *removed = (size_t)sqlite3_changes(db);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

edge_app_task_store_t edge_app_task_sqlite(sqlite_svc_t *db) {
    edge_app_task_store_t store = {
        .ctx = db,
        .insert = _insert,
        .get = _get,
        .find_by_key = _find_by_key,
        .update = _update,
        .purge_expired = _purge_expired,
    };
    return store;
}
