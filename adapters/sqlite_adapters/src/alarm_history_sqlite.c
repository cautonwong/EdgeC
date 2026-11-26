/**
 * @file alarm_history_sqlite.c
 * @brief SQLite alarm history store: schema, append/recover/query, export, retain.
 *
 * The alarm table keeps the v3 eight columns verbatim. rowid is the row
 * identity because v3 has no primary key and duplicate raise times are legal.
 * The history database is a separate file and a separate connection; it is
 * not part of the control database migration chain (`k_migrations`).
 *
 * The SnF `telemetry` table is deliberately not reused: it is an at-least-once
 * delivery queue without point/contract_version columns, while history is a
 * read-side query contract.
 */

#include "alarm_history_sqlite.h"

#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "alarm_history_internal.h"
#include "edge_digest.h"
#include "edge_tar.h"
#include "history_bundle.h"

#define ALARM_SCHEMA_VERSION 1
#define ALARM_SQL_MAX 512
#define ALARM_DEFAULT_CHUNK 1000u
#define ALARM_DB_PATH_MAX 479

#define ALARM_CSV_HEADER \
    "raise_time,recovery_time,alarm_code,reason_code,entity,tagname,addition,status\r\n"

#define ALARM_EXPORT_SELECT                                                                   \
    "SELECT raise_time, recovery_time, alarm_code, reason_code, entity, tagname, addition, "  \
    "status, rowid FROM alarmRealTab WHERE (status=0 AND recovery_time>0 AND "                \
    "recovery_time>=?1 AND recovery_time<=?2) OR (?3 AND status=1) ORDER BY raise_time ASC, " \
    "rowid ASC;"

static const char kSchema[] =
    "CREATE TABLE IF NOT EXISTS alarmRealTab ("
    " raise_time BIGINT UNSIGNED,"
    " recovery_time BIGINT UNSIGNED,"
    " alarm_code INT,"
    " reason_code INT,"
    " entity TEXT(64),"
    " tagname TEXT(96),"
    " addition TEXT(64),"
    " status TINYINT"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_alarm_status_raise ON alarmRealTab(status, raise_time DESC);"
    "CREATE INDEX IF NOT EXISTS idx_alarm_recover ON alarmRealTab(status, alarm_code, "
    "reason_code, raise_time);";

static edge_history_result_t io_result(void) {
    if (errno == ENOSPC || errno == EDQUOT) return EDGE_HISTORY_NO_SPACE;
    return EDGE_HISTORY_IO;
}

static edge_history_result_t pragma_int(sqlite3 *db, const char *sql, int *out) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(db));
    }
    edge_history_result_t rc = EDGE_HISTORY_IO;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *out = sqlite3_column_int(stmt, 0);
        rc = EDGE_HISTORY_OK;
    } else {
        rc = map_sqlite(sqlite3_errcode(db));
    }
    sqlite3_finalize(stmt);
    return rc;
}

static edge_history_result_t pragma_u64(sqlite3 *db, const char *sql, uint64_t *out) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(db));
    }
    edge_history_result_t rc = EDGE_HISTORY_IO;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sqlite3_int64 value = sqlite3_column_int64(stmt, 0);
        *out = value > 0 ? (uint64_t)value : 0;
        rc = EDGE_HISTORY_OK;
    } else {
        rc = map_sqlite(sqlite3_errcode(db));
    }
    sqlite3_finalize(stmt);
    return rc;
}

/* Keep the corrupt file for forensics; move the sidecars with it so a later
 * open on the original path cannot pair a stale WAL with a new database. */
static void isolate_corrupt(const char *db_path) {
    char target[ALARM_PATH_MAX];
    if (snprintf(target, sizeof(target), "%s.corrupt-%lld", db_path, (long long)time(NULL)) >=
        (int)sizeof(target)) {
        return;
    }
    if (rename(db_path, target) != 0) return;
    static const char *const suffixes[] = {"-wal", "-shm"};
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        char side[ALARM_PATH_MAX];
        char side_target[ALARM_PATH_MAX];
        if (snprintf(side, sizeof(side), "%s%s", db_path, suffixes[i]) >= (int)sizeof(side) ||
            snprintf(side_target, sizeof(side_target), "%s%s", target, suffixes[i]) >=
                (int)sizeof(side_target)) {
            continue;
        }
        (void)rename(side, side_target);
    }
}

static int text_fits(const char *text, size_t cap) {
    return memchr(text, '\0', cap) != NULL;
}

static int copy_column(sqlite3_stmt *stmt, int column, char *dst, size_t cap) {
    const unsigned char *text = sqlite3_column_text(stmt, column);
    int bytes = sqlite3_column_bytes(stmt, column);
    if (!text) {
        if (cap) dst[0] = '\0';
        return 0;
    }
    if (bytes < 0 || (size_t)bytes >= cap) return -1;
    memcpy(dst, text, (size_t)bytes);
    dst[bytes] = '\0';
    return 0;
}

static edge_history_result_t alarm_append(void *ctx, const edge_alarm_record_t *record) {
    alarm_history_store_t *store = (alarm_history_store_t *)ctx;
    if (!store || !store->db || !record) return EDGE_HISTORY_INVALID;
    if (record->status > EDGE_ALARM_STATUS_ACTIVE) return EDGE_HISTORY_INVALID;
    if (record->raise_time_ms < 0 || record->recovery_time_ms < 0) return EDGE_HISTORY_INVALID;
    if (record->status == EDGE_ALARM_STATUS_ACTIVE && record->recovery_time_ms != 0) {
        return EDGE_HISTORY_INVALID;
    }
    if (record->status == EDGE_ALARM_STATUS_RECOVERY && record->recovery_time_ms <= 0) {
        return EDGE_HISTORY_INVALID;
    }
    if (!text_fits(record->entity, sizeof(record->entity)) ||
        !text_fits(record->tagname, sizeof(record->tagname)) ||
        !text_fits(record->addition, sizeof(record->addition))) {
        return EDGE_HISTORY_INVALID;
    }
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(store->db,
                           "INSERT INTO alarmRealTab (raise_time, recovery_time, alarm_code, "
                           "reason_code, entity, tagname, addition, status) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(store->db));
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)record->raise_time_ms);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)record->recovery_time_ms);
    sqlite3_bind_int(stmt, 3, record->alarm_code);
    sqlite3_bind_int(stmt, 4, record->reason_code);
    sqlite3_bind_text(stmt, 5, record->entity, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, record->tagname, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, record->addition, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, (int)record->status);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? EDGE_HISTORY_OK : map_sqlite(rc);
}

static edge_history_result_t alarm_recover(void *ctx, int32_t alarm_code, int32_t reason_code,
                                           int64_t raise_time_ms, int64_t recovery_time_ms) {
    alarm_history_store_t *store = (alarm_history_store_t *)ctx;
    if (!store || !store->db || recovery_time_ms <= 0 || raise_time_ms < 0) {
        return EDGE_HISTORY_INVALID;
    }
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(store->db,
                           "UPDATE alarmRealTab SET status=0, recovery_time=? "
                           "WHERE status=1 AND alarm_code=? AND reason_code=? AND raise_time=?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(store->db));
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)recovery_time_ms);
    sqlite3_bind_int(stmt, 2, alarm_code);
    sqlite3_bind_int(stmt, 3, reason_code);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)raise_time_ms);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? EDGE_HISTORY_OK : map_sqlite(rc);
}

static edge_history_result_t alarm_query(void *ctx, int active, uint32_t offset, uint32_t limit,
                                         edge_alarm_record_t *out, uint32_t cap,
                                         uint32_t *out_count, uint32_t *total) {
    alarm_history_store_t *store = (alarm_history_store_t *)ctx;
    if (!store || !store->db || !out || !out_count || !total || (active != 0 && active != 1)) {
        return EDGE_HISTORY_INVALID;
    }
    *out_count = 0;
    *total = 0;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(store->db, "SELECT count(*) FROM alarmRealTab WHERE status=?;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(store->db));
    }
    sqlite3_bind_int(stmt, 1, active);
    sqlite3_int64 matched = 0;
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) matched = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_ROW) return map_sqlite(rc);
    *total = matched > (sqlite3_int64)UINT32_MAX ? UINT32_MAX : (uint32_t)matched;

    if (offset > EDGE_ALARM_QUERY_OFFSET_MAX) offset = EDGE_ALARM_QUERY_OFFSET_MAX;
    uint32_t page = limit > EDGE_ALARM_QUERY_PAGE_MAX ? EDGE_ALARM_QUERY_PAGE_MAX : limit;
    if (page > cap) page = cap;
    if (page == 0) return EDGE_HISTORY_OK;

    if (sqlite3_prepare_v2(store->db,
                           "SELECT raise_time, recovery_time, alarm_code, reason_code, entity, "
                           "tagname, addition, status FROM alarmRealTab WHERE status=? "
                           "ORDER BY raise_time DESC, rowid DESC LIMIT ? OFFSET ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(store->db));
    }
    sqlite3_bind_int(stmt, 1, active);
    sqlite3_bind_int(stmt, 2, (int)page);
    sqlite3_bind_int(stmt, 3, (int)offset);
    uint32_t count = 0;
    int step = SQLITE_ROW;
    while (count < page) {
        step = sqlite3_step(stmt);
        if (step != SQLITE_ROW) break;
        edge_alarm_record_t *row = &out[count];
        memset(row, 0, sizeof(*row));
        row->raise_time_ms = (int64_t)sqlite3_column_int64(stmt, 0);
        row->recovery_time_ms = (int64_t)sqlite3_column_int64(stmt, 1);
        row->alarm_code = (int32_t)sqlite3_column_int(stmt, 2);
        row->reason_code = (int32_t)sqlite3_column_int(stmt, 3);
        row->status = (uint8_t)sqlite3_column_int(stmt, 7);
        if (copy_column(stmt, 4, row->entity, sizeof(row->entity)) != 0 ||
            copy_column(stmt, 5, row->tagname, sizeof(row->tagname)) != 0 ||
            copy_column(stmt, 6, row->addition, sizeof(row->addition)) != 0) {
            sqlite3_finalize(stmt);
            return EDGE_HISTORY_INVALID;
        }
        count++;
    }
    sqlite3_finalize(stmt);
    if (step != SQLITE_ROW && step != SQLITE_DONE) return map_sqlite(step);
    *out_count = count;
    return EDGE_HISTORY_OK;
}

/* Length-prefixed fields plus the rowid, so no delimiter appears in the hash
 * input and the same row always hashes the same regardless of text content. */
static int fp_update_row(edge_sha256_t *ctx, sqlite3_stmt *stmt) {
    if (fp_update_u64(ctx, (uint64_t)sqlite3_column_int64(stmt, 8)) != 0 ||
        fp_update_u64(ctx, (uint64_t)sqlite3_column_int64(stmt, 0)) != 0 ||
        fp_update_u64(ctx, (uint64_t)sqlite3_column_int64(stmt, 1)) != 0 ||
        fp_update_u64(ctx, (uint64_t)sqlite3_column_int64(stmt, 2)) != 0 ||
        fp_update_u64(ctx, (uint64_t)sqlite3_column_int64(stmt, 3)) != 0) {
        return -1;
    }
    for (int column = 4; column <= 6; column++) {
        if (fp_update_text(ctx, sqlite3_column_text(stmt, column),
                           sqlite3_column_bytes(stmt, column)) != 0) {
            return -1;
        }
    }
    return fp_update_u32(ctx, (uint32_t)sqlite3_column_int(stmt, 7));
}

static int csv_text(FILE *out, const unsigned char *text, int bytes) {
    if (bytes <= 0 || !text) return 0;
    int quote = 0;
    for (int i = 0; i < bytes; i++) {
        if (text[i] == ',' || text[i] == '"' || text[i] == '\r' || text[i] == '\n') {
            quote = 1;
            break;
        }
    }
    if (quote && fputc('"', out) == EOF) return -1;
    for (int i = 0; i < bytes; i++) {
        if (text[i] == '"' && fputc('"', out) == EOF) return -1;
        if (fputc((int)text[i], out) == EOF) return -1;
    }
    if (quote && fputc('"', out) == EOF) return -1;
    return 0;
}

static int csv_row(FILE *out, sqlite3_stmt *stmt) {
    int written =
        fprintf(out, "%" PRId64 ",%" PRId64 ",%" PRId32 ",%" PRId32 ",",
                (int64_t)sqlite3_column_int64(stmt, 0), (int64_t)sqlite3_column_int64(stmt, 1),
                (int32_t)sqlite3_column_int(stmt, 2), (int32_t)sqlite3_column_int(stmt, 3));
    if (written < 0) return -1;
    for (int column = 4; column <= 6; column++) {
        if (csv_text(out, sqlite3_column_text(stmt, column), sqlite3_column_bytes(stmt, column)) !=
                0 ||
            fputc(',', out) == EOF) {
            return -1;
        }
    }
    return fprintf(out, "%d\r\n", sqlite3_column_int(stmt, 7)) < 0 ? -1 : 0;
}

/* max_recovery covers recovered rows only; it is the alarm watermark. */
static edge_history_result_t fp_query(sqlite3 *db, int64_t from_ms, int64_t to_ms,
                                      int include_active, FILE *csv, uint64_t *rows,
                                      int64_t *max_recovery,
                                      char fingerprint[EDGE_HISTORY_SHA256_SIZE], char *reason,
                                      size_t reason_size) {
    if (rows) *rows = 0;
    if (max_recovery) *max_recovery = 0;
    edge_sha256_t *ctx = edge_sha256_start();
    if (!ctx) return fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, ALARM_EXPORT_SELECT, -1, &stmt, NULL) != SQLITE_OK) {
        edge_sha256_free(ctx);
        return db_fail(db, map_sqlite(sqlite3_errcode(db)), reason, reason_size, "query failed");
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)from_ms);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)to_ms);
    sqlite3_bind_int(stmt, 3, include_active ? 1 : 0);
    uint64_t count = 0;
    int64_t max_recovery_value = 0;
    edge_history_result_t rc = EDGE_HISTORY_OK;
    int step = SQLITE_ROW;
    while ((step = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (csv && csv_row(csv, stmt) != 0) {
            rc = fail(io_result(), reason, reason_size, "csv write failed");
            break;
        }
        if (fp_update_row(ctx, stmt) != 0) {
            rc = fail(EDGE_HISTORY_IO, reason, reason_size, "digest update failed");
            break;
        }
        int64_t recovery = (int64_t)sqlite3_column_int64(stmt, 1);
        if (sqlite3_column_int(stmt, 7) == 0 && recovery > max_recovery_value) {
            max_recovery_value = recovery;
        }
        count++;
    }
    if (rc == EDGE_HISTORY_OK && step != SQLITE_DONE) {
        rc = db_fail(db, map_sqlite(sqlite3_errcode(db)), reason, reason_size, "query failed");
    }
    sqlite3_finalize(stmt);
    if (rc == EDGE_HISTORY_OK) {
        if (edge_sha256_finish_hex(ctx, fingerprint) != 0) {
            rc = fail(EDGE_HISTORY_IO, reason, reason_size, "digest final failed");
        } else {
            if (rows) *rows = count;
            if (max_recovery) *max_recovery = max_recovery_value;
        }
    }
    edge_sha256_free(ctx);
    return rc;
}

static edge_history_result_t snapshot_db(sqlite3 *src, const char *dst, char *reason,
                                         size_t reason_size) {
    sqlite3 *snap = NULL;
    int rc = sqlite3_open_v2(dst, &snap, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        if (snap) sqlite3_close(snap);
        return fail(map_sqlite(rc), reason, reason_size, "cannot create snapshot");
    }
    sqlite3_backup *backup = sqlite3_backup_init(snap, "main", src, "main");
    if (!backup) {
        edge_history_result_t result = db_fail(snap, map_sqlite(sqlite3_errcode(snap)), reason,
                                               reason_size, "snapshot init failed");
        sqlite3_close(snap);
        return result;
    }
    int step = sqlite3_backup_step(backup, -1);
    int finish = sqlite3_backup_finish(backup);
    sqlite3_close(snap);
    if (step != SQLITE_DONE || finish != SQLITE_OK) {
        return fail(EDGE_HISTORY_IO, reason, reason_size, "database snapshot failed");
    }
    (void)chmod(dst, 0600);
    return EDGE_HISTORY_OK;
}

static int has_suffix(const char *text, const char *suffix) {
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);
    return text_len >= suffix_len && strcmp(text + text_len - suffix_len, suffix) == 0;
}

/* Unverifiable bundles are skipped; they cannot prove coverage. */
static edge_history_result_t load_cover(alarm_history_store_t *store, const char *dir, int64_t cap,
                                        uint64_t *windows, char *reason, size_t reason_size) {
    *windows = 0;
    edge_history_result_t rc =
        exec_sql(store->db, "DROP TABLE IF EXISTS temp.history_cover;"
                            "CREATE TEMP TABLE history_cover("
                            "from_ms INTEGER NOT NULL, to_ms INTEGER NOT NULL);");
    if (rc != EDGE_HISTORY_OK) {
        return fail(rc, reason, reason_size, "cannot create the cover table");
    }
    DIR *handle = opendir(dir);
    if (!handle) return fail(EDGE_HISTORY_IO, reason, reason_size, "cannot open export dir");
    sqlite3_stmt *insert = NULL;
    if (sqlite3_prepare_v2(store->db,
                           "INSERT INTO history_cover(from_ms, to_ms) VALUES (?, ?);", -1, &insert,
                           NULL) != SQLITE_OK) {
        closedir(handle);
        return db_fail(store->db, map_sqlite(sqlite3_errcode(store->db)), reason, reason_size,
                       "cover insert prepare failed");
    }
    struct dirent *entry = NULL;
    while ((entry = readdir(handle)) != NULL) {
        const char *name = entry->d_name;
        if (strncmp(name, "history-alarm-", 14) != 0 || !has_suffix(name, ".tar")) continue;
        char path[ALARM_PATH_MAX];
        if (snprintf(path, sizeof(path), "%s/%s", dir, name) >= (int)sizeof(path)) continue;
        edge_history_bundle_info_t bundle;
        if (edge_history_bundle_verify(path, "alarm", ALARM_SCHEMA_VERSION, &bundle, reason,
                                       reason_size) != EDGE_HISTORY_OK) {
            if (reason && reason_size) reason[0] = '\0';
            continue;
        }
        int64_t from = bundle.from_ms;
        int64_t to = bundle.to_ms;
        if (cap > 0 && to > cap) to = cap;
        if (from > to) continue;
        char fingerprint[EDGE_HISTORY_SHA256_SIZE];
        uint64_t rows = 0;
        rc = fp_query(store->db, bundle.from_ms, bundle.to_ms, bundle.includes_active, NULL, &rows,
                      NULL, fingerprint, reason, reason_size);
        if (rc == EDGE_HISTORY_OK && rows > 0 &&
            strcmp(fingerprint, bundle.source_fingerprint) != 0) {
            rc = fail(EDGE_HISTORY_CONFLICT, reason, reason_size,
                      "source changed since a verified bundle");
        }
        if (rc != EDGE_HISTORY_OK) break;
        sqlite3_bind_int64(insert, 1, (sqlite3_int64)from);
        sqlite3_bind_int64(insert, 2, (sqlite3_int64)to);
        if (sqlite3_step(insert) != SQLITE_DONE) {
            rc = db_fail(store->db, map_sqlite(sqlite3_errcode(store->db)), reason, reason_size,
                         "cover insert failed");
            break;
        }
        sqlite3_reset(insert);
        (*windows)++;
    }
    sqlite3_finalize(insert);
    closedir(handle);
    return rc;
}

static edge_history_result_t fill_export_info(const char *path,
                                              const edge_history_bundle_info_t *bundle,
                                              edge_history_export_info_t *info, int already,
                                              char *reason, size_t reason_size) {
    struct stat st;
    if (stat(path, &st) != 0) return fail(io_result(), reason, reason_size, "bundle is missing");
    info->rows = bundle->row_count;
    info->watermark_ms = bundle->watermark_ms;
    info->bytes = (unsigned long long)st.st_size;
    snprintf(info->source_fingerprint, sizeof(info->source_fingerprint), "%s",
             bundle->source_fingerprint);
    if (edge_sha256_file(path, info->bundle_sha256, NULL) != 0) {
        return fail(io_result(), reason, reason_size, "bundle digest failed");
    }
    info->already = already;
    return EDGE_HISTORY_OK;
}

static edge_history_result_t alarm_export(void *ctx, const edge_history_export_req_t *req,
                                          edge_history_export_info_t *info, char *reason,
                                          size_t reason_size) {
    alarm_history_store_t *store = (alarm_history_store_t *)ctx;
    if (!store || !store->db || !req || !info) return EDGE_HISTORY_INVALID;
    memset(info, 0, sizeof(*info));
    if (reason && reason_size) reason[0] = '\0';
    if (!req->out_path || req->from_ms < 0 || req->to_ms < req->from_ms) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "export request is invalid");
    }
    char out[ALARM_PATH_MAX];
    char csv_tmp[ALARM_PATH_MAX];
    char dbsnap[ALARM_PATH_MAX];
    char bundle_tmp[ALARM_PATH_MAX];
    if (snprintf(out, sizeof(out), "%s", req->out_path) >= (int)sizeof(out) ||
        snprintf(csv_tmp, sizeof(csv_tmp), "%s.csv.tmp", out) >= (int)sizeof(csv_tmp) ||
        snprintf(dbsnap, sizeof(dbsnap), "%s.dbsnap", out) >= (int)sizeof(dbsnap) ||
        snprintf(bundle_tmp, sizeof(bundle_tmp), "%s.tmp", out) >= (int)sizeof(bundle_tmp)) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "path too long");
    }
    unlink(csv_tmp);
    unlink(dbsnap);
    unlink(bundle_tmp);

    int include_active = req->include_active != 0;
    struct stat st;
    if (stat(out, &st) == 0) {
        edge_history_bundle_info_t bundle;
        edge_history_result_t rc = edge_history_bundle_verify(out, "alarm", ALARM_SCHEMA_VERSION,
                                                              &bundle, reason, reason_size);
        if (rc == EDGE_HISTORY_OK) {
            if (bundle.from_ms != req->from_ms || bundle.to_ms != req->to_ms ||
                (bundle.includes_active != 0) != include_active) {
                return fail(EDGE_HISTORY_CONFLICT, reason, reason_size,
                            "bundle covers another window");
            }
            char fingerprint[EDGE_HISTORY_SHA256_SIZE];
            uint64_t selection_rows = 0;
            int64_t max_recovery = 0;
            rc = fp_query(store->db, req->from_ms, req->to_ms, include_active, NULL,
                          &selection_rows, &max_recovery, fingerprint, reason, reason_size);
            if (rc != EDGE_HISTORY_OK) return rc;
            if (selection_rows == 0 || strcmp(fingerprint, bundle.source_fingerprint) == 0) {
                return fill_export_info(out, &bundle, info, 1, reason, reason_size);
            }
            return fail(EDGE_HISTORY_CONFLICT, reason, reason_size,
                        "source changed since the bundle");
        }
        char bad[ALARM_PATH_MAX];
        if (snprintf(bad, sizeof(bad), "%s.bad-%lld", out, (long long)time(NULL)) >=
            (int)sizeof(bad)) {
            return fail(EDGE_HISTORY_INVALID, reason, reason_size, "path too long");
        }
        unlink(bad);
        if (rename(out, bad) != 0) {
            return fail(io_result(), reason, reason_size, "cannot isolate the bad bundle");
        }
    } else if (errno != ENOENT) {
        return fail(io_result(), reason, reason_size, "cannot inspect the bundle path");
    }

    edge_history_result_t rc = snapshot_db(store->db, dbsnap, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) {
        unlink(dbsnap);
        return rc;
    }
    sqlite3 *snapshot = NULL;
    rc = sqlite3_open_v2(dbsnap, &snapshot, SQLITE_OPEN_READWRITE, NULL) == SQLITE_OK
             ? EDGE_HISTORY_OK
             : EDGE_HISTORY_IO;
    if (rc != EDGE_HISTORY_OK) {
        if (snapshot) sqlite3_close(snapshot);
        unlink(dbsnap);
        return fail(rc, reason, reason_size, "cannot read the snapshot");
    }
    FILE *csv = edge_tar_fopen_private(csv_tmp, "wb");
    if (!csv) {
        sqlite3_close(snapshot);
        unlink(dbsnap);
        return fail(io_result(), reason, reason_size, "cannot create the csv");
    }
    size_t header_len = strlen(ALARM_CSV_HEADER);
    if (fwrite(ALARM_CSV_HEADER, 1, header_len, csv) != header_len) {
        rc = fail(io_result(), reason, reason_size, "csv write failed");
    }
    uint64_t rows = 0;
    int64_t max_recovery = 0;
    char fingerprint[EDGE_HISTORY_SHA256_SIZE];
    if (rc == EDGE_HISTORY_OK) {
        rc = fp_query(snapshot, req->from_ms, req->to_ms, include_active, csv, &rows, &max_recovery,
                      fingerprint, reason, reason_size);
    }
    if (rc == EDGE_HISTORY_OK && (fflush(csv) != 0 || fsync(fileno(csv)) != 0)) {
        rc = fail(io_result(), reason, reason_size, "csv sync failed");
    }
    if (fclose(csv) != 0 && rc == EDGE_HISTORY_OK) {
        rc = fail(io_result(), reason, reason_size, "csv close failed");
    }
    sqlite3_close(snapshot);
    unlink(dbsnap);
    if (rc != EDGE_HISTORY_OK) {
        unlink(csv_tmp);
        return rc;
    }

    const char *slash = strrchr(store->db_path, '/');
    const char *base = slash ? slash + 1 : store->db_path;
    edge_history_member_t member = {
        .name = "alarm_history.csv",
        .path = csv_tmp,
        .rows = rows,
        .has_rows = 1,
    };
    edge_history_bundle_desc_t desc = {
        .format = EDGE_HISTORY_FORMAT,
        .store = "alarm",
        .schema_version = ALARM_SCHEMA_VERSION,
        .from_ms = req->from_ms,
        .to_ms = req->to_ms,
        .created_ms = (int64_t)time(NULL) * 1000,
        .includes_active = include_active,
        .watermark_ms = max_recovery,
        .source_fingerprint = fingerprint,
        .source_db = base,
        .source_table = "alarmRealTab",
        .source_schema = "v3-8col",
        .members = &member,
        .member_count = 1,
        .row_count = rows,
    };
    rc = edge_history_bundle_write(out, &desc, store->export_fail_after, reason, reason_size);
    unlink(csv_tmp);
    if (rc != EDGE_HISTORY_OK) return rc;
    if (stat(out, &st) != 0) return fail(io_result(), reason, reason_size, "bundle is missing");
    info->rows = rows;
    info->watermark_ms = max_recovery;
    info->bytes = (unsigned long long)st.st_size;
    snprintf(info->source_fingerprint, sizeof(info->source_fingerprint), "%s", fingerprint);
    if (edge_sha256_file(out, info->bundle_sha256, NULL) != 0) {
        return fail(io_result(), reason, reason_size, "bundle digest failed");
    }
    info->already = 0;
    return EDGE_HISTORY_OK;
}

static edge_history_result_t alarm_verify_export(void *ctx, const char *path,
                                                 edge_history_export_info_t *info, char *reason,
                                                 size_t reason_size) {
    (void)ctx;
    if (!path || !info) return EDGE_HISTORY_INVALID;
    memset(info, 0, sizeof(*info));
    if (reason && reason_size) reason[0] = '\0';
    edge_history_bundle_info_t bundle;
    edge_history_result_t rc = edge_history_bundle_verify(path, "alarm", ALARM_SCHEMA_VERSION,
                                                          &bundle, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;
    return fill_export_info(path, &bundle, info, 0, reason, reason_size);
}

static edge_history_result_t count_recovered(sqlite3 *db, uint64_t *out, char *reason,
                                             size_t reason_size) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT count(*) FROM alarmRealTab WHERE status=0 AND recovery_time>0;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return db_fail(db, map_sqlite(sqlite3_errcode(db)), reason, reason_size, "count failed");
    }
    edge_history_result_t rc = EDGE_HISTORY_IO;
    int step = sqlite3_step(stmt);
    if (step == SQLITE_ROW) {
        *out = (uint64_t)sqlite3_column_int64(stmt, 0);
        rc = EDGE_HISTORY_OK;
    } else if (step != SQLITE_DONE) {
        rc = map_sqlite(step);
    }
    sqlite3_finalize(stmt);
    return rc;
}

static int append_sql(char *sql, size_t cap, size_t *used, const char *text) {
    size_t len = strlen(text);
    if (*used + len >= cap) return -1;
    memcpy(sql + *used, text, len + 1);
    *used += len;
    return 0;
}

static edge_history_result_t delete_chunk(alarm_history_store_t *store, int has_gate, int has_age,
                                          int64_t cutoff, uint64_t excess, uint32_t chunk,
                                          int *changed, char *reason, size_t reason_size) {
    char sql[ALARM_SQL_MAX];
    size_t used = 0;
    if (append_sql(sql, sizeof(sql), &used,
                   "DELETE FROM alarmRealTab WHERE rowid IN (SELECT rowid FROM alarmRealTab "
                   "WHERE status=0 AND recovery_time>0") != 0) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "delete statement too long");
    }
    if ((has_gate && append_sql(sql, sizeof(sql), &used,
                                " AND EXISTS (SELECT 1 FROM history_cover c WHERE recovery_time "
                                "BETWEEN c.from_ms AND c.to_ms)") != 0) ||
        (has_age && append_sql(sql, sizeof(sql), &used, " AND recovery_time<=?") != 0) ||
        (excess > 0 &&
         append_sql(sql, sizeof(sql), &used,
                    " AND rowid IN (SELECT rowid FROM alarmRealTab WHERE status=0 AND "
                    "recovery_time>0 ORDER BY raise_time ASC, rowid ASC LIMIT ?)") != 0) ||
        append_sql(sql, sizeof(sql), &used, " ORDER BY recovery_time ASC, rowid ASC LIMIT ?)") !=
            0) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "delete statement too long");
    }
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(store->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return db_fail(store->db, map_sqlite(sqlite3_errcode(store->db)), reason, reason_size,
                       "delete prepare failed");
    }
    int index = 1;
    if (has_age) sqlite3_bind_int64(stmt, index++, (sqlite3_int64)cutoff);
    if (excess > 0) sqlite3_bind_int64(stmt, index++, (sqlite3_int64)excess);
    sqlite3_bind_int64(stmt, index, (sqlite3_int64)chunk);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        return db_fail(store->db, map_sqlite(rc), reason, reason_size, "delete failed");
    }
    *changed = sqlite3_changes(store->db);
    return EDGE_HISTORY_OK;
}

static edge_history_result_t alarm_retain(void *ctx, const edge_history_retention_t *policy,
                                          edge_history_retention_result_t *result, char *reason,
                                          size_t reason_size) {
    alarm_history_store_t *store = (alarm_history_store_t *)ctx;
    if (!store || !store->db || !policy || !result) return EDGE_HISTORY_INVALID;
    memset(result, 0, sizeof(*result));
    if (reason && reason_size) reason[0] = '\0';

    int has_gate = 0;
    if (policy->require_verified_export) {
        if (!policy->export_dir || policy->export_dir[0] == '\0') {
            return fail(EDGE_HISTORY_INVALID, reason, reason_size,
                        "verified export required: export_dir is missing, so no bundle proves "
                        "coverage; history is not deleted");
        }
        has_gate = 1;
    }

    int has_age = 0;
    int64_t cutoff = 0;
    if (policy->max_age_days > 0 && policy->now_ms > 0) {
        int64_t span = (int64_t)policy->max_age_days * 86400000;
        cutoff = policy->now_ms - span;
        if (cutoff < 0) cutoff = 0;
        has_age = 1;
    }

    uint64_t recovered = 0;
    edge_history_result_t rc = count_recovered(store->db, &recovered, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;
    uint64_t excess = 0;
    if (policy->max_rows > 0 && recovered > policy->max_rows) {
        excess = recovered - (uint64_t)policy->max_rows;
    }
    if (!has_gate && !has_age && excess == 0) return EDGE_HISTORY_OK;

    uint64_t free_before = 0;
    uint64_t free_after = 0;
    int page_size = 0;
    (void)pragma_u64(store->db, "PRAGMA freelist_count;", &free_before);
    (void)pragma_int(store->db, "PRAGMA page_size;", &page_size);

    rc = exec_sql(store->db, "BEGIN IMMEDIATE;");
    if (rc != EDGE_HISTORY_OK) return rc;
    if (has_gate) {
        uint64_t windows = 0;
        rc = load_cover(store, policy->export_dir, policy->verified_watermark_ms, &windows, reason,
                        reason_size);
        if (rc != EDGE_HISTORY_OK) {
            (void)exec_sql(store->db, "ROLLBACK;");
            return rc;
        }
        if (windows == 0) {
            (void)exec_sql(store->db, "ROLLBACK;");
            return EDGE_HISTORY_OK;
        }
    }
    uint32_t chunk = policy->chunk_rows ? policy->chunk_rows : ALARM_DEFAULT_CHUNK;
    uint64_t deleted = 0;
    for (;;) {
        int changed = 0;
        rc = delete_chunk(store, has_gate, has_age, cutoff, excess, chunk, &changed, reason,
                          reason_size);
        if (rc != EDGE_HISTORY_OK) {
            (void)exec_sql(store->db, "ROLLBACK;");
            return rc;
        }
        deleted += (uint64_t)changed;
        if (changed == 0) break;
        if (excess > 0 && deleted >= excess) break;
    }
    rc = exec_sql(store->db, "COMMIT;");
    if (rc != EDGE_HISTORY_OK) {
        (void)exec_sql(store->db, "ROLLBACK;");
        return rc;
    }

    (void)pragma_u64(store->db, "PRAGMA freelist_count;", &free_after);
    if (free_after > free_before && page_size > 0) {
        result->freed_bytes = (free_after - free_before) * (uint64_t)page_size;
    }
    if (free_after > 0) (void)exec_sql(store->db, "PRAGMA incremental_vacuum;");

    result->deleted_rows = (uint32_t)(deleted > UINT32_MAX ? (uint64_t)UINT32_MAX : deleted);
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(store->db, "SELECT min(raise_time) FROM alarmRealTab;", -1, &stmt,
                           NULL) == SQLITE_OK &&
        sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        result->oldest_remaining_ms = (int64_t)sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return EDGE_HISTORY_OK;
}

static edge_history_result_t alarm_stats(void *ctx, edge_history_stats_t *out) {
    alarm_history_store_t *store = (alarm_history_store_t *)ctx;
    if (!store || !store->db || !out) return EDGE_HISTORY_INVALID;
    memset(out, 0, sizeof(*out));
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(
            store->db,
            "SELECT count(*), coalesce(sum(CASE WHEN status=1 THEN 1 ELSE 0 END), 0) "
            "FROM alarmRealTab;",
            -1, &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(store->db));
    }
    edge_history_result_t rc = EDGE_HISTORY_IO;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out->rows = (uint64_t)sqlite3_column_int64(stmt, 0);
        out->active_rows = (uint64_t)sqlite3_column_int64(stmt, 1);
        rc = EDGE_HISTORY_OK;
    }
    sqlite3_finalize(stmt);
    if (rc != EDGE_HISTORY_OK) return rc;

    uint64_t pages = 0;
    int page_size = 0;
    (void)pragma_u64(store->db, "PRAGMA page_count;", &pages);
    (void)pragma_int(store->db, "PRAGMA page_size;", &page_size);
    if (page_size > 0) out->bytes = pages * (uint64_t)page_size;

    if (sqlite3_prepare_v2(store->db, "SELECT min(raise_time), max(raise_time) FROM alarmRealTab;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(store->db));
    }
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            out->oldest_ms = (int64_t)sqlite3_column_int64(stmt, 0);
        }
        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
            out->newest_ms = (int64_t)sqlite3_column_int64(stmt, 1);
        }
    }
    sqlite3_finalize(stmt);
    return EDGE_HISTORY_OK;
}

edge_history_result_t alarm_history_sqlite_open(const char *db_path, edge_alarm_history_port_t *out,
                                                char *reason, size_t reason_size) {
    if (reason && reason_size) reason[0] = '\0';
    if (out) memset(out, 0, sizeof(*out));
    if (!out || !db_path || db_path[0] == '\0' || strlen(db_path) > ALARM_DB_PATH_MAX) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "invalid database path");
    }
    alarm_history_store_t *store = calloc(1, sizeof(*store));
    if (!store) return fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
    snprintf(store->db_path, sizeof(store->db_path), "%s", db_path);

    int rc = sqlite3_open_v2(db_path, &store->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (rc != SQLITE_OK) {
        edge_history_result_t result = map_sqlite(rc);
        if (store->db) sqlite3_close(store->db);
        if (result == EDGE_HISTORY_CORRUPT) isolate_corrupt(db_path);
        free(store);
        return fail(result, reason, reason_size, "cannot open the alarm history database");
    }
    sqlite3_busy_timeout(store->db, 5000);

    edge_history_result_t result = quick_check(store->db);
    if (result != EDGE_HISTORY_OK) {
        sqlite3_close(store->db);
        free(store);
        if (result == EDGE_HISTORY_CORRUPT) {
            isolate_corrupt(db_path);
            return fail(EDGE_HISTORY_CORRUPT, reason, reason_size,
                        "database is corrupt; moved aside");
        }
        return fail(result, reason, reason_size, "database check failed");
    }
    if (exec_sql(store->db, "PRAGMA journal_mode=WAL;") != EDGE_HISTORY_OK ||
        exec_sql(store->db, "PRAGMA synchronous=NORMAL;") != EDGE_HISTORY_OK) {
        sqlite3_close(store->db);
        free(store);
        return fail(EDGE_HISTORY_IO, reason, reason_size, "cannot configure the journal");
    }

    int version = 0;
    result = pragma_int(store->db, "PRAGMA user_version;", &version);
    if (result != EDGE_HISTORY_OK) {
        sqlite3_close(store->db);
        free(store);
        return fail(result, reason, reason_size, "cannot read the schema version");
    }
    if (version > ALARM_SCHEMA_VERSION) {
        sqlite3_close(store->db);
        free(store);
        return fail(EDGE_HISTORY_INVALID, reason, reason_size,
                    "database schema is newer than this platform");
    }
    if (version < ALARM_SCHEMA_VERSION) {
        /* Must precede the first table: incremental_vacuum reclaims only when
         * the header carries the auto_vacuum flag. */
        if (exec_sql(store->db, "PRAGMA auto_vacuum=INCREMENTAL;") != EDGE_HISTORY_OK ||
            exec_sql(store->db, kSchema) != EDGE_HISTORY_OK ||
            exec_sql(store->db, "PRAGMA user_version = 1;") != EDGE_HISTORY_OK) {
            sqlite3_close(store->db);
            free(store);
            return fail(EDGE_HISTORY_IO, reason, reason_size, "cannot create the alarm schema");
        }
    }

    out->ctx = store;
    out->append = alarm_append;
    out->recover = alarm_recover;
    out->query = alarm_query;
    out->admin.ctx = store;
    out->admin.export_range = alarm_export;
    out->admin.verify_export = alarm_verify_export;
    out->admin.retain = alarm_retain;
    out->admin.stats = alarm_stats;
    return EDGE_HISTORY_OK;
}

void alarm_history_sqlite_close(edge_alarm_history_port_t *port) {
    if (!port || !port->ctx) return;
    alarm_history_store_t *store = (alarm_history_store_t *)port->ctx;
    if (store->db) sqlite3_close(store->db);
    free(store);
    memset(port, 0, sizeof(*port));
}

void alarm_history_sqlite_set_export_fail_after(edge_alarm_history_port_t *port,
                                                unsigned long long bytes) {
    if (!port || !port->ctx) return;
    ((alarm_history_store_t *)port->ctx)->export_fail_after = bytes;
}
