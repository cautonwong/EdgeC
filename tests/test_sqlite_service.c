#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "sqlite_service.h"

static int get_user_version(sqlite3 *db) {
    sqlite3_stmt *stmt;
    int ver = 0;
    sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) ver = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return ver;
}

static int table_exists(sqlite3 *db, const char *name) {
    char sql[128];
    snprintf(sql, 128, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s';",
             name);
    sqlite3_stmt *stmt;
    int exists = 0;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) exists = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return exists;
}

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static void test_migration(void **state) {
    (void)state;
    const char *db_path = "test_mig.db";
    unlink(db_path);
    uv_loop_t loop;
    uv_loop_init(&loop);
    sqlite_config_t cfg = {.type = PROD_IEMS, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(&loop, &cfg);
    assert_non_null(svc);
    sqlite3 *check_db;
    sqlite3_open(db_path, &check_db);
    assert_int_equal(get_user_version(check_db), 1);
    assert_int_equal(table_exists(check_db, "telemetry"), 1);
    sqlite3_close(check_db);
    sqlite_svc_destroy(svc);
    uv_walk(&loop, on_walk_close, NULL);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    unlink(db_path);
}

static void test_sharding(void **state) {
    (void)state;
    const char *db_path = "test_shard.db";
    unlink(db_path);
    uv_loop_t loop;
    uv_loop_init(&loop);
    sqlite_config_t cfg = {
        .type = PROD_DCU, .db_path = db_path, .batch_size = 1, .flush_interval_ms = 100};
    sqlite_svc_t *svc = sqlite_svc_create(&loop, &cfg);
    PipeMsg m1 = {.ts = 1698796800000,
                  .tag_id = 1,
                  .type = PIPEMSG_SCALAR_F64,
                  .payload = {.val = {.f = 10.0}}};
    PipeMsg m2 = {.ts = 1701388800000,
                  .tag_id = 1,
                  .type = PIPEMSG_SCALAR_F64,
                  .payload = {.val = {.f = 20.0}}};
    sqlite_svc_insert(svc, &m1);
    sqlite_svc_insert(svc, &m2);
    for (int i = 0; i < 100; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(5000);
    }
    sqlite_svc_destroy(svc);
    sqlite3 *check_db = NULL;
    if (sqlite3_open_v2(db_path, &check_db, SQLITE_OPEN_READONLY, NULL) == SQLITE_OK) {
        assert_int_equal(table_exists(check_db, "meter_data_1698796800000"), 1);
        assert_int_equal(table_exists(check_db, "meter_data_1701388800000"), 1);
        sqlite3_close(check_db);
    }
    uv_walk(&loop, on_walk_close, NULL);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    unlink(db_path);
}

static void test_evse_mode(void **state) {
    (void)state;
    const char *db_path = "test_evse.db";
    unlink(db_path);
    uv_loop_t loop;
    uv_loop_init(&loop);
    sqlite_config_t cfg = {.type = PROD_EVSE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(&loop, &cfg);
    PipeMsg m = {0};
    assert_int_not_equal(sqlite_svc_insert(svc, &m), 0);
    sqlite_svc_destroy(svc);
    uv_walk(&loop, on_walk_close, NULL);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_migration),
        cmocka_unit_test(test_sharding),
        cmocka_unit_test(test_evse_mode),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
