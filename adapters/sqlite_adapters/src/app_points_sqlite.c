/**
 * @file app_points_sqlite.c
 * @brief App point contracts and slot allocation over the control database.
 */

#include "app_points_sqlite.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "sqlite_service.h"

static int fail(char *reason, size_t cap, const char *text) {
    if (reason && cap) snprintf(reason, cap, "%s", text);
    return -1;
}

static sqlite3 *handle_of(sqlite_svc_t *db) {
    return db ? (sqlite3 *)sqlite_svc_get_db_handle(db) : NULL;
}

static int exec_sql(sqlite3 *db, const char *sql) {
    return sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

/* Returns 1 when found, 0 when absent, -1 on error. */
static int find_alloc(sqlite3 *db, const char *app_id, int *base_slot) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT base_slot FROM app_slot_alloc WHERE app_id = ?1;", -1, &stmt,
                           NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    int found = rc == SQLITE_ROW ? 1 : (rc == SQLITE_DONE ? 0 : -1);
    if (found == 1) *base_slot = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return found;
}

/* Returns 0 on success, -1 on error, -2 when every block is taken. */
static int allocate_block(sqlite3 *db, const char *app_id, int *base_slot) {
    int used[APP_POINT_SLOT_BLOCKS];
    memset(used, 0, sizeof(used));
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT base_slot FROM app_slot_alloc;", -1, &stmt, NULL) !=
        SQLITE_OK)
        return -1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int block = sqlite3_column_int(stmt, 0) / APP_POINT_SLOT_BLOCK;
        if (block >= 0 && block < APP_POINT_SLOT_BLOCKS) used[block] = 1;
    }
    sqlite3_finalize(stmt);

    for (int i = 0; i < APP_POINT_SLOT_BLOCKS; i++) {
        if (used[i]) continue;
        if (sqlite3_prepare_v2(db,
                               "INSERT INTO app_slot_alloc(app_id, base_slot, slot_count) "
                               "VALUES(?1, ?2, ?3);",
                               -1, &stmt, NULL) != SQLITE_OK)
            return -1;
        sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, i * APP_POINT_SLOT_BLOCK);
        sqlite3_bind_int(stmt, 3, APP_POINT_SLOT_BLOCK);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE) return -1;
        *base_slot = i * APP_POINT_SLOT_BLOCK;
        return 0;
    }
    return -2;
}

static int slot_used(sqlite3 *db, int slot) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM app_point WHERE slot = ?1 LIMIT 1;", -1, &stmt,
                           NULL) != SQLITE_OK)
        return 1;
    sqlite3_bind_int(stmt, 1, slot);
    int used = sqlite3_step(stmt) == SQLITE_ROW ? 1 : 0;
    sqlite3_finalize(stmt);
    return used;
}

static int find_free_slot(sqlite3 *db, int base_slot, int *slot_out) {
    for (int i = 0; i < APP_POINT_SLOT_BLOCK; i++) {
        if (!slot_used(db, base_slot + i)) {
            *slot_out = base_slot + i;
            return 0;
        }
    }
    return -1;
}

static const app_point_spec_t *find_declared(const app_point_spec_t *points, size_t count,
                                             const char *name) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(points[i].name, name) == 0) return &points[i];
    }
    return NULL;
}

/* Delete rows the new manifest no longer declares, so their slots free up. */
static int delete_undeclared(sqlite3 *db, const char *app_id, const app_point_spec_t *points,
                             size_t count) {
    char names[APP_SPEC_MAX_POINTS][APP_SPEC_POINT_NAME_SIZE];
    size_t kept = 0;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT name FROM app_point WHERE app_id = ?1;", -1, &stmt, NULL) !=
        SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW && kept < APP_SPEC_MAX_POINTS) {
        const unsigned char *name = sqlite3_column_text(stmt, 0);
        snprintf(names[kept], sizeof(names[kept]), "%s", name ? (const char *)name : "");
        kept++;
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) return -1;

    for (size_t i = 0; i < kept; i++) {
        if (find_declared(points, count, names[i])) continue;
        if (sqlite3_prepare_v2(db, "DELETE FROM app_point WHERE app_id = ?1 AND name = ?2;", -1,
                               &stmt, NULL) != SQLITE_OK)
            return -1;
        sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, names[i], -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE) return -1;
    }
    return 0;
}

int app_points_sync(sqlite_svc_t *db, const char *app_id, const app_point_spec_t *points,
                    size_t count, char *reason, size_t reason_size) {
    if (!db || !app_id || !app_id[0] || (count && !points)) {
        return fail(reason, reason_size, "bad_args");
    }
    if (count > APP_SPEC_MAX_POINTS) return fail(reason, reason_size, "too many points");
    sqlite3 *handle = handle_of(db);
    if (!handle) return fail(reason, reason_size, "database unavailable");

    if (exec_sql(handle, "BEGIN IMMEDIATE;") != 0) {
        return fail(reason, reason_size, "database busy");
    }

    if (count == 0) {
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(handle, "DELETE FROM app_point WHERE app_id = ?1;", -1, &stmt,
                               NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        if (sqlite3_prepare_v2(handle, "DELETE FROM app_slot_alloc WHERE app_id = ?1;", -1, &stmt,
                               NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        if (exec_sql(handle, "COMMIT;") != 0) {
            return fail(reason, reason_size, "commit failed");
        }
        return 0;
    }

    int base_slot = -1;
    int found = find_alloc(handle, app_id, &base_slot);
    if (found < 0) {
        exec_sql(handle, "ROLLBACK;");
        return fail(reason, reason_size, "database error");
    }
    if (found == 0) {
        int rc = allocate_block(handle, app_id, &base_slot);
        if (rc != 0) {
            exec_sql(handle, "ROLLBACK;");
            return fail(reason, reason_size,
                        rc == -2 ? "slot region exhausted" : "database error");
        }
    }

    if (delete_undeclared(handle, app_id, points, count) != 0) {
        exec_sql(handle, "ROLLBACK;");
        return fail(reason, reason_size, "database error");
    }

    for (size_t i = 0; i < count; i++) {
        const app_point_spec_t *point = &points[i];
        int slot = -1;
        int type = 0;
        int version = 0;
        char unit[APP_SPEC_POINT_UNIT_SIZE] = "";
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(handle,
                               "SELECT slot, type, unit, contract_version FROM app_point "
                               "WHERE app_id = ?1 AND name = ?2;",
                               -1, &stmt, NULL) != SQLITE_OK) {
            exec_sql(handle, "ROLLBACK;");
            return fail(reason, reason_size, "database error");
        }
        sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, point->name, -1, SQLITE_STATIC);
        int step = sqlite3_step(stmt);
        int existing = step == SQLITE_ROW;
        if (existing) {
            slot = sqlite3_column_int(stmt, 0);
            type = sqlite3_column_int(stmt, 1);
            const unsigned char *unit_text = sqlite3_column_text(stmt, 2);
            snprintf(unit, sizeof(unit), "%s", unit_text ? (const char *)unit_text : "");
            version = sqlite3_column_int(stmt, 3);
        }
        sqlite3_finalize(stmt);
        if (step != SQLITE_ROW && step != SQLITE_DONE) {
            exec_sql(handle, "ROLLBACK;");
            return fail(reason, reason_size, "database error");
        }

        if (existing) {
            if (type != point->type || version != point->contract_version ||
                strcmp(unit, point->unit) != 0) {
                exec_sql(handle, "ROLLBACK;");
                snprintf(reason, reason_size, "point contract changed: %s", point->name);
                return -1;
            }
            if (sqlite3_prepare_v2(handle,
                                   "UPDATE app_point SET retention = ?3, stale_after_ms = ?4, "
                                   "stability = ?5, source = ?6 WHERE app_id = ?1 AND name = ?2;",
                                   -1, &stmt, NULL) != SQLITE_OK) {
                exec_sql(handle, "ROLLBACK;");
                return fail(reason, reason_size, "database error");
            }
            sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, point->name, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, point->retention);
            sqlite3_bind_int(stmt, 4, (int)point->stale_after_ms);
            sqlite3_bind_int(stmt, 5, point->stability);
            sqlite3_bind_int(stmt, 6, point->source);
            int rc = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            if (rc != SQLITE_DONE) {
                exec_sql(handle, "ROLLBACK;");
                return fail(reason, reason_size, "database error");
            }
            continue;
        }

        if (find_free_slot(handle, base_slot, &slot) != 0) {
            exec_sql(handle, "ROLLBACK;");
            return fail(reason, reason_size, "point block exhausted");
        }
        if (sqlite3_prepare_v2(handle,
                               "INSERT INTO app_point(app_id, name, slot, type, unit, "
                               "contract_version, retention, stale_after_ms, stability, source) "
                               "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10);",
                               -1, &stmt, NULL) != SQLITE_OK) {
            exec_sql(handle, "ROLLBACK;");
            return fail(reason, reason_size, "database error");
        }
        sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, point->name, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, slot);
        sqlite3_bind_int(stmt, 4, point->type);
        sqlite3_bind_text(stmt, 5, point->unit, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 6, point->contract_version);
        sqlite3_bind_int(stmt, 7, point->retention);
        sqlite3_bind_int(stmt, 8, (int)point->stale_after_ms);
        sqlite3_bind_int(stmt, 9, point->stability);
        sqlite3_bind_int(stmt, 10, point->source);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE) {
            exec_sql(handle, "ROLLBACK;");
            return fail(reason, reason_size, "database error");
        }
    }

    if (exec_sql(handle, "COMMIT;") != 0) {
        exec_sql(handle, "ROLLBACK;");
        return fail(reason, reason_size, "commit failed");
    }
    if (reason && reason_size) reason[0] = '\0';
    return 0;
}

int app_points_check(sqlite_svc_t *db, const char *app_id, const app_point_spec_t *points,
                     size_t count, char *reason, size_t reason_size) {
    if (!db || !app_id || (count && !points)) return fail(reason, reason_size, "bad_args");
    if (count == 0) return 0;
    if (count > APP_SPEC_MAX_POINTS) return fail(reason, reason_size, "too many points");
    sqlite3 *handle = handle_of(db);
    if (!handle) return fail(reason, reason_size, "database unavailable");

    int base_slot = -1;
    int found = find_alloc(handle, app_id, &base_slot);
    if (found < 0) return fail(reason, reason_size, "database error");
    if (found == 0) {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(handle, "SELECT COUNT(*) FROM app_slot_alloc;", -1, &stmt, NULL) !=
            SQLITE_OK)
            return fail(reason, reason_size, "database error");
        int blocks = sqlite3_step(stmt) == SQLITE_ROW ? sqlite3_column_int(stmt, 0) : 0;
        sqlite3_finalize(stmt);
        if (blocks >= APP_POINT_SLOT_BLOCKS) return fail(reason, reason_size, "slot region exhausted");
        return 0;
    }

    size_t new_names = 0;
    for (size_t i = 0; i < count; i++) {
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(handle,
                               "SELECT type, unit, contract_version FROM app_point "
                               "WHERE app_id = ?1 AND name = ?2;",
                               -1, &stmt, NULL) != SQLITE_OK) {
            return fail(reason, reason_size, "database error");
        }
        sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, points[i].name, -1, SQLITE_STATIC);
        int step = sqlite3_step(stmt);
        if (step == SQLITE_ROW) {
            int type = sqlite3_column_int(stmt, 0);
            const unsigned char *unit = sqlite3_column_text(stmt, 1);
            int version = sqlite3_column_int(stmt, 2);
            int mismatch = type != points[i].type || version != points[i].contract_version ||
                           strcmp(unit ? (const char *)unit : "", points[i].unit) != 0;
            sqlite3_finalize(stmt);
            if (mismatch) {
                snprintf(reason, reason_size, "point contract changed: %s", points[i].name);
                return -1;
            }
        } else if (step == SQLITE_DONE) {
            sqlite3_finalize(stmt);
            new_names++;
        } else {
            sqlite3_finalize(stmt);
            return fail(reason, reason_size, "database error");
        }
    }

    size_t kept = 0;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(handle,
                           "SELECT name FROM app_point WHERE slot >= ?1 AND slot < ?2;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return fail(reason, reason_size, "database error");
    }
    sqlite3_bind_int(stmt, 1, base_slot);
    sqlite3_bind_int(stmt, 2, base_slot + APP_POINT_SLOT_BLOCK);
    int step;
    while ((step = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 0);
        if (name && find_declared(points, count, (const char *)name)) kept++;
    }
    sqlite3_finalize(stmt);
    if (step != SQLITE_DONE) return fail(reason, reason_size, "database error");
    if (new_names > (size_t)APP_POINT_SLOT_BLOCK - kept) {
        return fail(reason, reason_size, "point block exhausted");
    }
    return 0;
}

int app_points_release(sqlite_svc_t *db, const char *app_id) {
    sqlite3 *handle = handle_of(db);
    if (!handle || !app_id) return -1;
    if (exec_sql(handle, "BEGIN IMMEDIATE;") != 0) return -1;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(handle, "DELETE FROM app_point WHERE app_id = ?1;", -1, &stmt, NULL) ==
        SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    if (sqlite3_prepare_v2(handle, "DELETE FROM app_slot_alloc WHERE app_id = ?1;", -1, &stmt,
                           NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return exec_sql(handle, "COMMIT;");
}

int app_points_list(sqlite_svc_t *db, const char *app_id, app_point_row_t *out, size_t cap,
                    size_t *count, char *reason, size_t reason_size) {
    if (count) *count = 0;
    sqlite3 *handle = handle_of(db);
    if (!handle || !app_id) return fail(reason, reason_size, "bad_args");

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(handle,
                           "SELECT name, unit, slot, type, contract_version, retention, "
                           "stale_after_ms, stability, source FROM app_point "
                           "WHERE app_id = ?1 ORDER BY slot;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return fail(reason, reason_size, "database error");
    }
    sqlite3_bind_text(stmt, 1, app_id, -1, SQLITE_STATIC);
    size_t total = 0;
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (out && total < cap) {
            app_point_row_t *row = &out[total];
            memset(row, 0, sizeof(*row));
            const unsigned char *name = sqlite3_column_text(stmt, 0);
            const unsigned char *unit = sqlite3_column_text(stmt, 1);
            snprintf(row->name, sizeof(row->name), "%s", name ? (const char *)name : "");
            snprintf(row->unit, sizeof(row->unit), "%s", unit ? (const char *)unit : "");
            row->slot = sqlite3_column_int(stmt, 2);
            row->type = sqlite3_column_int(stmt, 3);
            row->contract_version = sqlite3_column_int(stmt, 4);
            row->retention = sqlite3_column_int(stmt, 5);
            row->stale_after_ms = sqlite3_column_int(stmt, 6);
            row->stability = sqlite3_column_int(stmt, 7);
            row->source = sqlite3_column_int(stmt, 8);
        }
        total++;
    }
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return fail(reason, reason_size, "database error");
    if (count) *count = total;
    return 0;
}

int app_points_find_full(sqlite_svc_t *db, const char *full_name, char *app_id,
                         size_t app_id_size, int *slot) {
    sqlite3 *handle = handle_of(db);
    if (!handle || !full_name || !app_id || !slot) return -1;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(handle,
                           "SELECT p.app_id, p.slot FROM app_point p "
                           "JOIN app_registry r ON r.id = p.app_id "
                           "WHERE r.state != 0 AND ('app.' || p.app_id || '.' || p.name) = ?1 "
                           "LIMIT 1;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_text(stmt, 1, full_name, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    int found = rc == SQLITE_ROW ? 1 : (rc == SQLITE_DONE ? 0 : -1);
    if (found == 1) {
        const unsigned char *owner = sqlite3_column_text(stmt, 0);
        snprintf(app_id, app_id_size, "%s", owner ? (const char *)owner : "");
        *slot = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    return found;
}

int app_points_find_slot(sqlite_svc_t *db, int slot, char *app_id, size_t app_id_size, char *name,
                         size_t name_size) {
    sqlite3 *handle = handle_of(db);
    if (!handle || !app_id || !name) return -1;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(handle,
                           "SELECT app_id, name FROM app_point WHERE slot = ?1 LIMIT 1;", -1,
                           &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt, 1, slot);
    int rc = sqlite3_step(stmt);
    int found = rc == SQLITE_ROW ? 1 : (rc == SQLITE_DONE ? 0 : -1);
    if (found == 1) {
        const unsigned char *owner = sqlite3_column_text(stmt, 0);
        const unsigned char *point = sqlite3_column_text(stmt, 1);
        snprintf(app_id, app_id_size, "%s", owner ? (const char *)owner : "");
        snprintf(name, name_size, "%s", point ? (const char *)point : "");
    }
    sqlite3_finalize(stmt);
    return found;
}

int app_points_reconcile(sqlite_svc_t *db, size_t *released, char *reason, size_t reason_size) {
    if (released) *released = 0;
    sqlite3 *handle = handle_of(db);
    if (!handle) return fail(reason, reason_size, "bad_args");

    const char *orphan = "NOT IN (SELECT id FROM app_registry WHERE state != 0)";
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM app_point WHERE app_id %s;", orphan);
    if (exec_sql(handle, sql) != 0) return fail(reason, reason_size, "database error");
    size_t removed = (size_t)sqlite3_changes(handle);
    snprintf(sql, sizeof(sql), "DELETE FROM app_slot_alloc WHERE app_id %s;", orphan);
    if (exec_sql(handle, sql) != 0) return fail(reason, reason_size, "database error");
    removed += (size_t)sqlite3_changes(handle);
    if (released) *released = removed;
    if (reason && reason_size) reason[0] = '\0';
    return 0;
}
