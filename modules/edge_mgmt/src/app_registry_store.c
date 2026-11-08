#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "edgeos/app_registry_store.h"

#define APP_REGISTRY_COLUMNS                                                                 \
    "id, name, version, author, wasm_path, trigger_tag, entry_func, fault_policy, "          \
    "heap_limit_kb, timeout_ms, state, restart_count, created_at"

static void _read_row(sqlite3_stmt *stmt, app_manifest_t *m) {
    memset(m, 0, sizeof(*m));
    snprintf(m->id, sizeof(m->id), "%s", (const char *)sqlite3_column_text(stmt, 0));
    snprintf(m->name, sizeof(m->name), "%s", (const char *)sqlite3_column_text(stmt, 1));
    snprintf(m->version, sizeof(m->version), "%s", (const char *)sqlite3_column_text(stmt, 2));
    snprintf(m->author, sizeof(m->author), "%s", (const char *)sqlite3_column_text(stmt, 3));
    snprintf(m->wasm_path, sizeof(m->wasm_path), "%s", (const char *)sqlite3_column_text(stmt, 4));
    m->trigger_tag = (uint32_t)sqlite3_column_int(stmt, 5);
    snprintf(m->entry_func, sizeof(m->entry_func), "%s",
             (const char *)sqlite3_column_text(stmt, 6));
    m->fault_policy = (fault_policy_t)sqlite3_column_int(stmt, 7);
    m->heap_limit_kb = (uint32_t)sqlite3_column_int(stmt, 8);
    m->timeout_ms = (uint32_t)sqlite3_column_int(stmt, 9);
    m->state = (app_state_t)sqlite3_column_int(stmt, 10);
    m->restart_count = (uint32_t)sqlite3_column_int(stmt, 11);
    m->created_at = (uint64_t)sqlite3_column_int64(stmt, 12);
}

int app_registry_save(sqlite_svc_t *svc, const app_manifest_t *m) {
    if (!svc || !m) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;

    const char *sql =
        "INSERT OR REPLACE INTO app_registry (" APP_REGISTRY_COLUMNS ") "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, m->id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, m->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, m->version, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, m->author, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, m->wasm_path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, (int)m->trigger_tag);
    sqlite3_bind_text(stmt, 7, m->entry_func, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 8, (int)m->fault_policy);
    sqlite3_bind_int(stmt, 9, (int)m->heap_limit_kb);
    sqlite3_bind_int(stmt, 10, (int)m->timeout_ms);
    sqlite3_bind_int(stmt, 11, (int)m->state);
    sqlite3_bind_int(stmt, 12, (int)m->restart_count);
    sqlite3_bind_int64(stmt, 13, (sqlite3_int64)m->created_at);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int app_registry_get(sqlite_svc_t *svc, const char *app_id, app_manifest_t *out_manifest) {
    if (!svc || !app_id || !out_manifest) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;

    const char *sql = "SELECT " APP_REGISTRY_COLUMNS " FROM app_registry WHERE id = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) _read_row(stmt, out_manifest);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_ROW) ? 0 : -1;
}

int app_registry_delete(sqlite_svc_t *svc, const char *app_id) {
    if (!svc || !app_id) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM app_registry WHERE id = ?;", -1, &stmt, NULL) !=
        SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int app_registry_list(sqlite_svc_t *svc, app_manifest_t *out_apps, int max_count) {
    if (!svc || !out_apps || max_count <= 0) return 0;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return 0;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT " APP_REGISTRY_COLUMNS " FROM app_registry;", -1, &stmt,
                           NULL) != SQLITE_OK)
        return 0;

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        _read_row(stmt, &out_apps[count]);
        count++;
    }
    sqlite3_finalize(stmt);
    return count;
}
