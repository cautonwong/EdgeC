/**
 * @file test_snf_delivery_confirm.c
 * @brief SnF「真送达」确认语义（ADR-002 北向 N1-3）
 *
 * ## 为什么需要这组测试
 *
 * `publish_fn` 返回 0 只代表**入队成功**（`MQTTAsync_sendMessage` 接受了这条消息），
 * 不代表 broker 收到。原来的实现拿到 0 就直接
 * `UPDATE telemetry SET synced = 1`，于是：
 *
 *   断网瞬间 → 消息进了 Paho 的内存队列（PERSISTENCE_NONE）→ SnF 已标"已同步"
 *   → 进程退出队列消失 → **这条数据永久丢失，且没有任何日志**。
 *
 * 修法：整批确认。只有 `pending_fn()` 报"没有在途消息"时才标 synced；
 * 否则不标，下一轮重发（at-least-once：允许重复，不允许丢）。
 *
 * 这三条用例锁住的就是这个语义。
 */

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

#include "edge_log.h"
#include "snf_manager.h"
#include "snf_sqlite_adapter.h"
#include "sqlite_service.h"

/* ---- 可控的 mock 传输 ---------------------------------------------------- */

static int g_publish_rc;      /* publish_fn 的返回值 */
static uint32_t g_pending;    /* pending_fn 报的在途条数 */
static int g_publish_calls;
static uint32_t g_pending_after_publish;
static uint32_t g_failures;
static uint32_t g_fail_on_publish;

static uint32_t mock_take_failures(void* ud) {
    (void)ud;
    return __atomic_exchange_n(&g_failures, 0, __ATOMIC_SEQ_CST);
}

static int mock_publish(void* ud, const PipeMsg* msg) {
    (void)ud;
    (void)msg;
    g_publish_calls++;
    if (g_pending_after_publish) g_pending = g_pending_after_publish;
    if (g_fail_on_publish) g_failures = g_fail_on_publish;
    return g_publish_rc;
}

static uint32_t mock_pending(void* ud) {
    (void)ud;
    return g_pending;
}

/* ---- 辅助 ---------------------------------------------------------------- */

static sqlite_svc_t* make_db(uv_loop_t* loop, const char* path) {
    unlink(path);
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE,
                           .db_path = path,
                           .batch_size = 0,
                           .flush_interval_ms = 0};
    return sqlite_svc_create(loop, &cfg);
}

static int count_where(sqlite_svc_t* db, const char* sql) {
    sqlite3* h = (sqlite3*)sqlite_svc_get_db_handle(db);
    sqlite3_stmt* stmt = NULL;
    int n = -1;
    if (sqlite3_prepare_v2(h, sql, -1, &stmt, NULL) == SQLITE_OK &&
        sqlite3_step(stmt) == SQLITE_ROW) {
        n = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return n;
}

static int count_unsynced(sqlite_svc_t* db) {
    return count_where(db, "SELECT count(*) FROM telemetry WHERE synced = 0;");
}

static int count_synced(sqlite_svc_t* db) {
    return count_where(db, "SELECT count(*) FROM telemetry WHERE synced = 1;");
}

static void buffer_n(snf_manager_t* mgr, int n, uint32_t tag) {
    for (int i = 0; i < n; i++) {
        PipeMsg msg = {0};
        msg.ts = (uint64_t)(1000 + i);
        msg.tag_id = tag;
        msg.payload.val.f = (double)i;
        msg.type = PIPEMSG_TYPE_SCALAR;
        assert_int_equal(snf_manager_buffer_data(mgr, &msg), 0);
    }
}

static void pump(uv_loop_t* loop, int iterations) {
    for (int i = 0; i < iterations; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        usleep(5000);
    }
}

/* 每个用例独立的存储：snf_manager_create() 用的是全局 static 存储，
 * 用例之间会互相踩，所以这里一律用 create_in。 */
static snf_manager_storage_t g_storage;
static snf_store_port_t g_store;

static snf_manager_t* make_snf(sqlite_svc_t* db, uv_loop_t* loop, uint32_t pending,
                               int publish_rc) {
    g_publish_rc = publish_rc;
    g_pending = pending;
    g_publish_calls = 0;
    g_pending_after_publish = 0;
    g_failures = 0;
    g_fail_on_publish = 0;

    g_store = snf_sqlite_store_port(db);
    snf_config_t cfg = {.store = &g_store,
                        .loop = loop,
                        .publish_fn = mock_publish,
                        .pending_fn = mock_pending,
                        .failures_fn = mock_take_failures,
                        .sync_batch_size = 10,
                        .sync_interval_ms = 20};
    return snf_manager_create_in(&g_storage, &cfg);
}

static void teardown(snf_manager_t* mgr, sqlite_svc_t* db, uv_loop_t* loop) {
    snf_manager_destroy(mgr);
    pump(loop, 10); /* 让 uv_close 的回调跑完，否则下一个用例复用存储会踩到野句柄 */
    sqlite_svc_destroy(db);
}

/* ---- 用例 ---------------------------------------------------------------- */

/**
 * 有在途消息时**一条都不能标 synced**。
 *
 * 这正是原实现丢数据的地方：publish 返回 0 就标了。
 */
static void test_inflight_blocks_confirm(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, "snf_confirm_inflight.db");
    assert_non_null(db);

    snf_manager_t* mgr = make_snf(db, loop, /*pending=*/3, /*rc=*/0);
    assert_non_null(mgr);

    buffer_n(mgr, 4, 1001);
    assert_int_equal(count_unsynced(db), 4);

    snf_manager_set_online(mgr, true);
    pump(loop, 40);

    /* 消息确实交给传输了…… */
    assert_true(g_publish_calls >= 4);
    /* ……但 broker 还没确认，所以一条都不能算送达。 */
    assert_int_equal(count_synced(db), 0);
    assert_int_equal(count_unsynced(db), 4);

    teardown(mgr, db, loop);
}

/**
 * 在途清空后，下一轮必须把这一批标成已确认。
 *
 * 防止"修成永远不确认"——那是另一种数据积压。
 */
static void test_confirms_after_inflight_clears(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, "snf_confirm_drain.db");
    assert_non_null(db);

    snf_manager_t* mgr = make_snf(db, loop, /*pending=*/2, /*rc=*/0);
    assert_non_null(mgr);

    buffer_n(mgr, 3, 1002);
    snf_manager_set_online(mgr, true);

    pump(loop, 30);
    assert_int_equal(count_synced(db), 0); /* 还在途 */

    g_pending = 0; /* PUBACK 到齐 */
    pump(loop, 40);

    assert_int_equal(count_synced(db), 3);
    assert_int_equal(count_unsynced(db), 0);

    teardown(mgr, db, loop);
}

/**
 * publish 直接失败（未连接 / 入队被拒）时，一条都不能标。
 *
 * 对应北向管理器里删掉的"printf 假成功"：未连接必须返回非 0，
 * 否则 SnF 会把它当成功。
 */
static void test_publish_failure_keeps_unsynced(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, "snf_confirm_fail.db");
    assert_non_null(db);

    snf_manager_t* mgr = make_snf(db, loop, /*pending=*/0, /*rc=*/-1);
    assert_non_null(mgr);

    buffer_n(mgr, 5, 1003);
    snf_manager_set_online(mgr, true);

    pump(loop, 40);

    assert_true(g_publish_calls > 0);      /* 试过了 */
    assert_int_equal(count_synced(db), 0); /* 没成功 */
    assert_int_equal(count_unsynced(db), 5);

    teardown(mgr, db, loop);
}


/**
 * 有在途消息时，同一批不能每个 timer 都重新 publish。
 * 旧实现会在 pending 不清零时每 20ms 重复发送同一批。
 */
static void test_inflight_does_not_republish(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, "snf_confirm_no_dup.db");
    assert_non_null(db);

    snf_manager_t* mgr = make_snf(db, loop, /*pending=*/1, /*rc=*/0);
    assert_non_null(mgr);

    buffer_n(mgr, 3, 1004);
    snf_manager_set_online(mgr, true);
    pump(loop, 40);
    int first_calls = g_publish_calls;
    assert_true(first_calls >= 3);

    pump(loop, 40);
    assert_int_equal(g_publish_calls, first_calls);
    assert_int_equal(count_synced(db), 0);

    g_pending = 0;
    pump(loop, 40);
    assert_int_equal(count_synced(db), 3);
    assert_int_equal(g_publish_calls, first_calls);

    teardown(mgr, db, loop);
}

static void test_publish_failure_blocks_confirm(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, "snf_confirm_failure.db");
    assert_non_null(db);

    snf_manager_t* mgr = make_snf(db, loop, /*pending=*/0, /*rc=*/0);
    assert_non_null(mgr);
    g_fail_on_publish = 1;

    buffer_n(mgr, 2, 1006);
    snf_manager_set_online(mgr, true);
    pump(loop, 40);
    assert_int_equal(count_synced(db), 0);
    assert_int_equal(count_unsynced(db), 2);

    g_fail_on_publish = 0;
    pump(loop, 40);
    assert_int_equal(count_synced(db), 2);

    teardown(mgr, db, loop);
}

static void test_pending_after_publish_blocks_confirm(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, "snf_confirm_after_publish.db");
    assert_non_null(db);

    snf_manager_t* mgr = make_snf(db, loop, /*pending=*/0, /*rc=*/0);
    assert_non_null(mgr);
    g_pending_after_publish = 2;

    buffer_n(mgr, 2, 1005);
    snf_manager_set_online(mgr, true);
    pump(loop, 40);

    assert_true(g_publish_calls >= 2);
    assert_int_equal(count_synced(db), 0);
    assert_int_equal(count_unsynced(db), 2);

    g_pending = 0;
    pump(loop, 40);
    assert_int_equal(count_synced(db), 2);

    teardown(mgr, db, loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_inflight_blocks_confirm),
        cmocka_unit_test(test_publish_failure_blocks_confirm),
        cmocka_unit_test(test_pending_after_publish_blocks_confirm),
        cmocka_unit_test(test_confirms_after_inflight_clears),
        cmocka_unit_test(test_publish_failure_keeps_unsynced),
        cmocka_unit_test(test_inflight_does_not_republish),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
