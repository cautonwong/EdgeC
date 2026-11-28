/**
 * @file test_dcu_698_devmgr.c
 * @brief Unit tests for DCU-D4-04b Class 19 (4300) device management domain engine.
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dcu_698_devmgr.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

typedef struct {
    int data_calls;
    int event_calls;
    int demand_calls;
    int next_rc;
} mock_devmgr_ctx_t;

static int mock_data_init(void *ctx) {
    mock_devmgr_ctx_t *m = (mock_devmgr_ctx_t *)ctx;
    m->data_calls++;
    return m->next_rc;
}

static int mock_event_init(void *ctx) {
    mock_devmgr_ctx_t *m = (mock_devmgr_ctx_t *)ctx;
    m->event_calls++;
    return m->next_rc;
}

static int mock_demand_init(void *ctx) {
    mock_devmgr_ctx_t *m = (mock_devmgr_ctx_t *)ctx;
    m->demand_calls++;
    return m->next_rc;
}

static void test_devmgr_encode_a2_descriptor(void **state) {
    (void)state;
    dcu_698_devmgr_reset_info();

    uint8_t buf[128];
    size_t out_len = 0;
    assert_int_equal(dcu_698_devmgr_encode_attr(2, buf, sizeof(buf), &out_len), 0);

    const char *expected = "EdgeC-DCU-4300";
    size_t elen = strlen(expected);
    assert_int_equal(out_len, 2 + elen);
    assert_int_equal(buf[0], DLT698_TAG_VISIBLE_STRING);
    assert_int_equal(buf[1], elen);
    assert_memory_equal(buf + 2, expected, elen);
}

static void test_devmgr_encode_a3_version_info(void **state) {
    (void)state;
    dcu_698_devmgr_reset_info();

    uint8_t buf[128];
    size_t out_len = 0;
    assert_int_equal(dcu_698_devmgr_encode_attr(3, buf, sizeof(buf), &out_len), 0);

    /* structure(6): 4 + 4 + 6 + 4 + 6 + 8 = 32 chars + 6*2 tags/lens + 2 struct head = 46 */
    assert_int_equal(out_len, 46);
    assert_int_equal(buf[0], DLT698_TAG_STRUCTURE);
    assert_int_equal(buf[1], 6);

    const char *fields[6] = {"EDGC", "V1.0", "261128", "H1.0", "260101", "EDGC0001"};
    const size_t flens[6] = {4, 4, 6, 4, 6, 8};
    size_t off = 2;
    for (size_t i = 0; i < 6; i++) {
        assert_int_equal(buf[off], DLT698_TAG_VISIBLE_STRING);
        assert_int_equal(buf[off + 1], flens[i]);
        assert_memory_equal(buf + off + 2, fields[i], flens[i]);
        off += 2 + flens[i];
    }
    assert_int_equal(off, out_len);
}

static void test_devmgr_encode_a4_prod_date(void **state) {
    (void)state;
    dcu_698_devmgr_reset_info();

    uint8_t buf[32];
    size_t out_len = 0;
    assert_int_equal(dcu_698_devmgr_encode_attr(4, buf, sizeof(buf), &out_len), 0);

    /* date_time_s: tag(1) + 7 bytes = 8 */
    assert_int_equal(out_len, 8);
    assert_int_equal(buf[0], DLT698_TAG_DATE_TIME_S);
    assert_int_equal(buf[1], 0x07); /* 2026 >> 8 */
    assert_int_equal(buf[2], 0xEA); /* 2026 & 0xFF */
    assert_int_equal(buf[3], 11);
    assert_int_equal(buf[4], 28);
    assert_int_equal(buf[5], 0);
    assert_int_equal(buf[6], 0);
    assert_int_equal(buf[7], 0);
}

static void test_devmgr_encode_a5_protocols(void **state) {
    (void)state;
    dcu_698_devmgr_reset_info();

    uint8_t buf[128];
    size_t out_len = 0;
    assert_int_equal(dcu_698_devmgr_encode_attr(5, buf, sizeof(buf), &out_len), 0);

    /* array(2): "DL/T698.45"(10) + "DL/T645-2007"(12) => 2 + (2+10) + (2+12) = 28 */
    assert_int_equal(out_len, 28);
    assert_int_equal(buf[0], DLT698_TAG_ARRAY);
    assert_int_equal(buf[1], 2);
    assert_int_equal(buf[2], DLT698_TAG_VISIBLE_STRING);
    assert_int_equal(buf[3], 10);
    assert_memory_equal(buf + 4, "DL/T698.45", 10);
    assert_int_equal(buf[14], DLT698_TAG_VISIBLE_STRING);
    assert_int_equal(buf[15], 12);
    assert_memory_equal(buf + 16, "DL/T645-2007", 12);
}

static void test_devmgr_encode_cap_boundaries_and_unknown_attr(void **state) {
    (void)state;
    dcu_698_devmgr_reset_info();

    uint8_t buf[64];
    size_t out_len = 999;

    /* cap == 0 -> -1 */
    assert_int_equal(dcu_698_devmgr_encode_attr(2, buf, 0, &out_len), -1);
    assert_int_equal(out_len, 0);

    /* NULL buf -> -1 */
    assert_int_equal(dcu_698_devmgr_encode_attr(2, NULL, sizeof(buf), &out_len), -1);

    /* cap too small for A2/A3/A4/A5 -> -1 without out-of-bounds write */
    uint8_t guard[64];
    memset(guard, 0xAA, sizeof(guard));
    assert_int_equal(dcu_698_devmgr_encode_attr(2, guard, 5, &out_len), -1);
    assert_int_equal(guard[5], 0xAA);
    memset(guard, 0xAA, sizeof(guard));
    assert_int_equal(dcu_698_devmgr_encode_attr(3, guard, 45, &out_len), -1);
    assert_int_equal(guard[45], 0xAA);
    memset(guard, 0xAA, sizeof(guard));
    assert_int_equal(dcu_698_devmgr_encode_attr(4, guard, 7, &out_len), -1);
    assert_int_equal(guard[7], 0xAA);
    memset(guard, 0xAA, sizeof(guard));
    assert_int_equal(dcu_698_devmgr_encode_attr(5, guard, 27, &out_len), -1);
    assert_int_equal(guard[27], 0xAA);

    /* Unknown attr -> EDGE_SLAVE_SRC_UNDEFINED */
    assert_int_equal(dcu_698_devmgr_encode_attr(99, buf, sizeof(buf), &out_len),
                     EDGE_SLAVE_SRC_UNDEFINED);
}

static void test_devmgr_methods_dispatch_and_honest_dar5(void **state) {
    (void)state;
    mock_devmgr_ctx_t ctx = {.next_rc = 0};
    dcu_devmgr_ops_t ops = {
        .data_init = mock_data_init,
        .event_init = mock_event_init,
        .demand_init = mock_demand_init,
    };

    /* M1/M4 must always return DAR 5 (EDGE_SLAVE_SRC_CLASS_MISMATCH) even when ops provided */
    assert_int_equal(dcu_698_devmgr_call(1, &ops, &ctx), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_devmgr_call(4, &ops, &ctx), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(ctx.data_calls, 0);
    assert_int_equal(ctx.event_calls, 0);
    assert_int_equal(ctx.demand_calls, 0);

    /* Uninjected ops -> DAR 5 */
    assert_int_equal(dcu_698_devmgr_call(3, NULL, &ctx), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_devmgr_call(5, NULL, &ctx), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_devmgr_call(6, NULL, &ctx), EDGE_SLAVE_SRC_CLASS_MISMATCH);

    dcu_devmgr_ops_t empty_ops = {0};
    assert_int_equal(dcu_698_devmgr_call(3, &empty_ops, &ctx), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_devmgr_call(5, &empty_ops, &ctx), EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_devmgr_call(6, &empty_ops, &ctx), EDGE_SLAVE_SRC_CLASS_MISMATCH);

    /* Injected ops -> exact dispatch without cross-triggering */
    assert_int_equal(dcu_698_devmgr_call(3, &ops, &ctx), 0);
    assert_int_equal(ctx.data_calls, 1);
    assert_int_equal(ctx.event_calls, 0);
    assert_int_equal(ctx.demand_calls, 0);

    assert_int_equal(dcu_698_devmgr_call(5, &ops, &ctx), 0);
    assert_int_equal(ctx.data_calls, 1);
    assert_int_equal(ctx.event_calls, 1);
    assert_int_equal(ctx.demand_calls, 0);

    assert_int_equal(dcu_698_devmgr_call(6, &ops, &ctx), 0);
    assert_int_equal(ctx.data_calls, 1);
    assert_int_equal(ctx.event_calls, 1);
    assert_int_equal(ctx.demand_calls, 1);

    /* Unknown method -> EDGE_SLAVE_SRC_UNDEFINED */
    assert_int_equal(dcu_698_devmgr_call(99, &ops, &ctx), EDGE_SLAVE_SRC_UNDEFINED);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_devmgr_encode_a2_descriptor),
        cmocka_unit_test(test_devmgr_encode_a3_version_info),
        cmocka_unit_test(test_devmgr_encode_a4_prod_date),
        cmocka_unit_test(test_devmgr_encode_a5_protocols),
        cmocka_unit_test(test_devmgr_encode_cap_boundaries_and_unknown_attr),
        cmocka_unit_test(test_devmgr_methods_dispatch_and_honest_dar5),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
