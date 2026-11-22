#include <cmocka.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "dcu_698_record.h"
#include "dcu_inventory.h"
#include "dcu_router.h"
#include "dcu_storage.h"

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
    assert_int_equal(dcu_storage_write_freeze(&g_storage, addr, OAD_DAILY_FREEZE, T1, OAD_VOLTAGE,
                                              231.7),
                     0);
    assert_int_equal(dcu_storage_write_freeze(&g_storage, addr, OAD_DAILY_FREEZE, T1, OAD_CURRENT,
                                              4.951),
                     0);
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

static size_t mk_range(uint8_t *sel, const uint8_t *start, const uint8_t *end,
                       const uint32_t *cols, size_t n) {
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
    const uint8_t want0[] = {0x1C, 0x07, 0xEA, 0x0B, 0x16, 0x00, 0x00, 0x00,
                             0x06, 0x00, 0x12, 0xD6, 0x44};
    assert_int_equal(row_len, sizeof(want0));
    assert_memory_equal(row, want0, sizeof(want0));

    assert_int_equal(dcu_698_record_row(&g_ctx, OAD_DAILY_FREEZE, 1, row, sizeof(row), &row_len),
                     0);
    const uint8_t want1[] = {0x1C, 0x07, 0xEA, 0x0B, 0x17, 0x00, 0x00, 0x00,
                             0x06, 0x00, 0x12, 0xD6, 0xA8};
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
    /* 表48 date_time：weekday 在 hour 前；用 07 当日、00 时区分两种解析。 */
    const uint8_t sel[] = {0x01, 0x20, 0x21, 0x02, 0x00, 0x19, 0x07, 0xEA, 0x0B, 0x16,
                           0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x21,
                           0x02, 0x00};
    size_t rows = 0;
    assert_int_equal(dcu_698_record_begin(&g_ctx, OAD_DAILY_FREEZE, sel, sizeof(sel), &rows), 0);
    assert_int_equal(rows, 1);

    uint8_t row[32];
    size_t row_len = 0;
    assert_int_equal(dcu_698_record_row(&g_ctx, OAD_DAILY_FREEZE, 0, row, sizeof(row), &row_len),
                     0);
    assert_int_equal(row_len, 8);
    assert_int_equal(row[0], 0x1C);
    assert_int_equal(row[5], 0x00); /* hour */
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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_record_all_rows_two_columns, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_range_is_half_open, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_empty_range, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_slot_selects_meter, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_selector1_date_time_layout, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_rcsd_zero_means_all_columns, setup, teardown),
        cmocka_unit_test_setup_teardown(test_record_missing_column_fails_row, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
