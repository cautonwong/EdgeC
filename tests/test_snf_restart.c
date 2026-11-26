/**
 * @file test_snf_restart.c
 * @brief SnF persistence across a manager restart (P4-03b).
 *
 * The batch a manager keeps in memory is only an optimisation. The durable
 * record is the `telemetry.synced` flag: rows are marked synced only after the
 * transport confirms them, so a restart must replay every unsynced row in ts
 * order and must not replay a row that was already confirmed. An in-flight
 * batch at shutdown is therefore replayed, not lost.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "snf_manager.h"
#include "snf_sqlite_adapter.h"
#include "sqlite_service.h"

#define MAX_PUBLISHED 128

static int g_publish_rc;
static uint32_t g_pending;
static uint32_t g_failures;
static uint64_t g_published[MAX_PUBLISHED];
static int g_published_count;

static uv_loop_t *g_loop;
static sqlite_svc_t *g_db;
static snf_manager_t *g_mgr;
static const char *g_db_path;

static int mock_publish(void *ud, const PipeMsg *msg, uint64_t wall_ts_us) {
    (void)ud;
    (void)wall_ts_us;
    if (g_publish_rc == 0 && g_published_count < MAX_PUBLISHED) {
        g_published[g_published_count++] = msg->ts;
    }
    return g_publish_rc;
}

static uint32_t mock_pending(void *ud) {
    (void)ud;
    return g_pending;
}

static uint32_t mock_failures(void *ud) {
    (void)ud;
    return __atomic_exchange_n(&g_failures, 0, __ATOMIC_SEQ_CST);
}

/* ---- 文件型 DB 与 SQL 断言 ---------------------------------------------- */

static void remove_db_files(const char *path) {
    if (!path) return;
    unlink(path);
    char sidecar[256];
    snprintf(sidecar, sizeof(sidecar), "%s-wal", path);
    unlink(sidecar);
    snprintf(sidecar, sizeof(sidecar), "%s-shm", path);
    unlink(sidecar);
}

static sqlite_svc_t *open_db(uv_loop_t *loop, const char *path, bool fresh) {
    if (fresh) remove_db_files(path);
    g_db_path = path;
    sqlite_config_t cfg = {
        .strategy = STRATEGY_LITE, .db_path = path, .batch_size = 0, .flush_interval_ms = 0};
    g_db = sqlite_svc_create(loop, &cfg);
    return g_db;
}

static void close_db(void) {
    if (!g_db) return;
    sqlite_svc_destroy(g_db);
    g_db = NULL;
}

static int scalar(sqlite_svc_t *db, const char *sql) {
    sqlite3 *h = (sqlite3 *)sqlite_svc_get_db_handle(db);
    if (!h) return -1;
    sqlite3_stmt *stmt = NULL;
    int n = -1;
    if (sqlite3_prepare_v2(h, sql, -1, &stmt, NULL) == SQLITE_OK &&
        sqlite3_step(stmt) == SQLITE_ROW) {
        n = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return n;
}

static int unsynced(sqlite_svc_t *db) {
    return scalar(db, "SELECT count(*) FROM telemetry WHERE synced = 0;");
}

static int synced(sqlite_svc_t *db) {
    return scalar(db, "SELECT count(*) FROM telemetry WHERE synced = 1;");
}

/* ---- manager 装配 -------------------------------------------------------- */

typedef struct {
    snf_manager_storage_t storage;
    snf_store_port_t store;
} snf_rig_t;

/* 句柄内存必须比用例函数活得久：断言失败时 teardown 仍要销毁定时器。 */
static snf_rig_t g_rig;

static snf_manager_t *start_mgr(uv_loop_t *loop, uint32_t pending) {
    g_rig.store = snf_sqlite_store_port(g_db);
    snf_config_t cfg = {.store = &g_rig.store,
                        .loop = loop,
                        .publish_fn = mock_publish,
                        .pending_fn = mock_pending,
                        .failures_fn = mock_failures,
                        .sync_batch_size = SNF_MAX_BATCH,
                        .sync_interval_ms = 20};
    g_publish_rc = 0;
    g_pending = pending;
    g_failures = 0;
    g_published_count = 0;
    g_mgr = snf_manager_create_in(&g_rig.storage, &cfg);
    return g_mgr;
}

static void pump(uv_loop_t *loop, int iterations) {
    for (int i = 0; i < iterations; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        usleep(5000);
    }
}

static void pump_until_drained(uv_loop_t *loop, sqlite_svc_t *db) {
    for (int i = 0; i < 200; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (unsynced(db) == 0) return;
        usleep(5000);
    }
}

static void stop_mgr(uv_loop_t *loop) {
    if (!g_mgr) return;
    snf_manager_destroy(g_mgr);
    pump(loop, 10); /* 跑完 uv_close 回调，避免复用存储时踩野句柄 */
    g_mgr = NULL;
}

static void buffer_n(snf_manager_t *mgr, uint64_t first_ts, int n, uint32_t tag) {
    for (int i = 0; i < n; i++) {
        PipeMsg msg = {0};
        msg.ts = first_ts + (uint64_t)i;
        msg.tag_id = tag;
        msg.payload.val.f = (double)i;
        msg.type = PIPEMSG_TYPE_SCALAR;
        assert_int_equal(snf_manager_buffer_data(mgr, &msg), 0);
    }
}

/* 子进程模式：写完 8 行直接 _exit，不 destroy、不 sqlite3_close，模拟进程崩溃。
 * WAL 文件仍在，父进程重开时必须能读到这些未确认行。 */
static void crash_writer(const char *path) {
    g_loop = uv_default_loop();
    if (!open_db(g_loop, path, true)) _exit(2);
    if (!start_mgr(g_loop, /*pending=*/0)) _exit(3);
    for (int i = 0; i < 8; i++) {
        PipeMsg msg = {0};
        msg.ts = 12000 + (uint64_t)i;
        msg.tag_id = 3001;
        msg.payload.val.f = (double)i;
        msg.type = PIPEMSG_TYPE_SCALAR;
        if (snf_manager_buffer_data(g_mgr, &msg) != 0) _exit(4);
    }
    _exit(0);
}

static int fixture_setup(void **state) {
    (void)state;
    g_loop = uv_default_loop();
    g_db = NULL;
    g_mgr = NULL;
    g_db_path = NULL;
    return 0;
}

/* 断言失败时 cmocka 仍会执行这里：句柄一定销毁，DB 文件一定删除，避免串扰。 */
static int fixture_teardown(void **state) {
    (void)state;
    stop_mgr(g_loop);
    close_db();
    remove_db_files(g_db_path);
    g_db_path = NULL;
    return 0;
}

/* ---- 用例 ---------------------------------------------------------------- */

static void test_restart_replays_unsynced_in_order(void **state) {
    (void)state;
    const char *path = "snf_restart_replay.db";
    assert_non_null(open_db(g_loop, path, true));

    snf_manager_t *mgr = start_mgr(g_loop, /*pending=*/0);
    assert_non_null(mgr);

    buffer_n(mgr, 5000, 8, 1001);
    assert_int_equal(unsynced(g_db), 8);
    assert_int_equal(synced(g_db), 0);

    stop_mgr(g_loop);
    close_db();

    assert_non_null(open_db(g_loop, path, false));
    assert_int_equal(unsynced(g_db), 8); /* 缓冲行随 DB 存活 */
    assert_int_equal(synced(g_db), 0);

    mgr = start_mgr(g_loop, /*pending=*/0);
    assert_non_null(mgr);
    snf_manager_set_online(mgr, true);
    pump_until_drained(g_loop, g_db);

    assert_int_equal(g_published_count, 8);
    for (int i = 0; i < 8; i++) {
        assert_int_equal(g_published[i], (uint64_t)(5000 + i));
    }
    assert_int_equal(synced(g_db), 8);
    assert_int_equal(unsynced(g_db), 0);
}

static void test_restart_inflight_rows_replay(void **state) {
    (void)state;
    const char *path = "snf_restart_inflight.db";
    assert_non_null(open_db(g_loop, path, true));

    /* pending 恒为 3：发布成功但永远等不到完整 ACK，批次留在内存、DB 行保持未确认。 */
    snf_manager_t *mgr = start_mgr(g_loop, /*pending=*/3);
    assert_non_null(mgr);

    buffer_n(mgr, 7000, 5, 1002);
    snf_manager_set_online(mgr, true);
    pump(g_loop, 40);

    assert_int_equal(g_published_count, 5);
    assert_int_equal(unsynced(g_db), 5);
    assert_int_equal(synced(g_db), 0);

    stop_mgr(g_loop);
    close_db();

    assert_non_null(open_db(g_loop, path, false));
    assert_int_equal(unsynced(g_db), 5); /* 重启时在途的行仍是未确认，必须能重发 */

    mgr = start_mgr(g_loop, /*pending=*/0);
    assert_non_null(mgr);
    snf_manager_set_online(mgr, true);
    pump_until_drained(g_loop, g_db);

    assert_int_equal(g_published_count, 5);
    for (int i = 0; i < 5; i++) {
        assert_int_equal(g_published[i], (uint64_t)(7000 + i));
    }
    assert_int_equal(synced(g_db), 5);
    assert_int_equal(unsynced(g_db), 0);
}

static void test_restart_confirmed_rows_not_replayed(void **state) {
    (void)state;
    const char *path = "snf_restart_confirmed.db";
    assert_non_null(open_db(g_loop, path, true));

    snf_manager_t *mgr = start_mgr(g_loop, /*pending=*/0);
    assert_non_null(mgr);

    buffer_n(mgr, 8000, 3, 1003);
    snf_manager_set_online(mgr, true);
    pump_until_drained(g_loop, g_db);
    assert_int_equal(synced(g_db), 3);
    assert_int_equal(g_published_count, 3);

    snf_manager_set_online(mgr, false);
    buffer_n(mgr, 9000, 2, 1003);
    assert_int_equal(unsynced(g_db), 2);
    assert_int_equal(synced(g_db), 3);

    stop_mgr(g_loop);
    close_db();

    assert_non_null(open_db(g_loop, path, false));
    assert_int_equal(unsynced(g_db), 2);
    assert_int_equal(synced(g_db), 3);

    mgr = start_mgr(g_loop, /*pending=*/0);
    assert_non_null(mgr);
    snf_manager_set_online(mgr, true);
    pump_until_drained(g_loop, g_db);

    /* 已确认的 3 条不得重发：发布记录只有重启前的 2 条离线行。 */
    assert_int_equal(g_published_count, 2);
    assert_int_equal(g_published[0], 9000);
    assert_int_equal(g_published[1], 9001);
    assert_int_equal(synced(g_db), 5);
    assert_int_equal(unsynced(g_db), 0);
    assert_int_equal(scalar(g_db, "SELECT count(*) FROM telemetry;"), 5);
    assert_int_equal(
        scalar(g_db, "SELECT count(*) FROM (SELECT DISTINCT ts, tag_id FROM telemetry);"), 5);
}

/**
 * 进程崩溃（不是 graceful close）后，未确认行必须还能补发。
 *
 * 子进程 exec 自身写完 8 行就 _exit，SQLite 不会 checkpoint；数据只在 WAL 尾部。
 * 父进程重开同一文件必须看到 8 条 synced=0 并完成补发。
 * 边界：journal_mode=WAL + synchronous=NORMAL 下掉电可能丢 WAL 尾部，见 P4-03b
 * 残余风险；本用例只证明进程崩溃。
 */
static void test_process_crash_replays_unsynced_rows(void **state) {
    (void)state;
    const char *path = "snf_restart_crash.db";
    remove_db_files(path);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        execl("/proc/self/exe", "test_snf_restart", "--crash-writer", path, (char *)NULL);
        _exit(127);
    }
    int status = 0;
    assert_int_equal(waitpid(pid, &status, 0), pid);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);

    /* 子进程没有 close，数据还在 WAL 里；这证明走的是崩溃路径而不是 checkpoint。 */
    char wal_path[256];
    snprintf(wal_path, sizeof(wal_path), "%s-wal", path);
    assert_int_equal(access(wal_path, F_OK), 0);

    /* 崩溃时没有 close：重开必须从 WAL 恢复全部未确认行。 */
    assert_non_null(open_db(g_loop, path, false));
    assert_int_equal(unsynced(g_db), 8);
    assert_int_equal(synced(g_db), 0);

    snf_manager_t *mgr = start_mgr(g_loop, /*pending=*/0);
    assert_non_null(mgr);
    snf_manager_set_online(mgr, true);
    pump_until_drained(g_loop, g_db);

    assert_int_equal(g_published_count, 8);
    for (int i = 0; i < 8; i++) {
        assert_int_equal(g_published[i], (uint64_t)(12000 + i));
    }
    assert_int_equal(synced(g_db), 8);
    assert_int_equal(unsynced(g_db), 0);
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "--crash-writer") == 0) crash_writer(argv[2]);

    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_restart_replays_unsynced_in_order, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_restart_inflight_rows_replay, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_restart_confirmed_rows_not_replayed, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_process_crash_replays_unsynced_rows, fixture_setup,
                                        fixture_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
