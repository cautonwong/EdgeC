#include <cmocka.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <uv.h>

#include "dcu_698_record.h"
#include "dcu_inventory.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "dlt698_codec.h"
#include "edge_driver.h"
#include "edge_slave_plane.h"
#include "net_buf.h"

extern const edge_driver_t dlt698_driver;

#define T1 1795305600ULL /* 2026-11-22T00:00:00Z */
#define T2 1795392000ULL /* 2026-11-23T00:00:00Z */

static dcu_storage_t g_storage;
static dcu_inventory_t g_inv;
static dcu_698_record_ctx_t g_ctx;
static char g_db_path[] = "/tmp/dcu-record-XXXXXX";
static int g_fd = -1;

static int setup(void **state) {
    (void)state;
    strcpy(g_db_path, "/tmp/dcu-record-XXXXXX");
    g_fd = mkstemp(g_db_path);
    assert_true(g_fd >= 0);
    close(g_fd);
    assert_int_equal(dcu_storage_init(&g_storage, uv_default_loop(), g_db_path), 0);
    memset(&g_inv, 0, sizeof(g_inv));
    assert_int_equal(dcu_inventory_init(&g_inv), 0);
    assert_int_equal(dcu_inventory_add(&g_inv, "000000000001", "DLT645", 2400, 22), 0);
    assert_int_equal(dcu_inventory_add(&g_inv, "000000000002", "DLT645", 2400, 22), 0);

    const char *addr = "000000000001";
    assert_int_equal(dcu_storage_write_freeze(&g_storage, addr, OAD_DAILY_FREEZE, T1,
                                              OAD_ACTIVE_ENERGY, 12345.0),
                     0);
    assert_int_equal(
        dcu_storage_write_freeze(&g_storage, addr, OAD_DAILY_FREEZE, T1, OAD_VOLTAGE, 231.7), 0);
    assert_int_equal(
        dcu_storage_write_freeze(&g_storage, addr, OAD_DAILY_FREEZE, T1, OAD_CURRENT, 4.951), 0);
    assert_int_equal(dcu_storage_write_freeze(&g_storage, addr, OAD_DAILY_FREEZE, T2,
                                              OAD_ACTIVE_ENERGY, 12346.0),
                     0);
    assert_int_equal(dcu_storage_write_freeze(&g_storage, "000000000002", OAD_DAILY_FREEZE, T1,
                                              OAD_ACTIVE_ENERGY, 999.0),
                     0);

    dcu_698_record_init(&g_ctx, &g_storage, &g_inv);
    return 0;
}

static int teardown(void **state) {
    (void)state;
    dcu_698_record_end(&g_ctx, 0);
    dcu_inventory_destroy(&g_inv);
    dcu_storage_destroy(&g_storage);
    unlink(g_db_path);
    char wal[64];
    snprintf(wal, sizeof(wal), "%s-wal", g_db_path);
    unlink(wal);
    snprintf(wal, sizeof(wal), "%s-shm", g_db_path);
    unlink(wal);
    return 0;
}

static void put_dts(uint8_t *p, int year, int month, int day) {
    p[0] = 0x1C;
    p[1] = (uint8_t)(year >> 8);
    p[2] = (uint8_t)year;
    p[3] = (uint8_t)month;
    p[4] = (uint8_t)day;
    p[5] = 0;
    p[6] = 0;
    p[7] = 0;
}

static size_t mk_no_sel(uint8_t *sel, const uint32_t *cols, size_t n) {
    size_t i = 0;
    sel[i++] = 0x00;
    sel[i++] = (uint8_t)n;
    for (size_t c = 0; c < n; c++) {
        sel[i++] = 0x00;
        sel[i++] = (uint8_t)(cols[c] >> 24);
        sel[i++] = (uint8_t)(cols[c] >> 16);
        sel[i++] = (uint8_t)(cols[c] >> 8);
        sel[i++] = (uint8_t)cols[c];
    }
    return i;
}

static size_t mk_range(uint8_t *sel, const uint8_t *start, const uint8_t *end, const uint32_t *cols,
                       size_t n) {
    size_t i = 0;
    sel[i++] = 0x02;
    sel[i++] = 0x20;
    sel[i++] = 0x21;
    sel[i++] = 0x02;
    sel[i++] = 0x00;
    memcpy(sel + i, start, 8);
    i += 8;
    memcpy(sel + i, end, 8);
    i += 8;
    sel[i++] = 0x00; /* no interval */
    sel[i++] = (uint8_t)n;
    for (size_t c = 0; c < n; c++) {
        sel[i++] = 0x00;
        sel[i++] = (uint8_t)(cols[c] >> 24);
        sel[i++] = (uint8_t)(cols[c] >> 16);
        sel[i++] = (uint8_t)(cols[c] >> 8);
        sel[i++] = (uint8_t)cols[c];
    }
    return i;
}

static void test_record_all_rows_two_columns(void **state) {
    (void)state;
    const uint32_t cols[] = {0x20210200U, OAD_ACTIVE_ENERGY};
    uint8_t sel[64];
    size_t sel_len = mk_no_sel(sel, cols, 2);

    size_t rows = 0;
    assert_int_equal(dcu_698_record_begin(&g_ctx, OAD_DAILY_FREEZE, sel, sel_len, &rows), 0);
    assert_int_equal(rows, 2);

    uint8_t row[64];
    size_t row_len = 0;
    assert_int_equal(dcu_698_record_row(&g_ctx, OAD_DAILY_FREEZE, 0, row, sizeof(row), &row_len),
                     0);
    const uint8_t want0[] = {0x1C, 0x07, 0xEA, 0x0B, 0x16, 0x08, 0x00,
                             0x00, 0x06, 0x00, 0x12, 0xD6, 0x44};
    assert_int_equal(row_len, sizeof(want0));
    assert_memory_equal(row, want0, sizeof(want0));

    assert_int_equal(dcu_698_record_row(&g_ctx, OAD_DAILY_FREEZE, 1, row, sizeof(row), &row_len),
                     0);
    const uint8_t want1[] = {0x1C, 0x07, 0xEA, 0x0B, 0x17, 0x08, 0x00,
                             0x00, 0x06, 0x00, 0x12, 0xD6, 0xA8};
    assert_int_equal(row_len, sizeof(want1));
    assert_memory_equal(row, want1, sizeof(want1));
    assert_int_equal(dcu_698_record_end(&g_ctx, OAD_DAILY_FREEZE), 0);
}

static void test_record_range_is_half_open(void **state) {
    (void)state;
    const uint32_t cols[] = {0x20210200U};
    uint8_t start[8], end[8];
    put_dts(start, 2026, 11, 22);
    put_dts(end, 2026, 11, 23);
    uint8_t sel[64];
    size_t sel_len = mk_range(sel, start, end, cols, 1);

    size_t rows = 0;
    assert_int_equal(dcu_698_record_begin(&g_ctx, OAD_DAILY_FREEZE, sel, sel_len, &rows), 0);
    assert_int_equal(rows, 1);
    dcu_698_record_end(&g_ctx, OAD_DAILY_FREEZE);
}

static void test_record_empty_range(void **state) {
    (void)state;
    const uint32_t cols[] = {0x20210200U};
    uint8_t start[8], end[8];
    put_dts(start, 2027, 1, 1);
    put_dts(end, 2027, 1, 2);
    uint8_t sel[64];
    size_t sel_len = mk_range(sel, start, end, cols, 1);

    size_t rows = 99;
    assert_int_equal(dcu_698_record_begin(&g_ctx, OAD_DAILY_FREEZE, sel, sel_len, &rows), 0);
    assert_int_equal(rows, 0);
    dcu_698_record_end(&g_ctx, OAD_DAILY_FREEZE);
}

static void test_record_slot_selects_meter(void **state) {
    (void)state;
    const uint32_t cols[] = {0x20210200U, OAD_ACTIVE_ENERGY};
    uint8_t sel[64];
    size_t sel_len = mk_no_sel(sel, cols, 2);

    size_t rows = 0;
    assert_int_equal(dcu_698_record_begin(&g_ctx, 0x50040201U, sel, sel_len, &rows), 0);
    assert_int_equal(rows, 1);
    uint8_t row[64];
    size_t row_len = 0;
    assert_int_equal(dcu_698_record_row(&g_ctx, 0x50040201U, 0, row, sizeof(row), &row_len), 0);
    assert_int_equal(row[8], 0x06);
    assert_int_equal(row[9], 0x00);
    assert_int_equal(row[10], 0x01);
    assert_int_equal(row[11], 0x86);
    assert_int_equal(row[12], 0x3C); /* 999.00 kWh ×100 = 99900 */
    dcu_698_record_end(&g_ctx, 0x50040201U);

    assert_int_equal(dcu_698_record_begin(&g_ctx, 0x500402FAU, sel, sel_len, &rows),
                     EDGE_SLAVE_SRC_NOT_FOUND);
}

static void test_record_selector1_date_time_layout(void **state) {
    (void)state;
    /* 表48 date_time：weekday 在 hour 前；weekday=07、hour=08 区分两种解析。
     * T1 是 2026-11-22T00:00Z，CST-8 下本地 08:00，Selector1 精确匹配该时刻。 */
    const uint8_t sel[] = {0x01, 0x20, 0x21, 0x02, 0x00, 0x19, 0x07, 0xEA, 0x0B, 0x16, 0x07,
                           0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x21, 0x02, 0x00};
    size_t rows = 0;
    assert_int_equal(dcu_698_record_begin(&g_ctx, OAD_DAILY_FREEZE, sel, sizeof(sel), &rows), 0);
    assert_int_equal(rows, 1);

    uint8_t row[32];
    size_t row_len = 0;
    assert_int_equal(dcu_698_record_row(&g_ctx, OAD_DAILY_FREEZE, 0, row, sizeof(row), &row_len),
                     0);
    assert_int_equal(row_len, 8);
    assert_int_equal(row[0], 0x1C);
    assert_int_equal(row[5], 0x08);
    dcu_698_record_end(&g_ctx, OAD_DAILY_FREEZE);
}

static void test_record_rcsd_zero_means_all_columns(void **state) {
    (void)state;
    const uint8_t sel[] = {0x00, 0x00};
    size_t rows = 0;
    assert_int_equal(dcu_698_record_begin(&g_ctx, OAD_DAILY_FREEZE, sel, sizeof(sel), &rows), 0);
    assert_int_equal(rows, 2);

    uint8_t row[64];
    size_t row_len = 0;
    assert_int_equal(dcu_698_record_row(&g_ctx, OAD_DAILY_FREEZE, 0, row, sizeof(row), &row_len),
                     0);
    /* 冻结时间 + 本页三个源（电能/电压/电流），三列类型都必须出现。 */
    assert_int_equal(row_len, 21);
    bool has_energy = false, has_voltage = false, has_current = false;
    for (size_t i = 0; i < row_len; i++) {
        if (row[i] == 0x06 && i + 1 < row_len) has_energy = true;
        if (row[i] == 0x12) has_voltage = true;
        if (row[i] == 0x05) has_current = true;
    }
    assert_true(has_energy && has_voltage && has_current);
    dcu_698_record_end(&g_ctx, OAD_DAILY_FREEZE);
}

static void test_record_missing_column_fails_row(void **state) {
    (void)state;
    const uint32_t cols[] = {0x20210200U, OAD_ACTIVE_POWER_TOTAL};
    uint8_t sel[64];
    size_t sel_len = mk_no_sel(sel, cols, 2);

    size_t rows = 0;
    assert_int_equal(dcu_698_record_begin(&g_ctx, OAD_DAILY_FREEZE, sel, sel_len, &rows), 0);
    assert_int_equal(rows, 2);
    uint8_t row[64];
    size_t row_len = 0;
    assert_int_equal(dcu_698_record_row(&g_ctx, OAD_DAILY_FREEZE, 0, row, sizeof(row), &row_len),
                     EDGE_SLAVE_SRC_NOT_FOUND);
    dcu_698_record_end(&g_ctx, OAD_DAILY_FREEZE);
}

#define OVERFLOW_CAP 4096
#define OVERFLOW_CANARY 2048
static uint8_t g_slave_backing[OVERFLOW_CAP + OVERFLOW_CANARY];
static size_t mk_acl_read(uint8_t *buf, uint16_t oi) {
    size_t o = 0;
    buf[o++] = 0x01;
    buf[o++] = 1;
    buf[o++] = 0x02;
    buf[o++] = 2;
    buf[o++] = 0x50;
    buf[o++] = (uint8_t)(oi >> 8);
    buf[o++] = (uint8_t)oi;
    buf[o++] = 0x02;
    buf[o++] = 2;
    buf[o++] = 0x01;
    buf[o++] = 1;
    buf[o++] = 0x02;
    buf[o++] = 2;
    buf[o++] = 0x11;
    buf[o++] = 2;
    buf[o++] = 0x16;
    buf[o++] = 1;
    buf[o++] = 0x01;
    buf[o++] = 0;
    return o;
}
static int fake_object_list(void *ctx, uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    (void)ctx;
    if (oad != 0x44000200u) return EDGE_SLAVE_SRC_NOT_FOUND;
    uint8_t acl[32];
    size_t n = mk_acl_read(acl, 0x5004);
    if (n > cap) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    memcpy(buf, acl, n);
    *len = n;
    return 0;
}
static size_t mk_get_record_frame(uint8_t *out, const uint8_t *apdu, size_t apdu_len) {
    size_t n = 0;
    out[n++] = DLT698_HEAD;
    size_t len_off = n;
    n += 2;
    out[n++] = 0x43;
    out[n++] = 0x00;
    out[n++] = 0x01;
    out[n++] = 0x10;
    uint16_t l = (uint16_t)(9 + apdu_len + 3 - 2);
    out[len_off] = (uint8_t)(l & 0xFF);
    out[len_off + 1] = (uint8_t)(l >> 8);
    uint16_t hcs = dlt698_calculate_fcs(out + 1, n - 1);
    out[n++] = (uint8_t)(hcs & 0xFF);
    out[n++] = (uint8_t)(hcs >> 8);
    memcpy(out + n, apdu, apdu_len);
    n += apdu_len;
    uint16_t fcs = dlt698_calculate_fcs(out + 1, n - 1);
    out[n++] = (uint8_t)(fcs & 0xFF);
    out[n++] = (uint8_t)(fcs >> 8);
    out[n++] = DLT698_END;
    return n;
}
static void test_record_page_overflow_returns_dar2(void **state) {
    (void)state;
    for (size_t i = 0; i < DCU_RECORD_MAX_ROWS; i++) {
        uint64_t t = T1 + (uint64_t)i * 86400ULL;
        assert_int_equal(dcu_storage_write_freeze(&g_storage, "000000000001", OAD_DAILY_FREEZE, t,
                                                  OAD_ACTIVE_ENERGY, 1000.0 + (double)i),
                         0);
        assert_int_equal(
            dcu_storage_write_freeze(&g_storage, "000000000001", OAD_DAILY_FREEZE, t, OAD_VOLTAGE,
                                     220.0),
            0);
        assert_int_equal(
            dcu_storage_write_freeze(&g_storage, "000000000001", OAD_DAILY_FREEZE, t, OAD_CURRENT,
                                     5.0),
            0);
    }
    const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00};
    uint8_t apdu[3 + sizeof(req)];
    apdu[0] = DLT698_SVC_GET_REQ;
    apdu[1] = DLT698_CHOICE_GET_RECORD;
    apdu[2] = 0x05;
    memcpy(apdu + 3, req, sizeof(req));
    uint8_t frame[64];
    size_t flen = mk_get_record_frame(frame, apdu, sizeof(apdu));

    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = fake_object_list;
    src.record.begin = dcu_698_record_begin;
    src.record.row = dcu_698_record_row;
    src.record.end = dcu_698_record_end;

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    memset(g_slave_backing, 0xAA, sizeof(g_slave_backing));
    net_buf_t out;
    memset(&out, 0, sizeof(out));
    net_buf_simple_init_with_data(&out.b, g_slave_backing, OVERFLOW_CAP);
    size_t consumed = 0;
    assert_int_equal(dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, &g_ctx,
                                                      &out, &consumed),
                     0);
    assert_true(out.b.len <= OVERFLOW_CAP);

    dlt698_addr_t sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, out.b.data, out.b.len);
    nb.len = out.b.len;
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload,
                                            &payload_len),
                     0);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(choice, DLT698_CHOICE_GET_RECORD);
    /* payload = OAD | RCSD | 结果选择 | DAR | FollowReport | 时间标签（PIID 已取出） */
    assert_int_equal(payload_len, 9);
    assert_int_equal(payload[5], DLT698_RECORD_RESULT_DAR);
    assert_int_equal(payload[6], DLT698_DAR_TEMPORARY_FAILURE);

    for (size_t i = OVERFLOW_CAP; i < sizeof(g_slave_backing); i++) {
        assert_int_equal(g_slave_backing[i], 0xAA);
    }
}
static size_t g_fake_record_total;
static size_t g_fake_record_row_len;
static int fake_record_begin(void *ctx, uint32_t oad, const uint8_t *sel, size_t sel_len,
                             size_t *total) {
    (void)ctx;
    (void)oad;
    (void)sel;
    (void)sel_len;
    *total = g_fake_record_total;
    return 0;
}
static int fake_record_row(void *ctx, uint32_t oad, size_t index, uint8_t *row, size_t cap,
                           size_t *row_len) {
    (void)ctx;
    (void)oad;
    (void)index;
    if (g_fake_record_row_len > cap) return EDGE_SLAVE_SRC_TEMPORARY;
    memset(row, 0x41, g_fake_record_row_len);
    *row_len = g_fake_record_row_len;
    return 0;
}
static int fake_record_end(void *ctx, uint32_t oad) {
    (void)ctx;
    (void)oad;
    return 0;
}
static void assert_trailer_boundary_dar2(size_t total, size_t row_len) {
    const uint8_t req[] = {0x50, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00};
    uint8_t apdu[3 + sizeof(req)];
    apdu[0] = DLT698_SVC_GET_REQ;
    apdu[1] = DLT698_CHOICE_GET_RECORD;
    apdu[2] = 0x05;
    memcpy(apdu + 3, req, sizeof(req));
    uint8_t frame[64];
    size_t flen = mk_get_record_frame(frame, apdu, sizeof(apdu));
    edge_slave_source_ops_t src;
    memset(&src, 0, sizeof(src));
    src.object_list = fake_object_list;
    src.record.begin = fake_record_begin;
    src.record.row = fake_record_row;
    src.record.end = fake_record_end;
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    static uint8_t session[4096];
    memset(session, 0, sizeof(session));
    g_fake_record_total = total;
    g_fake_record_row_len = row_len;
    memset(g_slave_backing, 0xAA, sizeof(g_slave_backing));
    net_buf_t out;
    memset(&out, 0, sizeof(out));
    net_buf_simple_init_with_data(&out.b, g_slave_backing, OVERFLOW_CAP);
    size_t consumed = 0;
    assert_int_equal(dlt698_driver.req_slave_exchange(priv, session, frame, flen, &src, NULL, &out,
                                                      &consumed),
                     0);
    assert_true(out.b.len <= OVERFLOW_CAP);
    dlt698_addr_t sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, out.b.data, out.b.len);
    nb.len = out.b.len;
    assert_int_equal(dlt698_decode_frame_ex(&nb, &sa, &svc, &choice, &piid, &payload,
                                            &payload_len),
                     0);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(payload_len, 9);
    assert_int_equal(payload[5], DLT698_RECORD_RESULT_DAR);
    assert_int_equal(payload[6], DLT698_DAR_TEMPORARY_FAILURE);
    for (size_t i = OVERFLOW_CAP; i < sizeof(g_slave_backing); i++) {
        assert_int_equal(g_slave_backing[i], 0xAA);
    }
}
static void test_record_page_trailer_boundary_returns_dar2(void **state) {
    (void)state;
    assert_trailer_boundary_dar2(194, 21);
    assert_trailer_boundary_dar2(163, 25);
    assert_trailer_boundary_dar2(151, 27);
}
/* date_time_s 是设备本地民用时间：固定东八区，冻结记录字节与区间窗口才可确定。 */
static char g_saved_tz[128];
static int g_had_tz;

static int setup_timezone(void **state) {
    (void)state;
    const char *tz = getenv("TZ");
    g_had_tz = tz != NULL;
    if (g_had_tz) snprintf(g_saved_tz, sizeof(g_saved_tz), "%s", tz);
    setenv("TZ", "CST-8", 1);
    tzset();
    return 0;
}

static int teardown_timezone(void **state) {
    (void)state;
    if (g_had_tz) {
        setenv("TZ", g_saved_tz, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_record_all_rows_two_columns, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_range_is_half_open, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_empty_range, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_slot_selects_meter, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_selector1_date_time_layout, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_rcsd_zero_means_all_columns, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_missing_column_fails_row, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_page_overflow_returns_dar2, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_page_trailer_boundary_returns_dar2, setup,
                                        teardown),
    };
    return cmocka_run_group_tests(tests, setup_timezone, teardown_timezone);
}
