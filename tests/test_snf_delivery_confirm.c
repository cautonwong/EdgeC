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
static uint64_t g_published_ts[256]; /* 成功交给传输的 mono ts，按调用顺序 */
static uint64_t g_published_wall[256]; /* 发布回调收到的记录墙钟 */
static uint32_t g_published_tags[256];
static int g_published_ts_count;

/* 顺序敏感模式：失败计数只在 pending_fn 被采样之后才可见。用来固定“传输先发布
 * failures、后递减 inflight，管理器先读 pending、后读 failures”的契约交错。 */
static int g_order_arm_at; /* 第 N 次 publish 后布防；0 = 关闭 */
static bool g_order_armed;
static bool g_failure_visible;

static uv_loop_t* g_loop;
static sqlite_svc_t* g_db;
static snf_manager_t* g_mgr;
static const char* g_db_path;

static uint32_t mock_take_failures(void* ud) {
    (void)ud;
    if (g_failure_visible) {
        g_failure_visible = false;
        return 1;
    }
    return __atomic_exchange_n(&g_failures, 0, __ATOMIC_SEQ_CST);
}

static int mock_publish(void* ud, const PipeMsg* msg, uint64_t wall_ts_us) {
    (void)ud;
    g_publish_calls++;
    if (g_publish_rc == 0 &&
        g_published_ts_count < (int)(sizeof(g_published_ts) / sizeof(g_published_ts[0]))) {
        g_published_tags[g_published_ts_count] = msg->tag_id;
        g_published_wall[g_published_ts_count] = wall_ts_us;
        g_published_ts[g_published_ts_count++] = msg->ts;
    }
    if (g_order_arm_at > 0 && g_publish_calls == g_order_arm_at) g_order_armed = true;
    if (g_pending_after_publish) g_pending = g_pending_after_publish;
    if (g_fail_on_publish) g_failures = g_fail_on_publish;
    return g_publish_rc;
}

static uint32_t mock_pending(void* ud) {
    (void)ud;
    if (g_order_armed && g_pending == 0) {
        g_failure_visible = true;
        g_order_armed = false;
    }
    return g_pending;
}

static int mock_publish_noop(void* ud, const PipeMsg* msg, uint64_t wall_ts_us) {
    (void)ud;
    (void)msg;
    (void)wall_ts_us;
    return 0;
}

/* ---- 辅助 ---------------------------------------------------------------- */

static void remove_db_files(const char* path) {
    if (!path) return;
    unlink(path);
    char sidecar[256];
    snprintf(sidecar, sizeof(sidecar), "%s-wal", path);
    unlink(sidecar);
    snprintf(sidecar, sizeof(sidecar), "%s-shm", path);
    unlink(sidecar);
}

static sqlite_svc_t* make_db(uv_loop_t* loop, const char* path) {
    remove_db_files(path);
    g_db_path = path;
    sqlite_config_t cfg = {.strategy = STRATEGY_LITE,
                           .db_path = path,
                           .batch_size = 0,
                           .flush_interval_ms = 0};
    g_db = sqlite_svc_create(loop, &cfg);
    return g_db;
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

static snf_manager_t* make_snf_ex(sqlite_svc_t* db, uv_loop_t* loop, uint32_t pending,
                                  int publish_rc, uint32_t batch_size) {
    g_publish_rc = publish_rc;
    g_pending = pending;
    g_publish_calls = 0;
    g_pending_after_publish = 0;
    g_failures = 0;
    g_fail_on_publish = 0;
    g_published_ts_count = 0;
    g_order_arm_at = 0;
    g_order_armed = false;
    g_failure_visible = false;

    g_store = snf_sqlite_store_port(db);
    snf_config_t cfg = {.store = &g_store,
                        .loop = loop,
                        .publish_fn = mock_publish,
                        .pending_fn = mock_pending,
                        .failures_fn = mock_take_failures,
                        .sync_batch_size = batch_size,
                        .sync_interval_ms = 20};
    g_mgr = snf_manager_create_in(&g_storage, &cfg);
    return g_mgr;
}

static snf_manager_t* make_snf(sqlite_svc_t* db, uv_loop_t* loop, uint32_t pending,
                               int publish_rc) {
    return make_snf_ex(db, loop, pending, publish_rc, 10);
}

static int fixture_setup(void** state) {
    (void)state;
    g_loop = uv_default_loop();
    g_db = NULL;
    g_mgr = NULL;
    g_db_path = NULL;
    return 0;
}

/* 断言失败时 cmocka 仍会执行这里：句柄一定销毁，DB 文件一定删除，避免串扰。 */
static int fixture_teardown(void** state) {
    (void)state;
    if (g_mgr) {
        snf_manager_destroy(g_mgr);
        pump(g_loop, 10); /* 跑完 uv_close 回调后才能复用 caller-allocated 存储 */
        g_mgr = NULL;
    }
    if (g_db) {
        sqlite_svc_destroy(g_db);
        g_db = NULL;
    }
    remove_db_files(g_db_path);
    g_db_path = NULL;
    return 0;
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
}

/**
 * 连续发布失败不丢行；链路恢复后按 ts 序排空，每条只成功发布一次。
 */
static void test_reconnect_drains_in_order_after_consecutive_failures(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, "snf_reconnect_drain.db");
    assert_non_null(db);

    snf_manager_t* mgr = make_snf(db, loop, /*pending=*/0, /*rc=*/-1);
    assert_non_null(mgr);

    buffer_n(mgr, 6, 1010);
    snf_manager_set_online(mgr, true);
    pump(loop, 60);

    assert_true(g_publish_calls >= 3);       /* 跨过三个以上失败轮次 */
    assert_int_equal(g_published_ts_count, 0);
    assert_int_equal(count_unsynced(db), 6); /* 失败不丢行 */
    assert_int_equal(count_synced(db), 0);

    g_publish_rc = 0; /* 链路恢复 */
    pump(loop, 60);

    assert_int_equal(count_synced(db), 6);
    assert_int_equal(count_unsynced(db), 0);
    assert_int_equal(g_published_ts_count, 6);
    for (int i = 0; i < 6; i++) {
        assert_int_equal(g_published_ts[i], (uint64_t)(1000 + i)); /* ts 升序，无丢行/重发 */
    }
}

/**
 * 在途批次遇到传输层异步失败时，不得确认。
 *
 * 批次发布成功、pending>0 时会留在内存等 ACK。若传输层随后报告发送失败并把
 * pending 清零，旧实现只看 pending 就 mark_synced，失败的行永久丢失。确认前
 * 必须先取 failures：>0 则丢弃 batch 让下一轮重发。
 */
static void test_async_failure_while_batch_pending_blocks_confirm(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, "snf_async_failure.db");
    assert_non_null(db);

    snf_manager_t* mgr = make_snf(db, loop, /*pending=*/2, /*rc=*/0);
    assert_non_null(mgr);

    buffer_n(mgr, 3, 1011);
    snf_manager_set_online(mgr, true);
    pump(loop, 40);

    assert_int_equal(g_publish_calls, 3); /* 第一批已交给传输 */
    assert_int_equal(g_published_ts_count, 3);
    assert_int_equal(count_synced(db), 0); /* 在途，不确认 */
    assert_int_equal(count_unsynced(db), 3);

    g_pending = 0; /* 传输层把这批丢了…… */
    g_failures = 1; /* ……并上报一次异步失败 */
    pump(loop, 40);

    /* 必须先重发再确认：3 条首发 + 3 条重发。 */
    assert_int_equal(g_publish_calls, 6);
    assert_int_equal(g_published_ts_count, 6);
    for (int i = 0; i < 3; i++) {
        assert_int_equal(g_published_ts[3 + i], (uint64_t)(1000 + i));
    }
    assert_int_equal(count_synced(db), 3);
    assert_int_equal(count_unsynced(db), 0);
}

/**
 * 首发路径：失败在 pending 采样之后才可见时，管理器必须先采样 pending、再采样
 * failures。旧顺序（failures → pending）会在两次读取之间漏掉这次失败并直接确认。
 */
static void test_fresh_publish_samples_pending_before_failures(void** state) {
    (void)state;
    sqlite_svc_t* db = make_db(g_loop, "snf_order_fresh.db");
    assert_non_null(db);
    snf_manager_t* mgr = make_snf(db, g_loop, /*pending=*/0, /*rc=*/0);
    assert_non_null(mgr);
    g_order_arm_at = 2; /* 两条都发布后，失败才在 pending 采样后可见 */

    buffer_n(mgr, 2, 1030);
    snf_manager_set_online(mgr, true);
    pump(g_loop, 60);

    /* 首发 2 条 + 看到失败后的重发 2 条，行只在重发后确认。 */
    assert_int_equal(g_publish_calls, 4);
    assert_int_equal(count_synced(db), 2);
    assert_int_equal(count_unsynced(db), 0);
}

/**
 * batch 路径：批次在途不确认；pending 归零且失败在 pending 采样后可见时，必须先
 * 丢弃 batch 重发，再确认。顺序敏感 mock 保证 failures 只在 pending 之后可见。
 */
static void test_batch_confirm_samples_pending_before_failures(void** state) {
    (void)state;
    sqlite_svc_t* db = make_db(g_loop, "snf_order_batch.db");
    assert_non_null(db);
    snf_manager_t* mgr = make_snf(db, g_loop, /*pending=*/1, /*rc=*/0);
    assert_non_null(mgr);
    g_order_arm_at = 3;

    buffer_n(mgr, 3, 1031);
    snf_manager_set_online(mgr, true);
    pump(g_loop, 40);
    assert_int_equal(g_publish_calls, 3); /* 第一批在途，不重发 */
    assert_int_equal(count_synced(db), 0);

    g_pending = 0; /* 在途归零，失败尚未发布 */
    pump(g_loop, 60);

    /* 不确认，先重发 3 条，再确认。 */
    assert_int_equal(g_publish_calls, 6);
    assert_int_equal(count_synced(db), 3);
    assert_int_equal(count_unsynced(db), 0);
}

/**
 * 对照：失败先于 pending 归零可见时，批次继续保留；归零后再丢弃并重发，
 * 而不是把失败行确认掉。
 */
static void test_failure_seen_before_pending_drain_still_replays(void** state) {
    (void)state;
    sqlite_svc_t* db = make_db(g_loop, "snf_failure_before_drain.db");
    assert_non_null(db);
    snf_manager_t* mgr = make_snf(db, g_loop, /*pending=*/2, /*rc=*/0);
    assert_non_null(mgr);

    buffer_n(mgr, 3, 1032);
    snf_manager_set_online(mgr, true);
    pump(g_loop, 40);
    assert_int_equal(g_publish_calls, 3);
    assert_int_equal(count_synced(db), 0);

    g_failures = 1; /* 失败先于 pending 归零可见 */
    pump(g_loop, 40);
    assert_int_equal(g_publish_calls, 3);
    assert_int_equal(count_synced(db), 0);
    assert_int_equal(count_unsynced(db), 3);

    g_pending = 0;
    pump(g_loop, 60);
    assert_int_equal(g_publish_calls, 6); /* 丢弃后重发 */
    assert_int_equal(count_synced(db), 3);
    assert_int_equal(count_unsynced(db), 0);
}

/**
 * pending_fn 非空而 failures_fn 缺失时必须 fail closed：只看 pending 会在异步
 * 失败后确认丢数，头文件的 at-least-once 承诺无法成立。
 */
static void test_config_rejects_pending_without_failures(void** state) {
    (void)state;
    snf_store_port_t store = {0};
    snf_config_t cfg = {.store = &store,
                        .loop = g_loop,
                        .publish_fn = mock_publish_noop,
                        .pending_fn = mock_pending,
                        .failures_fn = NULL};
    snf_manager_t* mgr = snf_manager_create_in(&g_storage, &cfg);
    g_mgr = mgr; /* 回归时若创建成功，fixture 仍会销毁它，避免句柄串扰 */
    assert_null(mgr);
}

/**
 * 两个回调都为空是显式的“不跟踪在途”信任模式：允许创建，但不承诺
 * at-least-once（见 snf_manager.h）。
 */
static void test_config_allows_explicit_untracked_mode(void** state) {
    (void)state;
    snf_store_port_t store = {0};
    snf_config_t cfg = {.store = &store,
                        .loop = g_loop,
                        .publish_fn = mock_publish_noop,
                        .pending_fn = NULL,
                        .failures_fn = NULL};
    g_mgr = snf_manager_create_in(&g_storage, &cfg);
    assert_non_null(g_mgr);
}

/**
 * 行身份是 (wall_ts_us, tag_id)，不是 wall_ts_us。标确认必须同时匹配两者；
 * 同一个墙钟微秒的两个 tag 跨 SNF_MAX_BATCH 边界时，两条都必须真正发布。
 */
static void test_row_identity_same_wall_two_tags_survive_batch_boundary(void** state) {
    (void)state;
    sqlite_svc_t* db = make_db(g_loop, "snf_row_identity.db");
    assert_non_null(db);

    const uint32_t tag_a = 2001;
    const uint32_t tag_b = 2002;
    const uint64_t wall0 = 1600000000000000ULL;
    snf_store_port_t store = snf_sqlite_store_port(db);
    for (int i = 0; i < 63; i++) {
        PipeMsg msg = {.ts = (uint64_t)(1000 + i), .tag_id = tag_a, .payload.val.f = (double)i};
        msg.type = PIPEMSG_TYPE_SCALAR;
        assert_int_equal(store.insert(store.ctx, &msg, wall0 + (uint64_t)i), 0);
    }
    /* 同一个墙钟微秒、两个 tag：主键允许并存，标记不能按 wall 单列误伤。 */
    const uint64_t same_wall = wall0 + 63;
    PipeMsg last_a = {.ts = 2000, .tag_id = tag_a, .payload.val.f = 62.0};
    last_a.type = PIPEMSG_TYPE_SCALAR;
    assert_int_equal(store.insert(store.ctx, &last_a, same_wall), 0);
    PipeMsg same_wall_b = {.ts = 2001, .tag_id = tag_b, .payload.val.f = 99.0};
    same_wall_b.type = PIPEMSG_TYPE_SCALAR;
    assert_int_equal(store.insert(store.ctx, &same_wall_b, same_wall), 0);

    snf_manager_t* mgr = make_snf_ex(db, g_loop, /*pending=*/0, /*rc=*/0, SNF_MAX_BATCH);
    assert_non_null(mgr);
    snf_manager_set_online(mgr, true);
    pump(g_loop, 120);

    /* 64 条首批 + 1 条次批，两条同墙钟的行都在发布记录里。 */
    assert_int_equal(g_published_ts_count, 65);
    assert_int_equal(count_synced(db), 65);
    assert_int_equal(count_unsynced(db), 0);

    bool saw_a = false;
    bool saw_b = false;
    uint64_t prev = 0;
    for (int i = 0; i < g_published_ts_count; i++) {
        assert_true(g_published_wall[i] >= prev);
        prev = g_published_wall[i];
        if (g_published_wall[i] == same_wall && g_published_tags[i] == tag_a) saw_a = true;
        if (g_published_wall[i] == same_wall && g_published_tags[i] == tag_b) saw_b = true;
    }
    assert_true(saw_a);
    assert_true(saw_b);
}

/** SNF_MAX_BATCH 两侧各一条：63 / 64 / 65 条必须全部按序排空。 */
static void check_batch_boundary(int n, const char* path, uint32_t tag) {
    uv_loop_t* loop = uv_default_loop();
    sqlite_svc_t* db = make_db(loop, path);
    assert_non_null(db);

    snf_manager_t* mgr = make_snf_ex(db, loop, /*pending=*/0, /*rc=*/0, SNF_MAX_BATCH);
    assert_non_null(mgr);

    buffer_n(mgr, n, tag);
    snf_manager_set_online(mgr, true);
    pump(loop, 120);

    assert_int_equal(count_synced(db), n);
    assert_int_equal(count_unsynced(db), 0);
    assert_int_equal(g_published_ts_count, n); /* 无重发、无丢行 */
    for (int i = 0; i < n; i++) {
        assert_int_equal(g_published_ts[i], (uint64_t)(1000 + i));
    }
}

static void test_batch_boundary_below(void** state) {
    (void)state;
    check_batch_boundary(63, "snf_batch_63.db", 1020);
}

static void test_batch_boundary_at(void** state) {
    (void)state;
    check_batch_boundary(64, "snf_batch_64.db", 1021);
}

static void test_batch_boundary_above(void** state) {
    (void)state;
    check_batch_boundary(65, "snf_batch_65.db", 1022);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_inflight_blocks_confirm, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_publish_failure_blocks_confirm, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_pending_after_publish_blocks_confirm, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_confirms_after_inflight_clears, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_publish_failure_keeps_unsynced, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_inflight_does_not_republish, fixture_setup,
                                        fixture_teardown),
        cmocka_unit_test_setup_teardown(test_reconnect_drains_in_order_after_consecutive_failures,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_async_failure_while_batch_pending_blocks_confirm,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_fresh_publish_samples_pending_before_failures,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_batch_confirm_samples_pending_before_failures,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_failure_seen_before_pending_drain_still_replays,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_config_rejects_pending_without_failures,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_config_allows_explicit_untracked_mode,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_row_identity_same_wall_two_tags_survive_batch_boundary,
                                        fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_batch_boundary_below, fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_batch_boundary_at, fixture_setup, fixture_teardown),
        cmocka_unit_test_setup_teardown(test_batch_boundary_above, fixture_setup, fixture_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
