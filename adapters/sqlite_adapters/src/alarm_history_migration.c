#include "alarm_history_internal.h"

/*
 * v3 rows have no primary key and duplicate rows are legal, so a row is
 * identified by a SHA-256 over its eight values. alarm_migration_row remembers
 * which identities reached the target; a rerun appends only new ones. Each row
 * commits together with its state entry, so a crash leaves either both or
 * neither.
 *
 * An over-long text value (entity 64, tagname 96, addition 64 bytes) aborts the
 * whole migration per the append contract; triage it by hand, never truncate.
 * The real v3 opcalarm.db is not available on this machine, so the literal
 * schema check is covered only by fixtures and must be re-verified against it.
 */
#define ALARM_MIGRATION_STATE_SQL                        \
    "CREATE TABLE IF NOT EXISTS alarm_migration_row("    \
    " source_sha256 TEXT NOT NULL,"                      \
    " row_hash TEXT NOT NULL UNIQUE);"                   \
    "CREATE TABLE IF NOT EXISTS alarm_migration_source(" \
    " source_sha256 TEXT PRIMARY KEY,"                   \
    " source_path TEXT NOT NULL,"                        \
    " source_bytes INTEGER NOT NULL,"                    \
    " row_count INTEGER NOT NULL,"                       \
    " active_rows INTEGER NOT NULL,"                     \
    " recovered_rows INTEGER NOT NULL);"

#define ALARM_MIGRATION_SELECT                                                               \
    "SELECT raise_time, recovery_time, alarm_code, reason_code, entity, tagname, addition, " \
    "status FROM alarmRealTab ORDER BY raise_time ASC, rowid ASC;"

typedef struct {
    uint64_t source_rows;
    uint64_t imported;
    uint64_t skipped;
    uint64_t active;
    uint64_t recovered;
    uint64_t imported_active;
    uint64_t imported_recovered;
    uint64_t skipped_active;
    uint64_t skipped_recovered;
} alarm_migration_counts_t;

static edge_history_result_t table_exists(sqlite3 *db, const char *name, int *exists, char *reason,
                                          size_t reason_size) {
    *exists = 0;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return db_fail(db, map_sqlite(sqlite3_errcode(db)), reason, reason_size,
                       "cannot inspect the migration state");
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    int step = sqlite3_step(stmt);
    if (step == SQLITE_ROW) {
        *exists = 1;
    } else if (step != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return db_fail(db, map_sqlite(step), reason, reason_size,
                       "cannot inspect the migration state");
    }
    sqlite3_finalize(stmt);
    return EDGE_HISTORY_OK;
}

static edge_history_result_t validate_source_schema(sqlite3 *src, char *reason,
                                                    size_t reason_size) {
    static const char *const names[] = {"raise_time", "recovery_time", "alarm_code", "reason_code",
                                        "entity",     "tagname",       "addition",   "status"};
    static const char *const types[] = {
        "BIGINT UNSIGNED", "BIGINT UNSIGNED", "INT",      "INT",
        "TEXT(64)",        "TEXT(96)",        "TEXT(64)", "TINYINT"};
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(src,
                           "SELECT count(*) FROM sqlite_master WHERE type='table' AND "
                           "name='alarmRealTab';",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return db_fail(src, map_sqlite(sqlite3_errcode(src)), reason, reason_size,
                       "cannot read the source schema");
    }
    int step = sqlite3_step(stmt);
    int present = step == SQLITE_ROW && sqlite3_column_int(stmt, 0) > 0;
    sqlite3_finalize(stmt);
    if (step != SQLITE_ROW && step != SQLITE_DONE) {
        return db_fail(src, map_sqlite(step), reason, reason_size, "cannot read the source schema");
    }
    if (!present) return fail(EDGE_HISTORY_INVALID, reason, reason_size, "alarmRealTab is missing");

    if (sqlite3_prepare_v2(src, "PRAGMA table_info(alarmRealTab);", -1, &stmt, NULL) != SQLITE_OK) {
        return db_fail(src, map_sqlite(sqlite3_errcode(src)), reason, reason_size,
                       "cannot read the source schema");
    }
    int columns = 0;
    edge_history_result_t rc = EDGE_HISTORY_OK;
    while ((step = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(stmt, 1);
        const char *type = (const char *)sqlite3_column_text(stmt, 2);
        if (columns >= 8 || !name || !type || strcmp(name, names[columns]) != 0 ||
            strcmp(type, types[columns]) != 0) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size,
                      "alarmRealTab does not match the v3 eight-column schema");
            break;
        }
        columns++;
    }
    if (rc == EDGE_HISTORY_OK && step != SQLITE_DONE) {
        rc = db_fail(src, map_sqlite(step), reason, reason_size, "cannot read the source schema");
    }
    sqlite3_finalize(stmt);
    if (rc == EDGE_HISTORY_OK && columns != 8) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size,
                    "alarmRealTab does not match the v3 eight-column schema");
    }
    return rc;
}

static edge_history_result_t source_counts(sqlite3 *src, alarm_migration_counts_t *counts,
                                           char *reason, size_t reason_size) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(src,
                           "SELECT count(*), coalesce(sum(status=1), 0), "
                           "coalesce(sum(status=0), 0) FROM alarmRealTab;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return db_fail(src, map_sqlite(sqlite3_errcode(src)), reason, reason_size,
                       "cannot count the source rows");
    }
    int step = sqlite3_step(stmt);
    edge_history_result_t rc = EDGE_HISTORY_IO;
    if (step == SQLITE_ROW) {
        counts->source_rows = (uint64_t)sqlite3_column_int64(stmt, 0);
        counts->active = (uint64_t)sqlite3_column_int64(stmt, 1);
        counts->recovered = (uint64_t)sqlite3_column_int64(stmt, 2);
        rc = EDGE_HISTORY_OK;
    }
    sqlite3_finalize(stmt);
    if (rc != EDGE_HISTORY_OK) {
        return db_fail(src, rc, reason, reason_size, "cannot count the source rows");
    }
    if (counts->active + counts->recovered != counts->source_rows) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size,
                    "alarmRealTab has rows whose status is not 0 or 1");
    }
    return EDGE_HISTORY_OK;
}

static edge_history_result_t bad_source_row(uint64_t row_number, const char *what, char *reason,
                                            size_t reason_size) {
    if (reason && reason_size) {
        snprintf(reason, reason_size, "source row %" PRIu64 " %s", row_number, what);
    }
    return EDGE_HISTORY_INVALID;
}

/* v3 text columns are C strings; an embedded NUL would not round-trip. */
static int migration_text(sqlite3_stmt *stmt, int column, char *dst, size_t cap) {
    const unsigned char *text = sqlite3_column_text(stmt, column);
    int bytes = sqlite3_column_bytes(stmt, column);
    if (!text || bytes < 0 || (size_t)bytes >= cap || memchr(text, '\0', (size_t)bytes) != NULL) {
        return -1;
    }
    memcpy(dst, text, (size_t)bytes);
    dst[bytes] = '\0';
    return 0;
}

static edge_history_result_t read_migration_row(sqlite3_stmt *stmt, uint64_t row_number,
                                                edge_alarm_record_t *record, char *reason,
                                                size_t reason_size) {
    memset(record, 0, sizeof(*record));
    for (int column = 0; column < 4; column++) {
        if (sqlite3_column_type(stmt, column) != SQLITE_INTEGER) {
            return bad_source_row(row_number, "has a non-integer key column", reason, reason_size);
        }
    }
    if (sqlite3_column_type(stmt, 7) != SQLITE_INTEGER) {
        return bad_source_row(row_number, "has a non-integer status", reason, reason_size);
    }
    sqlite3_int64 raise_time = sqlite3_column_int64(stmt, 0);
    sqlite3_int64 recovery_time = sqlite3_column_int64(stmt, 1);
    sqlite3_int64 alarm_code = sqlite3_column_int64(stmt, 2);
    sqlite3_int64 reason_code = sqlite3_column_int64(stmt, 3);
    sqlite3_int64 status = sqlite3_column_int64(stmt, 7);
    if (raise_time < 0 || recovery_time < 0 || alarm_code < (sqlite3_int64)INT32_MIN ||
        alarm_code > (sqlite3_int64)INT32_MAX || reason_code < (sqlite3_int64)INT32_MIN ||
        reason_code > (sqlite3_int64)INT32_MAX || (status != 0 && status != 1)) {
        return bad_source_row(row_number, "has a value outside the v3 range", reason, reason_size);
    }
    record->raise_time_ms = (int64_t)raise_time;
    record->recovery_time_ms = (int64_t)recovery_time;
    record->alarm_code = (int32_t)alarm_code;
    record->reason_code = (int32_t)reason_code;
    record->status = (uint8_t)status;
    if (migration_text(stmt, 4, record->entity, sizeof(record->entity)) != 0 ||
        migration_text(stmt, 5, record->tagname, sizeof(record->tagname)) != 0 ||
        migration_text(stmt, 6, record->addition, sizeof(record->addition)) != 0) {
        return bad_source_row(row_number, "has a missing, over-long or non-text column", reason,
                              reason_size);
    }
    if ((record->status == EDGE_ALARM_STATUS_ACTIVE && record->recovery_time_ms != 0) ||
        (record->status == EDGE_ALARM_STATUS_RECOVERY && record->recovery_time_ms <= 0)) {
        return bad_source_row(row_number, "does not match the v3 active/recovered contract", reason,
                              reason_size);
    }
    return EDGE_HISTORY_OK;
}

static int migration_row_hash(const edge_alarm_record_t *record,
                              char hex[EDGE_HISTORY_SHA256_SIZE]) {
    edge_sha256_t *ctx = edge_sha256_start();
    if (!ctx) return -1;
    int ok = fp_update_u64(ctx, (uint64_t)record->raise_time_ms) == 0 &&
             fp_update_u64(ctx, (uint64_t)record->recovery_time_ms) == 0 &&
             fp_update_u32(ctx, (uint32_t)record->alarm_code) == 0 &&
             fp_update_u32(ctx, (uint32_t)record->reason_code) == 0 &&
             fp_update_text(ctx, (const unsigned char *)record->entity,
                            (int)strlen(record->entity)) == 0 &&
             fp_update_text(ctx, (const unsigned char *)record->tagname,
                            (int)strlen(record->tagname)) == 0 &&
             fp_update_text(ctx, (const unsigned char *)record->addition,
                            (int)strlen(record->addition)) == 0 &&
             fp_update_u32(ctx, (uint32_t)record->status) == 0;
    int rc = ok ? edge_sha256_finish_hex(ctx, hex) : -1;
    edge_sha256_free(ctx);
    return rc;
}

static edge_history_result_t migration_lookup(alarm_history_store_t *store, sqlite3_stmt *lookup,
                                              const char *hash, int *found, char *reason,
                                              size_t reason_size) {
    *found = 0;
    sqlite3_bind_text(lookup, 1, hash, -1, SQLITE_STATIC);
    int step = sqlite3_step(lookup);
    if (step == SQLITE_ROW) {
        *found = 1;
    } else if (step != SQLITE_DONE) {
        sqlite3_reset(lookup);
        return db_fail(store->db, map_sqlite(step), reason, reason_size,
                       "cannot read the migration state");
    }
    sqlite3_reset(lookup);
    return EDGE_HISTORY_OK;
}

static edge_history_result_t migration_import(edge_alarm_history_port_t *dst,
                                              alarm_history_store_t *store,
                                              const edge_alarm_record_t *record,
                                              const char *source_sha, const char *hash,
                                              char *reason, size_t reason_size) {
    edge_history_result_t rc = exec_sql(store->db, "BEGIN IMMEDIATE;");
    if (rc != EDGE_HISTORY_OK) {
        return fail(rc, reason, reason_size, "cannot start the import transaction");
    }
    rc = dst->append(dst->ctx, record);
    if (rc != EDGE_HISTORY_OK) {
        edge_history_result_t result =
            db_fail(store->db, rc, reason, reason_size, "destination rejected a source row");
        (void)exec_sql(store->db, "ROLLBACK;");
        return result;
    }
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(store->db,
                           "INSERT INTO alarm_migration_row(source_sha256, row_hash) "
                           "VALUES (?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        edge_history_result_t result =
            db_fail(store->db, map_sqlite(sqlite3_errcode(store->db)), reason, reason_size,
                    "cannot record the imported row");
        (void)exec_sql(store->db, "ROLLBACK;");
        return result;
    }
    sqlite3_bind_text(stmt, 1, source_sha, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hash, -1, SQLITE_STATIC);
    int step = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (step != SQLITE_DONE) {
        edge_history_result_t result = db_fail(store->db, map_sqlite(step), reason, reason_size,
                                               "cannot record the imported row");
        (void)exec_sql(store->db, "ROLLBACK;");
        return result;
    }
    rc = exec_sql(store->db, "COMMIT;");
    if (rc != EDGE_HISTORY_OK) {
        (void)exec_sql(store->db, "ROLLBACK;");
        return fail(rc, reason, reason_size, "cannot commit the imported row");
    }
    return EDGE_HISTORY_OK;
}

static edge_history_result_t migration_state_count(alarm_history_store_t *store,
                                                   const char *source_sha, uint64_t *count,
                                                   char *reason, size_t reason_size) {
    *count = 0;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(store->db,
                           "SELECT count(*) FROM alarm_migration_row WHERE source_sha256=?;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return db_fail(store->db, map_sqlite(sqlite3_errcode(store->db)), reason, reason_size,
                       "cannot count the migration state");
    }
    sqlite3_bind_text(stmt, 1, source_sha, -1, SQLITE_STATIC);
    int step = sqlite3_step(stmt);
    edge_history_result_t rc = EDGE_HISTORY_IO;
    if (step == SQLITE_ROW) {
        *count = (uint64_t)sqlite3_column_int64(stmt, 0);
        rc = EDGE_HISTORY_OK;
    }
    sqlite3_finalize(stmt);
    if (rc != EDGE_HISTORY_OK) {
        return db_fail(store->db, rc, reason, reason_size, "cannot count the migration state");
    }
    return rc;
}

static edge_history_result_t migration_verify(alarm_history_store_t *store, const char *source_sha,
                                              const alarm_migration_counts_t *counts,
                                              uint64_t state_before, int state_exists, int dry_run,
                                              char *reason, size_t reason_size) {
    if (counts->imported + counts->skipped != counts->source_rows) {
        return fail(EDGE_HISTORY_VERIFY, reason, reason_size,
                    "imported and skipped rows do not add up to the source rows");
    }
    if (counts->imported_active + counts->skipped_active != counts->active ||
        counts->imported_recovered + counts->skipped_recovered != counts->recovered) {
        return fail(EDGE_HISTORY_VERIFY, reason, reason_size,
                    "active and recovered counts do not match the source rows");
    }
    if (!state_exists) return EDGE_HISTORY_OK;
    uint64_t state_after = 0;
    edge_history_result_t rc =
        migration_state_count(store, source_sha, &state_after, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;
    uint64_t expected_after = state_before;
    if (!dry_run) expected_after += counts->imported;
    if (state_after != expected_after) {
        return fail(EDGE_HISTORY_VERIFY, reason, reason_size,
                    dry_run ? "dry run changed the target migration state"
                            : "target migration state does not match the imported rows");
    }
    return EDGE_HISTORY_OK;
}

static edge_history_result_t migration_record_source(
    alarm_history_store_t *store, const char *src_path, const char *source_sha, uint64_t file_bytes,
    const alarm_migration_counts_t *counts, int dry_run, char *reason, size_t reason_size) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(store->db,
                           "SELECT row_count, active_rows, recovered_rows FROM "
                           "alarm_migration_source WHERE source_sha256=?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return db_fail(store->db, map_sqlite(sqlite3_errcode(store->db)), reason, reason_size,
                       "cannot read the migration source identity");
    }
    sqlite3_bind_text(stmt, 1, source_sha, -1, SQLITE_STATIC);
    int step = sqlite3_step(stmt);
    int found = step == SQLITE_ROW;
    edge_history_result_t rc = EDGE_HISTORY_OK;
    if (found) {
        uint64_t rows = (uint64_t)sqlite3_column_int64(stmt, 0);
        uint64_t active = (uint64_t)sqlite3_column_int64(stmt, 1);
        uint64_t recovered = (uint64_t)sqlite3_column_int64(stmt, 2);
        if (rows != counts->source_rows || active != counts->active ||
            recovered != counts->recovered) {
            rc = fail(EDGE_HISTORY_VERIFY, reason, reason_size,
                      "recorded source row counts do not match the source");
        }
    } else if (step != SQLITE_DONE) {
        rc = db_fail(store->db, map_sqlite(step), reason, reason_size,
                     "cannot read the migration source identity");
    }
    sqlite3_finalize(stmt);
    if (rc != EDGE_HISTORY_OK || found || dry_run) return rc;

    if (sqlite3_prepare_v2(store->db,
                           "INSERT INTO alarm_migration_source(source_sha256, source_path, "
                           "source_bytes, row_count, active_rows, recovered_rows) "
                           "VALUES (?, ?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return db_fail(store->db, map_sqlite(sqlite3_errcode(store->db)), reason, reason_size,
                       "cannot record the migration source identity");
    }
    sqlite3_bind_text(stmt, 1, source_sha, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, src_path, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)file_bytes);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)counts->source_rows);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)counts->active);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)counts->recovered);
    step = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (step != SQLITE_DONE) {
        return db_fail(store->db, map_sqlite(step), reason, reason_size,
                       "cannot record the migration source identity");
    }
    return EDGE_HISTORY_OK;
}

edge_history_result_t alarm_history_sqlite_migrate_v3(const char *src_db_path,
                                                      edge_alarm_history_port_t *dst, int dry_run,
                                                      alarm_history_migration_result_t *out,
                                                      char *reason, size_t reason_size) {
    if (reason && reason_size) reason[0] = '\0';
    if (out) memset(out, 0, sizeof(*out));
    if (!out || !dst || !dst->ctx || !dst->append || !src_db_path || src_db_path[0] == '\0') {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "migration request is invalid");
    }
    alarm_history_store_t *store = (alarm_history_store_t *)dst->ctx;
    if (!store->db) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "destination is closed");
    }

    char file_sha[EDGE_HISTORY_SHA256_SIZE];
    unsigned long long file_bytes = 0;
    if (edge_sha256_file(src_db_path, file_sha, &file_bytes) != 0) {
        return fail(EDGE_HISTORY_IO, reason, reason_size, "cannot read the source database file");
    }

    sqlite3 *src = NULL;
    int open_rc = sqlite3_open_v2(src_db_path, &src, SQLITE_OPEN_READONLY, NULL);
    if (open_rc != SQLITE_OK) {
        edge_history_result_t rc = map_sqlite(open_rc);
        if (src) sqlite3_close(src);
        return fail(rc, reason, reason_size, "cannot open the source database read-only");
    }
    sqlite3_busy_timeout(src, 5000);

    alarm_migration_counts_t counts;
    memset(&counts, 0, sizeof(counts));
    edge_history_result_t rc = quick_check(src);
    if (rc != EDGE_HISTORY_OK) {
        sqlite3_close(src);
        return fail(rc, reason, reason_size, "source database failed quick_check");
    }
    rc = validate_source_schema(src, reason, reason_size);
    if (rc == EDGE_HISTORY_OK) rc = source_counts(src, &counts, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) {
        sqlite3_close(src);
        return rc;
    }

    int rows_state = 0;
    int source_state = 0;
    if (dry_run) {
        rc = table_exists(store->db, "alarm_migration_row", &rows_state, reason, reason_size);
        if (rc == EDGE_HISTORY_OK) {
            rc = table_exists(store->db, "alarm_migration_source", &source_state, reason,
                              reason_size);
        }
    } else {
        rc = exec_sql(store->db, ALARM_MIGRATION_STATE_SQL);
        if (rc == EDGE_HISTORY_OK) {
            rows_state = 1;
            source_state = 1;
        } else {
            rc = db_fail(store->db, rc, reason, reason_size,
                         "cannot create the migration state tables");
        }
    }
    if (rc != EDGE_HISTORY_OK) {
        sqlite3_close(src);
        return rc;
    }

    uint64_t state_before = 0;
    if (rows_state) {
        rc = migration_state_count(store, file_sha, &state_before, reason, reason_size);
    }
    sqlite3_stmt *lookup = NULL;
    sqlite3_stmt *select = NULL;
    if (rc == EDGE_HISTORY_OK && rows_state &&
        sqlite3_prepare_v2(store->db, "SELECT 1 FROM alarm_migration_row WHERE row_hash=?;", -1,
                           &lookup, NULL) != SQLITE_OK) {
        rc = db_fail(store->db, map_sqlite(sqlite3_errcode(store->db)), reason, reason_size,
                     "cannot read the migration state");
    }
    if (rc == EDGE_HISTORY_OK &&
        sqlite3_prepare_v2(src, ALARM_MIGRATION_SELECT, -1, &select, NULL) != SQLITE_OK) {
        rc = db_fail(src, map_sqlite(sqlite3_errcode(src)), reason, reason_size,
                     "cannot read the source rows");
    }

    uint64_t row_number = 0;
    int step = SQLITE_ROW;
    while (rc == EDGE_HISTORY_OK && (step = sqlite3_step(select)) == SQLITE_ROW) {
        row_number++;
        edge_alarm_record_t record;
        rc = read_migration_row(select, row_number, &record, reason, reason_size);
        if (rc != EDGE_HISTORY_OK) break;
        char hash[EDGE_HISTORY_SHA256_SIZE];
        if (migration_row_hash(&record, hash) != 0) {
            rc = fail(EDGE_HISTORY_IO, reason, reason_size, "cannot hash a source row");
            break;
        }
        int found = 0;
        if (rows_state) {
            rc = migration_lookup(store, lookup, hash, &found, reason, reason_size);
            if (rc != EDGE_HISTORY_OK) break;
        }
        if (found) {
            counts.skipped++;
            if (record.status == EDGE_ALARM_STATUS_ACTIVE) {
                counts.skipped_active++;
            } else {
                counts.skipped_recovered++;
            }
            continue;
        }
        if (dry_run) {
            counts.imported++;
            if (record.status == EDGE_ALARM_STATUS_ACTIVE) {
                counts.imported_active++;
            } else {
                counts.imported_recovered++;
            }
            continue;
        }
        rc = migration_import(dst, store, &record, file_sha, hash, reason, reason_size);
        if (rc != EDGE_HISTORY_OK) break;
        counts.imported++;
        if (record.status == EDGE_ALARM_STATUS_ACTIVE) {
            counts.imported_active++;
        } else {
            counts.imported_recovered++;
        }
    }
    if (rc == EDGE_HISTORY_OK && step != SQLITE_DONE) {
        rc = db_fail(src, map_sqlite(step), reason, reason_size, "cannot read the source rows");
    }
    sqlite3_finalize(lookup);
    sqlite3_finalize(select);
    sqlite3_close(src);

    out->source_rows = counts.source_rows;
    out->imported_rows = counts.imported;
    out->skipped_rows = counts.skipped;
    out->active_rows = counts.active;
    out->recovered_rows = counts.recovered;
    if (rc != EDGE_HISTORY_OK) return rc;

    rc = migration_verify(store, file_sha, &counts, state_before, rows_state, dry_run, reason,
                          reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;
    if (!source_state) return EDGE_HISTORY_OK;
    return migration_record_source(store, src_db_path, file_sha, (uint64_t)file_bytes, &counts,
                                   dry_run, reason, reason_size);
}
