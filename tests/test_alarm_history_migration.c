#define _GNU_SOURCE

#include <cmocka.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "alarm_history_sqlite.h"
#include "edge_digest.h"

static const char *kSrc = "test_alarm_migration_src.db";
static const char *kDst = "test_alarm_migration_dst.db";

static const char kV3Schema[] =
    "CREATE TABLE IF NOT EXISTS alarmRealTab ("
    " raise_time BIGINT UNSIGNED, recovery_time BIGINT UNSIGNED,"
    " alarm_code INT, reason_code INT, entity TEXT(64), tagname TEXT(96),"
    " addition TEXT(64), status TINYINT);";

static void unlink_sidecars(const char *path) {
    static const char *const suffixes[] = {"-wal", "-shm", "-journal"};
    char side[512];
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        snprintf(side, sizeof(side), "%s%s", path, suffixes[i]);
        unlink(side);
    }
}

static void reset_env(void) {
    unlink(kSrc);
    unlink(kDst);
    unlink_sidecars(kSrc);
    unlink_sidecars(kDst);
}

static sqlite3 *open_unchecked(const char *path) {
    sqlite3 *db = NULL;
    assert_int_equal(sqlite3_open(path, &db), SQLITE_OK);
    sqlite3_busy_timeout(db, 5000);
    return db;
}

static void exec_sql(sqlite3 *db, const char *sql) {
    assert_int_equal(sqlite3_exec(db, sql, NULL, NULL, NULL), SQLITE_OK);
}

static sqlite3 *open_source_rw(void) {
    sqlite3 *db = open_unchecked(kSrc);
    exec_sql(db, kV3Schema);
    return db;
}

static void insert_row(sqlite3 *db, int64_t raise_time, int64_t recovery_time, int alarm_code,
                       int reason_code, const char *entity, const char *tagname,
                       const char *addition, int status) {
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(
        sqlite3_prepare_v2(db, "INSERT INTO alarmRealTab VALUES (?, ?, ?, ?, ?, ?, ?, ?);", -1,
                           &stmt, NULL),
        SQLITE_OK);
    sqlite3_bind_int64(stmt, 1, raise_time);
    sqlite3_bind_int64(stmt, 2, recovery_time);
    sqlite3_bind_int(stmt, 3, alarm_code);
    sqlite3_bind_int(stmt, 4, reason_code);
    sqlite3_bind_text(stmt, 5, entity, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, tagname, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, addition, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 8, status);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
}

static void make_main_source(void) {
    sqlite3 *db = open_source_rw();
    insert_row(db, 1000, 2000, 10, 11, "entity-'a'", "tag,one", "add\"x\"", 0);
    insert_row(db, 2000, 4000, 20, 21, "entity-b", "tag-b", "add-b", 0);
    insert_row(db, 3000, 0, 30, 31, "entity-c", "tag-c", "add-c", 1);
    insert_row(db, 3000, 0, 40, 41, "entity-d", "tag-d", "add-d", 1);
    insert_row(db, 5000, 9000, 50, 51, "entity-e", "tag-e", "add-e", 0);
    assert_int_equal(sqlite3_close(db), SQLITE_OK);
}

static void make_recovered_source(int rows) {
    sqlite3 *db = open_source_rw();
    for (int i = 0; i < rows; i++) {
        int64_t raise_time = 1000 + (int64_t)i * 500;
        insert_row(db, raise_time, raise_time + 4000, 100 + i, 200 + i, "entity", "tag", "add", 0);
    }
    assert_int_equal(sqlite3_close(db), SQLITE_OK);
}

static void open_target(edge_alarm_history_port_t *port) {
    char reason[EDGE_HISTORY_REASON_SIZE];
    assert_int_equal(alarm_history_sqlite_open(kDst, port, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
}

static edge_history_result_t migrate(const char *src, edge_alarm_history_port_t *port, int dry_run,
                                     alarm_history_migration_result_t *result) {
    char reason[EDGE_HISTORY_REASON_SIZE] = {0};
    edge_history_result_t rc =
        alarm_history_sqlite_migrate_v3(src, port, dry_run, result, reason, sizeof(reason));
    if (rc != EDGE_HISTORY_OK) fprintf(stderr, "migration failed: %s\n", reason);
    return rc;
}

static uint32_t target_rows(edge_alarm_history_port_t *port, int active) {
    edge_alarm_record_t rows[16];
    uint32_t count = 0;
    uint32_t total = 0;
    assert_int_equal(port->query(port->ctx, active, 0, 16, rows, 16, &count, &total),
                     EDGE_HISTORY_OK);
    return total;
}

static void assert_source_file_unchanged(const struct stat *before) {
    struct stat after;
    assert_int_equal(stat(kSrc, &after), 0);
    assert_int_equal(after.st_size, before->st_size);
    assert_int_equal(after.st_mtim.tv_sec, before->st_mtim.tv_sec);
    assert_int_equal(after.st_mtim.tv_nsec, before->st_mtim.tv_nsec);
}

static void test_first_migration(void **state) {
    (void)state;
    reset_env();
    make_main_source();
    edge_alarm_history_port_t port;
    open_target(&port);

    alarm_history_migration_result_t result;
    assert_int_equal(migrate(kSrc, &port, 0, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.source_rows, 5);
    assert_int_equal(result.imported_rows, 5);
    assert_int_equal(result.skipped_rows, 0);
    assert_int_equal(result.active_rows, 2);
    assert_int_equal(result.recovered_rows, 3);
    assert_int_equal(result.imported_rows + result.skipped_rows, result.source_rows);
    assert_int_equal(target_rows(&port, 1), 2);
    assert_int_equal(target_rows(&port, 0), 3);

    edge_alarm_record_t rows[8];
    uint32_t count = 0;
    uint32_t total = 0;
    assert_int_equal(port.query(port.ctx, 1, 0, 8, rows, 8, &count, &total), EDGE_HISTORY_OK);
    assert_int_equal(count, 2);
    for (uint32_t i = 0; i < count; i++) {
        assert_int_equal(rows[i].status, EDGE_ALARM_STATUS_ACTIVE);
        assert_int_equal(rows[i].recovery_time_ms, 0);
        assert_int_equal(rows[i].raise_time_ms, 3000);
    }
    assert_int_equal(port.query(port.ctx, 0, 0, 8, rows, 8, &count, &total), EDGE_HISTORY_OK);
    assert_int_equal(count, 3);
    int found = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (rows[i].raise_time_ms != 1000) continue;
        found = 1;
        assert_string_equal(rows[i].entity, "entity-'a'");
        assert_string_equal(rows[i].tagname, "tag,one");
        assert_string_equal(rows[i].addition, "add\"x\"");
    }
    assert_true(found);
    alarm_history_sqlite_close(&port);

    sqlite3 *raw = open_unchecked(kDst);
    char sha[EDGE_HISTORY_SHA256_SIZE];
    assert_int_equal(edge_sha256_file(kSrc, sha, NULL), 0);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(raw,
                                        "SELECT row_count, active_rows, recovered_rows FROM "
                                        "alarm_migration_source WHERE source_sha256=?;",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    sqlite3_bind_text(stmt, 1, sha, -1, SQLITE_STATIC);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int64(stmt, 0), 5);
    assert_int_equal(sqlite3_column_int64(stmt, 1), 2);
    assert_int_equal(sqlite3_column_int64(stmt, 2), 3);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_close(raw), SQLITE_OK);
}

static void test_rerun_zero_new(void **state) {
    (void)state;
    reset_env();
    make_main_source();
    edge_alarm_history_port_t port;
    open_target(&port);
    alarm_history_migration_result_t first;
    assert_int_equal(migrate(kSrc, &port, 0, &first), EDGE_HISTORY_OK);

    alarm_history_migration_result_t second;
    assert_int_equal(migrate(kSrc, &port, 0, &second), EDGE_HISTORY_OK);
    assert_int_equal(second.source_rows, 5);
    assert_int_equal(second.imported_rows, 0);
    assert_int_equal(second.skipped_rows, 5);
    assert_int_equal(target_rows(&port, 1), 2);
    assert_int_equal(target_rows(&port, 0), 3);
    alarm_history_sqlite_close(&port);

    sqlite3 *raw = open_unchecked(kDst);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(raw, "SELECT count(*) FROM alarmRealTab;", -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int64(stmt, 0), 5);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_close(raw), SQLITE_OK);
}

static void test_partial_resume(void **state) {
    (void)state;
    reset_env();
    make_recovered_source(5);
    edge_alarm_history_port_t port;
    open_target(&port);

    sqlite3 *raw = open_unchecked(kDst);
    exec_sql(raw,
             "CREATE TRIGGER block_row BEFORE INSERT ON alarmRealTab "
             "WHEN NEW.raise_time = 1500 BEGIN SELECT RAISE(ABORT, 'blocked'); END;");
    assert_int_equal(sqlite3_close(raw), SQLITE_OK);

    alarm_history_migration_result_t first;
    assert_int_not_equal(migrate(kSrc, &port, 0, &first), EDGE_HISTORY_OK);
    assert_int_equal(first.imported_rows, 1);
    assert_int_equal(target_rows(&port, 0), 1);

    raw = open_unchecked(kDst);
    exec_sql(raw, "DROP TRIGGER block_row;");
    assert_int_equal(sqlite3_close(raw), SQLITE_OK);

    alarm_history_migration_result_t second;
    assert_int_equal(migrate(kSrc, &port, 0, &second), EDGE_HISTORY_OK);
    assert_int_equal(second.source_rows, 5);
    assert_int_equal(second.skipped_rows, 1);
    assert_int_equal(second.imported_rows, 4);
    assert_int_equal(target_rows(&port, 0), 5);
    alarm_history_sqlite_close(&port);
}

static void test_reject_bad_source(void **state) {
    (void)state;
    reset_env();
    edge_alarm_history_port_t port;
    open_target(&port);
    alarm_history_migration_result_t result;

    sqlite3 *db = open_unchecked(kSrc);
    assert_int_equal(sqlite3_close(db), SQLITE_OK);
    assert_int_equal(migrate(kSrc, &port, 0, &result), EDGE_HISTORY_INVALID);

    unlink(kSrc);
    db = open_unchecked(kSrc);
    exec_sql(db,
             "CREATE TABLE alarmRealTab (raise_time BIGINT UNSIGNED, "
             "recovery_time BIGINT UNSIGNED, alarm_code INT, reason_code INT, "
             "entity TEXT(64), tagname TEXT(96), addition TEXT(64));");
    assert_int_equal(sqlite3_close(db), SQLITE_OK);
    assert_int_equal(migrate(kSrc, &port, 0, &result), EDGE_HISTORY_INVALID);

    unlink(kSrc);
    make_main_source();
    assert_int_equal(truncate(kSrc, 64), 0);
    assert_int_equal(migrate(kSrc, &port, 0, &result), EDGE_HISTORY_CORRUPT);

    assert_int_equal(target_rows(&port, 0), 0);
    assert_int_equal(target_rows(&port, 1), 0);
    alarm_history_sqlite_close(&port);

    sqlite3 *raw = open_unchecked(kDst);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(raw,
                                        "SELECT count(*) FROM sqlite_master WHERE type='table' "
                                        "AND name LIKE 'alarm_migration_%';",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int64(stmt, 0), 0);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_close(raw), SQLITE_OK);
}

static void test_dry_run_no_write(void **state) {
    (void)state;
    reset_env();
    make_main_source();
    struct stat before;
    assert_int_equal(stat(kSrc, &before), 0);
    edge_alarm_history_port_t port;
    open_target(&port);

    alarm_history_migration_result_t result;
    assert_int_equal(migrate(kSrc, &port, 1, &result), EDGE_HISTORY_OK);
    assert_int_equal(result.source_rows, 5);
    assert_int_equal(result.imported_rows, 5);
    assert_int_equal(result.skipped_rows, 0);
    assert_int_equal(result.active_rows, 2);
    assert_int_equal(result.recovered_rows, 3);
    assert_int_equal(target_rows(&port, 0), 0);
    assert_int_equal(target_rows(&port, 1), 0);
    alarm_history_sqlite_close(&port);

    sqlite3 *raw = open_unchecked(kDst);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(raw,
                                        "SELECT count(*) FROM sqlite_master WHERE type='table' "
                                        "AND name LIKE 'alarm_migration_%';",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int64(stmt, 0), 0);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_close(raw), SQLITE_OK);
    assert_source_file_unchanged(&before);
}

static void test_dry_run_after_source_appended(void **state) {
    (void)state;
    reset_env();
    make_recovered_source(2);
    edge_alarm_history_port_t port;
    open_target(&port);
    alarm_history_migration_result_t first;
    assert_int_equal(migrate(kSrc, &port, 0, &first), EDGE_HISTORY_OK);
    assert_int_equal(first.imported_rows, 2);
    assert_int_equal(first.skipped_rows, 0);

    sqlite3 *src = open_unchecked(kSrc);
    insert_row(src, 3000, 7000, 30, 31, "entity", "tag", "add", 0);
    assert_int_equal(sqlite3_close(src), SQLITE_OK);

    alarm_history_migration_result_t dry;
    assert_int_equal(migrate(kSrc, &port, 1, &dry), EDGE_HISTORY_OK);
    assert_int_equal(dry.source_rows, 3);
    assert_int_equal(dry.imported_rows, 1);
    assert_int_equal(dry.skipped_rows, 2);
    assert_int_equal(target_rows(&port, 1), 0);
    assert_int_equal(target_rows(&port, 0), 2);
    alarm_history_sqlite_close(&port);

    sqlite3 *raw = open_unchecked(kDst);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(
        sqlite3_prepare_v2(raw, "SELECT count(*) FROM alarm_migration_row;", -1, &stmt, NULL),
        SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int64(stmt, 0), 2);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_prepare_v2(raw,
                                        "SELECT row_count, active_rows, recovered_rows FROM "
                                        "alarm_migration_source;",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int64(stmt, 0), 2);
    assert_int_equal(sqlite3_column_int64(stmt, 1), 0);
    assert_int_equal(sqlite3_column_int64(stmt, 2), 2);
    assert_int_equal(sqlite3_step(stmt), SQLITE_DONE);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_close(raw), SQLITE_OK);
}

static void test_source_file_unchanged(void **state) {
    (void)state;
    reset_env();
    make_main_source();
    struct stat before;
    assert_int_equal(stat(kSrc, &before), 0);
    edge_alarm_history_port_t port;
    open_target(&port);
    alarm_history_migration_result_t result;
    assert_int_equal(migrate(kSrc, &port, 0, &result), EDGE_HISTORY_OK);
    alarm_history_sqlite_close(&port);
    assert_source_file_unchanged(&before);
}

static void test_count_mismatch_rejected(void **state) {
    (void)state;
    reset_env();
    make_main_source();
    edge_alarm_history_port_t port;
    open_target(&port);
    alarm_history_migration_result_t first;
    assert_int_equal(migrate(kSrc, &port, 0, &first), EDGE_HISTORY_OK);
    alarm_history_sqlite_close(&port);

    sqlite3 *raw = open_unchecked(kDst);
    exec_sql(raw, "UPDATE alarm_migration_source SET row_count = row_count + 1;");
    assert_int_equal(sqlite3_close(raw), SQLITE_OK);

    open_target(&port);
    alarm_history_migration_result_t second;
    assert_int_equal(migrate(kSrc, &port, 0, &second), EDGE_HISTORY_VERIFY);
    assert_int_equal(second.imported_rows, 0);
    assert_int_equal(second.skipped_rows, 5);
    assert_int_equal(target_rows(&port, 1), 2);
    assert_int_equal(target_rows(&port, 0), 3);
    alarm_history_sqlite_close(&port);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_first_migration),
        cmocka_unit_test(test_rerun_zero_new),
        cmocka_unit_test(test_partial_resume),
        cmocka_unit_test(test_reject_bad_source),
        cmocka_unit_test(test_dry_run_no_write),
        cmocka_unit_test(test_dry_run_after_source_appended),
        cmocka_unit_test(test_source_file_unchanged),
        cmocka_unit_test(test_count_mismatch_rejected),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
