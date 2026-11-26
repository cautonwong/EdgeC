#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
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

    assert_int_equal(_scalar_int(db, "PRAGMA user_version;"), 18);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM edge_settings;"), 2);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_registry;"), 0);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_grant;"), 0);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_task;"), 0);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM schema_migration;"), 18);
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

    assert_int_equal(_scalar_int(db, "PRAGMA user_version;"), 18);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_registry;"), 1);
    assert_int_equal(_scalar_int(db, "SELECT trigger_tag FROM app_registry WHERE id='demo';"), 42);
    assert_int_equal(_scalar_int(db, "SELECT tick_ms FROM app_registry WHERE id='demo';"), 0);
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
          "PRAGMA user_version=19;");
    sqlite3_close(raw);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    int err = 0;
    sqlite_svc_t *svc = sqlite_svc_create_ex(loop, &cfg, &err);
    assert_null(svc);
    assert_int_equal(err, SQLITE_SVC_ERR_SCHEMA_TOO_NEW);
    assert_null(sqlite_svc_create(loop, &cfg));

    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    assert_int_equal(_scalar_int(raw, "PRAGMA user_version;"), 19);
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
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM schema_migration;"), 18);
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
    assert_int_equal(_scalar_int(raw, "PRAGMA user_version;"), 18);
    sqlite3_close(raw);
    unlink(path);
}

/* 回归：v16 库升级到 v17 时新增参数表，既有注册表行不丢。 */
static void test_v16_upgrade_adds_app_settings(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_migration_v16.db";
    unlink(path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    _exec(db,
          "INSERT INTO app_registry (id, name, version, state) "
          "VALUES ('keep', 'Keep App', '1.0.0', 1);");
    sqlite_svc_destroy(svc);

    sqlite3 *raw = NULL;
    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    _exec(raw,
          "DROP TABLE app_settings;"
          "DROP TABLE app_setting_schema;"
          "DELETE FROM schema_migration WHERE version=17;"
          "PRAGMA user_version=16;");
    sqlite3_close(raw);

    svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_int_equal(_scalar_int(db, "PRAGMA user_version;"), 18);
    assert_int_equal(
        _scalar_int(db,
                    "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name IN "
                    "('app_settings','app_setting_schema');"),
        2);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM app_registry WHERE id='keep';"), 1);

    sqlite_svc_destroy(svc);
    unlink(path);
}

/* 回归：v17 旧 telemetry（主键 ts）升级到 v18 时按墙钟主键重建。
 * 旧库无 shipped 数据，迁移按裁决丢弃旧行；ts 列保留供排障。 */
static void test_v17_upgrade_rebuilds_telemetry_wall_key(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_migration_v17.db";
    unlink(path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    _exec(db,
          "DROP TABLE telemetry;"
          "CREATE TABLE telemetry (ts INT, tag_id INT, val REAL, q INT, synced INT DEFAULT 0, "
          "PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;"
          "INSERT INTO telemetry (ts, tag_id, val, q) VALUES (111, 7, 1.0, 0);"
          "DELETE FROM schema_migration WHERE version=18;"
          "PRAGMA user_version=17;");
    sqlite_svc_destroy(svc);

    svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_int_equal(_scalar_int(db, "PRAGMA user_version;"), 18);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM telemetry;"), 0);
    assert_int_equal(_scalar_int(db,
                                 "SELECT COUNT(*) FROM pragma_table_info('telemetry') "
                                 "WHERE name='wall_ts_us';"),
                     1);

    PipeMsg m = {.ts = 42, .tag_id = 7, .quality = 0, .type = PIPEMSG_TYPE_SCALAR};
    assert_int_equal(sqlite_svc_insert_at(svc, &m, 1600000000000000ULL), 0);
    assert_int_equal(_scalar_int(db,
                                 "SELECT COUNT(*) FROM telemetry WHERE wall_ts_us = "
                                 "1600000000000000 AND tag_id = 7;"),
                     1);

    sqlite_svc_destroy(svc);
    unlink(path);
}

/* 子进程模式：打开同一 DB。用于并发迁移与 TOCTOU 回归。 */
static int _open_probe(const char *path, const char *ready_file) {
    if (ready_file) {
        FILE *f = fopen(ready_file, "w");
        if (f) fclose(f);
    }
    uv_loop_t *loop = uv_default_loop();
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    int err = 0;
    sqlite_svc_t *svc = sqlite_svc_create_ex(loop, &cfg, &err);
    if (!svc) return 3;
    sqlite_svc_destroy(svc);
    return 0;
}

static bool _wait_ready(const char *ready_file) {
    for (int i = 0; i < 200; i++) {
        if (access(ready_file, F_OK) == 0) return true;
        usleep(10000);
    }
    return false;
}

/**
 * TOCTOU 回归：B 在事务外读到 v17，A 随后提交 v18 并留下新行，B 再进入迁移。
 * 迁移必须在 BEGIN IMMEDIATE 后重读版本，跳过已应用的 v18，不能 DROP 掉 A 的行。
 *
 * 用父进程持写锁制造确定性交错：B 读到 17 后卡在 BEGIN IMMEDIATE；父进程在锁内
 * 把 user_version 改成 18 再提交；B 获得锁后必须看到 18。
 */
static void test_concurrent_migration_preserves_committed_row(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_migration_toctou.db";
    const char *ready = "test_sqlite_migration_toctou.ready";
    unlink(path);
    unlink(ready);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    PipeMsg marker = {.ts = 7, .tag_id = 0x77, .payload.val.f = 1.5};
    marker.type = PIPEMSG_TYPE_SCALAR;
    assert_int_equal(sqlite_svc_insert_at(svc, &marker, 1600000000000123ULL), 0);
    sqlite_svc_destroy(svc);

    /* 伪装成 v17：B 的快照会读到 17，但表里已有 A 的行。 */
    sqlite3 *raw = NULL;
    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    _exec(raw, "DELETE FROM schema_migration WHERE version=18; PRAGMA user_version=17;");
    _exec(raw, "BEGIN IMMEDIATE;");

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        execl("/proc/self/exe", "test_sqlite_migration", "--open-probe", path, ready,
              (char *)NULL);
        _exit(127);
    }
    assert_true(_wait_ready(ready));
    usleep(300000); /* 让子进程先读 17 并卡在 BEGIN IMMEDIATE */

    _exec(raw, "PRAGMA user_version=18;");
    _exec(raw, "COMMIT;");

    int status = 0;
    assert_int_equal(waitpid(pid, &status, 0), pid);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);

    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    assert_int_equal(_scalar_int(raw, "PRAGMA user_version;"), 18);
    assert_int_equal(_scalar_int(raw, "SELECT COUNT(*) FROM telemetry WHERE tag_id = 0x77;"), 1);
    assert_int_equal(_scalar_int(raw, "SELECT COUNT(*) FROM schema_migration;"), 18);
    sqlite3_close(raw);
    unlink(path);
    unlink(ready);
}

/* 两个 opener 同时迁移一个全新库：都必须成功，且只应用一遍。 */
static void test_concurrent_fresh_open_both_succeed(void **state) {
    (void)state;
    const char *path = "test_sqlite_migration_fresh_race.db";
    unlink(path);

    pid_t a = fork();
    assert_true(a >= 0);
    if (a == 0) {
        execl("/proc/self/exe", "test_sqlite_migration", "--open-probe", path, (char *)NULL);
        _exit(127);
    }
    pid_t b = fork();
    assert_true(b >= 0);
    if (b == 0) {
        execl("/proc/self/exe", "test_sqlite_migration", "--open-probe", path, (char *)NULL);
        _exit(127);
    }

    int status_a = 0;
    int status_b = 0;
    assert_int_equal(waitpid(a, &status_a, 0), a);
    assert_int_equal(waitpid(b, &status_b, 0), b);
    assert_true(WIFEXITED(status_a));
    assert_true(WIFEXITED(status_b));
    assert_int_equal(WEXITSTATUS(status_a), 0);
    assert_int_equal(WEXITSTATUS(status_b), 0);

    sqlite3 *raw = NULL;
    assert_int_equal(sqlite3_open(path, &raw), SQLITE_OK);
    assert_int_equal(_scalar_int(raw, "PRAGMA user_version;"), 18);
    assert_int_equal(_scalar_int(raw, "SELECT COUNT(*) FROM schema_migration;"), 18);
    sqlite3_close(raw);
    unlink(path);
}

/* 回归：v17 库里的旧形状分表 telemetry_YYYYMM 在 v18 升级时也要重建，
 * 否则 SHARDING 写入会因缺 wall_ts_us 静默丢失。 */
static void test_v17_shard_upgrade_rebuilds_and_writes(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_migration_shard.db";
    unlink(path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = path};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    _exec(db,
          "CREATE TABLE telemetry_197001 (ts INT, tag_id INT, val REAL, q INT, "
          "synced INT DEFAULT 0, PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;"
          "INSERT INTO telemetry_197001 (ts, tag_id, val, q) VALUES (1, 9, 1.0, 0);"
          "DELETE FROM schema_migration WHERE version=18;"
          "PRAGMA user_version=17;");
    sqlite_svc_destroy(svc);

    /* v18 升级：基表与分表一起重建。 */
    svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_int_equal(_scalar_int(db, "PRAGMA user_version;"), 18);
    assert_int_equal(_scalar_int(db,
                                 "SELECT COUNT(*) FROM pragma_table_info('telemetry_197001') "
                                 "WHERE name='wall_ts_us';"),
                     1);
    assert_int_equal(_scalar_int(db, "SELECT COUNT(*) FROM telemetry_197001;"), 0);
    sqlite_svc_destroy(svc);

    /* SHARDING 写入必须落库：wall_ts_us=1600000000000123 µs 属于 2020-09。 */
    sqlite_config_t shard_cfg = {
        .strategy = STRATEGY_SHARDING, .db_path = path, .batch_size = 8, .flush_interval_ms = 60000};
    svc = sqlite_svc_create(loop, &shard_cfg);
    assert_non_null(svc);
    PipeMsg m = {.ts = 42, .tag_id = 11, .payload.val.f = 3.5, .type = PIPEMSG_TYPE_SCALAR};
    assert_int_equal(sqlite_svc_insert_at(svc, &m, 1600000000000123ULL), 0);
    assert_int_equal(sqlite_svc_flush_sync(svc), 0);

    db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    assert_int_equal(_scalar_int(db,
                                 "SELECT COUNT(*) FROM telemetry_202009 WHERE wall_ts_us = "
                                 "1600000000000123 AND tag_id = 11;"),
                     1);
    sqlite_svc_destroy(svc);
    unlink(path);
}

/* 旧形状分表若绕过迁移直接存在，flush 必须 fail closed 并保留批次，不能静默丢行。 */
static void test_sharding_flush_fails_closed_on_stale_shard(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_stale_shard.db";
    unlink(path);

    sqlite_config_t cfg = {
        .strategy = STRATEGY_SHARDING, .db_path = path, .batch_size = 8, .flush_interval_ms = 60000};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    _exec(db,
          "CREATE TABLE telemetry_202009 (ts INT, tag_id INT, val REAL, q INT, "
          "synced INT DEFAULT 0, PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;");

    PipeMsg m = {.ts = 43, .tag_id = 12, .payload.val.f = 4.5, .type = PIPEMSG_TYPE_SCALAR};
    assert_int_equal(sqlite_svc_insert_at(svc, &m, 1600000000000123ULL), 0);
    assert_int_equal(sqlite_svc_flush_sync(svc), -1); /* stale shard -> fail closed */

    /* 删除旧表后重试：新形状建表并落库，证明失败可恢复且没有丢批次。 */
    _exec(db, "DROP TABLE telemetry_202009;");
    assert_int_equal(sqlite_svc_flush_sync(svc), 0);
    assert_int_equal(_scalar_int(db,
                                 "SELECT COUNT(*) FROM telemetry_202009 WHERE wall_ts_us = "
                                 "1600000000000123 AND tag_id = 12;"),
                     1);
    sqlite_svc_destroy(svc);
    unlink(path);
}

/* 分表存在但列不全时，prepare 失败必须让 flush_sync 返回失败，不能 COMMIT 假成功。 */
static void test_sharding_flush_reports_prepare_failure(void **state) {
    uv_loop_t *loop = (uv_loop_t *)*state;
    const char *path = "test_sqlite_bad_shard.db";
    unlink(path);

    sqlite_config_t cfg = {
        .strategy = STRATEGY_SHARDING, .db_path = path, .batch_size = 8, .flush_interval_ms = 60000};
    sqlite_svc_t *svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(svc);
    /* 有 wall_ts_us，但缺 val：_ensure_shard_exists 会放行，prepare 必须失败。 */
    _exec(db,
          "CREATE TABLE telemetry_202010 (wall_ts_us INTEGER NOT NULL, tag_id INT, ts INT, "
          "q INT, synced INT DEFAULT 0, PRIMARY KEY(wall_ts_us, tag_id)) WITHOUT ROWID;");

    PipeMsg m = {.ts = 44, .tag_id = 13, .payload.val.f = 5.5, .type = PIPEMSG_TYPE_SCALAR};
    assert_int_equal(sqlite_svc_insert_at(svc, &m, 1601600000000000ULL), 0);
    assert_int_equal(sqlite_svc_flush_sync(svc), -1);
    sqlite_svc_destroy(svc);
    unlink(path);
}

int main(int argc, char **argv) {
    if (argc >= 3 && strcmp(argv[1], "--open-probe") == 0) {
        return _open_probe(argv[2], argc >= 4 ? argv[3] : NULL);
    }

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_v3_upgrade_preserves_edge_settings, setup, teardown),
        cmocka_unit_test_setup_teardown(test_v4_upgrade_preserves_app_registry, setup, teardown),
        cmocka_unit_test_setup_teardown(test_v16_upgrade_adds_app_settings, setup, teardown),
        cmocka_unit_test_setup_teardown(test_v17_upgrade_rebuilds_telemetry_wall_key, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_v17_shard_upgrade_rebuilds_and_writes, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_sharding_flush_fails_closed_on_stale_shard, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_sharding_flush_reports_prepare_failure, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_concurrent_migration_preserves_committed_row,
                                        setup, teardown),
        cmocka_unit_test_setup_teardown(test_concurrent_fresh_open_both_succeed, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_too_new_schema_rejected, setup, teardown),
        cmocka_unit_test_setup_teardown(test_checksum_mismatch_rejected, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
