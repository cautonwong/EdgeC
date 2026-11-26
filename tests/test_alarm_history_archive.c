/**
 * @file test_alarm_history_archive.c
 * @brief Alarm history SQLite adapter and edgeos-history/1 bundle behavior.
 */

#define _GNU_SOURCE

#include <cjson/cJSON.h>
#include <cmocka.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "alarm_history_sqlite.h"
#include "edge_digest.h"
#include "edge_tar.h"
#include "history_bundle.h"

static const char *kDb = "test_alarm_history.db";
static const char *kDir = "test_alarm_history_export";

static void remove_dir_contents(const char *dir) {
    DIR *handle = opendir(dir);
    if (!handle) return;
    struct dirent *entry = NULL;
    char path[512];
    while ((entry = readdir(handle)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name) < (int)sizeof(path)) {
            unlink(path);
        }
    }
    closedir(handle);
    rmdir(dir);
}

static int dir_has(const char *dir, const char *prefix, const char *needle) {
    DIR *handle = opendir(dir);
    if (!handle) return 0;
    int found = 0;
    struct dirent *entry = NULL;
    while ((entry = readdir(handle)) != NULL) {
        if (strncmp(entry->d_name, prefix, strlen(prefix)) != 0) continue;
        if (needle && strstr(entry->d_name, needle) == NULL) continue;
        found = 1;
        break;
    }
    closedir(handle);
    return found;
}

static void reset_env(void) {
    unlink(kDb);
    char side[512];
    snprintf(side, sizeof(side), "%s-wal", kDb);
    unlink(side);
    snprintf(side, sizeof(side), "%s-shm", kDb);
    unlink(side);
    remove_dir_contents(kDir);
    DIR *handle = opendir(".");
    if (handle) {
        static const char corrupt_prefix[] = "test_alarm_history.db.corrupt-";
        struct dirent *entry = NULL;
        char path[512];
        while ((entry = readdir(handle)) != NULL) {
            if (strncmp(entry->d_name, corrupt_prefix, strlen(corrupt_prefix)) != 0) continue;
            if (snprintf(path, sizeof(path), "%s", entry->d_name) < (int)sizeof(path)) {
                unlink(path);
            }
        }
        closedir(handle);
    }
    assert_int_equal(mkdir(kDir, 0700), 0);
}

static void open_fresh(edge_alarm_history_port_t *port) {
    reset_env();
    char reason[EDGE_HISTORY_REASON_SIZE];
    assert_int_equal(alarm_history_sqlite_open(kDb, port, reason, sizeof(reason)), EDGE_HISTORY_OK);
    assert_non_null(port->ctx);
}

static edge_alarm_record_t make_record(int64_t raise_time, int64_t recovery_time, uint8_t status,
                                       int32_t code) {
    edge_alarm_record_t record;
    memset(&record, 0, sizeof(record));
    record.raise_time_ms = raise_time;
    record.recovery_time_ms = recovery_time;
    record.alarm_code = code;
    record.reason_code = code + 1;
    record.status = status;
    snprintf(record.entity, sizeof(record.entity), "entity-%d", (int)code);
    snprintf(record.tagname, sizeof(record.tagname), "tag-%" PRId64, raise_time);
    snprintf(record.addition, sizeof(record.addition), "add-%d", (int)code);
    return record;
}

static void append_record(edge_alarm_history_port_t *port, edge_alarm_record_t record) {
    assert_int_equal(port->append(port->ctx, &record), EDGE_HISTORY_OK);
}

static void append_recovered(edge_alarm_history_port_t *port, int64_t raise_time,
                             int64_t recovery_time) {
    append_record(port, make_record(raise_time, recovery_time, EDGE_ALARM_STATUS_RECOVERY, 1));
}

static void append_active(edge_alarm_history_port_t *port, int64_t raise_time) {
    append_record(port, make_record(raise_time, 0, EDGE_ALARM_STATUS_ACTIVE, 2));
}

static void export_path(int64_t from_ms, int64_t to_ms, char *out, size_t cap) {
    snprintf(out, cap, "%s/history-alarm-%" PRId64 "-%" PRId64 ".tar", kDir, from_ms, to_ms);
}

static edge_history_result_t do_export(edge_alarm_history_port_t *port, int64_t from_ms,
                                       int64_t to_ms, const char *out,
                                       edge_history_export_info_t *info) {
    edge_history_export_req_t request = {
        .from_ms = from_ms,
        .to_ms = to_ms,
        .out_path = out,
        .include_active = 0,
        .include_current = 0,
    };
    char reason[EDGE_HISTORY_REASON_SIZE] = {0};
    edge_history_result_t rc =
        port->admin.export_range(port->admin.ctx, &request, info, reason, sizeof(reason));
    if (rc != EDGE_HISTORY_OK) fprintf(stderr, "export failed: %s\n", reason);
    return rc;
}

static edge_history_result_t do_verify(edge_alarm_history_port_t *port, const char *path,
                                       edge_history_export_info_t *info) {
    char reason[EDGE_HISTORY_REASON_SIZE] = {0};
    edge_history_result_t rc =
        port->admin.verify_export(port->admin.ctx, path, info, reason, sizeof(reason));
    if (rc != EDGE_HISTORY_OK) fprintf(stderr, "verify failed: %s\n", reason);
    return rc;
}

static edge_history_result_t do_retain(edge_alarm_history_port_t *port,
                                       const edge_history_retention_t *policy,
                                       edge_history_retention_result_t *result) {
    char reason[EDGE_HISTORY_REASON_SIZE] = {0};
    edge_history_result_t rc =
        port->admin.retain(port->admin.ctx, policy, result, reason, sizeof(reason));
    if (rc != EDGE_HISTORY_OK) fprintf(stderr, "retain failed: %s\n", reason);
    return rc;
}

static void export_window(edge_alarm_history_port_t *port, int64_t from_ms, int64_t to_ms) {
    char out[512];
    export_path(from_ms, to_ms, out, sizeof(out));
    edge_history_export_info_t info;
    assert_int_equal(do_export(port, from_ms, to_ms, out, &info), EDGE_HISTORY_OK);
    assert_int_equal(info.already, 0);
}

static uint32_t count_rows(edge_alarm_history_port_t *port, int active) {
    edge_alarm_record_t rows[8];
    uint32_t count = 0;
    uint32_t total = 0;
    assert_int_equal(port->query(port->ctx, active, 0, 8, rows, 8, &count, &total),
                     EDGE_HISTORY_OK);
    return total;
}

static int file_sha256(const char *path, char hex[EDGE_HISTORY_SHA256_SIZE]) {
    return edge_sha256_file(path, hex, NULL);
}

static char *read_member_text(const char *path, const char *name, size_t *len) {
    unsigned char *data = NULL;
    char sha[EDGE_HISTORY_SHA256_SIZE];
    size_t size = 0;
    if (edge_tar_read_member(path, name, 64u * 1024u, &data, &size, sha) != 0) return NULL;
    if (len) *len = size;
    return (char *)data;
}

static void test_schema_idempotent(void **state) {
    (void)state;
    reset_env();
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_alarm_history_port_t first;
    assert_int_equal(alarm_history_sqlite_open(kDb, &first, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    alarm_history_sqlite_close(&first);
    edge_alarm_history_port_t second;
    assert_int_equal(alarm_history_sqlite_open(kDb, &second, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    alarm_history_sqlite_close(&second);

    sqlite3 *db = NULL;
    assert_int_equal(sqlite3_open(kDb, &db), SQLITE_OK);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, "PRAGMA table_info(alarmRealTab);", -1, &stmt, NULL),
                     SQLITE_OK);
    static const char *const names[] = {"raise_time", "recovery_time", "alarm_code", "reason_code",
                                        "entity",     "tagname",       "addition",   "status"};
    static const char *const types[] = {
        "BIGINT UNSIGNED", "BIGINT UNSIGNED", "INT",      "INT",
        "TEXT(64)",        "TEXT(96)",        "TEXT(64)", "TINYINT"};
    int columns = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        assert_true(columns < 8);
        assert_int_equal(sqlite3_column_int(stmt, 0), columns);
        assert_string_equal((const char *)sqlite3_column_text(stmt, 1), names[columns]);
        assert_string_equal((const char *)sqlite3_column_text(stmt, 2), types[columns]);
        columns++;
    }
    sqlite3_finalize(stmt);
    assert_int_equal(columns, 8);

    int has_status_raise = 0;
    int has_recover = 0;
    assert_int_equal(sqlite3_prepare_v2(db, "PRAGMA index_list(alarmRealTab);", -1, &stmt, NULL),
                     SQLITE_OK);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *index_name = (const char *)sqlite3_column_text(stmt, 1);
        if (index_name && strcmp(index_name, "idx_alarm_status_raise") == 0) has_status_raise = 1;
        if (index_name && strcmp(index_name, "idx_alarm_recover") == 0) has_recover = 1;
    }
    sqlite3_finalize(stmt);
    assert_int_equal(has_status_raise, 1);
    assert_int_equal(has_recover, 1);

    assert_int_equal(sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, NULL), SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int(stmt, 0), 1);
    sqlite3_finalize(stmt);

    assert_int_equal(sqlite3_exec(db, "PRAGMA user_version=2;", NULL, NULL, NULL), SQLITE_OK);
    sqlite3_close(db);
    edge_alarm_history_port_t newer;
    assert_int_equal(alarm_history_sqlite_open(kDb, &newer, reason, sizeof(reason)),
                     EDGE_HISTORY_INVALID);
    assert_null(newer.ctx);
    assert_non_null(strstr(reason, "newer"));
}

static void test_append_recover_query(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);

    edge_alarm_record_t active = make_record(1000, 0, EDGE_ALARM_STATUS_ACTIVE, 7);
    snprintf(active.entity, sizeof(active.entity), "O'Brien, unit");
    snprintf(active.tagname, sizeof(active.tagname), "tag\"quoted\"");
    snprintf(active.addition, sizeof(active.addition), "line1\nline2");
    append_record(&port, active);
    append_active(&port, 2000);

    assert_int_equal(port.recover(port.ctx, 7, 8, 1000, 5000), EDGE_HISTORY_OK);
    append_recovered(&port, 4000, 6000);
    append_recovered(&port, 5000, 7000);

    edge_alarm_record_t rows[8];
    uint32_t count = 0;
    uint32_t total = 0;
    assert_int_equal(port.query(port.ctx, 1, 0, 8, rows, 8, &count, &total), EDGE_HISTORY_OK);
    assert_int_equal(count, 1);
    assert_int_equal(total, 1);
    assert_int_equal(rows[0].raise_time_ms, 2000);
    assert_int_equal(rows[0].status, EDGE_ALARM_STATUS_ACTIVE);

    assert_int_equal(port.query(port.ctx, 0, 0, 8, rows, 8, &count, &total), EDGE_HISTORY_OK);
    assert_int_equal(count, 3);
    assert_int_equal(total, 3);
    assert_int_equal(rows[0].raise_time_ms, 5000);
    assert_int_equal(rows[1].raise_time_ms, 4000);
    assert_int_equal(rows[2].raise_time_ms, 1000);
    assert_int_equal(rows[2].recovery_time_ms, 5000);
    assert_int_equal(rows[2].status, EDGE_ALARM_STATUS_RECOVERY);
    assert_string_equal(rows[2].entity, "O'Brien, unit");
    assert_string_equal(rows[2].tagname, "tag\"quoted\"");
    assert_string_equal(rows[2].addition, "line1\nline2");

    alarm_history_sqlite_close(&port);
}

static void test_param_validation(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);

    edge_alarm_record_t record = make_record(100, 200, EDGE_ALARM_STATUS_RECOVERY, 1);
    memset(record.entity, 'x', sizeof(record.entity));
    assert_int_equal(port.append(port.ctx, &record), EDGE_HISTORY_INVALID);

    record = make_record(100, 200, EDGE_ALARM_STATUS_RECOVERY, 1);
    memset(record.tagname, 'x', sizeof(record.tagname));
    assert_int_equal(port.append(port.ctx, &record), EDGE_HISTORY_INVALID);

    record = make_record(100, 200, EDGE_ALARM_STATUS_RECOVERY, 1);
    record.status = 2;
    assert_int_equal(port.append(port.ctx, &record), EDGE_HISTORY_INVALID);

    record = make_record(100, 5, EDGE_ALARM_STATUS_ACTIVE, 1);
    assert_int_equal(port.append(port.ctx, &record), EDGE_HISTORY_INVALID);

    record = make_record(100, 0, EDGE_ALARM_STATUS_RECOVERY, 1);
    assert_int_equal(port.append(port.ctx, &record), EDGE_HISTORY_INVALID);

    assert_int_equal(port.recover(port.ctx, 1, 2, 100, 0), EDGE_HISTORY_INVALID);

    for (int i = 0; i < 7; i++) append_recovered(&port, 1000 + i, 5000 + i);

    edge_alarm_record_t rows[5];
    uint32_t count = 99;
    uint32_t total = 0;
    assert_int_equal(port.query(port.ctx, 0, 0, UINT32_MAX, rows, 5, &count, &total),
                     EDGE_HISTORY_OK);
    assert_int_equal(count, 5);
    assert_int_equal(total, 7);

    assert_int_equal(port.query(port.ctx, 0, UINT32_MAX, UINT32_MAX, rows, 5, &count, &total),
                     EDGE_HISTORY_OK);
    assert_int_equal(count, 0);
    assert_int_equal(total, 7);

    assert_int_equal(port.query(port.ctx, 0, 0, 0, rows, 5, &count, &total), EDGE_HISTORY_OK);
    assert_int_equal(count, 0);
    assert_int_equal(total, 7);

    assert_int_equal(port.query(port.ctx, -1, 0, 5, rows, 5, &count, &total), EDGE_HISTORY_INVALID);
    alarm_history_sqlite_close(&port);
}

static void test_retention_protects_active(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    for (int i = 0; i < 20; i++) append_recovered(&port, 1000 + i, 5000 + i);
    for (int i = 0; i < 5; i++) append_active(&port, 2000 + i);

    edge_history_retention_t policy = {0};
    policy.max_rows = 10;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 10);
    assert_int_equal(count_rows(&port, 0), 10);
    assert_int_equal(count_rows(&port, 1), 5);
    alarm_history_sqlite_close(&port);
}

static void test_export_gate(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);
    append_recovered(&port, 500, 9000);
    append_recovered(&port, 300, 20000);
    append_active(&port, 100);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.export_dir = kDir;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 0);
    assert_int_equal(count_rows(&port, 0), 3);

    char out[512];
    export_path(1000, 10000, out, sizeof(out));
    edge_history_export_info_t info;
    assert_int_equal(do_export(&port, 1000, 10000, out, &info), EDGE_HISTORY_OK);
    assert_int_equal(info.rows, 2);
    assert_int_equal(info.watermark_ms, 9000);
    assert_int_equal(info.already, 0);

    edge_history_export_info_t verified;
    assert_int_equal(do_verify(&port, out, &verified), EDGE_HISTORY_OK);
    assert_string_equal(verified.source_fingerprint, info.source_fingerprint);

    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 2);
    assert_int_equal(count_rows(&port, 1), 1);
    assert_int_equal(count_rows(&port, 0), 1);

    edge_alarm_record_t rows[4];
    uint32_t count = 0;
    uint32_t total = 0;
    assert_int_equal(port.query(port.ctx, 0, 0, 4, rows, 4, &count, &total), EDGE_HISTORY_OK);
    assert_int_equal(count, 1);
    assert_int_equal(rows[0].raise_time_ms, 300);
    assert_int_equal(rows[0].recovery_time_ms, 20000);
    alarm_history_sqlite_close(&port);
}

static void test_retain_window_gap(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 100, 500);
    append_recovered(&port, 200, 1500);
    append_recovered(&port, 300, 2500);
    export_window(&port, 0, 1000);
    export_window(&port, 2000, 3000);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.export_dir = kDir;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 2);
    assert_int_equal(count_rows(&port, 0), 1);
    edge_alarm_record_t rows[4];
    uint32_t count = 0;
    uint32_t total = 0;
    assert_int_equal(port.query(port.ctx, 0, 0, 4, rows, 4, &count, &total), EDGE_HISTORY_OK);
    assert_int_equal(count, 1);
    assert_int_equal(rows[0].recovery_time_ms, 1500);
    alarm_history_sqlite_close(&port);
}

static void test_retain_requires_export_dir(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.max_age_days = 1;
    policy.now_ms = 200000000;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_INVALID);
    assert_int_equal(result.deleted_rows, 0);
    assert_int_equal(count_rows(&port, 0), 1);
    alarm_history_sqlite_close(&port);
}

static void test_retain_caller_watermark_not_coverage(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);
    append_recovered(&port, 2000, 3000);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.verified_watermark_ms = 5000;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_INVALID);
    assert_int_equal(result.deleted_rows, 0);
    assert_int_equal(count_rows(&port, 0), 2);

    policy.export_dir = kDir;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 0);
    assert_int_equal(count_rows(&port, 0), 2);
    alarm_history_sqlite_close(&port);
}

static void test_retain_overlapping_windows(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 100, 400);
    append_recovered(&port, 200, 700);
    append_recovered(&port, 300, 1200);
    export_window(&port, 0, 1000);
    export_window(&port, 500, 1500);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.export_dir = kDir;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 3);
    assert_int_equal(count_rows(&port, 0), 0);
    alarm_history_sqlite_close(&port);
}

static void test_retain_contained_windows(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 100, 500);
    append_recovered(&port, 200, 1500);
    append_recovered(&port, 300, 2500);
    export_window(&port, 0, 3000);
    export_window(&port, 1000, 2000);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.export_dir = kDir;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 3);
    assert_int_equal(count_rows(&port, 0), 0);
    alarm_history_sqlite_close(&port);
}

static void test_retain_caps_at_caller_watermark(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 100, 500);
    append_recovered(&port, 200, 1500);
    append_recovered(&port, 300, 2500);
    export_window(&port, 0, 3000);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.export_dir = kDir;
    policy.verified_watermark_ms = 1500;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 2);
    assert_int_equal(count_rows(&port, 0), 1);
    edge_alarm_record_t rows[4];
    uint32_t count = 0;
    uint32_t total = 0;
    assert_int_equal(port.query(port.ctx, 0, 0, 4, rows, 4, &count, &total), EDGE_HISTORY_OK);
    assert_int_equal(count, 1);
    assert_int_equal(rows[0].recovery_time_ms, 2500);
    alarm_history_sqlite_close(&port);
}

static int line_endings_of(const char *text, size_t len) {
    int count = 0;
    for (size_t i = 0; i + 1 < len; i++) {
        if (text[i] == '\r' && text[i + 1] == '\n') count++;
    }
    return count;
}

static void test_export_content(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    edge_alarm_record_t first = make_record(1000, 2000, EDGE_ALARM_STATUS_RECOVERY, 3);
    snprintf(first.entity, sizeof(first.entity), "O'Brien, unit");
    snprintf(first.tagname, sizeof(first.tagname), "tag\"quoted\"");
    snprintf(first.addition, sizeof(first.addition), "a,b");
    append_record(&port, first);
    append_recovered(&port, 2000, 3000);

    char out[512];
    export_path(0, 10000, out, sizeof(out));
    edge_history_export_info_t info;
    assert_int_equal(do_export(&port, 0, 10000, out, &info), EDGE_HISTORY_OK);
    assert_int_equal(info.rows, 2);
    assert_int_equal(info.watermark_ms, 3000);
    assert_int_equal(info.already, 0);
    assert_int_equal(strlen(info.source_fingerprint), 64);
    char bundle_sha[EDGE_HISTORY_SHA256_SIZE];
    assert_int_equal(file_sha256(out, bundle_sha), 0);
    assert_string_equal(info.bundle_sha256, bundle_sha);

    char names[EDGE_TAR_MAX_MEMBERS][EDGE_TAR_NAME_SIZE];
    size_t name_count = 0;
    assert_int_equal(edge_tar_list(out, names, EDGE_TAR_MAX_MEMBERS, &name_count), 0);
    assert_int_equal(name_count, 2);
    assert_string_equal(names[0], "alarm_history.csv");
    assert_string_equal(names[1], "manifest.json");

    size_t manifest_len = 0;
    char *manifest = read_member_text(out, "manifest.json", &manifest_len);
    assert_non_null(manifest);
    cJSON *root = cJSON_ParseWithLength(manifest, manifest_len);
    free(manifest);
    assert_non_null(root);
    assert_string_equal(cJSON_GetObjectItem(root, "format")->valuestring, EDGE_HISTORY_FORMAT);
    assert_string_equal(cJSON_GetObjectItem(root, "store")->valuestring, "alarm");
    assert_int_equal(cJSON_GetObjectItem(root, "schema_version")->valueint, 1);
    assert_int_equal(cJSON_GetObjectItem(root, "row_count")->valueint, 2);
    assert_int_equal(cJSON_GetObjectItem(root, "from_ms")->valueint, 0);
    assert_int_equal(cJSON_GetObjectItem(root, "to_ms")->valueint, 10000);
    assert_int_equal(cJSON_GetObjectItem(root, "watermark_ms")->valueint, 3000);
    assert_false(cJSON_IsTrue(cJSON_GetObjectItem(root, "includes_active")));
    assert_int_equal(strlen(cJSON_GetObjectItem(root, "source_fingerprint")->valuestring), 64);
    const cJSON *files = cJSON_GetObjectItem(root, "files");
    assert_int_equal(cJSON_GetArraySize(files), 1);
    const cJSON *file = cJSON_GetArrayItem(files, 0);
    assert_string_equal(cJSON_GetObjectItem(file, "name")->valuestring, "alarm_history.csv");
    assert_int_equal(cJSON_GetObjectItem(file, "rows")->valueint, 2);
    assert_true(cJSON_GetObjectItem(file, "size")->valueint > 0);
    assert_int_equal(strlen(cJSON_GetObjectItem(file, "sha256")->valuestring), 64);
    const cJSON *source = cJSON_GetObjectItem(root, "source");
    assert_string_equal(cJSON_GetObjectItem(source, "table")->valuestring, "alarmRealTab");
    cJSON_Delete(root);

    size_t csv_len = 0;
    char *csv = read_member_text(out, "alarm_history.csv", &csv_len);
    assert_non_null(csv);
    assert_true(csv_len > 0);
    assert_true(strncmp(csv,
                        "raise_time,recovery_time,alarm_code,reason_code,entity,tagname,"
                        "addition,status\r\n",
                        strlen("raise_time,recovery_time,alarm_code,reason_code,entity,tagname,"
                               "addition,status\r\n")) == 0);
    int line_endings = 0;
    for (size_t i = 0; i + 1 < csv_len; i++) {
        if (csv[i] == '\r' && csv[i + 1] == '\n') line_endings++;
    }
    assert_int_equal(line_endings, 3);
    assert_non_null(strstr(csv, "\"O'Brien, unit\""));
    assert_non_null(strstr(csv, "\"tag\"\"quoted\"\"\""));
    assert_non_null(strstr(csv, "\"a,b\""));
    free(csv);

    char empty_out[512];
    export_path(100000, 200000, empty_out, sizeof(empty_out));
    edge_history_export_info_t empty_info;
    assert_int_equal(do_export(&port, 100000, 200000, empty_out, &empty_info), EDGE_HISTORY_OK);
    assert_int_equal(empty_info.rows, 0);
    assert_int_equal(empty_info.watermark_ms, 0);
    size_t empty_csv_len = 0;
    char *empty_csv = read_member_text(empty_out, "alarm_history.csv", &empty_csv_len);
    assert_non_null(empty_csv);
    assert_int_equal(line_endings_of(empty_csv, empty_csv_len), 1);
    free(empty_csv);
    alarm_history_sqlite_close(&port);
}

static void test_rerun_already(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);
    append_recovered(&port, 2000, 3000);

    char out[512];
    export_path(0, 10000, out, sizeof(out));
    edge_history_export_info_t first;
    assert_int_equal(do_export(&port, 0, 10000, out, &first), EDGE_HISTORY_OK);
    char sha_before[EDGE_HISTORY_SHA256_SIZE];
    assert_int_equal(file_sha256(out, sha_before), 0);

    edge_history_export_info_t second;
    assert_int_equal(do_export(&port, 0, 10000, out, &second), EDGE_HISTORY_OK);
    assert_int_equal(second.already, 1);
    assert_int_equal(second.rows, first.rows);
    assert_string_equal(second.source_fingerprint, first.source_fingerprint);
    char sha_after[EDGE_HISTORY_SHA256_SIZE];
    assert_int_equal(file_sha256(out, sha_after), 0);
    assert_string_equal(sha_before, sha_after);
    alarm_history_sqlite_close(&port);
}

static void test_rerun_conflict(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);

    char out[512];
    export_path(0, 10000, out, sizeof(out));
    edge_history_export_info_t first;
    assert_int_equal(do_export(&port, 0, 10000, out, &first), EDGE_HISTORY_OK);
    char sha_before[EDGE_HISTORY_SHA256_SIZE];
    assert_int_equal(file_sha256(out, sha_before), 0);

    append_recovered(&port, 2000, 3000);
    edge_history_export_info_t second;
    assert_int_equal(do_export(&port, 0, 10000, out, &second), EDGE_HISTORY_CONFLICT);
    char sha_after[EDGE_HISTORY_SHA256_SIZE];
    assert_int_equal(file_sha256(out, sha_after), 0);
    assert_string_equal(sha_before, sha_after);
    assert_int_equal(do_verify(&port, out, &second), EDGE_HISTORY_OK);
    assert_int_equal(count_rows(&port, 0), 2);
    alarm_history_sqlite_close(&port);
}

static void test_retired_rerun(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);
    append_recovered(&port, 2000, 3000);

    char out[512];
    export_path(0, 10000, out, sizeof(out));
    edge_history_export_info_t info;
    assert_int_equal(do_export(&port, 0, 10000, out, &info), EDGE_HISTORY_OK);
    char sha_before[EDGE_HISTORY_SHA256_SIZE];
    assert_int_equal(file_sha256(out, sha_before), 0);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.export_dir = kDir;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 2);
    assert_int_equal(count_rows(&port, 0), 0);

    edge_history_export_info_t rerun;
    assert_int_equal(do_export(&port, 0, 10000, out, &rerun), EDGE_HISTORY_OK);
    assert_int_equal(rerun.already, 1);
    assert_string_equal(rerun.source_fingerprint, info.source_fingerprint);
    char sha_after[EDGE_HISTORY_SHA256_SIZE];
    assert_int_equal(file_sha256(out, sha_after), 0);
    assert_string_equal(sha_before, sha_after);
    alarm_history_sqlite_close(&port);
}

static void test_source_change_blocks_delete(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);

    char out[512];
    export_path(0, 10000, out, sizeof(out));
    edge_history_export_info_t info;
    assert_int_equal(do_export(&port, 0, 10000, out, &info), EDGE_HISTORY_OK);

    append_recovered(&port, 2000, 3000);
    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.export_dir = kDir;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_CONFLICT);
    assert_int_equal(result.deleted_rows, 0);
    assert_int_equal(count_rows(&port, 0), 2);
    alarm_history_sqlite_close(&port);
}

static int tamper_bundle(const char *path) {
    FILE *file = fopen(path, "r+b");
    if (!file) return -1;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }
    long size = ftell(file);
    if (size <= 0) {
        fclose(file);
        return -1;
    }
    char *data = malloc((size_t)size);
    if (!data) {
        fclose(file);
        return -1;
    }
    if (fseek(file, 0, SEEK_SET) != 0 || fread(data, 1, (size_t)size, file) != (size_t)size) {
        free(data);
        fclose(file);
        return -1;
    }
    const char *needle = "raise_time,recovery_time";
    char *header = NULL;
    for (long i = 0; i + 24 < size; i++) {
        if (memcmp(data + i, needle, strlen(needle)) == 0) {
            header = data + i;
            break;
        }
    }
    if (!header) {
        free(data);
        fclose(file);
        return -1;
    }
    char *newline = memmem(header, (size_t)((data + size) - header), "\r\n", 2);
    if (!newline || newline + 2 >= data + size) {
        free(data);
        fclose(file);
        return -1;
    }
    newline[2] = newline[2] == '9' ? '8' : (char)(newline[2] + 1);
    if (fseek(file, 0, SEEK_SET) != 0 || fwrite(data, 1, (size_t)size, file) != (size_t)size) {
        free(data);
        fclose(file);
        return -1;
    }
    free(data);
    return fclose(file) == 0 ? 0 : -1;
}

static void test_corrupt_bundle(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);
    append_recovered(&port, 2000, 3000);

    char out[512];
    export_path(0, 10000, out, sizeof(out));
    edge_history_export_info_t info;
    assert_int_equal(do_export(&port, 0, 10000, out, &info), EDGE_HISTORY_OK);
    assert_int_equal(tamper_bundle(out), 0);

    edge_history_export_info_t tampered;
    assert_int_equal(do_verify(&port, out, &tampered), EDGE_HISTORY_VERIFY);
    assert_int_equal(count_rows(&port, 0), 2);

    edge_history_export_info_t replacement;
    assert_int_equal(do_export(&port, 0, 10000, out, &replacement), EDGE_HISTORY_OK);
    assert_int_equal(replacement.already, 0);
    assert_true(dir_has(kDir, "history-alarm-", ".bad-"));
    assert_int_equal(do_verify(&port, out, &replacement), EDGE_HISTORY_OK);
    assert_int_equal(count_rows(&port, 0), 2);
    alarm_history_sqlite_close(&port);
}

static void test_restart(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);
    append_recovered(&port, 2000, 3000);
    append_recovered(&port, 3000, 4000);

    char out[512];
    export_path(0, 10000, out, sizeof(out));
    edge_history_export_info_t info;
    assert_int_equal(do_export(&port, 0, 10000, out, &info), EDGE_HISTORY_OK);
    alarm_history_sqlite_close(&port);

    char reason[EDGE_HISTORY_REASON_SIZE];
    assert_int_equal(alarm_history_sqlite_open(kDb, &port, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    edge_history_stats_t stats;
    assert_int_equal(port.admin.stats(port.admin.ctx, &stats), EDGE_HISTORY_OK);
    assert_int_equal(stats.rows, 3);
    assert_int_equal(stats.active_rows, 0);
    assert_true(stats.bytes > 0);
    assert_int_equal(stats.oldest_ms, 1000);
    assert_int_equal(stats.newest_ms, 3000);

    edge_history_retention_t policy = {0};
    policy.require_verified_export = 1;
    policy.export_dir = kDir;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 3);
    assert_int_equal(count_rows(&port, 0), 0);
    assert_int_equal(result.oldest_remaining_ms, 0);
    alarm_history_sqlite_close(&port);
}

static void test_clock_back(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 100, 5000000);
    append_recovered(&port, 200, 50000000);
    append_recovered(&port, 300, 100000000);

    edge_history_retention_t policy = {0};
    policy.max_age_days = 1;
    policy.now_ms = 1000;
    edge_history_retention_result_t result;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 0);
    assert_int_equal(count_rows(&port, 0), 3);

    policy.now_ms = 100000000;
    assert_int_equal(do_retain(&port, &policy, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_rows, 1);
    assert_int_equal(count_rows(&port, 0), 2);
    alarm_history_sqlite_close(&port);
}

static void test_corrupt_db(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    append_recovered(&port, 1000, 2000);
    alarm_history_sqlite_close(&port);

    assert_int_equal(truncate(kDb, 128), 0);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_alarm_history_port_t corrupt;
    assert_int_equal(alarm_history_sqlite_open(kDb, &corrupt, reason, sizeof(reason)),
                     EDGE_HISTORY_CORRUPT);
    assert_null(corrupt.ctx);
    assert_int_not_equal(access(kDb, F_OK), 0);
    assert_true(dir_has(".", "test_alarm_history.db.corrupt-", NULL));
}

static int path_exists_with_suffix(const char *base, const char *suffix) {
    size_t len = strlen(base) + strlen(suffix) + 1;
    char *path = malloc(len);
    if (!path) return 1;
    snprintf(path, len, "%s%s", base, suffix);
    int exists = access(path, F_OK) == 0;
    free(path);
    return exists;
}

static void test_disk_full_partial_write(void **state) {
    (void)state;
    edge_alarm_history_port_t port;
    open_fresh(&port);
    for (int i = 0; i < 5; i++) append_recovered(&port, 1000 + i, 5000 + i);

    char out[512];
    export_path(0, 10000, out, sizeof(out));
    alarm_history_sqlite_set_export_fail_after(&port, 8);
    edge_history_export_info_t info;
    assert_int_equal(do_export(&port, 0, 10000, out, &info), EDGE_HISTORY_NO_SPACE);
    assert_false(path_exists_with_suffix(out, ""));
    assert_false(path_exists_with_suffix(out, ".tmp"));
    assert_false(path_exists_with_suffix(out, ".csv.tmp"));
    assert_false(path_exists_with_suffix(out, ".dbsnap"));
    assert_int_equal(count_rows(&port, 0), 5);

    alarm_history_sqlite_set_export_fail_after(&port, 0);
    assert_int_equal(do_export(&port, 0, 10000, out, &info), EDGE_HISTORY_OK);
    assert_int_equal(info.rows, 5);
    assert_int_equal(do_verify(&port, out, &info), EDGE_HISTORY_OK);
    alarm_history_sqlite_close(&port);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_schema_idempotent),
        cmocka_unit_test(test_append_recover_query),
        cmocka_unit_test(test_param_validation),
        cmocka_unit_test(test_retention_protects_active),
        cmocka_unit_test(test_export_gate),
        cmocka_unit_test(test_retain_window_gap),
        cmocka_unit_test(test_retain_requires_export_dir),
        cmocka_unit_test(test_retain_caller_watermark_not_coverage),
        cmocka_unit_test(test_retain_overlapping_windows),
        cmocka_unit_test(test_retain_contained_windows),
        cmocka_unit_test(test_retain_caps_at_caller_watermark),
        cmocka_unit_test(test_export_content),
        cmocka_unit_test(test_rerun_already),
        cmocka_unit_test(test_rerun_conflict),
        cmocka_unit_test(test_retired_rerun),
        cmocka_unit_test(test_source_change_blocks_delete),
        cmocka_unit_test(test_corrupt_bundle),
        cmocka_unit_test(test_restart),
        cmocka_unit_test(test_clock_back),
        cmocka_unit_test(test_corrupt_db),
        cmocka_unit_test(test_disk_full_partial_write),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
