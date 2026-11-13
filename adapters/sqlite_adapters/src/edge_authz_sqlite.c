/**
 * @file edge_authz_sqlite.c
 * @brief SQLite adapter for the edge_mgmt authorization repository port.
 */

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "edge_authz_sqlite.h"
#include "sqlite_service.h"

static void _copy_col(sqlite3_stmt *stmt, int index, char *dst, size_t capacity) {
    const unsigned char *text = sqlite3_column_text(stmt, index);
    snprintf(dst, capacity, "%s", text ? (const char *)text : "");
}

static int _get_grant(void *ctx, const char *app_id, const char *capability,
                      edge_app_grant_t *out) {
    sqlite_svc_t *svc = (sqlite_svc_t *)ctx;
    if (!svc || !app_id || !capability || !out) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;

    const char *sql = "SELECT scope_json, state, purpose, expires_at, sensitivity_max, granted_by, "
                      "granted_at, revoked_at FROM app_grant "
                      "WHERE app_id = ? AND capability = ? LIMIT 1;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, capability, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    int found = 0;
    if (rc == SQLITE_ROW) {
        memset(out, 0, sizeof(*out));
        snprintf(out->app_id, sizeof(out->app_id), "%s", app_id);
        snprintf(out->capability, sizeof(out->capability), "%s", capability);
        _copy_col(stmt, 0, out->scope_json, sizeof(out->scope_json));
        out->state = (edge_grant_state_t)sqlite3_column_int(stmt, 1);
        _copy_col(stmt, 2, out->purpose, sizeof(out->purpose));
        out->expires_at = (uint64_t)sqlite3_column_int64(stmt, 3);
        out->sensitivity_max = (uint8_t)sqlite3_column_int(stmt, 4);
        _copy_col(stmt, 5, out->granted_by, sizeof(out->granted_by));
        out->granted_at = (uint64_t)sqlite3_column_int64(stmt, 6);
        out->revoked_at = (uint64_t)sqlite3_column_int64(stmt, 7);
        found = 1;
    } else if (rc != SQLITE_DONE) {
        found = -1;
    }
    sqlite3_finalize(stmt);
    return found;
}

static int _get_point_profile(void *ctx, const char *scope, edge_point_profile_t *out) {
    sqlite_svc_t *svc = (sqlite_svc_t *)ctx;
    if (!svc || !scope || !out) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT sensitivity FROM point_profile WHERE scope = ? LIMIT 1;", -1,
                           &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, scope, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    int found = 0;
    if (rc == SQLITE_ROW) {
        memset(out, 0, sizeof(*out));
        snprintf(out->scope, sizeof(out->scope), "%s", scope);
        out->sensitivity = (uint8_t)sqlite3_column_int(stmt, 0);
        found = 1;
    } else if (rc != SQLITE_DONE) {
        found = -1;
    }
    sqlite3_finalize(stmt);
    return found;
}

static int _get_publisher_trust(void *ctx, const char *app_id, edge_publisher_trust_t *out) {
    sqlite_svc_t *svc = (sqlite_svc_t *)ctx;
    if (!svc || !app_id || !out) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(
            db, "SELECT trust_level, verified FROM publisher_trust WHERE publisher_id = ? LIMIT 1;",
            -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    int found = 0;
    if (rc == SQLITE_ROW) {
        memset(out, 0, sizeof(*out));
        snprintf(out->app_id, sizeof(out->app_id), "%s", app_id);
        out->trust_level = (uint8_t)sqlite3_column_int(stmt, 0);
        out->verified = sqlite3_column_int(stmt, 1) != 0;
        found = 1;
    } else if (rc != SQLITE_DONE) {
        found = -1;
    }
    sqlite3_finalize(stmt);
    return found;
}

edge_authz_repo_t edge_authz_sqlite(sqlite_svc_t *db) {
    edge_authz_repo_t repo = {
        .ctx = db,
        .get_grant = _get_grant,
        .get_point_profile = _get_point_profile,
        .get_publisher_trust = _get_publisher_trust,
    };
    return repo;
}

static int _save_grant(void *ctx, const edge_app_grant_t *grant) {
    sqlite_svc_t *svc = (sqlite_svc_t *)ctx;
    if (!svc || !grant) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;

    const char *sql =
        "INSERT OR REPLACE INTO app_grant (app_id, capability, scope_json, state, purpose, "
        "expires_at, sensitivity_max, granted_by, granted_at, revoked_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, grant->app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, grant->capability, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, grant->scope_json, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, (int)grant->state);
    sqlite3_bind_text(stmt, 5, grant->purpose, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)grant->expires_at);
    sqlite3_bind_int(stmt, 7, (int)grant->sensitivity_max);
    sqlite3_bind_text(stmt, 8, grant->granted_by, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 9, (sqlite3_int64)grant->granted_at);
    sqlite3_bind_int64(stmt, 10, (sqlite3_int64)grant->revoked_at);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

static int _list_grants(void *ctx, const char *app_id, edge_app_grant_t *out, size_t max,
                        size_t *count) {
    sqlite_svc_t *svc = (sqlite_svc_t *)ctx;
    if (!svc || !out || !count) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;
    *count = 0;

    const char *sql =
        "SELECT app_id, capability, scope_json, state, purpose, expires_at, sensitivity_max, "
        "granted_by, granted_at, revoked_at FROM app_grant "
        "WHERE (?1 = '' OR app_id = ?1) ORDER BY app_id, capability LIMIT ?2;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, app_id ? app_id : "", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)max);

    size_t n = 0;
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && n < max) {
        edge_app_grant_t *g = &out[n];
        memset(g, 0, sizeof(*g));
        _copy_col(stmt, 0, g->app_id, sizeof(g->app_id));
        _copy_col(stmt, 1, g->capability, sizeof(g->capability));
        _copy_col(stmt, 2, g->scope_json, sizeof(g->scope_json));
        g->state = (edge_grant_state_t)sqlite3_column_int(stmt, 3);
        _copy_col(stmt, 4, g->purpose, sizeof(g->purpose));
        g->expires_at = (uint64_t)sqlite3_column_int64(stmt, 5);
        g->sensitivity_max = (uint8_t)sqlite3_column_int(stmt, 6);
        _copy_col(stmt, 7, g->granted_by, sizeof(g->granted_by));
        g->granted_at = (uint64_t)sqlite3_column_int64(stmt, 8);
        g->revoked_at = (uint64_t)sqlite3_column_int64(stmt, 9);
        n++;
    }
    sqlite3_finalize(stmt);
    *count = n;
    return rc == SQLITE_DONE || rc == SQLITE_ROW ? 0 : -1;
}

static int _delete_grant(void *ctx, const char *app_id, const char *capability) {
    sqlite_svc_t *svc = (sqlite_svc_t *)ctx;
    if (!svc || !app_id || !capability) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "DELETE FROM app_grant WHERE app_id = ? AND capability = ?;", -1,
                           &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, capability, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

static int _set_publisher_trust(void *ctx, const char *app_id, uint8_t trust_level,
                                bool verified) {
    sqlite_svc_t *svc = (sqlite_svc_t *)ctx;
    if (!svc || !app_id) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    if (!db) return -1;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
                           "INSERT OR REPLACE INTO publisher_trust "
                           "(publisher_id, trust_level, verified) VALUES (?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)trust_level);
    sqlite3_bind_int(stmt, 3, verified ? 1 : 0);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

edge_authz_store_t edge_authz_sqlite_store(sqlite_svc_t *db) {
    edge_authz_store_t store = {
        .ctx = db,
        .get_grant = _get_grant,
        .save_grant = _save_grant,
        .delete_grant = _delete_grant,
        .list_grants = _list_grants,
        .set_publisher_trust = _set_publisher_trust,
    };
    return store;
}
