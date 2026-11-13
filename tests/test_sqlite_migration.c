#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "sqlite_service.h"

// 旧库在 schema_migration 表出现之前的状态：v3 不含 app_registry。
#define LEGACY_V3_DDL                                                                       \
    "CREATE TABLE telemetry (ts INT, tag_id INT, val REAL, q INT, synced INT DEFAULT 0, "   \
    "PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;"                                               \
    "CREATE TABLE inventory (id TEXT PRIMARY KEY, type TEXT, addr INT, protocol TEXT, "     \
    "enabled INT DEFAULT 1);"                                                               \
    "CREATE TABLE edge_settings (product TEXT, tag_id INT, val BLOB, PRIMARY KEY(product, " \
    "tag_id));"                                                                             \
    "CREATE INDEX idx_telemetry_tag ON telemetry(tag_id);"

#define APP_REGISTRY_DDL                                                                      \
    "CREATE TABLE app_registry (id TEXT PRIMARY KEY, name TEXT, version TEXT, author TEXT, "  \
    "wasm_path TEXT, trigger_tag INT, entry_func TEXT, fault_policy INT, heap_limit_kb INT, " \
    "timeout_ms INT, state INT, restart_count INT, created_at INT);"

static void _exec(sqlite3 *db, const char *sql) {
    char *err = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) fprintf(stderr, "sqlite exec failed: %s\n", err ? err : sql);
    assert_int_equal(rc, SQLITE_OK);
    sqlite3_free(err);
}

static int _scalar_int(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt = NULL;
    int value = -1;
    assert_int_equal(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL), SQLITE_OK);
    if (sqlite3_step(stmt) == SQLITE_ROW) value = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return value;
}

static void _scalar_text(sqlite3 *db, const char *sql, char *out, size_t out_size) {
    sqlite3_stmt *stmt = NULL;
    out[0] = '\0';
    assert_int_equal(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL), SQLITE_OK);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        if (text) snprintf(out, out_size, "%s", (const char *)text);
    }
    sqlite3_finalize(stmt);
}

static void _on_close_cb(uv_handle_t *handle, void *arg) {
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
    uv_walk(loop, _on_close_cb, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    free(loop);
    return 0;
}

// 回归：v3 迁移曾 DROP edge_settings，升级后旧配置必须还在。
static void test_v3_upgrade_preserves_edge_settings(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_migration_v3.db";
    unlink(path);

    sqlite3 *raw = NULL;
    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    _exec(raw, LEGACY_V3_DDL);
    _exec(raw,
          "INSERT INTO edge_settings VALUES ('cfg', 1, x'0102');"
          "INSERT INTO edge_settings VALUES ('cfg', 2, x'030405');"
          "PRAGMA user_version=3;");
    sqlite3_close(raw);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);

    assert_int_equal(_scalar_int(db, "PRAGMA user_version;"), 14);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM edge_settings;"), 2);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_registry;"), 0);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_grant;"), 0);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_task;"), 0);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM schema_migration;"), 14);
    assert_int_equal(_scalar_int(db,
                                 "SELECT COUNT(*) FROM pragma_table_info('app_registry') "
                                 "WHERE name IN ('prev_version','prev_manifest_json',"
                                 "'prev_wasm_path');"),
                     3);

    char hex[16];
    _scalar_text(db, "SELECT hex(val) FROM edge_settings WHERE tag_id=1;", hex, sizeof(hex));
    assert_string_equal(hex, "0102");

    sqlite_svc_destroy(svc);
    unlink(path);
}

// 回归：v4 升级到 v5 时 app_registry 既有行与 edge_settings 都不能丢。
static void test_v4_upgrade_preserves_app_registry(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_migration_v4.db";
    unlink(path);

    sqlite3 *raw = NULL;
    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    _exec(raw, LEGACY_V3_DDL);
    _exec(raw, APP_REGISTRY_DDL);
    _exec(raw,
          "INSERT INTO app_registry (id, name, version, trigger_tag) "
          "VALUES ('demo', 'Demo App', '1.2.3', 42);"
          "INSERT INTO edge_settings VALUES ('cfg', 9, x'ff00');"
          "PRAGMA user_version=4;");
    sqlite3_close(raw);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);

    assert_int_equal(_scalar_int(db, "PRAGMA user_version;"), 14);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_registry;"), 1);
    assert_int_equal(_scalar_int(db, "SELECT trigger_tag FROM app_registry WHERE id='demo';"), 42);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM edge_settings;"), 1);

    char name[32];
    _scalar_text(db, "SELECT name FROM app_registry WHERE id='demo';", name, sizeof(name));
    assert_string_equal(name, "Demo App");

    sqlite_svc_destroy(svc);
    unlink(path);
}

// 库版本高于程序支持版本：拒绝启动，返回明确错误码，且不得改动库。
static void test_too_new_schema_rejected(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_migration_too_new.db";
    unlink(path);

    sqlite3 *raw = NULL;
    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    _exec(raw,
          "CREATE TABLE canary (v INT);"
          "INSERT INTO canary VALUES (7);"
          "PRAGMA user_version=15;");
    sqlite3_close(raw);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    int err = 0;
    sqlite_svc_t *svc = sqlite_svc_create_ex(loop, &cfg, &err);
    assert_null(svc);
    assert_int_equal(err, SQLITE_SVC_ERR_SCHEMA_TOO_NEW);
    assert_null(sqlite_svc_create(loop, &cfg));

    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    assert_int_equal(_scalar_int(raw, "PRAGMA user_version;"), 15);
    assert_int_equal(_scalar_int(raw, "SELECT v FROM canary;"), 7);
    assert_int_equal(_scalar_int(raw,
                                 "SELECT COUNT(*) FROM sqlite_master WHERE type='table' "
                                 "AND name='schema_migration';"),
                     0);
    sqlite3_close(raw);
    unlink(path);
}

// 迁移记录被篡改（checksum 不符）：拒绝启动。
static void test_checksum_mismatch_rejected(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_migration_checksum.db";
    unlink(path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM schema_migration;"), 14);
    sqlite_svc_destroy(svc);

    sqlite3 *raw = NULL;
    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    _exec(raw, "UPDATE schema_migration SET checksum='0000000000000000' WHERE version=1;");
    sqlite3_close(raw);

    int err = 0;
    svc = sqlite_svc_create_ex(loop, &cfg, &err);
    assert_null(svc);
    assert_int_equal(err, -1);

    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    assert_int_equal(_scalar_int(raw, "PRAGMA user_version;"), 14);
    sqlite3_close(raw);
    unlink(path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_v3_upgrade_preserves_edge_settings, setup, teardown),
        cmocka_unit_test_setup_teardown(test_v4_upgrade_preserves_app_registry, setup, teardown),
        cmocka_unit_test_setup_teardown(test_too_new_schema_rejected, setup, teardown),
        cmocka_unit_test_setup_teardown(test_checksum_mismatch_rejected, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
