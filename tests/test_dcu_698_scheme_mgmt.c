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

#include "dcu_698_appconn.h"
#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_record.h"
#include "dcu_698_rules.h"
#include "dcu_driver_adapter.h"
#include "dcu_inventory.h"
#include "dcu_north_adapter.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_scheme_store.h"
#include "dcu_storage.h"
#include "dlt698_codec.h"
#include "modbus_codec.h"
#include "edge_slave_plane.h"

#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))
#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))

typedef struct {
    uint8_t b[2048];
    size_t n;
} body_t;

typedef struct {
    uv_loop_t loop;
    dcu_storage_t storage;
    dcu_rule_cache_t rule_cache;
    dcu_poller_t poller;
    dcu_inventory_t inv;
    char path[64];
} bench_t;

static void _u8(body_t *b, uint8_t v) {
    b->b[b->n++] = v;
}

static void _u16(body_t *b, uint8_t tag, uint16_t v) {
    _u8(b, tag);
    _u8(b, (uint8_t)(v >> 8));
    _u8(b, (uint8_t)v);
}

static void _oad(body_t *b, uint32_t oad) {
    _u8(b, (uint8_t)(oad >> 24));
    _u8(b, (uint8_t)(oad >> 16));
    _u8(b, (uint8_t)(oad >> 8));
    _u8(b, (uint8_t)oad);
}

static void _tsa(body_t *b, const uint8_t *bytes, uint8_t len) {
    _u8(b, len);
    for (uint8_t i = 0; i < len; i++) _u8(b, bytes[i]);
}

static void _octets(body_t *b, const uint8_t *data, uint8_t len) {
    _u8(b, DLT698_TAG_OCTET_STRING);
    _u8(b, len);
    for (uint8_t i = 0; i < len; i++) _u8(b, data[i]);
}

static int _bench_init(bench_t *b) {
    memset(b, 0, sizeof(*b));
    strcpy(b->path, "/tmp/dcu_scheme_mgmt_XXXXXX");
    int fd = mkstemp(b->path);
    if (fd < 0) return -1;
    close(fd);
    if (uv_loop_init(&b->loop) != 0) return -1;
    if (dcu_storage_init(&b->storage, &b->loop, b->path) != 0) return -1;
    if (dcu_inventory_init(&b->inv) != 0) return -1;
    if (dcu_poller_init(&b->poller) != 0) return -1;
    if (dcu_scheme_store_init(&b->storage, NULL, 0) != 0) return -1;
    if (dcu_698_rules_init(&b->storage, &b->rule_cache) != 0) return -1;
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    if (dcu_698_object_model_init(&b->inv, &b->poller, sink, NULL, NULL, NULL, NULL, bus) != 0) {
        return -1;
    }
    return 0;
}

static void _bench_free(bench_t *b) {
    dcu_698_rules_detach();
    dcu_698_object_model_reset();
    dcu_poller_destroy(&b->poller);
    dcu_inventory_destroy(&b->inv);
    dcu_storage_destroy(&b->storage);
    /* storage 的冲刷定时器是异步关闭的：先跑一圈循环，uv_loop_close 才不因句柄未解绑而失败。 */
    uv_run(&b->loop, UV_RUN_NOWAIT);
    uv_loop_close(&b->loop);
    unlink(b->path);
    char side[80];
    snprintf(side, sizeof(side), "%s-wal", b->path);
    unlink(side);
    snprintf(side, sizeof(side), "%s-shm", b->path);
    unlink(side);
}

static int _bench_reopen_storage(bench_t *b) {
    dcu_storage_destroy(&b->storage);
    /* 旧 storage 的定时器是异步关闭的；先把它收完，再在同一块 timer 内存上 init。 */
    uv_run(&b->loop, UV_RUN_NOWAIT);
    if (dcu_storage_init(&b->storage, &b->loop, b->path) != 0) return -1;
    if (dcu_scheme_store_init(&b->storage, NULL, 0) != 0) return -1;
    return dcu_698_rules_init(&b->storage, &b->rule_cache);
}

static int _action(uint16_t oi, uint8_t method, const body_t *body) {
    return dcu_north_accept_action(NULL, MK_OMD(oi, method), body->b, body->n);
}

static int _get(uint32_t oad, body_t *out) {
    size_t len = 0;
    memset(out, 0, sizeof(*out));
    int rc = dcu_north_resolve_axdr(NULL, oad, out->b, sizeof(out->b), &len);
    out->n = len;
    return rc;
}


#define TAG_CSD 0x5B

static void _di_array(body_t *b, uint8_t width, uint8_t count, uint32_t di) {
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, count);
    for (uint8_t i = 0; i < count; i++) {
        _u8(b, DLT698_TAG_OCTET_STRING);
        _u8(b, width);
        for (uint8_t k = 0; k < width; k++) _u8(b, (uint8_t)(di >> (8 * k)));
    }
}

static void _rule_unit(body_t *b, uint32_t oad, uint8_t has_m4, uint32_t m4, uint8_t has_s4,
                       uint32_t s4, uint8_t has_m2, uint32_t m2, uint8_t has_s2, uint32_t s2,
                       const uint8_t *frame, uint8_t frame_len) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _u8(b, TAG_CSD);
    _u8(b, 0);
    _oad(b, oad);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 3);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _di_array(b, 4, has_m4, m4);
    _di_array(b, 4, has_s4, s4);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _di_array(b, 2, has_m2, m2);
    _di_array(b, 2, has_s2, s2);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 1);
    _u8(b, DLT698_TAG_OCTET_STRING);
    _u8(b, frame_len);
    if (frame) {
        for (uint8_t i = 0; i < frame_len; i++) _u8(b, frame[i]);
    }
}

static void _add_body(body_t *b, uint8_t count) {
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, count);
}

static void _delete_body(body_t *b, uint8_t count) {
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, count);
}

static void _delete_csd(body_t *b, uint32_t oad) {
    _u8(b, TAG_CSD);
    _u8(b, 0);
    _oad(b, oad);
}

static void _event_content(body_t *b, uint8_t type, const uint32_t *roads, const uint32_t *related,
                           uint8_t road_count) {
    if (type == 1) {
        _u8(b, DLT698_TAG_NULL);
        return;
    }
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, road_count);
    for (uint8_t i = 0; i < road_count; i++) {
        _u8(b, 0x52); /* ROAD：OAD + 关联 OAD，成员不带内层标签（与 CSD ROAD 同布局） */
        _oad(b, roads[i]);
        _u8(b, DLT698_TAG_UNSIGNED);
        _u8(b, 1);
        _oad(b, related[i]);
    }
}

static void _event_unit(body_t *b, uint8_t id, uint8_t type, const uint32_t *roads,
                        const uint32_t *related, uint8_t road_count, uint8_t ms_choice,
                        bool report, uint16_t depth) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 5);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, id);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, type);
    _event_content(b, type, roads, related, road_count);
    _u8(b, DCU_698_TAG_MS);
    _u8(b, ms_choice);
    if (ms_choice == 3) {
        _u8(b, 1);
        _u8(b, 2);
        _u8(b, 0x11);
        _u8(b, 0x22);
    }
    _u8(b, DLT698_TAG_BOOLEAN);
    _u8(b, report ? 1 : 0);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, depth);
}

static void _tp_ctrl(body_t *b, bool wait, uint16_t timeout, uint8_t compare, uint8_t feature,
                     uint16_t cut_start, uint16_t cut_len) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 4);
    _u8(b, DLT698_TAG_BOOLEAN);
    _u8(b, wait ? 1 : 0);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, timeout);
    _u8(b, DLT698_TAG_ENUM);
    _u8(b, compare);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 3);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, feature);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, cut_start);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, cut_len);
}

static void _tp_frames(body_t *b, uint8_t count) {
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, count);
    for (uint8_t i = 0; i < count; i++) {
        _u8(b, DLT698_TAG_STRUCTURE);
        _u8(b, 2);
        _u8(b, DLT698_TAG_UNSIGNED);
        _u8(b, (uint8_t)(i + 1));
        const uint8_t payload[2] = {(uint8_t)('A' + i), 'B'};
        _octets(b, payload, sizeof(payload));
    }
}

static void _tp_content(body_t *b, uint16_t seq, const uint8_t *addr, uint8_t addr_len,
                        uint16_t pre_script, uint16_t post_script, bool wait, uint8_t compare,
                        uint8_t frame_count) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 6);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, seq);
    _tsa(b, addr, addr_len);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, pre_script);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, post_script);
    _tp_ctrl(b, wait, 0, compare, 0, 0, 0);
    _tp_frames(b, frame_count);
}

static void _tp_scheme_one(body_t *b, uint8_t id, bool wait, uint8_t compare) {
    static const uint8_t addr[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 3);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, id);
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, 1);
    _tp_content(b, 1, addr, sizeof(addr), 0, 0, wait, compare, 1);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, 0);
}

static void test_event_scheme_round_trip_and_persistence(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    static const uint32_t roads0[] = {0x50040200u};
    static const uint32_t related0[] = {0x00100200u};
    body_t add = {0};
    _u8(&add, DLT698_TAG_ARRAY);
    _u8(&add, 2);
    _event_unit(&add, 7, 0, roads0, related0, 1, 1, true, 32);
    _event_unit(&add, 9, 1, NULL, NULL, 0, 1, false, 0);
    assert_int_equal(_action(OI_EVENT_SCHEME, 127, &add), 0);

    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_EVENT_SCHEME_UNIT, 2) | 7, &got), 0);
    body_t want = {0};
    _event_unit(&want, 7, 0, roads0, related0, 1, 1, true, 32);
    assert_int_equal(got.n, want.n);
    assert_memory_equal(got.b, want.b, want.n);

    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_EVENT_SCHEME, 2), &got), 0);
    assert_int_equal(got.b[0], DLT698_TAG_ARRAY);
    assert_int_equal(got.b[1], 2);

    assert_int_equal(_bench_reopen_storage(&b), 0);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_EVENT_SCHEME_UNIT, 2) | 7, &got), 0);
    assert_int_equal(got.n, want.n);
    assert_memory_equal(got.b, want.b, want.n);
    _bench_free(&b);
}

static void test_event_wire_shape_is_exact(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    static const uint32_t roads[] = {0x50040200u};
    static const uint32_t related[] = {0x00100200u};
    body_t add = {0};
    _u8(&add, DLT698_TAG_ARRAY);
    _u8(&add, 1);
    _event_unit(&add, 7, 0, roads, related, 1, 1, true, 32);
    assert_int_equal(_action(OI_EVENT_SCHEME, 127, &add), 0);
    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_EVENT_SCHEME_UNIT, 2) | 7, &got), 0);
    const uint8_t want[] = {0x02, 0x05, 0x11, 0x07, 0x02, 0x02, 0x11, 0x00, 0x01, 0x01,
                            0x52, 0x50, 0x04, 0x02, 0x00, 0x11, 0x01, 0x00, 0x10, 0x02,
                            0x00, 0x5C, 0x01, 0x03, 0x01, 0x12, 0x00, 0x20};
    assert_int_equal(got.n, sizeof(want));
    assert_memory_equal(got.b, want, sizeof(want));
    _bench_free(&b);
}

static void test_event_delete_update_clear_and_dar(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    static const uint32_t roads[] = {0x50040200u};
    static const uint32_t related[] = {0x00100200u};
    body_t add = {0};
    _u8(&add, DLT698_TAG_ARRAY);
    _u8(&add, 1);
    _event_unit(&add, 7, 0, roads, related, 1, 1, true, 0);
    assert_int_equal(_action(OI_EVENT_SCHEME, 127, &add), 0);

    /* 编号 0 保留。 */
    body_t zero = {0};
    _u8(&zero, DLT698_TAG_ARRAY);
    _u8(&zero, 1);
    _event_unit(&zero, 0, 1, NULL, NULL, 0, 1, false, 0);
    assert_int_equal(_action(OI_EVENT_SCHEME, 127, &zero), EDGE_SLAVE_SRC_DENIED);

    /* 采集类型 1 的内容必须是 NULL；这里故意塞 ROAD 数组，按形状错拒绝。 */
    body_t bad_content = {0};
    _u8(&bad_content, DLT698_TAG_ARRAY);
    _u8(&bad_content, 1);
    _u8(&bad_content, DLT698_TAG_STRUCTURE);
    _u8(&bad_content, 5);
    _u8(&bad_content, DLT698_TAG_UNSIGNED);
    _u8(&bad_content, 8);
    _u8(&bad_content, DLT698_TAG_STRUCTURE);
    _u8(&bad_content, 2);
    _u8(&bad_content, DLT698_TAG_UNSIGNED);
    _u8(&bad_content, 1);
    _u8(&bad_content, DLT698_TAG_ARRAY);
    _u8(&bad_content, 1);
    _u8(&bad_content, 0x52);
    _oad(&bad_content, roads[0]);
    _u8(&bad_content, DLT698_TAG_UNSIGNED);
    _u8(&bad_content, 0);
    _u8(&bad_content, DCU_698_TAG_MS);
    _u8(&bad_content, 1);
    _u8(&bad_content, DLT698_TAG_BOOLEAN);
    _u8(&bad_content, 0);
    _u16(&bad_content, DLT698_TAG_LONG_UNSIGNED, 0);
    assert_int_equal(_action(OI_EVENT_SCHEME, 127, &bad_content), EDGE_SLAVE_SRC_TYPE_MISMATCH);

    body_t ms3 = {0};
    _u8(&ms3, DLT698_TAG_ARRAY);
    _u8(&ms3, 1);
    _event_unit(&ms3, 8, 1, NULL, NULL, 0, 3, false, 0);
    assert_int_equal(_action(OI_EVENT_SCHEME, 127, &ms3), EDGE_SLAVE_SRC_TEMPORARY);

    body_t upd = {0};
    _u8(&upd, DLT698_TAG_STRUCTURE);
    _u8(&upd, 2);
    _u8(&upd, DLT698_TAG_UNSIGNED);
    _u8(&upd, 7);
    _u8(&upd, DLT698_TAG_BOOLEAN);
    _u8(&upd, 0);
    assert_int_equal(_action(OI_EVENT_SCHEME, 130, &upd), 0);
    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_EVENT_SCHEME_UNIT, 2) | 7, &got), 0);
    assert_int_equal(got.b[got.n - 5], DLT698_TAG_BOOLEAN);
    assert_int_equal(got.b[got.n - 4], 0x00);
    upd.b[3] = 8;
    assert_int_equal(_action(OI_EVENT_SCHEME, 130, &upd), EDGE_SLAVE_SRC_NOT_FOUND);

    body_t del = {0};
    _u8(&del, DLT698_TAG_ARRAY);
    _u8(&del, 1);
    _u8(&del, DLT698_TAG_UNSIGNED);
    _u8(&del, 8);
    assert_int_equal(_action(OI_EVENT_SCHEME, 128, &del), EDGE_SLAVE_SRC_NOT_FOUND);
    del.b[3] = 7;
    assert_int_equal(_action(OI_EVENT_SCHEME, 128, &del), 0);
    assert_int_equal(_get(MK_OAD(OI_EVENT_SCHEME_UNIT, 2) | 7, &got), EDGE_SLAVE_SRC_NOT_FOUND);

    /* 129 Clear 只接受 NULL。 */
    body_t clear = {0};
    _u8(&clear, DLT698_TAG_NULL);
    assert_int_equal(_action(OI_EVENT_SCHEME, 127, &add), 0);
    assert_int_equal(_action(OI_EVENT_SCHEME, 129, &clear), 0);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_EVENT_SCHEME, 2), &got), 0);
    assert_int_equal(got.b[1], 0);
    _bench_free(&b);
}

static void test_transparent_scheme_round_trip_and_dar(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    body_t add = {0};
    _tp_scheme_one(&add, 3, false, 0);
    assert_int_equal(_action(OI_TRANSPARENT_SCHEME, 127, &add), 0);

    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_SCHEME_UNIT, 2) | 3, &got), 0);
    body_t want = {0};
    _tp_scheme_one(&want, 3, false, 0);
    assert_int_equal(got.n, want.n);
    assert_memory_equal(got.b, want.b, want.n);

    body_t wait = {0};
    _tp_scheme_one(&wait, 4, true, 0);
    assert_int_equal(_action(OI_TRANSPARENT_SCHEME, 127, &wait), EDGE_SLAVE_SRC_TEMPORARY);
    body_t compare = {0};
    _tp_scheme_one(&compare, 4, false, 1);
    assert_int_equal(_action(OI_TRANSPARENT_SCHEME, 127, &compare), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_SCHEME_UNIT, 2) | 4, &got),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    static const uint8_t addr2[] = {0xAA, 0xBB, 0xCC, 0xDD};
    body_t frame = {0};
    _u8(&frame, DLT698_TAG_STRUCTURE);
    _u8(&frame, 4);
    _u16(&frame, DLT698_TAG_LONG_UNSIGNED, 3);
    _tsa(&frame, addr2, sizeof(addr2));
    _tp_ctrl(&frame, false, 0, 0, 0, 0, 0);
    _tp_frames(&frame, 2);
    assert_int_equal(_action(OI_TRANSPARENT_SCHEME, 128, &frame), 0);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_SCHEME, 2), &got), 0);
    assert_int_equal(got.b[0], DLT698_TAG_ARRAY);
    assert_int_equal(got.b[1], 1);

    static const uint8_t addr_missing[] = {0x01, 0x02};
    body_t del = {0};
    _u8(&del, DLT698_TAG_STRUCTURE);
    _u8(&del, 2);
    _u8(&del, DLT698_TAG_UNSIGNED);
    _u8(&del, 3);
    _u8(&del, DLT698_TAG_ARRAY);
    _u8(&del, 1);
    _tsa(&del, addr_missing, sizeof(addr_missing));
    assert_int_equal(_action(OI_TRANSPARENT_SCHEME, 129, &del), EDGE_SLAVE_SRC_NOT_FOUND);

    body_t del_ids = {0};
    _u8(&del_ids, DLT698_TAG_ARRAY);
    _u8(&del_ids, 1);
    _u8(&del_ids, DLT698_TAG_UNSIGNED);
    _u8(&del_ids, 9);
    assert_int_equal(_action(OI_TRANSPARENT_SCHEME, 130, &del_ids), EDGE_SLAVE_SRC_NOT_FOUND);
    body_t clear = {0};
    _u8(&clear, DLT698_TAG_NULL);
    assert_int_equal(_action(OI_TRANSPARENT_SCHEME, 131, &clear), 0);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_SCHEME, 2), &got), 0);
    assert_int_equal(got.b[1], 0);
    _bench_free(&b);
}

static void test_transparent_wire_shape_is_exact(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    body_t add = {0};
    _tp_scheme_one(&add, 3, false, 0);
    assert_int_equal(_action(OI_TRANSPARENT_SCHEME, 127, &add), 0);
    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_SCHEME_UNIT, 2) | 3, &got), 0);
    const uint8_t want[] = {0x02, 0x03, 0x11, 0x03, 0x01, 0x01, 0x02, 0x06, 0x12, 0x00, 0x01,
                            0x06, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x12, 0x00, 0x00, 0x12,
                            0x00, 0x00, 0x02, 0x04, 0x03, 0x00, 0x12, 0x00, 0x00, 0x16, 0x00,
                            0x02, 0x03, 0x11, 0x00, 0x12, 0x00, 0x00, 0x12, 0x00, 0x00, 0x01,
                            0x01, 0x02, 0x02, 0x11, 0x01, 0x09, 0x02, 0x41, 0x42, 0x12, 0x00,
                            0x00};
    assert_int_equal(got.n, sizeof(want));
    assert_memory_equal(got.b, want, sizeof(want));
    _bench_free(&b);
}

static void test_transparent_results_round_trip(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    static const uint8_t addr[] = {0x12, 0x34, 0x56, 0x78};
    dcu_transparent_result_t *r = dcu_scheme_store_result_buf();
    memset(r, 0, sizeof(*r));
    r->scheme_id = 3;
    r->exec_time = 1795305600ULL; /* 2026-11-22T00:00:00Z */
    r->address.len = sizeof(addr);
    memcpy(r->address.bytes, addr, sizeof(addr));
    r->item_count = 1;
    r->items[0].seq = 1;
    r->items[0].resp_time = 1795305605ULL;
    r->items[0].len = 2;
    memcpy(r->items[0].data, "AB", 2);
    assert_int_equal(dcu_scheme_store_result_append(r), 0);

    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_RESULT, 2), &got), 0);
    assert_int_equal(got.b[0], DLT698_TAG_ARRAY);
    assert_int_equal(got.b[1], 1);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_RESULT_UNIT, 2) | 1, &got), 0);
    const uint8_t want[] = {0x02, 0x04, 0x11, 0x03, 0x1C, 0x07, 0xEA, 0x0B,
                            0x16, 0x00, 0x00, 0x00, 0x04, 0x12, 0x34, 0x56,
                            0x78, 0x01, 0x01, 0x02, 0x03, 0x11, 0x01, 0x1C,
                            0x07, 0xEA, 0x0B, 0x16, 0x00, 0x00, 0x05, 0x09,
                            0x02, 0x41, 0x42};
    assert_int_equal(got.n, sizeof(want));
    assert_memory_equal(got.b, want, sizeof(want));
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_RESULT_UNIT, 2) | 2, &got),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    r->result_id = 0;
    r->exec_time = 1;
    r->item_count = 0;
    assert_int_equal(dcu_scheme_store_result_append(r), 0);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_TRANSPARENT_RESULT_UNIT, 2) | 2, &got),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    _bench_free(&b);
}

static void _report_unit(body_t *b, uint8_t id, const uint32_t *channels, uint8_t channel_count,
                         uint8_t timeout_unit, uint16_t timeout_value, uint8_t max_times,
                         uint8_t content_type, uint32_t content_oad) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 5);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, id);
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, channel_count);
    for (uint8_t i = 0; i < channel_count; i++) {
        _u8(b, DLT698_TAG_OAD);
        _oad(b, channels[i]);
    }
    _u8(b, DLT698_TAG_TI);
    _u8(b, timeout_unit);
    _u8(b, (uint8_t)(timeout_value >> 8));
    _u8(b, (uint8_t)timeout_value);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, max_times);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, content_type);
    if (content_type == 0) {
        _u8(b, DLT698_TAG_OAD);
        _oad(b, content_oad);
    }
}

static void _report_unit_record(body_t *b, uint8_t id, uint32_t channel) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 5);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, id);
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, 1);
    _u8(b, DLT698_TAG_OAD);
    _oad(b, channel);
    _u8(b, DLT698_TAG_TI);
    _u8(b, 0);
    _u8(b, 0);
    _u8(b, 60);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, 3);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, 1);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 3);
    _u8(b, DLT698_TAG_OAD);
    _oad(b, 0x50040200u);
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, 0);
    _u8(b, DLT698_TAG_NULL);
}

static void test_report_scheme_round_trip_and_persistence(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    static const uint32_t ch11[] = {0x40010200u};
    static const uint32_t ch12[] = {0x40020200u, 0x40030200u};
    body_t add = {0};
    _u8(&add, DLT698_TAG_ARRAY);
    _u8(&add, 2);
    _report_unit(&add, 11, ch11, 1, 0, 60, 3, 0, 0x20000200u);
    _report_unit(&add, 12, ch12, 2, 1, 5, 1, 0, 0x20010200u);
    assert_int_equal(_action(OI_REPORT_SCHEME, 127, &add), 0);

    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_REPORT_SCHEME_UNIT, 2) | 11, &got), 0);
    body_t want = {0};
    _report_unit(&want, 11, ch11, 1, 0, 60, 3, 0, 0x20000200u);
    assert_int_equal(got.n, want.n);
    assert_memory_equal(got.b, want.b, want.n);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_REPORT_SCHEME, 2), &got), 0);
    assert_int_equal(got.b[0], DLT698_TAG_ARRAY);
    assert_int_equal(got.b[1], 2);

    assert_int_equal(_bench_reopen_storage(&b), 0);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_REPORT_SCHEME_UNIT, 2) | 12, &got), 0);
    body_t want12 = {0};
    _report_unit(&want12, 12, ch12, 2, 1, 5, 1, 0, 0x20010200u);
    assert_int_equal(got.n, want12.n);
    assert_memory_equal(got.b, want12.b, want12.n);
    _bench_free(&b);
}

static void test_report_wire_shape_is_exact(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    static const uint32_t channels[] = {0x40010200u};
    body_t add = {0};
    _u8(&add, DLT698_TAG_ARRAY);
    _u8(&add, 1);
    _report_unit(&add, 11, channels, 1, 0, 60, 3, 0, 0x20000200u);
    assert_int_equal(_action(OI_REPORT_SCHEME, 127, &add), 0);
    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_REPORT_SCHEME_UNIT, 2) | 11, &got), 0);
    const uint8_t want[] = {0x02, 0x05, 0x11, 0x0B, 0x01, 0x01, 0x51, 0x40, 0x01, 0x02,
                            0x00, 0x54, 0x00, 0x00, 0x3C, 0x11, 0x03, 0x02, 0x02, 0x11,
                            0x00, 0x51, 0x20, 0x00, 0x02, 0x00};
    assert_int_equal(got.n, sizeof(want));
    assert_memory_equal(got.b, want, sizeof(want));
    _bench_free(&b);
}

static void test_report_dar_and_delete(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    static const uint32_t channels[] = {0x40010200u};
    body_t add = {0};
    _u8(&add, DLT698_TAG_ARRAY);
    _u8(&add, 1);
    _report_unit(&add, 11, channels, 1, 0, 60, 3, 0, 0x20000200u);
    assert_int_equal(_action(OI_REPORT_SCHEME, 127, &add), 0);

    body_t zero = {0};
    _u8(&zero, DLT698_TAG_ARRAY);
    _u8(&zero, 1);
    _report_unit(&zero, 0, channels, 1, 0, 60, 3, 0, 0x20000200u);
    assert_int_equal(_action(OI_REPORT_SCHEME, 127, &zero), EDGE_SLAVE_SRC_DENIED);

    body_t record = {0};
    _u8(&record, DLT698_TAG_ARRAY);
    _u8(&record, 1);
    _report_unit_record(&record, 12, channels[0]);
    assert_int_equal(_action(OI_REPORT_SCHEME, 127, &record), EDGE_SLAVE_SRC_TEMPORARY);

    body_t no_channel = {0};
    _u8(&no_channel, DLT698_TAG_ARRAY);
    _u8(&no_channel, 1);
    _report_unit(&no_channel, 12, NULL, 0, 0, 60, 3, 0, 0x20000200u);
    assert_int_equal(_action(OI_REPORT_SCHEME, 127, &no_channel), EDGE_SLAVE_SRC_TEMPORARY);

    body_t bad_ti = {0};
    _u8(&bad_ti, DLT698_TAG_ARRAY);
    _u8(&bad_ti, 1);
    _report_unit(&bad_ti, 12, channels, 1, 4, 60, 3, 0, 0x20000200u);
    assert_int_equal(_action(OI_REPORT_SCHEME, 127, &bad_ti), EDGE_SLAVE_SRC_TYPE_MISMATCH);

    body_t del = {0};
    _u8(&del, DLT698_TAG_ARRAY);
    _u8(&del, 1);
    _u8(&del, DLT698_TAG_UNSIGNED);
    _u8(&del, 12);
    assert_int_equal(_action(OI_REPORT_SCHEME, 128, &del), EDGE_SLAVE_SRC_NOT_FOUND);
    del.b[3] = 11;
    assert_int_equal(_action(OI_REPORT_SCHEME, 128, &del), 0);
    body_t got = {0};
    assert_int_equal(_get(MK_OAD(OI_REPORT_SCHEME_UNIT, 2) | 11, &got),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    body_t clear = {0};
    _u8(&clear, DLT698_TAG_NULL);
    assert_int_equal(_action(OI_REPORT_SCHEME, 127, &add), 0);
    assert_int_equal(_action(OI_REPORT_SCHEME, 129, &clear), 0);
    got.n = 0;
    assert_int_equal(_get(MK_OAD(OI_REPORT_SCHEME, 2), &got), 0);
    assert_int_equal(got.b[1], 0);
    _bench_free(&b);
}


static void test_rule_unit_wire_vector_and_projection(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    body_t got;
    assert_int_equal(_get(MK_OAD(OI_RULE, 2) | 0, &got), 0);
    static const uint8_t expect[] = {
        0x02, 0x02,
        0x5B, 0x00, 0x00, 0x10, 0x02, 0x00,
        0x02, 0x03,
        0x02, 0x02,
        0x01, 0x01, 0x09, 0x04, 0x00, 0x00, 0x01, 0x00,
        0x01, 0x00,
        0x02, 0x02,
        0x01, 0x00, 0x01, 0x00,
        0x02, 0x01,
        0x09, 0x00,
    };
    assert_int_equal(got.n, sizeof(expect));
    assert_memory_equal(got.b, expect, sizeof(expect));

    body_t all;
    assert_int_equal(_get(MK_OAD(OI_RULE_LIBRARY, 2), &all), 0);
    assert_int_equal(all.b[0], DLT698_TAG_ARRAY);
    assert_int_equal(all.b[1], 15);
    assert_int_equal(all.n, 2 + 15 * sizeof(expect));
    assert_memory_equal(all.b + 2, expect, sizeof(expect));
    _bench_free(&b);
}

static void test_rule_add_spare_refreshes_and_persists(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    body_t body = {0};
    _add_body(&body, 1);
    _rule_unit(&body, OAD_VOLTAGE, 1, 0x02010100, 1, 0x02010101, 0, 0, 0, 0, NULL, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), 0);

    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT645", OAD_VOLTAGE), 0x02010100);
    uint16_t elem = 0;
    uint32_t projected = 0;
    while (dcu_698_rules_projected_oad(elem, &projected) == 0 && projected != OAD_VOLTAGE) elem++;
    assert_int_equal(projected, OAD_VOLTAGE);
    body_t got;
    assert_int_equal(_get(MK_OAD(OI_RULE, 2) | elem, &got), 0);
    static const uint8_t spare[] = {0x01, 0x01, 0x09, 0x04, 0x01, 0x01, 0x01, 0x02};
    bool found = false;
    for (size_t i = 0; i + sizeof(spare) <= got.n; i++) {
        if (memcmp(got.b + i, spare, sizeof(spare)) == 0) found = true;
    }
    assert_true(found);
    body_t lib;
    assert_int_equal(_get(MK_OAD(OI_RULE_LIBRARY, 2), &lib), 0);
    assert_int_equal(lib.b[0], DLT698_TAG_ARRAY);
    assert_int_equal(lib.b[1], 15);
    assert_int_equal(lib.n, 488);

    body_t body2 = {0};
    _add_body(&body2, 1);
    _rule_unit(&body2, OAD_VOLTAGE, 1, 0x02010109, 1, 0x02010101, 0, 0, 0, 0, NULL, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body2), 0);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT645", OAD_VOLTAGE), 0x02010109);

    assert_int_equal(_bench_reopen_storage(&b), 0);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT645", OAD_VOLTAGE), 0x02010109);
    _bench_free(&b);
}

static void test_rule_dar_cases(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    body_t body;

    memset(&body, 0, sizeof(body));
    _add_body(&body, 1);
    _rule_unit(&body, 0x90000200u, 1, 0x02010100, 0, 0, 0, 0, 0, 0, NULL, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), EDGE_SLAVE_SRC_TEMPORARY);

    memset(&body, 0, sizeof(body));
    _add_body(&body, 1);
    _u8(&body, DLT698_TAG_STRUCTURE);
    _u8(&body, 2);
    _u8(&body, TAG_CSD);
    _u8(&body, 1);
    _oad(&body, OAD_VOLTAGE);
    _u8(&body, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), EDGE_SLAVE_SRC_TEMPORARY);

    memset(&body, 0, sizeof(body));
    _add_body(&body, 1);
    _rule_unit(&body, OAD_CURRENT, 1, 0x02010100, 0, 0, 0, 0, 0, 0, NULL, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), EDGE_SLAVE_SRC_DENIED);

    memset(&body, 0, sizeof(body));
    _add_body(&body, 1);
    _rule_unit(&body, OAD_VOLTAGE, 0, 0, 0, 0, 0, 0, 0, 0, NULL, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), EDGE_SLAVE_SRC_TYPE_MISMATCH);

    memset(&body, 0, sizeof(body));
    _add_body(&body, 1);
    _rule_unit(&body, OAD_VOLTAGE, 0, 0, 1, 0x02010101, 0, 0, 0, 0, NULL, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), EDGE_SLAVE_SRC_TYPE_MISMATCH);

    memset(&body, 0, sizeof(body));
    _add_body(&body, 1);
    _u8(&body, DLT698_TAG_STRUCTURE);
    _u8(&body, 2);
    _u8(&body, TAG_CSD);
    _u8(&body, 0);
    _oad(&body, OAD_VOLTAGE);
    _u8(&body, DLT698_TAG_STRUCTURE);
    _u8(&body, 3);
    _u8(&body, DLT698_TAG_STRUCTURE);
    _u8(&body, 2);
    _di_array(&body, 4, 2, 0x02010100);
    _di_array(&body, 4, 0, 0);
    _u8(&body, DLT698_TAG_STRUCTURE);
    _u8(&body, 2);
    _di_array(&body, 2, 0, 0);
    _di_array(&body, 2, 0, 0);
    _u8(&body, DLT698_TAG_STRUCTURE);
    _u8(&body, 1);
    _u8(&body, DLT698_TAG_OCTET_STRING);
    _u8(&body, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), EDGE_SLAVE_SRC_TEMPORARY);

    body_t got;
    assert_int_equal(_get(MK_OAD(OI_RULE, 2) | 15, &got), EDGE_SLAVE_SRC_NOT_FOUND);

    memset(&body, 0, sizeof(body));
    _delete_body(&body, 1);
    _delete_csd(&body, 0x90000200u);
    assert_int_equal(_action(OI_RULE_LIBRARY, 128, &body), EDGE_SLAVE_SRC_TEMPORARY);

    memset(&body, 0, sizeof(body));
    _delete_body(&body, 1);
    _delete_csd(&body, OAD_REACTIVE_POWER_TOTAL);
    assert_int_equal(_action(OI_RULE_LIBRARY, 128, &body), 0);

    memset(&body, 0, sizeof(body));
    _delete_body(&body, 1);
    _delete_csd(&body, OAD_REACTIVE_POWER_TOTAL);
    assert_int_equal(_action(OI_RULE_LIBRARY, 128, &body), EDGE_SLAVE_SRC_NOT_FOUND);

    memset(&body, 0, sizeof(body));
    _u8(&body, DLT698_TAG_NULL);
    assert_int_equal(_action(OI_RULE_LIBRARY, 130, &body), EDGE_SLAVE_SRC_UNDEFINED);
    _bench_free(&b);
}

static void test_rule_delete_clear_keep_internal_rows(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);

    body_t del = {0};
    _delete_body(&del, 1);
    _delete_csd(&del, OAD_VOLTAGE);
    assert_int_equal(_action(OI_RULE_LIBRARY, 128, &del), 0);
    body_t all;
    assert_int_equal(_get(MK_OAD(OI_RULE_LIBRARY, 2), &all), 0);
    assert_int_equal(all.b[1], 14);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT645", OAD_VOLTAGE), 0);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT698", OAD_VOLTAGE), OAD_VOLTAGE);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("ModbusRTU", OAD_VOLTAGE), MODBUS_NATIVE(0, 1));

    body_t clear = {0};
    _u8(&clear, DLT698_TAG_NULL);
    assert_int_equal(_action(OI_RULE_LIBRARY, 129, &clear), 0);
    assert_int_equal(_get(MK_OAD(OI_RULE_LIBRARY, 2), &all), 0);
    assert_int_equal(all.b[1], 0);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT698", OAD_VOLTAGE), OAD_VOLTAGE);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("ModbusRTU", OAD_VOLTAGE), MODBUS_NATIVE(0, 1));

    assert_int_equal(_bench_reopen_storage(&b), 0);
    assert_int_equal(_get(MK_OAD(OI_RULE_LIBRARY, 2), &all), 0);
    assert_int_equal(all.b[1], 0);
    _bench_free(&b);
}

/* 相别行的 CSD 是主站可见的 698 OAD（0x20000201），不是内部影子 tag（0x20000300）。 */
static void test_rule_phase_csd_mapping(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);

    uint16_t count = 0;
    assert_int_equal(dcu_698_rules_projected_count(&count), 0);
    assert_int_equal(count, 15);
    uint16_t elem = 0;
    bool has_external = false, has_shadow = false;
    for (uint16_t i = 0; i < count; i++) {
        uint32_t tag = 0;
        dcu_rule_unit_t u;
        assert_int_equal(dcu_698_rules_projected_oad(i, &tag), 0);
        assert_int_equal(dcu_698_rules_unit_load(tag, &u), 0);
        if (u.csd == OAD_VOLTAGE_B_ELEM) {
            has_external = true;
            elem = i;
        }
        if (u.csd == TAG_VOLTAGE_B) has_shadow = true;
    }
    assert_true(has_external);
    assert_false(has_shadow);

    body_t body = {0};
    _add_body(&body, 1);
    _rule_unit(&body, OAD_VOLTAGE_B_ELEM, 1, 0x02010200, 1, 0x0201020A, 0, 0, 0, 0, NULL, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), 0);

    body_t got;
    assert_int_equal(_get(MK_OAD(OI_RULE, 2) | elem, &got), 0);
    assert_int_equal(got.n, body.n - 2);
    assert_memory_equal(got.b, body.b + 2, got.n);

    body_t lib;
    assert_int_equal(_get(MK_OAD(OI_RULE_LIBRARY, 2), &lib), 0);
    bool lib_external = false, lib_shadow = false;
    uint32_t csd_bytes = OAD_VOLTAGE_B_ELEM;
    uint8_t needle[6] = {0x5B, 0x00, (uint8_t)(csd_bytes >> 24), (uint8_t)(csd_bytes >> 16),
                         (uint8_t)(csd_bytes >> 8), (uint8_t)csd_bytes};
    uint32_t shadow = TAG_VOLTAGE_B;
    uint8_t shadow_needle[6] = {0x5B, 0x00, (uint8_t)(shadow >> 24), (uint8_t)(shadow >> 16),
                                (uint8_t)(shadow >> 8), (uint8_t)shadow};
    for (size_t i = 0; i + 6 <= lib.n; i++) {
        if (memcmp(lib.b + i, needle, 6) == 0) lib_external = true;
        if (memcmp(lib.b + i, shadow_needle, 6) == 0) lib_shadow = true;
    }
    assert_true(lib_external);
    assert_false(lib_shadow);

    memset(&body, 0, sizeof(body));
    _add_body(&body, 1);
    _rule_unit(&body, TAG_VOLTAGE_B, 1, 0x02010200, 0, 0, 0, 0, 0, 0, NULL, 0);
    assert_int_equal(_action(OI_RULE_LIBRARY, 127, &body), EDGE_SLAVE_SRC_TEMPORARY);

    body_t del = {0};
    _delete_body(&del, 1);
    _delete_csd(&del, OAD_VOLTAGE_B_ELEM);
    assert_int_equal(_action(OI_RULE_LIBRARY, 128, &del), 0);
    assert_int_equal(_get(MK_OAD(OI_RULE_LIBRARY, 2), &lib), 0);
    assert_int_equal(lib.b[1], 14);
    assert_int_equal(dcu_driver_adapter_oad_to_proto("DLT698", TAG_VOLTAGE_B),
                     OAD_VOLTAGE_B_ELEM);
    _bench_free(&b);
}


static int _mock_record_telemetry(void *ctx, const char *addr, uint32_t idx, uint32_t tag,
                                  double val) {
    (void)ctx;
    (void)addr;
    (void)idx;
    (void)tag;
    (void)val;
    return 0;
}

static int _mock_submit(void *ctx, uint32_t meter_idx, uint32_t tag_id, uint32_t task_mask,
                        poller_read_done_cb cb, void *ud) {
    (void)ctx;
    (void)task_mask;
    (void)meter_idx;
    (void)tag_id;
    if (cb) cb(ud, 0, 231.7);
    return 0;
}

/* 6040–6042 读真实 poller 墙钟；6032–6035 缺诚实来源，整项 DAR 2。 */
static void test_acq_timestamps_and_status_dar(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);

    body_t got;
    assert_int_equal(_get(MK_OAD(OI_ACQ_START_TIME, 2), &got), EDGE_SLAVE_SRC_TEMPORARY);

    assert_int_equal(dcu_inventory_add(&b.inv, "000000000001", "DLT645", 2400, 1), 0);
    uint32_t tag = OAD_VOLTAGE;
    assert_int_equal(dcu_poller_add_scheme(&b.poller, 1, 0, &tag, 1), 0);
    assert_int_equal(dcu_poller_add_task(&b.poller, 1, 0, 1), 0);
    b.poller.source_ops = (poller_source_ops_t){.ctx = NULL, .submit = _mock_submit};
    b.poller.sink_ops = (poller_sink_ops_t){.ctx = NULL,
                                           .record_telemetry = _mock_record_telemetry,
                                           .commit_batch = NULL};
    assert_int_equal(dcu_poller_step(&b.poller, &b.inv, 10), 0);

    const uint16_t ts_ois[] = {OI_ACQ_START_TIME, OI_ACQ_SUCCESS_TIME, OI_ACQ_STORE_TIME};
    for (size_t i = 0; i < sizeof(ts_ois) / sizeof(ts_ois[0]); i++) {
        assert_int_equal(_get(MK_OAD(ts_ois[i], 2), &got), 0);
        assert_int_equal(got.n, 8);
        assert_int_equal(got.b[0], DLT698_TAG_DATE_TIME_S);
        uint16_t year = (uint16_t)((got.b[1] << 8) | got.b[2]);
        assert_true(year >= 2020 && year <= 2100);
    }

    const uint16_t dar_ois[] = {OI_ACQ_STATUS_SET, OI_ACQ_STATUS};
    for (size_t i = 0; i < sizeof(dar_ois) / sizeof(dar_ois[0]); i++) {
        assert_int_equal(_get(MK_OAD(dar_ois[i], 2), &got), EDGE_SLAVE_SRC_TEMPORARY);
    }
    assert_int_equal(_get(MK_OAD(OI_ACQ_START_TIME, 3), &got), EDGE_SLAVE_SRC_UNDEFINED);
    _bench_free(&b);
}


typedef struct {
    dcu_poller_t *poller;
    poller_read_done_cb cb;
    void *ud;
    uint32_t mask;
    int submits;
} async_src_t;

static int _async_submit(void *ctx, uint32_t meter_idx, uint32_t tag_id, uint32_t task_mask,
                         poller_read_done_cb cb, void *ud) {
    (void)meter_idx;
    (void)tag_id;
    async_src_t *a = (async_src_t *)ctx;
    a->cb = cb;
    a->ud = ud;
    a->mask = task_mask;
    a->submits++;
    dcu_poller_note_tx(a->poller, task_mask);
    return 0;
}

static uint32_t _read_lu(const body_t *b, size_t *i) {
    assert_int_equal(b->b[*i], DLT698_TAG_LONG_UNSIGNED);
    uint32_t v = ((uint32_t)b->b[*i + 1] << 8) | (uint32_t)b->b[*i + 2];
    *i += 3;
    return v;
}

static void _read_time(const body_t *b, size_t *i, int *present) {
    assert_int_equal(b->b[*i], DLT698_TAG_DATE_TIME_S);
    *present = !(b->b[*i + 1] == 0xFF && b->b[*i + 2] == 0xFF);
    *i += 8;
}

/* 6035 状态机：到期→执行中→（回调结束）已执行；计数清零；6034 按 task_id 升序。 */
static void test_task_monitor_state_machine(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    assert_int_equal(dcu_inventory_add(&b.inv, "000000000001", "DLT645", 2400, 1), 0);
    uint32_t tag = OAD_VOLTAGE;
    assert_int_equal(dcu_poller_add_scheme(&b.poller, 1, 0, &tag, 1), 0);
    assert_int_equal(dcu_poller_add_task(&b.poller, 7, 0, 1), 0);
    async_src_t src = {.poller = &b.poller};
    b.poller.source_ops = (poller_source_ops_t){.ctx = &src, .submit = _async_submit};
    assert_int_equal(dcu_poller_step(&b.poller, &b.inv, 10), 0);

    body_t got;
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 7, &got), 0);
    size_t i = 0;
    assert_int_equal(got.b[i++], DLT698_TAG_STRUCTURE);
    assert_int_equal(got.b[i++], 8);
    assert_int_equal(got.b[i++], DLT698_TAG_UNSIGNED);
    assert_int_equal(got.b[i++], 7);
    assert_int_equal(got.b[i++], DLT698_TAG_ENUM);
    assert_int_equal(got.b[i++], 1);
    int start_ok = 0, end_ok = 0;
    _read_time(&got, &i, &start_ok);
    _read_time(&got, &i, &end_ok);
    assert_true(start_ok);
    assert_false(end_ok);
    assert_int_equal(_read_lu(&got, &i), 1);
    assert_int_equal(_read_lu(&got, &i), 0);
    assert_int_equal(_read_lu(&got, &i), 1);
    assert_int_equal(_read_lu(&got, &i), 0);
    assert_int_equal(i, got.n);
    assert_true(src.submits >= 1);

    dcu_poller_note_rx(&b.poller, src.mask);
    src.cb(src.ud, 0, 231.7);

    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 7, &got), 0);
    i = 0;
    assert_int_equal(got.b[i++], DLT698_TAG_STRUCTURE);
    assert_int_equal(got.b[i++], 8);
    assert_int_equal(got.b[i++], DLT698_TAG_UNSIGNED);
    assert_int_equal(got.b[i++], 7);
    assert_int_equal(got.b[i++], DLT698_TAG_ENUM);
    assert_int_equal(got.b[i++], 2);
    _read_time(&got, &i, &start_ok);
    _read_time(&got, &i, &end_ok);
    assert_true(start_ok && end_ok);
    assert_int_equal(_read_lu(&got, &i), 1);
    assert_int_equal(_read_lu(&got, &i), 1);
    assert_int_equal(_read_lu(&got, &i), 1);
    assert_int_equal(_read_lu(&got, &i), 1);

    /* 下一轮执行重新清零。 */
    assert_int_equal(dcu_poller_step(&b.poller, &b.inv, 10), 0);
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 7, &got), 0);
    assert_int_equal(got.b[5], 1); /* status 执行中 */
    i = 6;
    _read_time(&got, &i, &start_ok);
    _read_time(&got, &i, &end_ok);
    assert_int_equal(_read_lu(&got, &i), 1);
    assert_int_equal(_read_lu(&got, &i), 0); /* success 已清零 */

    /* 未执行的任务：从未 step。 */
    assert_int_equal(dcu_poller_add_task(&b.poller, 9, 5, 1), 0);
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 9, &got), 0);
    assert_int_equal(got.b[5], 0);
    assert_int_equal(got.b[7], 0xFF);
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 8, &got), EDGE_SLAVE_SRC_NOT_FOUND);

    body_t all;
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR_SET, 2), &all), 0);
    assert_int_equal(all.b[0], DLT698_TAG_ARRAY);
    assert_int_equal(all.b[1], 2);
    assert_int_equal(all.b[2], DLT698_TAG_STRUCTURE);
    assert_int_equal(all.b[4], DLT698_TAG_UNSIGNED);
    assert_int_equal(all.b[5], 7); /* task_id 升序：7 在 9 前 */
    _bench_free(&b);
}


typedef struct {
    dcu_poller_t *poller;
    poller_read_done_cb cb[8];
    void *ud[8];
    uint32_t mask[8];
    int n;
    int accepts;
} multi_src_t;

static int _multi_submit(void *ctx, uint32_t meter_idx, uint32_t tag_id, uint32_t task_mask,
                         poller_read_done_cb cb, void *ud) {
    (void)meter_idx;
    (void)tag_id;
    multi_src_t *m = (multi_src_t *)ctx;
    if (m->n >= 8) return -1;
    m->cb[m->n] = cb;
    m->ud[m->n] = ud;
    m->mask[m->n] = task_mask;
    m->n++;
    m->accepts++;
    dcu_poller_note_tx(m->poller, task_mask);
    return 0;
}

static void _deliver(multi_src_t *m, int idx, int status) {
    dcu_poller_note_rx(m->poller, m->mask[idx]);
    m->cb[idx](m->ud[idx], status, 231.7);
}

/* 1 表 2 tag：第一个回调后仍是执行中，第二个回调后才已执行；sent/recv 各 2。 */
static void test_task_completion_waits_for_all_pending(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    assert_int_equal(dcu_inventory_add(&b.inv, "000000000001", "DLT645", 2400, 1), 0);
    uint32_t tags[2] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(dcu_poller_add_scheme(&b.poller, 1, 0, tags, 2), 0);
    assert_int_equal(dcu_poller_add_task(&b.poller, 7, 0, 1), 0);
    multi_src_t src = {.poller = &b.poller};
    b.poller.source_ops = (poller_source_ops_t){.ctx = &src, .submit = _multi_submit};
    assert_int_equal(dcu_poller_step(&b.poller, &b.inv, 10), 0);
    assert_int_equal(src.n, 1);

    _deliver(&src, 0, 0);
    assert_int_equal(src.n, 2); /* 第二个 tag 已提交 */
    body_t got;
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 7, &got), 0);
    assert_int_equal(got.b[5], 1); /* 仍执行中 */

    _deliver(&src, 1, 0);
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 7, &got), 0);
    assert_int_equal(got.b[5], 2);
    size_t i2 = 6;
    int present = 0;
    _read_time(&got, &i2, &present);
    _read_time(&got, &i2, &present);
    assert_int_equal(_read_lu(&got, &i2), 1);
    assert_int_equal(_read_lu(&got, &i2), 1);
    assert_int_equal(_read_lu(&got, &i2), 2);
    assert_int_equal(_read_lu(&got, &i2), 2);
    _bench_free(&b);
}

static int _reject_submit(void *ctx, uint32_t meter_idx, uint32_t tag_id, uint32_t task_mask,
                          poller_read_done_cb cb, void *ud) {
    (void)ctx;
    (void)meter_idx;
    (void)tag_id;
    (void)task_mask;
    (void)cb;
    (void)ud;
    return -1;
}

/* 提交立即失败没有帧发出：sent/recv 不动。 */
static void test_submit_failure_counts_no_tx(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    assert_int_equal(dcu_inventory_add(&b.inv, "000000000001", "DLT645", 2400, 1), 0);
    uint32_t tag = OAD_VOLTAGE;
    assert_int_equal(dcu_poller_add_scheme(&b.poller, 1, 0, &tag, 1), 0);
    assert_int_equal(dcu_poller_add_task(&b.poller, 7, 0, 1), 0);
    b.poller.source_ops = (poller_source_ops_t){.ctx = NULL, .submit = _reject_submit};
    assert_int_equal(dcu_poller_step(&b.poller, &b.inv, 10), 0);
    body_t got;
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 7, &got), 0);
    size_t i = 6;
    int present = 0;
    _read_time(&got, &i, &present);
    _read_time(&got, &i, &present);
    assert_int_equal(_read_lu(&got, &i), 1);
    assert_int_equal(_read_lu(&got, &i), 0);
    assert_int_equal(_read_lu(&got, &i), 0); /* sent 不增 */
    assert_int_equal(_read_lu(&got, &i), 0); /* recv 不增 */
    _bench_free(&b);
}

/* 执行中增表：本轮作废重开，success 清零、total 重算，最终 success ≤ total 且新表被计。 */
static void test_archive_change_reopens_execution(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    assert_int_equal(dcu_inventory_add(&b.inv, "000000000001", "DLT645", 2400, 1), 0);
    uint32_t tag = OAD_VOLTAGE;
    assert_int_equal(dcu_poller_add_scheme(&b.poller, 1, 0, &tag, 1), 0);
    assert_int_equal(dcu_poller_add_task(&b.poller, 7, 0, 1), 0);
    multi_src_t src = {.poller = &b.poller};
    b.poller.source_ops = (poller_source_ops_t){.ctx = &src, .submit = _multi_submit};
    assert_int_equal(dcu_poller_step(&b.poller, &b.inv, 10), 0);
    assert_int_equal(src.n, 1);

    assert_int_equal(dcu_inventory_add(&b.inv, "000000000002", "DLT645", 2400, 1), 0);
    assert_int_equal(dcu_poller_step(&b.poller, &b.inv, 10), 0);
    body_t got;
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 7, &got), 0);
    assert_int_equal(got.b[5], 1);
    size_t i = 6;
    int present = 0;
    _read_time(&got, &i, &present);
    _read_time(&got, &i, &present);
    assert_int_equal(_read_lu(&got, &i), 2); /* total 重算 */
    assert_int_equal(_read_lu(&got, &i), 0); /* success 清零 */
    assert_int_equal(_read_lu(&got, &i), 0); /* sent 清零 */

    for (int pass = 0; pass < 8 && src.n > 0; pass++) {
        int idx = --src.n;
        _deliver(&src, idx, 0);
    }
    assert_int_equal(_get(MK_OAD(OI_TASK_MONITOR, 2) | 7, &got), 0);
    i = 6;
    _read_time(&got, &i, &present);
    _read_time(&got, &i, &present);
    uint32_t total = _read_lu(&got, &i);
    uint32_t success = _read_lu(&got, &i);
    assert_true(success <= total);
    assert_true(success >= 1);
    _bench_free(&b);
}


/* GetRecord 从硬编码 5004 改为 OI→freeze_oad；切换类诚实 DAR 2。 */
static void test_record_oi_maps_freeze_oad(void **state) {
    (void)state;
    bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    assert_int_equal(dcu_inventory_add(&b.inv, "000000000001", "DLT645", 2400, 1), 0);
    assert_int_equal(dcu_storage_write_freeze(&b.storage, "000000000001", 0x50040200, 1000,
                                              OAD_ACTIVE_ENERGY, 12.5),
                     0);
    assert_int_equal(dcu_storage_write_freeze(&b.storage, "000000000001", 0x50050200, 2000,
                                              OAD_ACTIVE_ENERGY, 13.5),
                     0);

    dcu_698_record_ctx_t ctx;
    dcu_698_record_init(&ctx, &b.storage, &b.inv);
    static const uint8_t no_selection[] = {0x00, 0x00}; /* RSD 无选择 + RCSD 全列 */
    size_t rows = 0;
    assert_int_equal(dcu_698_record_begin(&ctx, 0x50040200, no_selection, sizeof(no_selection),
                                          &rows),
                     0);
    assert_int_equal(rows, 1);
    assert_int_equal(dcu_698_record_begin(&ctx, 0x50050200, no_selection, sizeof(no_selection),
                                          &rows),
                     0);
    assert_int_equal(rows, 1);
    assert_int_equal(dcu_698_record_begin(&ctx, 0x50080200, no_selection, sizeof(no_selection),
                                          &rows),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(dcu_698_record_begin(&ctx, 0x60000200, no_selection, sizeof(no_selection),
                                          &rows),
                     EDGE_SLAVE_SRC_NOT_FOUND);
    _bench_free(&b);
}

static void test_object_list_advertises_new_ois(void **state) {
    (void)state;
    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    assert_int_equal(dcu_698_appconn_object_list(buf, sizeof(buf), &len), 0);
    assert_int_equal(buf[0], DLT698_TAG_ARRAY);
    assert_int_equal(buf[1], 0x81);
    assert_int_equal(buf[2], 138);
    static const uint8_t ev_scheme[] = {0x02, 0x02, 0x50, 0x60, 0x16};
    static const uint8_t tp_scheme[] = {0x02, 0x02, 0x50, 0x60, 0x18};
    static const uint8_t tp_result[] = {0x02, 0x02, 0x50, 0x60, 0x1A};
    static const uint8_t rp_scheme[] = {0x02, 0x02, 0x50, 0x60, 0x1C};
    static const uint8_t rule_lib[] = {0x02, 0x02, 0x50, 0x60, 0x1E};
    static const uint8_t rule_one[] = {0x02, 0x02, 0x50, 0x60, 0x1F};
    static const uint8_t status_set[] = {0x02, 0x02, 0x50, 0x60, 0x32};
    static const uint8_t status_one[] = {0x02, 0x02, 0x50, 0x60, 0x33};
    static const uint8_t monitor_set[] = {0x02, 0x02, 0x50, 0x60, 0x34};
    static const uint8_t monitor_one[] = {0x02, 0x02, 0x50, 0x60, 0x35};
    static const uint8_t start_ts[] = {0x02, 0x02, 0x50, 0x60, 0x40};
    static const uint8_t success_ts[] = {0x02, 0x02, 0x50, 0x60, 0x41};
    static const uint8_t store_ts[] = {0x02, 0x02, 0x50, 0x60, 0x42};
    static const uint8_t freeze_5000[] = {0x02, 0x02, 0x50, 0x50, 0x00};
    static const uint8_t freeze_5005[] = {0x02, 0x02, 0x50, 0x50, 0x05};
    static const uint8_t freeze_5011[] = {0x02, 0x02, 0x50, 0x50, 0x11};
    bool has_freeze = false, has_switch = false;
    bool has_ev = false, has_tp = false, has_res = false, has_rp = false, has_rl = false,
         has_ro = false, has_status = false;
    for (size_t i = 0; i + 5 <= len; i++) {
        if (memcmp(buf + i, ev_scheme, 5) == 0) has_ev = true;
        if (memcmp(buf + i, tp_scheme, 5) == 0) has_tp = true;
        if (memcmp(buf + i, tp_result, 5) == 0) has_res = true;
        if (memcmp(buf + i, rp_scheme, 5) == 0) has_rp = true;
        if (memcmp(buf + i, rule_lib, 5) == 0) has_rl = true;
        if (memcmp(buf + i, rule_one, 5) == 0) has_ro = true;
        if (memcmp(buf + i, status_set, 5) == 0 || memcmp(buf + i, status_one, 5) == 0 ||
            memcmp(buf + i, monitor_set, 5) == 0 || memcmp(buf + i, monitor_one, 5) == 0 ||
            memcmp(buf + i, start_ts, 5) == 0 || memcmp(buf + i, success_ts, 5) == 0 ||
            memcmp(buf + i, store_ts, 5) == 0) {
            has_status = true;
        }
        if (memcmp(buf + i, freeze_5000, 5) == 0 || memcmp(buf + i, freeze_5005, 5) == 0)
            has_freeze = true;
        if (memcmp(buf + i, freeze_5011, 5) == 0) has_switch = true;
    }
    assert_true(has_ev && has_tp && has_res && has_rp && has_rl && has_ro && has_status);
    assert_true(has_freeze);
    assert_false(has_switch); /* 切换类不进 ACL：无触发源且超驱动会话上限 */
}

static int setup_tz(void **state) {
    (void)state;
    setenv("TZ", "UTC", 1);
    tzset();
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_event_scheme_round_trip_and_persistence),
        cmocka_unit_test(test_event_wire_shape_is_exact),
        cmocka_unit_test(test_event_delete_update_clear_and_dar),
        cmocka_unit_test(test_transparent_scheme_round_trip_and_dar),
        cmocka_unit_test(test_transparent_wire_shape_is_exact),
        cmocka_unit_test(test_transparent_results_round_trip),
        cmocka_unit_test(test_report_scheme_round_trip_and_persistence),
        cmocka_unit_test(test_report_wire_shape_is_exact),
        cmocka_unit_test(test_report_dar_and_delete),
        cmocka_unit_test(test_rule_unit_wire_vector_and_projection),
        cmocka_unit_test(test_rule_add_spare_refreshes_and_persists),
        cmocka_unit_test(test_rule_dar_cases),
        cmocka_unit_test(test_rule_phase_csd_mapping),
        cmocka_unit_test(test_acq_timestamps_and_status_dar),
        cmocka_unit_test(test_task_monitor_state_machine),
        cmocka_unit_test(test_record_oi_maps_freeze_oad),
        cmocka_unit_test(test_task_completion_waits_for_all_pending),
        cmocka_unit_test(test_submit_failure_counts_no_tx),
        cmocka_unit_test(test_archive_change_reopens_execution),
        cmocka_unit_test(test_rule_delete_clear_keep_internal_rows),
        cmocka_unit_test(test_object_list_advertises_new_ois),
    };
    return cmocka_run_group_tests(tests, setup_tz, NULL);
}
