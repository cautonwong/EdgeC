#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_axdr.h"
#include "dcu_698_object_model.h"
#include "dcu_698_stat.h"
#include "dcu_inventory.h"
#include "dcu_north_adapter.h"
#include "dcu_poller.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))
#define MK_OMD(oi, method) (((uint32_t)(oi) << 16) | ((uint32_t)(method) << 8))

static dcu_inventory_t g_inv;
static dcu_poller_t g_poller;

static int setup_stat(void **state) {
    (void)state;
    assert_int_equal(dcu_inventory_init(&g_inv), 0);
    assert_int_equal(dcu_poller_init(&g_poller), 0);
    router_sink_ops_t sink = {0};
    router_bus_ops_t bus = {0};
    assert_int_equal(dcu_698_object_model_init(&g_inv, &g_poller, sink, NULL, NULL, NULL, NULL, bus),
                     0);
    dcu_698_stat_reset();
    return 0;
}

static int teardown_stat(void **state) {
    (void)state;
    dcu_698_object_model_reset();
    dcu_poller_destroy(&g_poller);
    dcu_inventory_destroy(&g_inv);
    return 0;
}

static size_t _encode_m3_c14(uint8_t *out, uint32_t oad, uint8_t cycles, uint8_t ti_unit,
                             uint16_t ti_val) {
    size_t off = 0;
    out[off++] = DLT698_TAG_STRUCTURE;
    out[off++] = 4;
    out[off++] = DLT698_TAG_OAD;
    out[off++] = (uint8_t)(oad >> 24);
    out[off++] = (uint8_t)(oad >> 16);
    out[off++] = (uint8_t)(oad >> 8);
    out[off++] = (uint8_t)oad;
    out[off++] = DLT698_TAG_ARRAY;
    out[off++] = 0;
    out[off++] = DLT698_TAG_UNSIGNED;
    out[off++] = cycles;
    out[off++] = DLT698_TAG_TI;
    out[off++] = ti_unit;
    out[off++] = (uint8_t)(ti_val >> 8);
    out[off++] = (uint8_t)ti_val;
    return off;
}

static size_t _encode_m3_c15_16(uint8_t *out, uint32_t oad, uint8_t cycles, uint8_t ti_unit,
                                uint16_t ti_val) {
    size_t off = 0;
    out[off++] = DLT698_TAG_STRUCTURE;
    out[off++] = 3;
    out[off++] = DLT698_TAG_OAD;
    out[off++] = (uint8_t)(oad >> 24);
    out[off++] = (uint8_t)(oad >> 16);
    out[off++] = (uint8_t)(oad >> 8);
    out[off++] = (uint8_t)oad;
    out[off++] = DLT698_TAG_UNSIGNED;
    out[off++] = cycles;
    out[off++] = DLT698_TAG_TI;
    out[off++] = ti_unit;
    out[off++] = (uint8_t)(ti_val >> 8);
    out[off++] = (uint8_t)ti_val;
    return off;
}

static size_t _encode_m4_oad(uint8_t *out, uint32_t oad) {
    out[0] = DLT698_TAG_OAD;
    out[1] = (uint8_t)(oad >> 24);
    out[2] = (uint8_t)(oad >> 16);
    out[3] = (uint8_t)(oad >> 8);
    out[4] = (uint8_t)oad;
    return 5;
}

static int32_t _be_i32(const uint8_t *p) {
    uint32_t u = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
    return (int32_t)u;
}

static uint32_t _be_u32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static void test_stat_unconfigured_returns_empty_array(void **state) {
    (void)state;
    static const uint16_t ois[] = {
        0x2100, 0x2101, 0x2102, 0x2103, 0x2104, 0x2110, 0x2111, 0x2112,
        0x2113, 0x2114, 0x2120, 0x2121, 0x2122, 0x2123, 0x2124,
    };
    uint8_t buf[64];
    for (size_t i = 0; i < sizeof(ois) / sizeof(ois[0]); i++) {
        size_t len = 0;
        assert_int_equal(dcu_north_resolve_axdr(NULL, MK_OAD(ois[i], 2), buf, sizeof(buf), &len), 0);
        assert_int_equal(len, 2);
        assert_int_equal(buf[0], DLT698_TAG_ARRAY);
        assert_int_equal(buf[1], 0x00);

        uint32_t count = 99;
        assert_int_equal(dcu_698_read(MK_OAD(ois[i], 2), &count), 0);
        assert_int_equal(count, 0);
    }
}

static void test_stat_interval_avg_extremum_ingest_and_axdr(void **state) {
    (void)state;
    uint8_t p[32];
    size_t plen = 0;
    const uint32_t src = 0x20000200u;

    /* Configure 0x2100 (Class 14), 0x2110 (Class 15), 0x2120 (Class 16) for src=0x20000200 */
    plen = _encode_m3_c14(p, src, 1, 1, 15);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2100, 3), p, plen), 0);

    plen = _encode_m3_c15_16(p, src, 1, 1, 15);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 3), p, plen), 0);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2120, 3), p, plen), 0);

    /* Before any samples: count == 0 */
    const dcu_stat_entry_t *entries = NULL;
    size_t cnt = 0;
    assert_int_equal(dcu_698_stat_get_entries(0x2110, &entries, &cnt), 0);
    assert_int_equal(cnt, 1);
    assert_int_equal(entries[0].count, 0);

    /* Ingest 3 samples with civil timestamps */
    const uint64_t ts1 = 1700000000ULL;
    const uint64_t ts2 = 1700000060ULL;
    const uint64_t ts3 = 1700000120ULL;
    dcu_698_stat_ingest(src, 220.5, ts1);
    dcu_698_stat_ingest(src, 235.25, ts2);
    dcu_698_stat_ingest(src, 210.0, ts3);

    /* Verify domain state */
    assert_int_equal(dcu_698_stat_get_entries(0x2120, &entries, &cnt), 0);
    assert_int_equal(cnt, 1);
    assert_int_equal(entries[0].count, 3);
    assert_true(entries[0].sum > 665.74 && entries[0].sum < 665.76);
    assert_true(entries[0].avg > 221.91 && entries[0].avg < 221.92);
    assert_true(entries[0].min == 210.0);
    assert_true(entries[0].max == 235.25);
    assert_int_equal((int)entries[0].min_ts_s, (int)ts3);
    assert_int_equal((int)entries[0].max_ts_s, (int)ts2);
    assert_int_equal((int)entries[0].last_ts_s, (int)ts3);

    /* Verify Class 14 A-XDR (0x21000200): elapsed=120s, count=3 */
    uint8_t buf[256];
    size_t len = 0;
    assert_int_equal(dcu_north_resolve_axdr(NULL, 0x21000200u, buf, sizeof(buf), &len), 0);
    assert_int_equal(buf[0], DLT698_TAG_ARRAY);
    assert_int_equal(buf[1], 1);
    assert_int_equal(buf[2], DLT698_TAG_STRUCTURE);
    assert_int_equal(buf[3], 2);
    assert_int_equal(buf[4], DLT698_TAG_OAD);
    assert_int_equal(_be_u32(buf + 5), src);
    assert_int_equal(buf[9], DLT698_TAG_ARRAY);
    assert_int_equal(buf[10], 1);
    assert_int_equal(buf[11], DLT698_TAG_STRUCTURE);
    assert_int_equal(buf[12], 2);
    assert_int_equal(buf[13], DLT698_TAG_DOUBLE_LONG_UNSIGNED);
    assert_int_equal(_be_u32(buf + 14), 120u);
    assert_int_equal(buf[18], DLT698_TAG_DOUBLE_LONG_UNSIGNED);
    assert_int_equal(_be_u32(buf + 19), 3u);
    assert_int_equal(len, 23);

    /* Verify Class 15 A-XDR (0x21100200): sum*100 = 66575, avg*100 = 22192 */
    assert_int_equal(dcu_north_resolve_axdr(NULL, 0x21100200u, buf, sizeof(buf), &len), 0);
    assert_int_equal(buf[0], DLT698_TAG_ARRAY);
    assert_int_equal(buf[1], 1);
    assert_int_equal(buf[2], DLT698_TAG_STRUCTURE);
    assert_int_equal(buf[3], 3);
    assert_int_equal(buf[4], DLT698_TAG_OAD);
    assert_int_equal(_be_u32(buf + 5), src);
    assert_int_equal(buf[9], DLT698_TAG_DOUBLE_LONG);
    assert_int_equal(_be_i32(buf + 10), 66575);
    assert_int_equal(buf[14], DLT698_TAG_DOUBLE_LONG);
    assert_int_equal(_be_i32(buf + 15), 22192);
    assert_int_equal(len, 19);

    /* Verify Class 16 A-XDR (0x21200200): max*100 = 23525 @ ts2, min*100 = 21000 @ ts3 */
    assert_int_equal(dcu_north_resolve_axdr(NULL, 0x21200200u, buf, sizeof(buf), &len), 0);
    assert_int_equal(buf[0], DLT698_TAG_ARRAY);
    assert_int_equal(buf[1], 1);
    assert_int_equal(buf[2], DLT698_TAG_STRUCTURE);
    assert_int_equal(buf[3], 5);
    assert_int_equal(buf[4], DLT698_TAG_OAD);
    assert_int_equal(_be_u32(buf + 5), src);
    assert_int_equal(buf[9], DLT698_TAG_DOUBLE_LONG);
    assert_int_equal(_be_i32(buf + 10), 23525);
    assert_int_equal(buf[14], DLT698_TAG_DATE_TIME_S);
    uint64_t decoded_max_ts = 0;
    assert_int_equal(dcu_698_date_time_s_to_epoch(buf + 14, 8, &decoded_max_ts), 0);
    assert_int_equal((int)decoded_max_ts, (int)ts2);
    assert_int_equal(buf[22], DLT698_TAG_DOUBLE_LONG);
    assert_int_equal(_be_i32(buf + 23), 21000);
    assert_int_equal(buf[27], DLT698_TAG_DATE_TIME_S);
    uint64_t decoded_min_ts = 0;
    assert_int_equal(dcu_698_date_time_s_to_epoch(buf + 27, 8, &decoded_min_ts), 0);
    assert_int_equal((int)decoded_min_ts, (int)ts3);
    assert_int_equal(len, 35);
}

static void test_stat_reset_and_delete(void **state) {
    (void)state;
    uint8_t p[32];
    size_t plen = 0;
    const uint32_t src1 = 0x20000200u;
    const uint32_t src2 = 0x20010200u;

    plen = _encode_m3_c15_16(p, src1, 1, 1, 5);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 3), p, plen), 0);
    plen = _encode_m3_c15_16(p, src2, 1, 1, 5);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 3), p, plen), 0);

    dcu_698_stat_ingest(src1, 10.0, 1700000000ULL);
    dcu_698_stat_ingest(src2, 20.0, 1700000000ULL);

    /* Method 1 reset: clears count/metrics, preserves both entries */
    uint8_t m1_arg[] = {DLT698_TAG_INTEGER, 0};
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 1), m1_arg, sizeof(m1_arg)), 0);

    const dcu_stat_entry_t *entries = NULL;
    size_t cnt = 0;
    assert_int_equal(dcu_698_stat_get_entries(0x2110, &entries, &cnt), 0);
    assert_int_equal(cnt, 2);
    assert_int_equal(entries[0].count, 0);
    assert_int_equal(entries[1].count, 0);

    /* Method 4 delete src1: array length drops from 2 to 1, src2 remains */
    plen = _encode_m4_oad(p, src1);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 4), p, plen), 0);
    assert_int_equal(dcu_698_stat_get_entries(0x2110, &entries, &cnt), 0);
    assert_int_equal(cnt, 1);
    assert_int_equal(entries[0].src_oad, src2);

    /* Deleting already-deleted src1 returns DAR 6 (NOT_FOUND) */
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 4), p, plen),
                     EDGE_SLAVE_SRC_NOT_FOUND);
}

static void test_stat_negative_and_capacity(void **state) {
    (void)state;
    uint8_t p[32];

    /* Malformed A-XDR -> DAR 7 (TYPE_MISMATCH) */
    uint8_t bad_tag[] = {DLT698_TAG_UNSIGNED, 1};
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 3), bad_tag, sizeof(bad_tag)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 4), bad_tag, sizeof(bad_tag)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 1), bad_tag, sizeof(bad_tag)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* Trailing byte -> DAR 7 */
    size_t plen = _encode_m4_oad(p, 0x20000200u);
    p[plen++] = 0xAA;
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 4), p, plen),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    /* Fill up to DCU_STAT_MAX_ENTRIES (8), 9th fails with TEMPORARY */
    for (uint32_t i = 0; i < DCU_STAT_MAX_ENTRIES; i++) {
        plen = _encode_m3_c15_16(p, 0x20000200u + i, 1, 1, 1);
        assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 3), p, plen), 0);
    }
    plen = _encode_m3_c15_16(p, 0x20000299u, 1, 1, 1);
    assert_int_equal(dcu_north_accept_action(NULL, MK_OMD(0x2110, 3), p, plen),
                     EDGE_SLAVE_SRC_TEMPORARY);

    /* Small buffer on resolve_axdr -> DAR 7 */
    uint8_t tiny[4];
    size_t out_len = 0;
    assert_int_equal(dcu_north_resolve_axdr(NULL, 0x21100200u, tiny, sizeof(tiny), &out_len),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_stat_unconfigured_returns_empty_array, setup_stat,
                                        teardown_stat),
        cmocka_unit_test_setup_teardown(test_stat_interval_avg_extremum_ingest_and_axdr,
                                        setup_stat, teardown_stat),
        cmocka_unit_test_setup_teardown(test_stat_reset_and_delete, setup_stat, teardown_stat),
        cmocka_unit_test_setup_teardown(test_stat_negative_and_capacity, setup_stat, teardown_stat),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
