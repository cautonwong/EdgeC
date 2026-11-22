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

#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "dcu_task_store.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

typedef struct {
    uv_loop_t loop;
    dcu_storage_t storage;
    dcu_poller_t poller;
    dcu_inventory_t inv;
    char path[64];
} task_bench_t;

typedef struct {
    uint8_t b[2048];
    size_t n;
} body_t;

static int _apply(void *ctx, dcu_poller_t *poller, const dcu_task_op_t *op) {
    return dcu_task_store_apply((dcu_storage_t *)ctx, poller, op);
}

static int _bench_init(task_bench_t *b) {
    memset(b, 0, sizeof(*b));
    strcpy(b->path, "/tmp/dcu_task_XXXXXX");
    int fd = mkstemp(b->path);
    if (fd < 0) return -1;
    close(fd);
    if (uv_loop_init(&b->loop) != 0) return -1;
    if (dcu_storage_init(&b->storage, &b->loop, b->path) != 0) return -1;
    if (dcu_inventory_init(&b->inv) != 0) return -1;
    if (dcu_poller_init(&b->poller) != 0) return -1;

    uint32_t tags[] = {OAD_VOLTAGE};
    assert_int_equal(dcu_poller_add_scheme(&b->poller, 1, 0, tags, 1), 0);
    assert_int_equal(dcu_task_store_init(&b->storage), 0);
    assert_int_equal(dcu_task_store_load(&b->storage, &b->poller), 0);

    router_sink_ops_t sink = {.ctx = &b->storage, .task_apply = _apply};
    router_bus_ops_t bus = {0};
    assert_int_equal(
        dcu_698_object_model_init(&b->inv, &b->poller, sink, NULL, NULL, NULL, NULL, bus), 0);
    return 0;
}

static void _bench_free(task_bench_t *b) {
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

static void _u8(body_t *b, uint8_t v) { b->b[b->n++] = v; }

static void _u16(body_t *b, uint8_t tag, uint16_t v) {
    _u8(b, tag);
    _u8(b, (uint8_t)(v >> 8));
    _u8(b, (uint8_t)v);
}

static void _ti(body_t *b, uint8_t unit, uint16_t value) {
    _u8(b, DLT698_TAG_TI);
    _u8(b, unit);
    _u8(b, (uint8_t)(value >> 8));
    _u8(b, (uint8_t)value);
}

static void _empty_time(body_t *b) {
    _u8(b, DLT698_TAG_DATE_TIME_S);
    for (int i = 0; i < 7; i++) _u8(b, 0);
}

static void _unit(body_t *b, uint8_t id, uint8_t unit, uint16_t value, uint8_t period_type) {
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 12);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, id);
    _ti(b, unit, value);
    _u8(b, DLT698_TAG_ENUM);
    _u8(b, 1); /* scheme type normal */
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, 1); /* scheme 1 */
    _empty_time(b);
    _empty_time(b);
    _ti(b, 0, 0);
    _u8(b, DLT698_TAG_UNSIGNED);
    _u8(b, 0); /* priority */
    _u8(b, DLT698_TAG_ENUM);
    _u8(b, DCU_TASK_STATUS_NORMAL);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, 0);
    _u16(b, DLT698_TAG_LONG_UNSIGNED, 0);
    _u8(b, DLT698_TAG_STRUCTURE);
    _u8(b, 2);
    _u8(b, DLT698_TAG_ENUM);
    _u8(b, period_type);
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, 0);
}

static void _add_begin(body_t *b, uint8_t count) {
    b->n = 0;
    _u8(b, DLT698_TAG_ARRAY);
    _u8(b, count);
}

static int _add_one(task_bench_t *bench, uint8_t id, uint8_t unit, uint16_t value,
                    uint8_t period_type) {
    (void)bench;
    body_t body = {0};
    _add_begin(&body, 1);
    _unit(&body, id, unit, value, period_type);
    return dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 127), body.b, body.n);
}

static int _delete_ids(task_bench_t *bench, const uint8_t *ids, uint8_t count) {
    (void)bench;
    body_t body = {0};
    _add_begin(&body, count);
    for (uint8_t i = 0; i < count; i++) {
        _u8(&body, DLT698_TAG_UNSIGNED);
        _u8(&body, ids[i]);
    }
    return dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 128), body.b, body.n);
}

static int _update_status(uint8_t id, uint8_t status) {
    body_t body = {0};
    _u8(&body, DLT698_TAG_STRUCTURE);
    _u8(&body, 2);
    _u8(&body, DLT698_TAG_UNSIGNED);
    _u8(&body, id);
    _u8(&body, DLT698_TAG_ENUM);
    _u8(&body, status);
    return dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 130), body.b, body.n);
}

static void test_add_task_changes_plan_and_persists(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);

    assert_int_equal(_add_one(&bench, 7, 1, 2, 2), 0);
    const dcu_task_unit_t *t = dcu_poller_task_find(&bench.poller, 7);
    assert_non_null(t);
    assert_int_equal(t->interval_s, 120);
    assert_int_equal(t->interval_unit, 1);
    assert_int_equal(t->interval_value, 2);
    assert_true(t->has_interval);
    assert_int_equal(t->status, DCU_TASK_STATUS_NORMAL);
    assert_int_equal(t->period_type, 2);

    dcu_poller_t reloaded;
    assert_int_equal(dcu_poller_init(&reloaded), 0);
    assert_int_equal(dcu_task_store_load(&bench.storage, &reloaded), 0);
    const dcu_task_unit_t *r = dcu_poller_task_find(&reloaded, 7);
    assert_non_null(r);
    assert_int_equal(r->interval_s, 120);
    assert_non_null(dcu_poller_task_find(&reloaded, 1));
    dcu_poller_destroy(&reloaded);

    _bench_free(&bench);
}

static void test_truncated_body_returns_7_and_leaves_plan(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);

    body_t body = {0};
    _add_begin(&body, 1);
    _unit(&body, 7, 1, 2, 0);
    size_t cut = body.n - 3; /* 切在 TI/字段中间 */
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 127), body.b, cut),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_null(dcu_poller_task_find(&bench.poller, 7));

    _bench_free(&bench);
}

static void test_wrong_tag_and_count_return_7(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);

    body_t body = {0};
    _add_begin(&body, 1);
    _unit(&body, 7, 0, 60, 0);

    body_t wrong_tag = body;
    wrong_tag.b[2] = DLT698_TAG_ARRAY; /* 单元应是 structure */
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 127), wrong_tag.b, wrong_tag.n),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    body_t wrong_count = body;
    wrong_count.b[3] = 11; /* 6013 是 12 字段 */
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 127), wrong_count.b, wrong_count.n),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    body_t trailing = body;
    _u8(&trailing, 0);
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 127), trailing.b, trailing.n),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    assert_null(dcu_poller_task_find(&bench.poller, 7));
    _bench_free(&bench);
}

static void test_period_change_reinitializes_clock(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);

    assert_int_equal(_add_one(&bench, 7, 0, 60, 0), 0);
    dcu_task_unit_t *t = (dcu_task_unit_t *)dcu_poller_task_find(&bench.poller, 7);
    assert_non_null(t);
    t->clock.next_tick_us = 555;

    assert_int_equal(_add_one(&bench, 7, 0, 120, 0), 0);
    t = (dcu_task_unit_t *)dcu_poller_task_find(&bench.poller, 7);
    assert_int_equal(t->interval_s, 120);
    assert_int_equal(t->clock.next_tick_us, 0);

    /* 同周期重下发：时钟相位保留。 */
    t->clock.next_tick_us = 777;
    assert_int_equal(_add_one(&bench, 7, 0, 120, 0), 0);
    assert_int_equal(t->clock.next_tick_us, 777);

    _bench_free(&bench);
}

static void test_delete_stops_task(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);
    assert_int_equal(_add_one(&bench, 7, 0, 60, 0), 0);

    uint8_t missing[] = {9};
    assert_int_equal(_delete_ids(&bench, missing, 1), EDGE_SLAVE_SRC_NOT_FOUND);
    assert_non_null(dcu_poller_task_find(&bench.poller, 7));

    uint8_t ids[] = {7};
    assert_int_equal(_delete_ids(&bench, ids, 1), 0);
    assert_null(dcu_poller_task_find(&bench.poller, 7));

    dcu_poller_t reloaded;
    assert_int_equal(dcu_poller_init(&reloaded), 0);
    assert_int_equal(dcu_task_store_load(&bench.storage, &reloaded), 0);
    assert_null(dcu_poller_task_find(&reloaded, 7));
    dcu_poller_destroy(&reloaded);

    _bench_free(&bench);
}

static void test_status_update_stops_and_resumes(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);
    assert_int_equal(_add_one(&bench, 7, 0, 60, 0), 0);
    dcu_task_unit_t *t = (dcu_task_unit_t *)dcu_poller_task_find(&bench.poller, 7);

    assert_int_equal(_update_status(7, DCU_TASK_STATUS_DISABLED), 0);
    assert_int_equal(t->status, DCU_TASK_STATUS_DISABLED);
    t->clock.next_tick_us = 444;

    assert_int_equal(_update_status(7, DCU_TASK_STATUS_NORMAL), 0);
    assert_int_equal(t->status, DCU_TASK_STATUS_NORMAL);
    assert_int_equal(t->clock.next_tick_us, 0);

    assert_int_equal(_update_status(9, DCU_TASK_STATUS_NORMAL), EDGE_SLAVE_SRC_NOT_FOUND);
    assert_int_equal(_update_status(7, 3), EDGE_SLAVE_SRC_TYPE_MISMATCH);

    _bench_free(&bench);
}

static void _encode_expected_unit(uint8_t *out, size_t *n, uint8_t id, uint8_t unit,
                                  uint16_t value, uint8_t period_type) {
    body_t b = {0};
    _unit(&b, id, unit, value, period_type);
    memcpy(out, b.b, b.n);
    *n = b.n;
}

static void test_a2_bytes_are_exact(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);

    uint8_t buf[EDGE_SLAVE_AXDR_MAX];
    size_t len = 0;
    uint8_t want[EDGE_SLAVE_AXDR_MAX];
    size_t want_n = 0;
    want[want_n++] = DLT698_TAG_ARRAY;
    want[want_n++] = 1; /* 首启默认任务 1: TI(秒,5), 方案 1, normal */
    size_t unit_len = 0;
    _encode_expected_unit(want + want_n, &unit_len, 1, 0, 5, 0);
    want_n += unit_len;
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_TASK_SCHEDULER, 2), buf, sizeof(buf), &len), 0);
    assert_int_equal(len, want_n);
    assert_memory_equal(buf, want, len);

    /* 6013 elem 0 = 最小现有 task_id；elem = task_id。 */
    len = 0;
    want_n = 0;
    _encode_expected_unit(want, &want_n, 1, 0, 5, 0);
    assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(OI_TASK_UNIT, 2), buf, sizeof(buf), &len),
                     0);
    assert_int_equal(len, want_n);
    assert_memory_equal(buf, want, len);

    assert_int_equal(_add_one(&bench, 7, 0, 60, 1), 0);
    len = 0;
    want_n = 0;
    _encode_expected_unit(want, &want_n, 7, 0, 60, 1);
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_TASK_UNIT, 2) | 7, buf, sizeof(buf), &len), 0);
    assert_int_equal(len, want_n);
    assert_memory_equal(buf, want, len);

    /* elem 9 没有任务 → DAR 6。 */
    assert_int_equal(
        dcu_north_resolve_axdr(NULL, MK_OAD(OI_TASK_UNIT, 2) | 9, buf, sizeof(buf), &len),
        EDGE_SLAVE_SRC_NOT_FOUND);

    _bench_free(&bench);
}

static void test_masks_and_unknown_methods(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);

    body_t body = {0};
    _u8(&body, 0);
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 131), body.b, body.n),
        EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 2), body.b, body.n),
        EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_TASK_UNIT, 1), body.b, body.n),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_TASK_UNIT, 3), body.b, body.n),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(0x9999, 1), body.b, body.n),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    _bench_free(&bench);
}

static void test_ti_zero_is_no_trigger(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);
    assert_int_equal(dcu_inventory_add(&bench.inv, "000000000001", "DLT645", 2400, 1), 0);

    assert_int_equal(_add_one(&bench, 7, 0, 0, 0), 0);
    const dcu_task_unit_t *t = dcu_poller_task_find(&bench.poller, 7);
    assert_non_null(t);
    assert_false(t->has_interval);
    assert_int_equal(t->interval_value, 0);
    ptrdiff_t slot = t - bench.poller.tasks;

    assert_int_equal(dcu_poller_step(&bench.poller, &bench.inv, 0), 0);
    assert_false(bench.poller.task_due[slot]);

    _bench_free(&bench);
}

static void test_month_ti_returns_2(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);
    assert_int_equal(_add_one(&bench, 7, 4, 1, 0), EDGE_SLAVE_SRC_TEMPORARY);
    assert_null(dcu_poller_task_find(&bench.poller, 7));
    _bench_free(&bench);
}

static void test_over_capacity_returns_2(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);

    body_t body = {0};
    _add_begin(&body, 17);
    for (uint8_t i = 1; i <= 17; i++) _unit(&body, (uint8_t)(i + 10), 0, 60, 0);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 127), body.b, body.n),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_null(dcu_poller_task_find(&bench.poller, 11));

    _bench_free(&bench);
}

static void test_task_id_zero_denied(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);
    assert_int_equal(_add_one(&bench, 0, 0, 60, 0), EDGE_SLAVE_SRC_DENIED);
    _bench_free(&bench);
}

static void test_unsupported_field_returns_2(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);

    body_t body = {0};
    _add_begin(&body, 1);
    _unit(&body, 7, 0, 60, 0);
    body.b[15] = 1; /* 第一个 date_time_s 的 year 高字节 */
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 127), body.b, body.n),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_null(dcu_poller_task_find(&bench.poller, 7));

    _bench_free(&bench);
}

static void test_clear_persists_across_reload(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);
    assert_non_null(dcu_poller_task_find(&bench.poller, 1));

    body_t clear = {0};
    _u8(&clear, DLT698_TAG_NULL);
    assert_int_equal(dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 129), clear.b, clear.n),
                     0);
    assert_null(dcu_poller_task_find(&bench.poller, 1));

    /* 重启不是"首次启动"：播种标记仍在，Clear 后保持为空。 */
    assert_int_equal(dcu_task_store_init(&bench.storage), 0);
    dcu_poller_t reloaded;
    assert_int_equal(dcu_poller_init(&reloaded), 0);
    assert_int_equal(dcu_task_store_load(&bench.storage, &reloaded), 0);
    assert_null(dcu_poller_task_find(&reloaded, 1));
    dcu_poller_destroy(&reloaded);

    _bench_free(&bench);
}

static void test_reset_action_via_raw_entry(void **state) {
    (void)state;
    task_bench_t bench;
    assert_int_equal(_bench_init(&bench), 0);
    assert_int_equal(_add_one(&bench, 7, 0, 60, 0), 0);
    dcu_task_unit_t *t = (dcu_task_unit_t *)dcu_poller_task_find(&bench.poller, 7);
    t->cursor_meter_idx = 9;
    t->monitor.success_meters = 3;

    const uint8_t null_param[] = {DLT698_TAG_NULL};
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 1), null_param, sizeof(null_param)),
        0);
    assert_int_equal(t->cursor_meter_idx, 0);
    assert_int_equal(t->monitor.success_meters, 0);
    assert_non_null(dcu_poller_task_find(&bench.poller, 7));

    const uint8_t wrong[] = {DLT698_TAG_UNSIGNED, 1};
    assert_int_equal(
        dcu_698_accept_action_axdr(MK_OMD(OI_TASK_SCHEDULER, 1), wrong, sizeof(wrong)),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);

    _bench_free(&bench);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_add_task_changes_plan_and_persists),
        cmocka_unit_test(test_truncated_body_returns_7_and_leaves_plan),
        cmocka_unit_test(test_wrong_tag_and_count_return_7),
        cmocka_unit_test(test_period_change_reinitializes_clock),
        cmocka_unit_test(test_delete_stops_task),
        cmocka_unit_test(test_status_update_stops_and_resumes),
        cmocka_unit_test(test_a2_bytes_are_exact),
        cmocka_unit_test(test_masks_and_unknown_methods),
        cmocka_unit_test(test_ti_zero_is_no_trigger),
        cmocka_unit_test(test_month_ti_returns_2),
        cmocka_unit_test(test_over_capacity_returns_2),
        cmocka_unit_test(test_task_id_zero_denied),
        cmocka_unit_test(test_unsupported_field_returns_2),
        cmocka_unit_test(test_clear_persists_across_reload),
        cmocka_unit_test(test_reset_action_via_raw_entry),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
