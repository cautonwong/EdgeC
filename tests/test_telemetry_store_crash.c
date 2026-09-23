#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "edge_telemetry_store.h"

// --- Query Collector Context ---

typedef struct {
    int count;
    pipe_msg_t collected[256];
} query_ctx_t;

static int query_collector_cb(void *user_ctx, const pipe_msg_t *msg) {
    query_ctx_t *ctx = (query_ctx_t *)user_ctx;
    if (ctx && ctx->count < 256) {
        ctx->collected[ctx->count++] = *msg;
    }
    return 0;
}

// --- Forward Callback Context ---

typedef struct {
    int forward_calls;
    size_t total_forwarded_msgs;
    pipe_msg_t last_msg;
} forward_ctx_t;

static int mock_telemetry_forward(const pipe_msg_t *msgs, size_t count, void *user_ctx) {
    forward_ctx_t *fctx = (forward_ctx_t *)user_ctx;
    if (fctx) {
        fctx->forward_calls++;
        fctx->total_forwarded_msgs += count;
        if (count > 0) {
            fctx->last_msg = msgs[count - 1];
        }
    }
    return 0;  // 成功
}

// --- Test Cases ---

static void test_telemetry_store_append_and_query(void **state) {
    (void)state;
    const char *wal_path = "/tmp/test_telem_append.wal";
    unlink(wal_path);

    edge_telemetry_store_cfg_t cfg = {
        .wal_path = wal_path,
        .flush_interval_ms = 1000,
        .batch_size = 50,
        .enable_gorilla_compression = false,
    };

    edge_telemetry_store_t *store = edge_telemetry_store_create(NULL, &cfg);
    assert_non_null(store);

    // 写入 100 条遥测数据
    for (int i = 0; i < 100; i++) {
        pipe_msg_t msg = {
            .tag_id = (uint32_t)(0x2001 + (i % 3)),
            .ts = (uint64_t)(1000 + i * 10),
            .type = PIPEMSG_TYPE_SCALAR,
            .payload.val.f = (float)(220.0f + (float)i * 0.1f),
        };
        int rc = edge_telemetry_store_append(store, &msg);
        assert_int_equal(rc, 0);
    }

    edge_telemetry_store_stats_t stats = {0};
    int rc = edge_telemetry_store_get_stats(store, &stats);
    assert_int_equal(rc, 0);
    assert_int_equal(stats.total_appended, 100);

    // 查询 Tag=0x2001, ts in [1000, 1060] (应有 i = 0, 3, 6 -> 1000, 1030, 1060: 3 条)
    query_ctx_t qctx = {0};
    int match_count =
        edge_telemetry_store_query(store, 0x2001, 1000, 1060, query_collector_cb, &qctx);
    assert_int_equal(match_count, 3);
    assert_int_equal(qctx.count, 3);
    assert_int_equal(qctx.collected[0].ts, 1000);
    assert_int_equal(qctx.collected[1].ts, 1030);
    assert_int_equal(qctx.collected[2].ts, 1060);

    edge_telemetry_store_destroy(store);
    unlink(wal_path);
}

static void test_telemetry_store_wal_crash_recovery(void **state) {
    (void)state;
    const char *wal_path = "/tmp/test_wal_crash_recovery.wal";
    unlink(wal_path);

    edge_telemetry_store_cfg_t cfg = {
        .wal_path = wal_path,
        .flush_interval_ms = 1000,
        .batch_size = 50,
        .enable_gorilla_compression = false,
    };

    // 1. 模拟第一代进程写入 50 条数据后意外终止
    edge_telemetry_store_t *store1 = edge_telemetry_store_create(NULL, &cfg);
    assert_non_null(store1);

    for (int i = 0; i < 50; i++) {
        pipe_msg_t msg = {
            .tag_id = 0x3001,
            .ts = (uint64_t)(5000 + i * 100),
            .type = PIPEMSG_TYPE_SCALAR,
            .payload.val.f = (float)(50.0f + (float)i * 0.05f),
        };
        edge_telemetry_store_append(store1, &msg);
    }
    edge_telemetry_store_flush_sync(store1);

    // 模拟进程崩溃关停 (只释放内存句柄，保留磁盘 WAL)
    edge_telemetry_store_destroy(store1);

    // 2. 第二代进程重启，从 WAL 中自动恢复崩溃一致性数据
    edge_telemetry_store_t *store2 = edge_telemetry_store_create(NULL, &cfg);
    assert_non_null(store2);

    edge_telemetry_store_stats_t stats2 = {0};
    edge_telemetry_store_get_stats(store2, &stats2);
    assert_int_equal(stats2.total_wal_flushed, 50);  // 100% 成功恢复 50 条记录

    // 验证恢复数据的准确性
    query_ctx_t qctx = {0};
    int match_count =
        edge_telemetry_store_query(store2, 0x3001, 5000, 10000, query_collector_cb, &qctx);
    assert_int_equal(match_count, 50);
    assert_int_equal(qctx.count, 50);
    assert_int_equal(qctx.collected[0].ts, 5000);
    assert_int_equal(qctx.collected[49].ts, 5000 + 49 * 100);

    edge_telemetry_store_destroy(store2);
    unlink(wal_path);
}

static void test_telemetry_store_gorilla_compression(void **state) {
    (void)state;
    const char *wal_path = "/tmp/test_gorilla_comp.wal";
    unlink(wal_path);

    edge_telemetry_store_cfg_t cfg = {
        .wal_path = wal_path,
        .flush_interval_ms = 1000,
        .batch_size = 100,
        .enable_gorilla_compression = true,
    };

    edge_telemetry_store_t *store = edge_telemetry_store_create(NULL, &cfg);
    assert_non_null(store);

    // 写入 256 条高频时序数据 (模拟 50Hz 采样)
    for (int i = 0; i < 256; i++) {
        pipe_msg_t msg = {
            .tag_id = 0x1001,
            .ts = (uint64_t)(1000000 + i * 20),
            .type = PIPEMSG_TYPE_SCALAR,
            .payload.val.f = 220.0f + 0.5f * (float)(i % 4),
        };
        edge_telemetry_store_append(store, &msg);
    }
    edge_telemetry_store_flush_sync(store);

    edge_telemetry_store_stats_t stats = {0};
    edge_telemetry_store_get_stats(store, &stats);
    assert_true(stats.compression_bytes_in > 0);
    assert_true(stats.compression_bytes_out > 0);

    printf("\n  [BENCHMARK] Gorilla TS Compression: In=%lu bytes, Out=%lu bytes\n",
           (unsigned long)stats.compression_bytes_in, (unsigned long)stats.compression_bytes_out);

    edge_telemetry_store_destroy(store);
    unlink(wal_path);
}

static void test_telemetry_store_snf_offline_buffering_and_drain(void **state) {
    (void)state;
    const char *wal_path = "/tmp/test_snf_drain.wal";
    unlink(wal_path);

    forward_ctx_t fctx = {0};
    edge_telemetry_store_cfg_t cfg = {
        .wal_path = wal_path,
        .flush_interval_ms = 1000,
        .forward_cb = mock_telemetry_forward,
        .forward_ctx = &fctx,
        .snf_max_offline_records = 500,
    };

    edge_telemetry_store_t *store = edge_telemetry_store_create(NULL, &cfg);
    assert_non_null(store);

    // 1. 设置为离线状态
    edge_telemetry_store_set_online(store, false);

    // 写入 100 条离线数据
    for (int i = 0; i < 100; i++) {
        pipe_msg_t msg = {
            .tag_id = 0x4001,
            .ts = (uint64_t)(20000 + i * 10),
            .type = PIPEMSG_TYPE_SCALAR,
            .payload.val.f = 10.0f + (float)i,
        };
        edge_telemetry_store_append(store, &msg);
    }

    edge_telemetry_store_stats_t stats = {0};
    edge_telemetry_store_get_stats(store, &stats);
    assert_int_equal(stats.current_snf_pending, 100);
    assert_int_equal(stats.total_snf_forwarded, 0);

    // 2. 切换回在线状态，触发 SnF 自动回放回灌
    edge_telemetry_store_set_online(store, true);

    edge_telemetry_store_get_stats(store, &stats);
    assert_int_equal(stats.current_snf_pending, 0);
    assert_int_equal(stats.total_snf_forwarded, 100);
    assert_int_equal(fctx.total_forwarded_msgs, 100);

    // 3. 在线状态下新消息直接透传
    pipe_msg_t live_msg = {
        .tag_id = 0x4001,
        .ts = 30000,
        .type = PIPEMSG_TYPE_SCALAR,
        .payload.val.f = 999.0f,
    };
    edge_telemetry_store_append(store, &live_msg);

    edge_telemetry_store_get_stats(store, &stats);
    assert_int_equal(stats.total_snf_forwarded, 101);
    assert_int_equal(fctx.last_msg.ts, 30000);

    edge_telemetry_store_destroy(store);
    unlink(wal_path);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_telemetry_store_append_and_query),
        cmocka_unit_test(test_telemetry_store_wal_crash_recovery),
        cmocka_unit_test(test_telemetry_store_gorilla_compression),
        cmocka_unit_test(test_telemetry_store_snf_offline_buffering_and_drain),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
