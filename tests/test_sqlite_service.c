#include <cmocka.h>
#include <setjmp.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <uv.h>

#include "sqlite_service.h"

// 辅助：获取 DB 版本
static int get_user_version(sqlite3 *db) {
    sqlite3_stmt *stmt;
    int ver = 0;
    sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ver = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return ver;
}

// 辅助：检查表是否存在
static int table_exists(sqlite3 *db, const char *name) {
    char sql[128];
    snprintf(sql, 128, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='%s';",
             name);
    sqlite3_stmt *stmt;
    int exists = 0;
    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return exists;
}

// Case 1: 验证 Schema 迁移
static void test_migration(void **state) {
    (void)state;
    const char *db_path = "test_mig.db";
    unlink(db_path);  // Clean start

    uv_loop_t loop;
    uv_loop_init(&loop);

    sqlite_config_t cfg = {.type = PROD_IEMS, .db_path = db_path};

    // 1. Init (Should create v1 schema)
    sqlite_svc_t *svc = sqlite_svc_create(&loop, &cfg);
    assert_non_null(svc);

    // Check version
    // 我们需要通过 private access 或 重新打开 db 来检查。这里简单起见，利用 svc 尚未公开 db 指针，
    // 我们手动打开同一个文件检查。
    sqlite3 *check_db;
    sqlite3_open(db_path, &check_db);

    assert_int_equal(get_user_version(check_db), 1);  // SCHEMA_VER_IEMS
    assert_int_equal(table_exists(check_db, "telemetry"), 1);

    sqlite3_close(check_db);
    sqlite_svc_destroy(svc);

    // Cleanup
    unlink(db_path);
    uv_loop_close(&loop);
}

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

// Case 2: 验证 DCU 分表逻辑
static void test_sharding(void **state) {
    (void)state;
    const char *db_path = "test_shard.db";
    unlink(db_path);

    uv_loop_t loop;
    uv_loop_init(&loop);

    sqlite_config_t cfg = {.type = PROD_DCU,
                           .db_path = db_path,
                           .batch_size = 1,
                           .flush_interval_ms = 100};

    sqlite_svc_t *svc = sqlite_svc_create(&loop, &cfg);
    assert_non_null(svc);

    // DCU 模式下，PipeMsg.ts 代表分表 ID (YYYYMM)
    // 写入 202311 和 202312
    PipeMsg m1 = {.ts = 202311, .tag_id = 1, .type = PIPEMSG_SCALAR_F64, .payload.val_f = 10.0};
    PipeMsg m2 = {.ts = 202312, .tag_id = 1, .type = PIPEMSG_SCALAR_F64, .payload.val_f = 20.0};
    
    sqlite_svc_insert(svc, &m1);
    sqlite_svc_insert(svc, &m2);
    
    // 运行 loop 等待异步写入完成
    for(int i=0; i<100; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        usleep(5000);
    }
    
    sqlite_svc_destroy(svc);
    
    // 验证
    sqlite3 *check_db = NULL;
    if (sqlite3_open_v2(db_path, &check_db, SQLITE_OPEN_READONLY, NULL) == SQLITE_OK) {
        assert_int_equal(table_exists(check_db, "meter_data_202311"), 1);
        assert_int_equal(table_exists(check_db, "meter_data_202312"), 1);
        sqlite3_close(check_db);
    }
    
    // 彻底清理 loop 上的残留句柄 (例如定时器)
    uv_walk(&loop, on_walk_close, NULL);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    
    unlink(db_path);
}

// Case 3: 验证 EVSE 模式配置
static void test_evse_mode(void **state) {
    (void)state;
    const char *db_path = "test_evse.db";
    unlink(db_path);

    uv_loop_t loop;
    uv_loop_init(&loop);

    sqlite_config_t cfg = {.type = PROD_EVSE, .db_path = db_path};
    sqlite_svc_t *svc = sqlite_svc_create(&loop, &cfg);

    // EVSE 禁止直接 insert
    PipeMsg m = {0};
    assert_int_not_equal(sqlite_svc_insert(svc, &m), 0);

    sqlite_svc_destroy(svc);
    unlink(db_path);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_migration),
        cmocka_unit_test(test_sharding),
        cmocka_unit_test(test_evse_mode),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
