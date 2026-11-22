#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_scheme_store.h"
#include "dcu_storage.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

typedef struct {
    uint8_t b[4096];
    size_t n;
} body_t;

typedef struct {
    uv_loop_t loop;
    dcu_storage_t storage;
    dcu_poller_t poller;
    dcu_inventory_t inv;
    char path[64];
} scheme_bench_t;

/* ---- 链式提交 mock ---- */

#define TRACE_MAX 64
static uint32_t g_trace_tag[TRACE_MAX];
static int g_trace_n;

static int _mock_submit(void *ctx, uint32_t meter_idx, uint32_t tag_id, poller_read_done_cb cb,
                        void *ud) {
    (void)ctx;
    (void)meter_idx;
    if (g_trace_n < TRACE_MAX) g_trace_tag[g_trace_n++] = tag_id;
    if (cb) cb(ud, 0, 220.0);
    return 0;
}

static int _mock_pub(void *ctx, uint32_t tag_id, uint32_t meter_idx, double val, uint64_t ts) {
    (void)ctx;
    (void)tag_id;
    (void)meter_idx;
    (void)val;
    (void)ts;
    return 0;
}

static int _apply_task(void *ctx, dcu_poller_t *poller, const dcu_task_op_t *op) {
    (void)ctx;
    (void)poller;
    (void)op;
    return EDGE_SLAVE_SRC_CLASS_MISMATCH;
}

static int _apply_scheme(void *ctx, dcu_poller_t *poller, const dcu_scheme_op_t *op) {
    return dcu_scheme_store_apply((dcu_storage_t *)ctx, poller, op);
}

static int _bench_init(scheme_bench_t *b) {
    memset(b, 0, sizeof(*b));
    strcpy(b->path, "/tmp/dcu_scheme_XXXXXX");
    int fd = mkstemp(b->path);
    if (fd < 0) return -1;
    close(fd);
    if (uv_loop_init(&b->loop) != 0) return -1;
    if (dcu_storage_init(&b->storage, &b->loop, b->path) != 0) return -1;
    if (dcu_inventory_init(&b->inv) != 0) return -1;
    if (dcu_poller_init(&b->poller) != 0) return -1;

    uint32_t seed[] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(dcu_scheme_store_init(&b->storage, seed, 2), 0);
    assert_int_equal(dcu_scheme_store_load(&b->storage, &b->poller), 0);

    router_sink_ops_t sink = {
        .ctx = &b->storage, .task_apply = _apply_task, .scheme_apply = _apply_scheme};
    router_bus_ops_t bus = {0};
    assert_int_equal(
        dcu_698_object_model_init(&b->inv, &b->poller, sink, NULL, NULL, NULL, NULL, bus), 0);
    return 0;
}

static void _bench_free(scheme_bench_t *b) {
    dcu_698_object_model_reset();
    dcu_poller_destroy(&b->poller);
    dcu_inventory_destroy(&b->inv);
    dcu_storage_destroy(&b->storage);
    uv_run(&b->loop, UV_RUN_NOWAIT);
    uv_loop_close(&b->loop);
    unlink(b->path);
    char side[80];
    snprintf(side, sizeof(side), "%s-wal", b->path);
    unlink(side);
    snprintf(side, sizeof(side), "%s-shm", b->path);
    unlink(side);
}

/* ---- A-XDR 构造 ---- */

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

static void _csd_oad(body_t *b, uint32_t oad) {
    _u8(b, DCU_698_TAG_CSD);
    _u8(b, 0);
    _oad(b, oad);
}

static void _csd_road(body_t *b, uint32_t oad, const uint32_t *related, uint8_t count) {
    _u8(b, DCU_698_TAG_CSD);
    _u8(b, 1);
    _oad(b, oad);
    _u8(b, count);
    for (uint8_t i = 0; i < count; i++) _oad(b, related[i]);
}

static void _capture_content(body_t *b, uint8_t type) {
    switch (type) {
        case 0:
        case 2:
            _u8(b, DLT698_TAG_NULL);
            break;
        case 1:
            _u8(b, DLT698_TAG_UNSIGNED);
            _u8(b, 0);
            break;
        case 3:
            _u8(b, DLT698_TAG_TI);
            _u8(b, 0);
            _u8(b, 0);
            _u8(b, 0);
            break;
        default:
            _u8(b, DLT698_TAG_STRUCTURE);
            _u8(b, 2);
            _u8(b, DLT698_TAG_TI);
            _u8(b, 0);
            _u8(b, 0);
            _u8(b, 0);
            _u16(b, DLT698_TAG_LONG_UNSIGNED, 0);
            break;
    }
}

static void _ms_payload(body_t *b, uint8_t choice) {
    _u8(b, DCU_698_TAG_MS);
    _u8(b, choice);
    switch (choice) {
        case 2:
            _u8(b, 1);
            _u8(b, 0);
            break;
        case 3:
            _u8(b, 1);
            _u8(b, 6);
            for (int i = 0; i < 6; i++) _u8(b, 0);
            break;
        case 4:
            _u8(b, 1);
            _u8(b, 0);
            _u8(b, 0);
            break;
        case 5:
        case 6:
        case 7:
            _u8(b, 1);
            _u8(b, DLT698_TAG_STRUCTURE);
            _u8(b, 3);
            _u8(b, DLT698_TAG_ENUM);
            _u8(b, 0);
            _u8(b, DLT698_TAG_NULL);
            _u8(b, DLT698_TAG_NULL);
            break;
        default:
            break;
    }
}

static void _unit(body_t *b, uint8_t id, uint16_t depth, uint8_t capture_type, const uint32_t *csds,
                  uint8_t csd_count, uint8_t ms_choice, uint8_t label) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 6);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, id);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, depth);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, capture_type);
    _capture_content(b, capture_type);
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, csd_count);
    for (uint8_t i = 0; i < csd_count; i++) _csd_oad(b, csds[i]);
    _ms_payload(b, ms_choice);
    _u8(b, DLT698_TAG_ENUM);
    _u8(b, label);
}

static void _add_begin(body_t *b, uint8_t count) {
    b->n = 0;
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, count);
}

static int _add_one(uint8_t id, uint16_t depth, uint8_t capture_type, const uint32_t *csds,
                    uint8_t csd_count, uint8_t ms_choice, uint8_t label) {
    body_t body = {0};
    _add_begin(&body, 1);
    _unit(&body, id, depth, capture_type, csds, csd_count, ms_choice, label);
    return dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), body.b, body.n);
}

static int _delete_ids(const uint8_t *ids, uint8_t count) {
    body_t body = {0};
    _add_begin(&body, count);
    for (uint8_t i = 0; i < count; i++) {
        _u8(&body, DLT698_TAG_UNSIGNED);
        _u8(&body, ids[i]);
    }
    return dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 128), body.b, body.n);
}

static int _set_csd(uint8_t id, const uint32_t *csds, uint8_t count) {
    body_t body = {0};
    _u8(&body, DLT698_TAG_STRUCTURE);
    _u8(&body, 2);
    _u8(&body, DLT698_TAG_UNSIGNED);
    _u8(&body, id);
    _u8(&body, DLT698_TAG_ARRAY);
    _u8(&body, count);
    for (uint8_t i = 0; i < count; i++) _csd_oad(&body, csds[i]);
    return dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 130), body.b, body.n);
}

static int _clear(void) {
    body_t body = {0};
    _u8(&body, DLT698_TAG_NULL);
    return dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 129), body.b, body.n);
}

static int _a2(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    return dcu_north_resolve_axdr(NULL, oad, buf, cap, len);
}

static void _step(scheme_bench_t *b) {
    poller_sink_ops_t sink = {0};
    poller_source_ops_t src = {.submit = _mock_submit};
    poller_bus_ops_t bus = {.publish_realtime = _mock_pub};
    dcu_poller_start(&b->poller, src, bus, sink);
    g_trace_n = 0;
    dcu_poller_step(&b->poller, &b->inv, 0);
}

/* ---- 采集链：CSD 选择的列就是实际提交的 tag ---- */

static void test_add_selected_columns_are_submitted(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    uint32_t csds[] = {OAD_ACTIVE_ENERGY, OAD_REACTIVE_ENERGY, OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(_add_one(7, 0, 0, csds, 4, 1, 0), 0);

    const dcu_scheme_t *s = dcu_poller_get_scheme(&b.poller, 7);
    assert_non_null(s);
    uint32_t want[2] = {s->data.normal.csd_list[0], s->data.normal.csd_list[2]};
    assert_int_equal(dcu_poller_add_task_ex(&b.poller, 9, 60, 7, 0x5), 0);
    assert_true(dcu_poller_scheme_referenced(&b.poller, 7));
    assert_true(dcu_poller_scheme_masked(&b.poller, 7));

    assert_int_equal(dcu_inventory_add(&b.inv, "000000000001", "dlt698", 9600, 1), 0);
    _step(&b);
    assert_int_equal(g_trace_n, 2);
    assert_int_equal(g_trace_tag[0], want[0]);
    assert_int_equal(g_trace_tag[1], want[1]);
    _bench_free(&b);
}

static void test_set_csd_replaces_and_delete_stops(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    uint32_t csds[] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 1, 0), 0);
    assert_int_equal(dcu_inventory_add(&b.inv, "000000000001", "dlt698", 9600, 1), 0);
    assert_int_equal(dcu_poller_add_task_ex(&b.poller, 9, 60, 7, 0x1), 0);
    _step(&b);
    assert_int_equal(g_trace_n, 1);
    assert_int_equal(g_trace_tag[0], OAD_VOLTAGE);

    assert_int_equal(dcu_poller_remove_task(&b.poller, 9), 0);
    uint32_t changed[] = {OAD_ACTIVE_ENERGY, OAD_GRID_FREQUENCY};
    assert_int_equal(_set_csd(7, changed, 2), 0);
    const dcu_scheme_t *s = dcu_poller_get_scheme(&b.poller, 7);
    assert_non_null(s);
    assert_int_equal(s->data.normal.csd_list[0], OAD_ACTIVE_ENERGY);
    assert_int_equal(s->data.normal.csd_list[1], OAD_GRID_FREQUENCY);
    assert_int_equal(dcu_poller_add_task_ex(&b.poller, 9, 60, 7, 0x1), 0);
    _step(&b);
    assert_int_equal(g_trace_n, 1);
    assert_int_equal(g_trace_tag[0], OAD_ACTIVE_ENERGY);

    assert_int_equal(dcu_poller_remove_task(&b.poller, 9), 0);
    assert_int_equal(_delete_ids((const uint8_t[]){7}, 1), 0);
    assert_null(dcu_poller_get_scheme(&b.poller, 7));
    _step(&b);
    assert_int_equal(g_trace_n, 0); /* 没有任务引用任何方案，链上没有可抄项 */
    _bench_free(&b);
}

static void test_referenced_scheme_refuses_csd_change_and_delete(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    uint32_t csds[] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 1, 0), 0);
    assert_int_equal(dcu_poller_add_task(&b.poller, 9, 60, 7), 0);

    uint32_t changed[] = {OAD_CURRENT, OAD_VOLTAGE};
    assert_int_equal(_set_csd(7, changed, 2), EDGE_SLAVE_SRC_DENIED);
    assert_int_equal(_add_one(7, 0, 0, changed, 2, 1, 0), EDGE_SLAVE_SRC_DENIED);
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 1, 0), 0); /* 列不变的重下发允许 */
    assert_int_equal(_delete_ids((const uint8_t[]){7}, 1), EDGE_SLAVE_SRC_NOT_FOUND);
    assert_int_equal(_clear(), EDGE_SLAVE_SRC_DENIED);
    assert_non_null(dcu_poller_get_scheme(&b.poller, 7));

    assert_int_equal(dcu_poller_remove_task(&b.poller, 9), 0);
    assert_int_equal(_clear(), 0);
    assert_int_equal(b.poller.scheme_count, 0);
    _bench_free(&b);
}

/* ---- 形状错误：DAR 7 且不改状态 ---- */

static void test_shape_errors_return_7(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    uint32_t csds[] = {OAD_VOLTAGE, OAD_CURRENT};
    body_t good = {0};
    _add_begin(&good, 1);
    _unit(&good, 7, 0, 0, csds, 2, 1, 0);

    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), good.b, good.n - 1),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    body_t wrong_tag = good;
    wrong_tag.b[2] = DLT698_TAG_ARRAY;
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), wrong_tag.b, wrong_tag.n),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);
    body_t wrong_count = good;
    wrong_count.b[3] = 5;
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), wrong_count.b, wrong_count.n),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);
    body_t trailing = good;
    _u8(&trailing, 0);
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), trailing.b, trailing.n),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(_add_one(7, 0, 5, csds, 2, 1, 0), EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* 采集内容：类型 0 必须恰好是 NULL。 */
    body_t bad_content = good;
    bad_content.b[13] = DLT698_TAG_UNSIGNED; /* 第 13 字节是采集内容的首字节 */
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), bad_content.b, bad_content.n),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    assert_int_equal(_add_one(7, 0, 0, csds, 2, 8, 0), EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* MS choice 2 的值截断：形状坏 -> 7。 */
    body_t bad_ms = {0};
    _add_begin(&bad_ms, 1);
    _u8(&bad_ms, DLT698_TAG_STRUCTURE);
    _u8(&bad_ms, 6);
    _u8(&bad_ms, DLT698_TAG_UNSIGNED);
    _u8(&bad_ms, 7);
    _u16(&bad_ms, DLT698_TAG_LONG_UNSIGNED, 0);
    _u8(&bad_ms, DLT698_TAG_STRUCTURE);
    _u8(&bad_ms, 2);
    _u8(&bad_ms, DLT698_TAG_UNSIGNED);
    _u8(&bad_ms, 0);
    _u8(&bad_ms, DLT698_TAG_NULL);
    _u8(&bad_ms, DLT698_TAG_ARRAY);
    _u8(&bad_ms, 1);
    _csd_oad(&bad_ms, OAD_VOLTAGE);
    _u8(&bad_ms, DCU_698_TAG_MS);
    _u8(&bad_ms, 2);
    _u8(&bad_ms, 1); /* 声明 1 个 unsigned，却没有值 */
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), bad_ms.b, bad_ms.n),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* ROAD 关联列表截断：形状坏 -> 7。 */
    body_t bad_road = {0};
    _add_begin(&bad_road, 1);
    _u8(&bad_road, DLT698_TAG_STRUCTURE);
    _u8(&bad_road, 6);
    _u8(&bad_road, DLT698_TAG_UNSIGNED);
    _u8(&bad_road, 7);
    _u16(&bad_road, DLT698_TAG_LONG_UNSIGNED, 0);
    _u8(&bad_road, DLT698_TAG_STRUCTURE);
    _u8(&bad_road, 2);
    _u8(&bad_road, DLT698_TAG_UNSIGNED);
    _u8(&bad_road, 0);
    _u8(&bad_road, DLT698_TAG_NULL);
    _u8(&bad_road, DLT698_TAG_ARRAY);
    _u8(&bad_road, 1);
    _u8(&bad_road, DCU_698_TAG_CSD);
    _u8(&bad_road, 1);
    _oad(&bad_road, OAD_DAILY_FREEZE);
    _u8(&bad_road, 4); /* 声明 4 个关联 OAD，一个都没给 */
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), bad_road.b, bad_road.n),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    assert_null(dcu_poller_get_scheme(&b.poller, 7));
    assert_int_equal(b.poller.scheme_count, 1);
    _bench_free(&b);
}

/* ---- 字段策略：标准合法但产品不执行 -> DAR 2 ---- */

static void test_policy_errors_return_2(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    uint32_t csds[] = {OAD_VOLTAGE, OAD_CURRENT};

    assert_int_equal(_add_one(7, 1, 0, csds, 2, 1, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 1, 1), EDGE_SLAVE_SRC_TEMPORARY);
    for (uint8_t type = 1; type <= 4; type++) {
        assert_int_equal(_add_one(7, 0, type, csds, 2, 1, 0), EDGE_SLAVE_SRC_TEMPORARY);
    }
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 0, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 2, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 3, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 4, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 5, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(_add_one(7, 0, 0, csds, 0, 1, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(_add_one(7, 0, 0, (const uint32_t[]){0x11111111}, 1, 1, 0),
                     EDGE_SLAVE_SRC_TEMPORARY);

    body_t road = {0};
    _add_begin(&road, 1);
    _u8(&road, DLT698_TAG_STRUCTURE);
    _u8(&road, 6);
    _u8(&road, DLT698_TAG_UNSIGNED);
    _u8(&road, 7);
    _u16(&road, DLT698_TAG_LONG_UNSIGNED, 0);
    _u8(&road, DLT698_TAG_STRUCTURE);
    _u8(&road, 2);
    _u8(&road, DLT698_TAG_UNSIGNED);
    _u8(&road, 0);
    _u8(&road, DLT698_TAG_NULL);
    _u8(&road, DLT698_TAG_ARRAY);
    _u8(&road, 1);
    _csd_road(&road, OAD_DAILY_FREEZE, (const uint32_t[]){OAD_ACTIVE_ENERGY}, 1);
    _ms_payload(&road, 1);
    _u8(&road, DLT698_TAG_ENUM);
    _u8(&road, 0);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 127), road.b, road.n),
                     EDGE_SLAVE_SRC_TEMPORARY);

    uint32_t too_many[MAX_SCHEME_OADS + 1];
    for (uint16_t i = 0; i < MAX_SCHEME_OADS + 1; i++) too_many[i] = OAD_VOLTAGE;
    assert_int_equal(_add_one(7, 0, 0, too_many, MAX_SCHEME_OADS + 1, 1, 0),
                     EDGE_SLAVE_SRC_TEMPORARY);

    assert_null(dcu_poller_get_scheme(&b.poller, 7));
    _bench_free(&b);
}

static void test_id_zero_and_batch_atomicity(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    uint32_t csds[] = {OAD_VOLTAGE};
    assert_int_equal(_add_one(0, 0, 0, csds, 1, 1, 0), EDGE_SLAVE_SRC_DENIED);
    assert_int_equal(_add_one(7, 0, 0, csds, 1, 1, 0), 0);
    assert_int_equal(_add_one(8, 0, 0, csds, 1, 1, 0), 0);
    assert_int_equal(dcu_poller_add_task(&b.poller, 9, 60, 8), 0);

    assert_int_equal(_delete_ids((const uint8_t[]){7, 8}, 2), EDGE_SLAVE_SRC_NOT_FOUND);
    assert_non_null(dcu_poller_get_scheme(&b.poller, 7));
    assert_non_null(dcu_poller_get_scheme(&b.poller, 8));
    assert_int_equal(dcu_poller_remove_task(&b.poller, 9), 0);
    assert_int_equal(_delete_ids((const uint8_t[]){9}, 1), EDGE_SLAVE_SRC_NOT_FOUND);
    assert_int_equal(_delete_ids((const uint8_t[]){7, 8}, 2), 0);
    assert_null(dcu_poller_get_scheme(&b.poller, 7));
    assert_null(dcu_poller_get_scheme(&b.poller, 8));
    _bench_free(&b);
}

/* ---- A2 字节精确 ---- */

static void _expect_unit(body_t *b, uint8_t id, const uint32_t *csds, uint8_t count) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 6);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, id);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, 0);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, 0);
    _u8(b, DLT698_TAG_NULL);
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, count);
    for (uint8_t i = 0; i < count; i++) _csd_oad(b, csds[i]);
    _u8(b, DCU_698_TAG_MS);
    _u8(b, 1);
    _u8(b, DLT698_TAG_ENUM);
    _u8(b, 0);
}

static void test_a2_bytes_are_exact(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);

    uint8_t buf[2048];
    size_t len = 0;
    body_t want = {0};
    _u8(&want, DLT698_TAG_ARRAY);
    _u8(&want, 1);
    _expect_unit(&want, 1, (const uint32_t[]){OAD_VOLTAGE, OAD_CURRENT}, 2);
    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME, 2), buf, sizeof(buf), &len), 0);
    assert_int_equal(len, want.n);
    assert_memory_equal(buf, want.b, len);

    /* 6015 元素 0 = 最小方案编号；元素 = 方案编号；缺失 -> DAR 6。 */
    len = 0;
    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME_UNIT, 2), buf, sizeof(buf), &len), 0);
    body_t one = {0};
    _expect_unit(&one, 1, (const uint32_t[]){OAD_VOLTAGE, OAD_CURRENT}, 2);
    assert_int_equal(len, one.n);
    assert_memory_equal(buf, one.b, len);

    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME_UNIT, 2) | 9, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    uint32_t csds[] = {0x20000200, 0x20000201};
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 1, 0), 0);
    len = 0;
    body_t both = {0};
    _u8(&both, DLT698_TAG_ARRAY);
    _u8(&both, 2);
    _expect_unit(&both, 1, (const uint32_t[]){OAD_VOLTAGE, OAD_CURRENT}, 2);
    _expect_unit(&both, 7, csds, 2);
    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME, 2), buf, sizeof(buf), &len), 0);
    assert_int_equal(len, both.n);
    assert_memory_equal(buf, both.b, len);

    len = 0;
    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME_UNIT, 2) | 7, buf, sizeof(buf), &len), 0);
    body_t seven = {0};
    _expect_unit(&seven, 7, csds, 2);
    assert_int_equal(len, seven.n);
    assert_memory_equal(buf, seven.b, len);

    /* 未非零字段的脏库：回 DAR 2，不吐畸形字节。 */
    dcu_scheme_t *s = dcu_poller_get_scheme(&b.poller, 7);
    assert_non_null(s);
    s->data.normal.capture_mode = 1;
    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME_UNIT, 2) | 7, buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_TEMPORARY);
    _bench_free(&b);
}

static void test_masks_and_unknown_methods(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    const uint8_t null_param[] = {DLT698_TAG_NULL};

    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 1), null_param, 1),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME, 131), null_param, 1),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME_UNIT, 1), null_param, 1),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_NORMAL_SCHEME_UNIT, 127), null_param, 1),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(0x9999, 1), null_param, 1),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    uint8_t buf[64];
    size_t len = 0;
    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME, 3), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME_UNIT, 1), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(_a2(MK_OAD(OI_NORMAL_SCHEME_UNIT, 3), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);
    _bench_free(&b);
}

/* ---- 持久化 ---- */

static void test_persistence_reload_and_seed_once(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    uint32_t csds[] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 1, 0), 0);
    uint32_t changed[] = {OAD_ACTIVE_ENERGY, OAD_GRID_FREQUENCY};
    assert_int_equal(_set_csd(7, changed, 2), 0);
    assert_int_equal(_add_one(8, 0, 0, csds, 2, 1, 0), 0);
    assert_int_equal(_delete_ids((const uint8_t[]){8}, 1), 0);

    /* 重启：先清空 RAM，再从库装载。 */
    uint32_t stale[] = {OAD_VOLTAGE};
    assert_int_equal(dcu_poller_add_scheme(&b.poller, 99, 0, stale, 1), 0);
    assert_int_equal(dcu_scheme_store_init(&b.storage, NULL, 0), 0);
    assert_int_equal(dcu_scheme_store_load(&b.storage, &b.poller), 0);
    assert_null(dcu_poller_get_scheme(&b.poller, 99));
    assert_null(dcu_poller_get_scheme(&b.poller, 8));
    assert_non_null(dcu_poller_get_scheme(&b.poller, 1));
    const dcu_scheme_t *s = dcu_poller_get_scheme(&b.poller, 7);
    assert_non_null(s);
    assert_int_equal(s->data.normal.csd_count, 2);
    assert_int_equal(s->data.normal.csd_list[0], OAD_ACTIVE_ENERGY);
    assert_int_equal(s->data.normal.csd_list[1], OAD_GRID_FREQUENCY);
    assert_int_equal(b.poller.scheme_count, 2); /* 方案 1 只播种一次，无重复 */
    _bench_free(&b);
}

static void test_clear_persists_across_reload(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    assert_int_equal(dcu_poller_add_task(&b.poller, 9, 60, 1), 0);
    assert_int_equal(_clear(), EDGE_SLAVE_SRC_DENIED);
    assert_int_equal(dcu_poller_remove_task(&b.poller, 9), 0);
    assert_int_equal(_clear(), 0);
    assert_int_equal(b.poller.scheme_count, 0);

    assert_int_equal(dcu_scheme_store_init(&b.storage, NULL, 0), 0);
    dcu_poller_t reloaded;
    assert_int_equal(dcu_poller_init(&reloaded), 0);
    assert_int_equal(dcu_scheme_store_load(&b.storage, &reloaded), 0);
    assert_int_equal(reloaded.scheme_count, 0);
    dcu_poller_destroy(&reloaded);
    _bench_free(&b);
}

static void test_sql_failure_rolls_back_db_and_ram(void **state) {
    (void)state;
    scheme_bench_t b;
    assert_int_equal(_bench_init(&b), 0);
    assert_int_equal(sqlite3_exec(b.storage.db,
                                  "CREATE TRIGGER fail_scheme_csd BEFORE INSERT ON dcu_scheme_csd "
                                  "BEGIN SELECT RAISE(ABORT, 'boom'); END;",
                                  NULL, NULL, NULL),
                     SQLITE_OK);

    uint32_t csds[] = {OAD_VOLTAGE, OAD_CURRENT};
    assert_int_equal(_add_one(7, 0, 0, csds, 2, 1, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_null(dcu_poller_get_scheme(&b.poller, 7));
    const dcu_scheme_t *s = dcu_poller_get_scheme(&b.poller, 1);
    assert_non_null(s);
    assert_int_equal(s->data.normal.csd_count, 2);

    assert_int_equal(sqlite3_exec(b.storage.db, "DROP TRIGGER fail_scheme_csd;", NULL, NULL, NULL),
                     SQLITE_OK);
    dcu_poller_t reloaded;
    assert_int_equal(dcu_poller_init(&reloaded), 0);
    assert_int_equal(dcu_scheme_store_load(&b.storage, &reloaded), 0);
    assert_null(dcu_poller_get_scheme(&reloaded, 7));
    dcu_poller_destroy(&reloaded);
    _bench_free(&b);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_add_selected_columns_are_submitted),
        cmocka_unit_test(test_set_csd_replaces_and_delete_stops),
        cmocka_unit_test(test_referenced_scheme_refuses_csd_change_and_delete),
        cmocka_unit_test(test_shape_errors_return_7),
        cmocka_unit_test(test_policy_errors_return_2),
        cmocka_unit_test(test_id_zero_and_batch_atomicity),
        cmocka_unit_test(test_a2_bytes_are_exact),
        cmocka_unit_test(test_masks_and_unknown_methods),
        cmocka_unit_test(test_persistence_reload_and_seed_once),
        cmocka_unit_test(test_clear_persists_across_reload),
        cmocka_unit_test(test_sql_failure_rolls_back_db_and_ram),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
