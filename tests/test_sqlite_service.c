#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "sqlite_service.h"

// --- 辅助工具 ---

static int get_user_version(sqlite3* db) {
    sqlite3_stmt* stmt;
    int ver = 0;
    sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) ver = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return ver;
}

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

// --- Lifecycle ---

static int setup(void** state) {
    uv_loop_t* loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    *state = loop;
    return 0;
}

static int teardown(void** state) {
    uv_loop_t* loop = (uv_loop_t*)*state;
    uv_walk(loop, on_walk_close, NULL);
    while (uv_loop_close(loop) == UV_EBUSY) uv_run(loop, UV_RUN_ONCE);
    free(loop);
    return 0;
}

// --- 1. 测试：生命周期与策略初始化 (Cover: create, destroy, get_db_handle) ---
static void test_lifecycle_and_strategies(void** state) {
    uv_loop_t* loop = (uv_loop_t*)*state;
    const char* db_path = "test_lifecycle.db";
    unlink(db_path);

    // 测试 LITE 策略
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t* svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    assert_non_null(sqlite_svc_get_db_handle(svc));
    sqlite_svc_destroy(svc);

    // 测试 SHARDING 策略
    cfg.strategy = STRATEGY_SHARDING;
    cfg.batch_size = 10;
    cfg.flush_interval_ms = 100;
    svc = sqlite_svc_create(loop, &cfg);
    assert_non_null(svc);
    sqlite_svc_destroy(svc);

    unlink(db_path);
}

// --- 2. 测试：时序数据插入与查询 (Cover: insert, query) ---
static int _query_row_count = 0;
static int _query_cb(void* user_data, PipeMsg* msg) {
    (void)user_data;
    assert_int_equal(msg->tag_id, 1001);
    _query_row_count++;
    return 0;
}

static void test_insert_and_query(void** state) {
    uv_loop_t* loop = (uv_loop_t*)*state;
    const char* db_path = "test_data.db";
    unlink(db_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t* svc = sqlite_svc_create(loop, &cfg);

    uint64_t now = 1704240000000ULL;
    PipeMsg m = {.ts = now, .tag_id = 1001, .quality = 0, .type = PIPEMSG_TYPE_SCALAR};
    m.payload.val.f = 220.5;

    // 插入
    assert_int_equal(sqlite_svc_insert(svc, &m), 0);

    // 查询
    _query_row_count = 0;
    assert_int_equal(sqlite_svc_query(svc, now - 1000, now + 1000, _query_cb, NULL), 0);
    assert_int_equal(_query_row_count, 1);

    sqlite_svc_destroy(svc);
    unlink(db_path);
}

// --- 3. 测试：事务控制 (Cover: begin_trans, commit_trans) ---
static void test_transactions(void** state) {
    uv_loop_t* loop = (uv_loop_t*)*state;
    sqlite_config_t cfg = {.strategy = STRATEGY_TRANSACTION, .db_path = ":memory:"};
    sqlite_svc_t* svc = sqlite_svc_create(loop, &cfg);

    assert_int_equal(sqlite_svc_begin_trans(svc), 0);
    // 模拟一些操作...
    assert_int_equal(sqlite_svc_commit_trans(svc), 0);

    sqlite_svc_destroy(svc);
}

// --- 4. 测试：异步 SQL 执行 (Cover: exec) ---
static int _async_exec_done = 0;
static void _async_exec_cb(void* user_data, int status, char** errmsg) {
    (void)user_data;
    (void)errmsg;
    assert_int_equal(status, SQLITE_OK);
    _async_exec_done = 1;
}

static void test_async_exec(void** state) {
    uv_loop_t* loop = (uv_loop_t*)*state;
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    sqlite_svc_t* svc = sqlite_svc_create(loop, &cfg);

    _async_exec_done = 0;
    sqlite_svc_exec(svc, "CREATE TABLE async_test (id INT);", _async_exec_cb, NULL);

    // 驱动 Loop 直到回调执行
    for (int i = 0; i < 50 && !_async_exec_done; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        usleep(10000);
    }
    assert_int_equal(_async_exec_done, 1);

    sqlite_svc_destroy(svc);
}

// --- 5. 测试：BLOB 参数持久化 (Cover: set_param, get_param) ---
static void test_param_blob_persistence(void** state) {
    uv_loop_t* loop = (uv_loop_t*)*state;
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    sqlite_svc_t* svc = sqlite_svc_create(loop, &cfg);

    // 测试结构体存储
    struct {
        int a;
        float b;
    } my_cfg = {42, 3.14f}, out_cfg = {0};
    assert_int_equal(sqlite_svc_set_param(svc, "UNIT_TEST", 9001, &my_cfg, sizeof(my_cfg)), 0);
    assert_int_equal(sqlite_svc_get_param(svc, "UNIT_TEST", 9001, &out_cfg, sizeof(out_cfg)),
                     sizeof(my_cfg));
    assert_int_equal(out_cfg.a, 42);
    assert_float_equal(out_cfg.b, 3.14f, 0.001);

    sqlite_svc_destroy(svc);
}

// --- 6. 测试：模式自动迁移 (Cover: Migration V1-V4) ---
static void test_migration_logic(void** state) {
    uv_loop_t* loop = (uv_loop_t*)*state;
    const char* db_path = "test_mig.db";
    unlink(db_path);

    sqlite_config_t cfg = {.strategy = STRATEGY_LITE, .db_path = db_path};
    sqlite_svc_t* svc = sqlite_svc_create(loop, &cfg);

    sqlite3* db = sqlite_svc_get_db_handle(svc);
    assert_int_equal(get_user_version(db), 5);  // 验证已达到最新版本 (V5: App Registry)

    sqlite_svc_destroy(svc);
    unlink(db_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_lifecycle_and_strategies, setup, teardown),
        cmocka_unit_test_setup_teardown(test_insert_and_query, setup, teardown),
        cmocka_unit_test_setup_teardown(test_transactions, setup, teardown),
        cmocka_unit_test_setup_teardown(test_async_exec, setup, teardown),
        cmocka_unit_test_setup_teardown(test_param_blob_persistence, setup, teardown),
        cmocka_unit_test_setup_teardown(test_migration_logic, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
