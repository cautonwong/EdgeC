#include <cmocka.h>
#include <setjmp.h>
#include <sqlite3.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "dcu_698_param.h"
#include "dcu_storage.h"
#include "edge_slave_plane.h"

#define MK_OAD(oi, attr) (((uint32_t)(oi) << 16) | ((uint32_t)(attr) << 8))

typedef struct {
    int saves;
    int fail;
} fake_store_t;

static fake_store_t g_fake;

static int fake_save(void *ctx, uint16_t oi, uint8_t attr, const uint8_t *buf, size_t len) {
    fake_store_t *f = (fake_store_t *)ctx;
    (void)oi;
    (void)attr;
    (void)buf;
    (void)len;
    f->saves++;
    return f->fail ? EDGE_SLAVE_SRC_TEMPORARY : 0;
}

static int fake_load(void *ctx, uint16_t oi, uint8_t attr, uint8_t *buf, size_t cap, size_t *len) {
    fake_store_t *f = (fake_store_t *)ctx;
    (void)oi;
    (void)attr;
    (void)buf;
    (void)cap;
    (void)len;
    (void)f;
    return 1;
}

typedef struct {
    struct tm value;
    int fail_set;
    int sets;
} fake_clock_t;

static fake_clock_t g_clock;

static int fake_clock_now(void *ctx, struct tm *out) {
    fake_clock_t *clock = (fake_clock_t *)ctx;
    *out = clock->value;
    return 0;
}

static int fake_clock_set(void *ctx, const struct tm *in) {
    fake_clock_t *clock = (fake_clock_t *)ctx;
    clock->sets++;
    if (clock->fail_set) return -1;
    clock->value = *in;
    return 0;
}

static void init_fake_clock(void) {
    g_clock = (fake_clock_t){
        .value = {
            .tm_year = 124, .tm_mon = 4, .tm_mday = 6, .tm_hour = 7, .tm_min = 8, .tm_sec = 9}};
    static const dcu_clock_port_t clock = {
        .now = fake_clock_now, .set = fake_clock_set, .ctx = &g_clock};
    dcu_698_param_set_clock(&clock);
}

static void init_fake_store(int fail) {
    g_fake = (fake_store_t){.saves = 0, .fail = fail};
    init_fake_clock();
    dcu_param_store_t store = {.ctx = &g_fake, .save = fake_save, .load = fake_load};
    dcu_698_param_init(&store);
}

static uint32_t mk_oad(uint16_t oi, uint8_t attr) {
    return ((uint32_t)oi << 16) | ((uint32_t)attr << 8);
}

static void assert_encode(uint32_t oad, const uint8_t *want, size_t want_len) {
    uint8_t buf[64];
    size_t len = 0;
    assert_int_equal(dcu_698_param_encode_axdr(oad, buf, sizeof(buf), &len), 0);
    assert_int_equal(len, want_len);
    assert_memory_equal(buf, want, want_len);
}

static void round_trip(uint32_t oad) {
    uint8_t first[64];
    size_t first_len = 0;
    assert_int_equal(dcu_698_param_encode_axdr(oad, first, sizeof(first), &first_len), 0);
    assert_int_equal(dcu_698_param_accept_axdr(oad, first, first_len), 0);

    uint8_t second[64];
    size_t second_len = 0;
    assert_int_equal(dcu_698_param_encode_axdr(oad, second, sizeof(second), &second_len), 0);
    assert_int_equal(second_len, first_len);
    assert_memory_equal(second, first, first_len);
}

static const uint8_t k_default_geo[] = {
    0x02, 0x03, 0x02, 0x04, 0x16, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x02, 0x04,
    0x16, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
};

static void test_defaults_and_round_trip_every_row(void **state) {
    (void)state;
    init_fake_store(0);

    const uint8_t enum0[] = {0x16, 0x00};
    const uint8_t u5[] = {0x02, 0x05, 0x11, 0, 0x11, 0, 0x11, 0, 0x11, 0, 0x11, 0};
    const uint8_t octets0[] = {0x09, 0x00};
    const uint8_t dlu1[] = {0x06, 0x00, 0x00, 0x00, 0x01};
    const uint8_t dlu2[] = {0x02, 0x02, 0x06, 0, 0, 0, 0, 0x06, 0, 0, 0, 0};
    const uint8_t dlu3[] = {0x02, 0x03, 0x06, 0, 0, 0, 0, 0x06, 0, 0, 0, 0, 0x06, 0, 0, 0, 0};

    uint8_t dt[64];
    size_t dt_len = 0;
    assert_int_equal(dcu_698_param_encode_axdr(mk_oad(0x4000, 2), dt, sizeof(dt), &dt_len), 0);
    assert_int_equal(dt_len, 8);
    assert_int_equal(dt[0], 0x1C);
    assert_true(dt[3] >= 1 && dt[3] <= 12);
    assert_true(dt[4] >= 1 && dt[4] <= 31);
    assert_true(dt[5] <= 23);
    assert_true(dt[6] <= 59);
    assert_true(dt[7] <= 59);

    assert_encode(mk_oad(0x4000, 3), enum0, sizeof(enum0));
    assert_encode(mk_oad(0x4000, 4), u5, sizeof(u5));
    assert_encode(mk_oad(0x4001, 2), octets0, sizeof(octets0));
    assert_encode(mk_oad(0x4002, 2), octets0, sizeof(octets0));
    assert_encode(mk_oad(0x4003, 2), octets0, sizeof(octets0));
    assert_encode(mk_oad(0x4004, 2), k_default_geo, sizeof(k_default_geo));
    assert_encode(mk_oad(0x401C, 2), dlu1, sizeof(dlu1));
    assert_encode(mk_oad(0x401D, 2), dlu1, sizeof(dlu1));
    assert_encode(mk_oad(0x401E, 2), dlu2, sizeof(dlu2));
    assert_encode(mk_oad(0x401F, 2), dlu3, sizeof(dlu3));
    assert_encode(mk_oad(0x4020, 2), dlu2, sizeof(dlu2));
    assert_encode(mk_oad(0x4021, 2), dlu3, sizeof(dlu3));

    static const uint32_t rows[] = {
        MK_OAD(0x4000, 2), MK_OAD(0x4000, 3), MK_OAD(0x4000, 4), MK_OAD(0x4001, 2),
        MK_OAD(0x4002, 2), MK_OAD(0x4003, 2), MK_OAD(0x4004, 2), MK_OAD(0x401C, 2),
        MK_OAD(0x401D, 2), MK_OAD(0x401E, 2), MK_OAD(0x401F, 2), MK_OAD(0x4020, 2),
        MK_OAD(0x4021, 2),
    };
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) round_trip(rows[i]);
}

static void test_datetime_attrs_and_ranges(void **state) {
    (void)state;
    init_fake_store(0);

    const uint8_t initial[] = {0x1C, 0x07, 0xE8, 0x05, 0x06, 0x07, 0x08, 0x09};
    assert_encode(mk_oad(0x4000, 2), initial, sizeof(initial));

    const uint8_t when[] = {0x1C, 0x07, 0xE9, 0x01, 0x02, 0x03, 0x04, 0x05};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), when, sizeof(when)), 0);
    assert_int_equal(g_clock.sets, 1);
    assert_encode(mk_oad(0x4000, 2), when, sizeof(when));

    const uint8_t mode[] = {0x16, 0x02};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 3), mode, sizeof(mode)), 0);
    assert_encode(mk_oad(0x4000, 3), mode, sizeof(mode));
    const uint8_t bad_mode[] = {0x16, 0x03};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 3), bad_mode, sizeof(bad_mode)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_encode(mk_oad(0x4000, 3), mode, sizeof(mode));

    const uint8_t precise[] = {0x02, 0x05, 0x11, 1, 0x11, 2, 0x11, 3, 0x11, 4, 0x11, 5};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 4), precise, sizeof(precise)), 0);
    assert_encode(mk_oad(0x4000, 4), precise, sizeof(precise));

    const uint8_t short_struct[] = {0x02, 0x04, 0x11, 1, 0x11, 2, 0x11, 3, 0x11, 4};
    assert_int_equal(
        dcu_698_param_accept_axdr(mk_oad(0x4000, 4), short_struct, sizeof(short_struct)),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);
    const uint8_t wrong_field[] = {0x02, 0x05, 0x11, 1, 0x11, 2, 0x11, 3, 0x11, 4, 0x12, 0, 5};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 4), wrong_field, sizeof(wrong_field)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_encode(mk_oad(0x4000, 4), precise, sizeof(precise));

    const uint8_t bad_month[] = {0x1C, 0x07, 0xE8, 0x0D, 0x06, 0x07, 0x08, 0x09};
    const uint8_t bad_hour[] = {0x1C, 0x07, 0xE8, 0x05, 0x06, 0x18, 0x07, 0x08};
    const uint8_t wrong_tag[] = {0x11, 0x01};
    uint8_t trailing[sizeof(when) + 1];
    memcpy(trailing, when, sizeof(when));
    trailing[sizeof(when)] = 0x00;
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), bad_month, sizeof(bad_month)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), bad_hour, sizeof(bad_hour)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), wrong_tag, sizeof(wrong_tag)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), trailing, sizeof(trailing)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(g_clock.sets, 1);
    assert_encode(mk_oad(0x4000, 2), when, sizeof(when));

    g_clock.fail_set = 1;
    const uint8_t later[] = {0x1C, 0x07, 0xEA, 0x02, 0x03, 0x04, 0x05, 0x06};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), later, sizeof(later)),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(g_clock.sets, 2);
    assert_encode(mk_oad(0x4000, 2), when, sizeof(when));
}

static void test_octet_string_lengths(void **state) {
    (void)state;
    init_fake_store(0);

    const uint8_t empty[] = {0x09, 0x00};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), empty, sizeof(empty)), 0);
    assert_encode(mk_oad(0x4001, 2), empty, sizeof(empty));

    uint8_t max[2 + 31];
    max[0] = 0x09;
    max[1] = 31;
    for (size_t i = 0; i < 31; i++) max[2 + i] = (uint8_t)(i + 1);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), max, sizeof(max)), 0);
    assert_encode(mk_oad(0x4001, 2), max, sizeof(max));

    uint8_t too_long[2 + 32];
    too_long[0] = 0x09;
    too_long[1] = 32;
    for (size_t i = 0; i < 32; i++) too_long[2 + i] = (uint8_t)i;
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), too_long, sizeof(too_long)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);

    const uint8_t visible[] = {0x0A, 0x01, 'A'};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), visible, sizeof(visible)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_encode(mk_oad(0x4001, 2), max, sizeof(max));
}

static void test_geo_nesting_and_validation(void **state) {
    (void)state;
    init_fake_store(0);

    const uint8_t geo[] = {
        0x02, 0x03, 0x02, 0x04, 0x16, 0x01, 0x11, 0xB4, 0x11, 0x3B, 0x11, 0x3B, 0x02, 0x04,
        0x16, 0x00, 0x11, 0x5A, 0x11, 0x00, 0x11, 0x01, 0x06, 0x01, 0x02, 0x03, 0x04,
    };
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4004, 2), geo, sizeof(geo)), 0);
    assert_encode(mk_oad(0x4004, 2), geo, sizeof(geo));

    uint8_t bad[sizeof(geo)];
    memcpy(bad, geo, sizeof(bad));
    bad[5] = 0x02;
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4004, 2), bad, sizeof(bad)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    memcpy(bad, geo, sizeof(bad));
    bad[7] = 181;
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4004, 2), bad, sizeof(bad)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    memcpy(bad, geo, sizeof(bad));
    bad[17] = 91;
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4004, 2), bad, sizeof(bad)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    memcpy(bad, geo, sizeof(bad));
    bad[22] = 0x11;
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4004, 2), bad, sizeof(bad)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4004, 2), geo, sizeof(geo) - 5),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_encode(mk_oad(0x4004, 2), geo, sizeof(geo));
}

static void test_limit_structure_order(void **state) {
    (void)state;
    init_fake_store(0);

    const uint8_t dlu[] = {0x06, 0x11, 0x22, 0x33, 0x44};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x401C, 2), dlu, sizeof(dlu)), 0);
    assert_encode(mk_oad(0x401C, 2), dlu, sizeof(dlu));
    const uint8_t dlu_wrong_tag[] = {0x12, 0x11, 0x22};
    assert_int_equal(
        dcu_698_param_accept_axdr(mk_oad(0x401C, 2), dlu_wrong_tag, sizeof(dlu_wrong_tag)),
        EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_encode(mk_oad(0x401C, 2), dlu, sizeof(dlu));

    const uint8_t three[] = {0x02, 0x03, 0x06, 0, 0, 0, 1, 0x06, 0, 0, 0, 2, 0x06, 0, 0, 0, 3};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x401F, 2), three, sizeof(three)), 0);
    assert_encode(mk_oad(0x401F, 2), three, sizeof(three));
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4021, 2), three, sizeof(three)), 0);
    assert_encode(mk_oad(0x4021, 2), three, sizeof(three));

    const uint8_t two[] = {0x02, 0x02, 0x06, 0, 0, 0, 1, 0x06, 0, 0, 0, 2};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x401E, 2), two, sizeof(two)), 0);
    assert_encode(mk_oad(0x401E, 2), two, sizeof(two));
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4020, 2), two, sizeof(two)), 0);
    assert_encode(mk_oad(0x4020, 2), two, sizeof(two));

    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x401F, 2), two, sizeof(two)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_encode(mk_oad(0x401F, 2), three, sizeof(three));
}

static void test_dar_5_4_6_7(void **state) {
    (void)state;
    init_fake_store(0);

    const uint8_t one[] = {0x11, 0x01};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 1), one, sizeof(one)),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 1), one, sizeof(one)),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 3), one, sizeof(one)),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 5), one, sizeof(one)),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4005, 2), one, sizeof(one)),
                     EDGE_SLAVE_SRC_NOT_FOUND);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4019, 2), one, sizeof(one)),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    uint8_t buf[64];
    size_t len = 0;
    assert_int_equal(dcu_698_param_encode_axdr(mk_oad(0x4000, 1), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_CLASS_MISMATCH);
    assert_int_equal(dcu_698_param_encode_axdr(mk_oad(0x4001, 3), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_UNDEFINED);
    assert_int_equal(dcu_698_param_encode_axdr(mk_oad(0x4005, 2), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_NOT_FOUND);

    const uint8_t visible[] = {0x0A, 0x01, 'A'};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), visible, sizeof(visible)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), one, sizeof(one)),
                     EDGE_SLAVE_SRC_TYPE_MISMATCH);
}

static void test_save_failure_leaves_value_unchanged(void **state) {
    (void)state;
    init_fake_store(1);

    const uint8_t value[] = {0x09, 0x03, 'a', 'b', 'c'};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), value, sizeof(value)),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(g_fake.saves, 1);
    const uint8_t empty[] = {0x09, 0x00};
    assert_encode(mk_oad(0x4001, 2), empty, sizeof(empty));

    const uint8_t when[] = {0x1C, 0x07, 0xE9, 0x01, 0x02, 0x03, 0x04, 0x05};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), when, sizeof(when)), 0);
    assert_encode(mk_oad(0x4000, 2), when, sizeof(when));
    assert_int_equal(g_fake.saves, 1);
    assert_int_equal(g_clock.sets, 1);

    assert_true(dcu_698_param_is_oi(0x4000));
    assert_true(dcu_698_param_is_oi(0x4021));
    assert_false(dcu_698_param_is_oi(0x4005));
    assert_true(dcu_698_param_has_attr(0x4000, 4));
    assert_false(dcu_698_param_has_attr(0x4000, 5));
}

static void test_datetime_without_clock_port_reports_temporary(void **state) {
    (void)state;
    init_fake_store(0);
    dcu_698_param_set_clock(NULL);

    const uint8_t when[] = {0x1C, 0x07, 0xE9, 0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t buf[64];
    size_t len = 0;
    assert_int_equal(dcu_698_param_encode_axdr(mk_oad(0x4000, 2), buf, sizeof(buf), &len),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), when, sizeof(when)),
                     EDGE_SLAVE_SRC_TEMPORARY);
    assert_int_equal(g_clock.sets, 0);
}

static int store_save(void *ctx, uint16_t oi, uint8_t attr, const uint8_t *buf, size_t len) {
    return dcu_storage_param_save((dcu_storage_t *)ctx, oi, attr, buf, len, "dlt698");
}

static int store_load(void *ctx, uint16_t oi, uint8_t attr, uint8_t *buf, size_t cap, size_t *len) {
    return dcu_storage_param_load((dcu_storage_t *)ctx, oi, attr, buf, cap, len);
}

static int query_int(sqlite3 *db, const char *sql) {
    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(db, sql, -1, &stmt, NULL), SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    int v = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return v;
}

static void test_persistence_reload_and_audit(void **state) {
    (void)state;
    char path[64];
    snprintf(path, sizeof(path), "/tmp/dcu_param_%ld.db", (long)getpid());
    char wal[80];
    snprintf(wal, sizeof(wal), "%s-wal", path);
    char shm[80];
    snprintf(shm, sizeof(shm), "%s-shm", path);
    unlink(path);
    unlink(wal);
    unlink(shm);

    uv_loop_t *loop = uv_default_loop();
    dcu_storage_t storage;
    assert_int_equal(dcu_storage_init(&storage, loop, path), 0);
    dcu_param_store_t port = {.ctx = &storage, .save = store_save, .load = store_load};
    init_fake_clock();
    dcu_698_param_init(&port);

    const uint8_t abc[] = {0x09, 0x03, 'a', 'b', 'c'};
    const uint8_t de[] = {0x09, 0x02, 'd', 'e'};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), abc, sizeof(abc)), 0);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4001, 2), de, sizeof(de)), 0);

    sqlite3_stmt *stmt = NULL;
    assert_int_equal(sqlite3_prepare_v2(storage.db,
                                        "SELECT old_value, new_value, source, attr FROM "
                                        "dcu_param_audit WHERE oi = 0x4001 "
                                        "ORDER BY id DESC LIMIT 1;",
                                        -1, &stmt, NULL),
                     SQLITE_OK);
    assert_int_equal(sqlite3_step(stmt), SQLITE_ROW);
    assert_int_equal(sqlite3_column_bytes(stmt, 0), (int)sizeof(abc));
    assert_memory_equal(sqlite3_column_blob(stmt, 0), abc, sizeof(abc));
    assert_int_equal(sqlite3_column_bytes(stmt, 1), (int)sizeof(de));
    assert_memory_equal(sqlite3_column_blob(stmt, 1), de, sizeof(de));
    assert_string_equal((const char *)sqlite3_column_text(stmt, 2), "dlt698");
    assert_int_equal(sqlite3_column_int(stmt, 3), 2);
    sqlite3_finalize(stmt);

    const uint8_t mode[] = {0x16, 0x01};
    const uint8_t precise[] = {0x02, 0x05, 0x11, 1, 0x11, 2, 0x11, 3, 0x11, 4, 0x11, 5};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 3), mode, sizeof(mode)), 0);
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 4), precise, sizeof(precise)), 0);
    assert_int_equal(query_int(storage.db,
                               "SELECT count(*) FROM dcu_params "
                               "WHERE oi = 0x4000 AND attr IN (3, 4);"),
                     2);
    assert_int_equal(
        query_int(storage.db, "SELECT count(*) FROM dcu_param_audit WHERE oi = 0x4000;"), 2);

    const uint8_t when[] = {0x1C, 0x07, 0xE9, 0x01, 0x02, 0x03, 0x04, 0x05};
    assert_int_equal(dcu_698_param_accept_axdr(mk_oad(0x4000, 2), when, sizeof(when)), 0);
    assert_int_equal(query_int(storage.db,
                               "SELECT count(*) FROM dcu_params "
                               "WHERE oi = 0x4000 AND attr = 2;"),
                     0);
    assert_int_equal(
        query_int(storage.db, "SELECT count(*) FROM dcu_param_audit WHERE oi = 0x4000;"), 2);

    dcu_698_param_reset();
    dcu_698_param_init(&port);
    assert_encode(mk_oad(0x4001, 2), de, sizeof(de));
    assert_encode(mk_oad(0x4000, 3), mode, sizeof(mode));
    assert_encode(mk_oad(0x4000, 4), precise, sizeof(precise));
    assert_encode(mk_oad(0x4000, 2), when, sizeof(when));

    dcu_storage_destroy(&storage);
    dcu_698_param_reset();

    dcu_storage_t storage2;
    assert_int_equal(dcu_storage_init(&storage2, loop, path), 0);
    dcu_param_store_t port2 = {.ctx = &storage2, .save = store_save, .load = store_load};
    dcu_698_param_init(&port2);
    assert_encode(mk_oad(0x4001, 2), de, sizeof(de));
    assert_encode(mk_oad(0x4000, 3), mode, sizeof(mode));
    assert_encode(mk_oad(0x4000, 4), precise, sizeof(precise));
    dcu_storage_destroy(&storage2);
    dcu_698_param_reset();

    unlink(path);
    unlink(wal);
    unlink(shm);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_defaults_and_round_trip_every_row),
        cmocka_unit_test(test_datetime_attrs_and_ranges),
        cmocka_unit_test(test_octet_string_lengths),
        cmocka_unit_test(test_geo_nesting_and_validation),
        cmocka_unit_test(test_limit_structure_order),
        cmocka_unit_test(test_dar_5_4_6_7),
        cmocka_unit_test(test_save_failure_leaves_value_unchanged),
        cmocka_unit_test(test_datetime_without_clock_port_reports_temporary),
        cmocka_unit_test(test_persistence_reload_and_audit),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
