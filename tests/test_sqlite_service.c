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
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) exists = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return exists;
}

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int setup(void **state) {
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;

    // 物理排干
    uv_run(loop, UV_RUN_DEFAULT);

    uv_walk(loop, on_walk_close, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
    }

    free(loop);
    return 0;
}

static void test_migration(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_mig.db";
    unlink(db_path);
    sqlite_config_t cfg = {.strategy = STRATEGY_SERIES, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *check_db;
    assert_int_equal(sqlite3_open(db_path, &check_db), SQLITE_OK);
    assert_int_equal(get_user_version(check_db), 1);
    assert_int_equal(table_exists(check_db, "telemetry"), 1);
    sqlite3_close(check_db);
    sqlite_svc_destroy(svc);
    unlink(db_path);
}

static void test_sharding(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_shard.db";
    unlink(db_path);
    sqlite_config_t cfg = {.strategy = STRATEGY_SHARDING,
                           .db_path = db_path,
                           .batch_size = 1,
                           .flush_interval_ms = 10};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);

    PipeMsg m1 = {.ts = 1698796800000,
                  .tag_id = 1,
                  .type = PIPEMSG_TYPE_SCALAR,
                  .payload = {.val = {.f = 10.0}}};
    PipeMsg m2 = {.ts = 1701388800000,
                  .tag_id = 1,
                  .type = PIPEMSG_TYPE_SCALAR,
                  .payload = {.val = {.f = 20.0}}};

    sqlite_svc_insert(svc, &m1);
    sqlite_svc_insert(svc, &m2);

    // 驱动 loop 以确保异步写入完成
    // sqlite_svc_destroy 内部也会尝试 flush 并等待 is_flushing
    sqlite_svc_destroy(svc);

    sqlite3 *check_db = NULL;
    assert_int_equal(sqlite3_open_v2(db_path, &check_db, SQLITE_OPEN_READONLY, NULL), SQLITE_OK);
    assert_int_equal(table_exists(check_db, "telemetry_202311"), 1);
    assert_int_equal(table_exists(check_db, "telemetry_202312"), 1);
    sqlite3_close(check_db);
    unlink(db_path);
}

static void test_lite_mode(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *db_path = "test_lite.db";
    unlink(db_path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    PipeMsg m = {.tag_id = 100, .payload = {.val = {.f = 55.5}}, .ts = 1000};
    assert_int_equal(sqlite_svc_insert(svc, &m), 0);
    sqlite_svc_destroy(svc);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_migration, setup, teardown),
        cmocka_unit_test_setup_teardown(test_sharding, setup, teardown),
        cmocka_unit_test_setup_teardown(test_lite_mode, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
