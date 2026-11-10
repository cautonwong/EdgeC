/**
 * @file edge_params_sqlite_adapter.c
 * @brief edge_params store port backed by the product SQLite service.
 */

#include "edge_params_sqlite_adapter.h"
#include "edge_log.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

static sqlite3 *_db(void *ctx) {
    return (sqlite3 *)sqlite_svc_get_db_handle((sqlite_svc_t *)ctx);
}

static int _ensure_schema(void *ctx) {
    sqlite3 *db = _db(ctx);
    if (!db) return -1;
    return sqlite3_exec(db,
                        "CREATE TABLE IF NOT EXISTS system_params ("
                        "  id INTEGER PRIMARY KEY,"
                        "  name TEXT UNIQUE,"
                        "  val_float REAL,"
                        "  val_int INTEGER"
                        ");",
                        NULL, NULL, NULL);
}

static int _insert_default_float(void *ctx, const char *name, double value) {
    sqlite3 *db = _db(ctx);
    if (!db) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT OR IGNORE INTO system_params (name, val_float) VALUES (?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, value);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _insert_default_int(void *ctx, const char *name, int64_t value) {
    sqlite3 *db = _db(ctx);
    if (!db) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT OR IGNORE INTO system_params (name, val_int) VALUES (?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)value);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _load_float(void *ctx, const char *name, double *out) {
    sqlite3 *db = _db(ctx);
    if (!db || !out) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT val_float FROM system_params WHERE name = ?;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            *out = sqlite3_column_double(stmt, 0);
            rc = 0;
        }
    }
    sqlite3_finalize(stmt);
    return rc;
}

static int _load_int(void *ctx, const char *name, int64_t *out) {
    sqlite3 *db = _db(ctx);
    if (!db || !out) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT val_int FROM system_params WHERE name = ?;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            *out = (int64_t)sqlite3_column_int64(stmt, 0);
            rc = 0;
        }
    }
    sqlite3_finalize(stmt);
    return rc;
}

static int _save_float(void *ctx, const char *name, double value) {
    sqlite3 *db = _db(ctx);
    if (!db) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE system_params SET val_float = ? WHERE name = ?;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_double(stmt, 1, value);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

edge_params_store_t edge_params_sqlite_store(sqlite_svc_t *db) {
    edge_params_store_t store = {
        .ctx = db,
        .ensure_schema = _ensure_schema,
        .insert_default_float = _insert_default_float,
        .insert_default_int = _insert_default_int,
        .load_float = _load_float,
        .load_int = _load_int,
        .save_float = _save_float,
    };
    return store;
}
