/**
 * @file test_edge_backup.c
 * @brief Backup bundle round trip on a migrated platform database.
 */

#include <cmocka.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "app_repo_sqlite.h"
#include "edgeos/edge_backup.h"
#include "sqlite_service.h"

static const char *kDb = "test_edge_backup.db";
static const char *kRestored = "test_edge_backup_restored.db";
static const char *kBundle = "test_edge_backup.eappbak";
static const char *kTampered = "test_edge_backup_tampered.eappbak";

static void seed(sqlite_svc_t *db) {
    edge_app_repo_t repo = app_repo_sqlite(db);
    app_manifest_t app;
    memset(&app, 0, sizeof(app));
    snprintf(app.id, sizeof(app.id), "backup-app");
    snprintf(app.name, sizeof(app.name), "backup app");
    snprintf(app.version, sizeof(app.version), "1.2.3");
    snprintf(app.wasm_path, sizeof(app.wasm_path), "/tmp/backup-app/app.wasm");
    app.state = APP_STATE_RUNNING;
    assert_int_equal(repo.save_app(repo.ctx, &app), 0);
}

static void test_backup_round_trip(void **state) {
    (void)state;
    unlink(kDb);
    unlink(kRestored);
    unlink(kBundle);
    unlink(kTampered);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = kDb};
    sqlite_svc_t *db = sqlite_svc_create(&loop, &cfg);
    assert_non_null(db);
    seed(db);
    char *db_handle = (char *)sqlite_svc_get_db_handle(db);
    assert_non_null(db_handle);

    char reason[EDGE_BACKUP_REASON_SIZE];
    assert_int_equal(edge_backup_create(kDb, kBundle, reason, sizeof(reason)), 0);
    assert_int_equal(edge_backup_verify(kBundle, reason, sizeof(reason)), 0);

    FILE *f = fopen(kBundle, "rb");
    assert_non_null(f);
    assert_int_equal(fseek(f, 0, SEEK_END), 0);
    long size = ftell(f);
    fclose(f);
    assert_true(size > 1024);

    FILE *src = fopen(kBundle, "rb");
    FILE *dst = fopen(kTampered, "wb");
    assert_non_null(src);
    assert_non_null(dst);
    char buf[1024];
    size_t got;
    while ((got = fread(buf, 1, sizeof(buf), src)) > 0) fwrite(buf, 1, got, dst);
    fclose(src);
    fclose(dst);

    /* Cutting the archive inside the database member must fail verification. */
    f = fopen(kTampered, "r+b");
    assert_non_null(f);
    assert_int_equal(ftruncate(fileno(f), size / 2), 0);
    fclose(f);
    assert_int_not_equal(edge_backup_verify(kTampered, reason, sizeof(reason)), 0);

    assert_int_equal(edge_backup_restore(kBundle, kRestored, reason, sizeof(reason)), 0);
    sqlite3 *restored = NULL;
    assert_int_equal(sqlite3_open(kRestored, &restored), SQLITE_OK);
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(restored, "SELECT version, state FROM app_registry WHERE id='backup-app';",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_string_equal((const char *)sqlite3_column_text(stmt, 0), "1.2.3");
    assert_int_equal(sqlite3_column_int(stmt, 1), APP_STATE_RUNNING);
    sqlite3_finalize(stmt);
    assert_int_equal(sqlite3_prepare_v2(restored, "PRAGMA user_version;", -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_int(stmt, 0), sqlite_service_schema_version());
    sqlite3_finalize(stmt);
    sqlite3_close(restored);

    sqlite_svc_destroy(db);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    unlink(kDb);
    unlink(kRestored);
    unlink(kBundle);
    char previous[128];
    snprintf(previous, sizeof(previous), "%s.before-restore", kRestored);
    unlink(previous);
    unlink(kTampered);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_backup_round_trip),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
