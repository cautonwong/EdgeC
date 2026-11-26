/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_northbound_wall_ts.c
 * @brief P4-03b1 golden path: record wall time reaches the Sparkplug frame.
 *
 * SnF stores wall_ts_us (epoch µs) at the persistence boundary. Replay must
 * encode that same value as the Sparkplug root and metric timestamp (epoch ms);
 * the realtime path must sample the wall clock at the send boundary instead of
 * using uv_hrtime (monotonic uptime). This test wires the real snf_manager,
 * SQLite store port, northbound manager, driver adapter and Sparkplug encoder,
 * and decodes the captured frame.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "north_driver_adapter.h"
#include "northbound_mgr.h"
#include "snf_sqlite_adapter.h"
#include "sqlite_service.h"
#include "thread_port_uv.h"

/* ---- 捕获传输：解码 NBIRTH/NDATA 的根/度量时间戳 ------------------------ */

static _Atomic int g_ndata_count;
static _Atomic uint64_t g_root_ms;
static _Atomic uint64_t g_metric_ms;
static _Atomic int g_nbirth_count;
static _Atomic uint64_t g_nbirth_root_ms;
static _Atomic uint64_t g_test_now_us;

static int _varint(const uint8_t *p, size_t len, size_t *pos, uint64_t *out) {
    uint64_t v = 0;
    unsigned shift = 0;
    while (*pos < len && shift < 64) {
        uint8_t b = p[(*pos)++];
        v |= (uint64_t)(b & 0x7fU) << shift;
        if ((b & 0x80U) == 0) {
            *out = v;
            return 0;
        }
        shift += 7;
    }
    return -1;
}

static int _skip_field(const uint8_t *p, size_t len, size_t *pos, uint8_t wire) {
    uint64_t v = 0;
    switch (wire) {
    case 0:
        return _varint(p, len, pos, &v);
    case 1:
        *pos += 8;
        break;
    case 2:
        if (_varint(p, len, pos, &v) != 0) return -1;
        *pos += (size_t)v;
        break;
    case 5:
        *pos += 4;
        break;
    default:
        return -1;
    }
    return *pos <= len ? 0 : -1;
}

static int _decode_metric_ts(const uint8_t *p, size_t len, uint64_t *ts_ms) {
    size_t pos = 0;
    *ts_ms = 0;
    while (pos < len) {
        uint64_t tag = 0;
        if (_varint(p, len, &pos, &tag) != 0) return -1;
        uint32_t field = (uint32_t)(tag >> 3);
        uint8_t wire = (uint8_t)(tag & 7U);
        if (field == 3 && wire == 0) {
            if (_varint(p, len, &pos, ts_ms) != 0) return -1;
        } else if (_skip_field(p, len, &pos, wire) != 0) {
            return -1;
        }
    }
    return 0;
}

static int _decode_payload(const uint8_t *p, size_t len, uint64_t *root_ms,
                           uint64_t *metric_ms) {
    size_t pos = 0;
    *root_ms = 0;
    *metric_ms = 0;
    while (pos < len) {
        uint64_t tag = 0;
        if (_varint(p, len, &pos, &tag) != 0) return -1;
        uint32_t field = (uint32_t)(tag >> 3);
        uint8_t wire = (uint8_t)(tag & 7U);
        if (field == 1 && wire == 0) {
            if (_varint(p, len, &pos, root_ms) != 0) return -1;
        } else if (field == 2 && wire == 2) {
            uint64_t mlen = 0;
            if (_varint(p, len, &pos, &mlen) != 0 || pos + (size_t)mlen > len) return -1;
            uint64_t mts = 0;
            if (_decode_metric_ts(p + pos, (size_t)mlen, &mts) != 0) return -1;
            if (*metric_ms == 0) *metric_ms = mts;
            pos += (size_t)mlen;
        } else if (_skip_field(p, len, &pos, wire) != 0) {
            return -1;
        }
    }
    return 0;
}

static int capture_publish(void *ctx, const char *topic, const uint8_t *payload, size_t len) {
    (void)ctx;
    if (!topic || !payload) return 0;
    uint64_t root = 0;
    uint64_t metric = 0;
    if (strstr(topic, "/NBIRTH/")) {
        if (_decode_payload(payload, len, &root, &metric) == 0) {
            atomic_store(&g_nbirth_root_ms, root);
            atomic_fetch_add(&g_nbirth_count, 1);
        }
    } else if (strstr(topic, "/NDATA/")) {
        if (_decode_payload(payload, len, &root, &metric) == 0) {
            atomic_store(&g_root_ms, root);
            atomic_store(&g_metric_ms, metric);
            atomic_fetch_add(&g_ndata_count, 1);
        }
    }
    return 0;
}

static bool capture_connected(void *ctx) {
    (void)ctx;
    return true;
}

/* 实时路径的 source：armed 后吐一条 mono ts=123 的记录。 */
static _Atomic bool g_emit_realtime;

static uint64_t test_now_us(void *ctx) {
    (void)ctx;
    return atomic_load(&g_test_now_us);
}

static int fake_read_next(void *ctx, uint64_t *seq, pipe_msg_t *out) {
    (void)ctx;
    if (!atomic_load(&g_emit_realtime)) return 1;
    atomic_store(&g_emit_realtime, false);
    memset(out, 0, sizeof(*out));
    out->ts = 123;
    out->tag_id = 0x2244;
    out->payload.val.f = 12.5;
    out->type = PIPEMSG_TYPE_SCALAR;
    if (seq) (*seq)++;
    return 0;
}

/* ---- fixture ------------------------------------------------------------- */

static uv_loop_t *g_loop;
static sqlite_svc_t *g_db;
static northbound_mgr_t g_mgr;
static bool g_mgr_inited;
static bool g_started;
static north_driver_adapter_storage_t g_driver_storage;
static const char *g_db_path = "test_northbound_wall_ts.db";

static void remove_db_files(const char *path) {
    if (!path) return;
    unlink(path);
    char sidecar[256];
    snprintf(sidecar, sizeof(sidecar), "%s-wal", path);
    unlink(sidecar);
    snprintf(sidecar, sizeof(sidecar), "%s-shm", path);
    unlink(sidecar);
}

static uint64_t _now_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

static int fixture_setup(void **state) {
    (void)state;
    g_loop = uv_default_loop();
    g_db = NULL;
    g_mgr_inited = false;
    g_started = false;
    atomic_store(&g_ndata_count, 0);
    atomic_store(&g_root_ms, 0);
    atomic_store(&g_metric_ms, 0);
    atomic_store(&g_nbirth_count, 0);
    atomic_store(&g_nbirth_root_ms, 0);
    atomic_store(&g_test_now_us, 0);
    remove_db_files(g_db_path);
    return 0;
}

static int fixture_teardown(void **state) {
    (void)state;
    if (g_started) {
        northbound_mgr_stop(&g_mgr);
        g_started = false;
    }
    if (g_mgr_inited) {
        northbound_mgr_destroy(&g_mgr);
        g_mgr_inited = false;
    }
    if (g_db) {
        sqlite_svc_destroy(g_db);
        g_db = NULL;
    }
    remove_db_files(g_db_path);
    return 0;
}

/* ---- 用例 ---------------------------------------------------------------- */

static void test_wall_ts_flows_from_snf_and_send_boundary(void **state) {
    (void)state;
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = g_db_path};
    g_db = sqlite_svc_create(g_loop, &db_cfg);
    assert_non_null(g_db);
    snf_store_port_t store = snf_sqlite_store_port(g_db);

    /* 历史记录：墙钟 1600000000000000 µs = 1600000000000 ms。 */
    PipeMsg replay = {.ts = 1000, .tag_id = 0x2233, .payload.val.f = 220.5};
    replay.type = PIPEMSG_TYPE_SCALAR;
    assert_int_equal(store.insert(store.ctx, &replay, 1600000000000000ULL), 0);

    nb_source_ops_t source = {.read_next = fake_read_next};
    nb_transport_ops_t transport = {
        .ctx = NULL, .publish = capture_publish, .is_connected = capture_connected};
    assert_int_equal(northbound_mgr_init_ex(&g_mgr, source, "wall_ts", &store, transport), 0);
    g_mgr_inited = true;
    north_driver_port_t port = north_driver_adapter_init(&g_driver_storage);
    northbound_mgr_set_driver_port(&g_mgr, port);
    northbound_mgr_set_wall_clock(&g_mgr, test_now_us, NULL);
    atomic_store(&g_emit_realtime, false);

    /* ack 精确断言：注入 1700000000000123 µs，编码必须是 1700000000000 ms。 */
    assert_int_equal(
        g_mgr.driver_port.attach(g_mgr.driver_port.ctx, "SparkplugB", &g_mgr.nb_driver), 0);
    atomic_store(&g_test_now_us, 1700000000000123ULL);
    atomic_store(&g_ndata_count, 0);
    assert_int_equal(northbound_mgr_publish_ack(&g_mgr, 0x2234, "rt_ack", true), 0);
    assert_int_equal(atomic_load(&g_ndata_count), 1);
    assert_int_equal(atomic_load(&g_root_ms), 1700000000000ULL);
    assert_int_equal(atomic_load(&g_metric_ms), 1700000000000ULL);

    /* ts_us=0 回落：驱动必须用当前 epoch 毫秒，不能编码 0。 */
    atomic_store(&g_test_now_us, 0);
    atomic_store(&g_ndata_count, 0);
    uint64_t zero_before_us = _now_us();
    assert_int_equal(northbound_mgr_publish_ack(&g_mgr, 0x2235, "rt_zero", true), 0);
    uint64_t zero_after_us = _now_us();
    assert_int_equal(atomic_load(&g_ndata_count), 1);
    uint64_t zero_root_ms = atomic_load(&g_root_ms);
    uint64_t zero_metric_ms = atomic_load(&g_metric_ms);
    assert_int_equal(zero_root_ms, zero_metric_ms);
    assert_true(zero_root_ms > 1000000000000ULL);
    assert_true(zero_root_ms + 1000ULL >= zero_before_us / 1000ULL);
    assert_true(zero_root_ms <= zero_after_us / 1000ULL + 1000ULL);

    g_mgr.driver_port.detach(g_mgr.driver_port.ctx, g_mgr.nb_driver);
    g_mgr.nb_driver = NULL;

    /* SnF 回放 + NBIRTH：线程起 poll(500ms) 置在线，SnF 周期(5s)补发。 */
    atomic_store(&g_ndata_count, 0);
    atomic_store(&g_nbirth_count, 0);
    assert_int_equal(northbound_mgr_start(&g_mgr), 0);
    g_started = true;
    for (int i = 0;
         i < 240 && (atomic_load(&g_ndata_count) == 0 || atomic_load(&g_nbirth_count) == 0); i++) {
        usleep(50000);
    }

    assert_true(atomic_load(&g_nbirth_count) >= 1);
    uint64_t nbirth_ms = atomic_load(&g_nbirth_root_ms);
    uint64_t now_ms = _now_us() / 1000ULL;
    assert_true(nbirth_ms > 1000000000000ULL); /* NBIRTH 用当前 epoch 毫秒 */
    assert_true(nbirth_ms + 10000ULL >= now_ms);
    assert_true(nbirth_ms <= now_ms + 10000ULL);

    assert_true(atomic_load(&g_ndata_count) >= 1);
    assert_int_equal(atomic_load(&g_root_ms), 1600000000000ULL);
    assert_int_equal(atomic_load(&g_metric_ms), 1600000000000ULL);

    /* 实时 source drain：注入固定 1700000000000123 µs，必须精确编码。 */
    atomic_store(&g_ndata_count, 0);
    atomic_store(&g_test_now_us, 1700000000000123ULL);
    atomic_store(&g_emit_realtime, true);
    for (int i = 0; i < 100 && atomic_load(&g_ndata_count) == 0; i++) usleep(50000);

    assert_int_equal(atomic_load(&g_ndata_count), 1);
    assert_int_equal(atomic_load(&g_root_ms), 1700000000000ULL);
    assert_int_equal(atomic_load(&g_metric_ms), 1700000000000ULL);

    northbound_mgr_stop(&g_mgr);
    g_started = false;
}

int main(void) {
    edge_thread_port_install_uv();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_wall_ts_flows_from_snf_and_send_boundary,
                                        fixture_setup, fixture_teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
