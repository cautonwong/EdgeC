/**
 * @file app_settings_sqlite.c
 * @brief SQLite adapter for per-app settings schema and values.
 */

#include "app_settings_sqlite.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "edgeos/app_settings.h"
#include "sqlite_service.h"

static int fail(char *reason, size_t cap, const char *text) {
    if (reason && cap) snprintf(reason, cap, "%s", text);
    return -1;
}

static sqlite3 *handle_of(sqlite_svc_t *db) {
    return db ? (sqlite3 *)sqlite_svc_get_db_handle(db) : NULL;
}

static int exec_sql(sqlite3 *db, const char *sql, const char *app_id) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

/* Returns 1 found (text valid until finalize), 0 absent, -1 error. */
static int select_schema(sqlite3 *db, const char *app_id, sqlite3_stmt **stmt, const char **text) {
    *stmt = NULL;
    *text = NULL;
    if (sqlite3_prepare_v2(db, "SELECT schema_json FROM app_setting_schema WHERE app_id = ?1;", -1,
                           stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(*stmt, 1, app_id, -1, SQLITE_STATIC);
    int rc = sqlite3_step(*stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char *value = sqlite3_column_text(*stmt, 0);
        if (!value) {
            sqlite3_finalize(*stmt);
            *stmt = NULL;
            return -1;
        }
        *text = (const char *)value;
        return 1;
    }
    sqlite3_finalize(*stmt);
    *stmt = NULL;
    return rc == SQLITE_DONE ? 0 : -1;
}

int app_settings_schema_save(sqlite_svc_t *db, const char *app_id, const char *schema_json) {
    sqlite3 *handle = handle_of(db);
    if (!handle || !app_id || !app_id[0]) return -1;
    if (!schema_json) {
        return exec_sql(handle, "DELETE FROM app_setting_schema WHERE app_id = ?1;", app_id);
    }

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(handle,
                           "INSERT OR REPLACE INTO app_setting_schema (app_id, schema_json) "
                           "VALUES (?1, ?2);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, schema_json, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

int app_settings_upgrade_check(sqlite_svc_t *db, const char *app_id, const char *new_schema_json,
                               char *reason, size_t reason_size) {
    sqlite3 *handle = handle_of(db);
    if (!handle || !app_id || !app_id[0]) return fail(reason, reason_size, "invalid");

    sqlite3_stmt *schema_stmt = NULL;
    const char *schema = NULL;
    int found = select_schema(handle, app_id, &schema_stmt, &schema);
    if (found < 0) return fail(reason, reason_size, "store_error");
    if (found == 0) return 0;
    static const char k_empty_schema[] = "{\"schemaVersion\":1,\"fields\":{}}";
    int rc = edge_app_settings_check_upgrade(
        schema, new_schema_json ? new_schema_json : k_empty_schema, reason, reason_size);
    sqlite3_finalize(schema_stmt);
    return rc;
}

int app_settings_value_set(sqlite_svc_t *db, const char *app_id, const char *key,
                           const char *value_json, char *reason, size_t reason_size) {
    sqlite3 *handle = handle_of(db);
    if (!handle || !app_id || !app_id[0] || !key || !key[0] || !value_json) {
        return fail(reason, reason_size, "invalid");
    }

    sqlite3_stmt *schema_stmt = NULL;
    const char *schema = NULL;
    int found = select_schema(handle, app_id, &schema_stmt, &schema);
    if (found < 0) {
        return fail(reason, reason_size, "store_error");
    }
    if (found == 0) {
        return fail(reason, reason_size, "no_schema");
    }
    int rc = edge_app_setting_validate_value(schema, key, value_json, reason, reason_size);
    sqlite3_finalize(schema_stmt);
    if (rc != 0) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(handle,
                           "INSERT OR REPLACE INTO app_settings (app_id, key, value_json) "
                           "VALUES (?1, ?2, ?3);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return fail(reason, reason_size, "store_error");
    }
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, value_json, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : fail(reason, reason_size, "store_error");
}

int app_settings_value_get(sqlite_svc_t *db, const char *app_id, const char *key, char *out,
                           size_t cap, char *reason, size_t reason_size) {
    sqlite3 *handle = handle_of(db);
    if (!handle || !app_id || !app_id[0] || !key || !key[0] || !out || cap == 0) {
        return fail(reason, reason_size, "invalid");
    }

    sqlite3_stmt *schema_stmt = NULL;
    const char *schema = NULL;
    int found = select_schema(handle, app_id, &schema_stmt, &schema);
    if (found < 0) {
        return fail(reason, reason_size, "store_error");
    }
    if (found == 0) {
        return fail(reason, reason_size, "not_found");
    }
    int declared = edge_app_setting_declared(schema, key);
    if (declared < 0) {
        sqlite3_finalize(schema_stmt);
        return fail(reason, reason_size, "store_error");
    }
    if (declared == 0) {
        sqlite3_finalize(schema_stmt);
        return fail(reason, reason_size, "not_found");
    }

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(handle,
                           "SELECT value_json FROM app_settings WHERE app_id = ?1 AND key = ?2;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_finalize(schema_stmt);
        return fail(reason, reason_size, "store_error");
    }
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, key, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        size_t len = text ? (size_t)sqlite3_column_bytes(stmt, 0) : 0;
        if (!text || len + 1 > cap) {
            sqlite3_finalize(stmt);
            sqlite3_finalize(schema_stmt);
            return fail(reason, reason_size, "too_large");
        }
        memcpy(out, text, len);
        out[len] = '\0';
        sqlite3_finalize(stmt);
        sqlite3_finalize(schema_stmt);
        return 1;
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(schema_stmt);
        return fail(reason, reason_size, "store_error");
    }

    rc = edge_app_setting_default_json(schema, key, out, cap, reason, reason_size);
    sqlite3_finalize(schema_stmt);
    if (rc == 0) {
        fail(reason, reason_size, "not_set");
        return 0;
    }
    return rc;
}

int app_settings_remove(sqlite_svc_t *db, const char *app_id) {
    sqlite3 *handle = handle_of(db);
    if (!handle || !app_id || !app_id[0]) return -1;
    if (exec_sql(handle, "DELETE FROM app_settings WHERE app_id = ?1;", app_id) != 0) return -1;
    return exec_sql(handle, "DELETE FROM app_setting_schema WHERE app_id = ?1;", app_id);
}
