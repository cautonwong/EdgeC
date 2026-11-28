#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "dlt698_codec.h"

// 1. 验证 long-unsigned (Tag 18) 编解码
static void test_axdr_long_unsigned(void** state) {
    (void)state;
    uint8_t buf[32];
    uint16_t offset = 0;
    uint16_t val = 2200;  // 220.0V (电压 long-unsigned)

    // 编码
    int ret = dlt698_encode_long_unsigned(buf, sizeof(buf), &offset, val);
    assert_int_equal(ret, 0);
    assert_int_equal(offset, 3);  // 1字节 tag + 2字节值
    assert_int_equal(buf[0], 18);
    assert_int_equal(buf[1], (2200 >> 8) & 0xFF);
    assert_int_equal(buf[2], 2200 & 0xFF);

    // 解码
    uint16_t dec_offset = 0;
    uint16_t dec_val = 0;
    ret = dlt698_decode_long_unsigned(buf, offset, &dec_offset, &dec_val);
    assert_int_equal(ret, 0);
    assert_int_equal(dec_offset, 3);
    assert_int_equal(dec_val, 2200);
}

// 2. 验证 double-long (Tag 5) 编解码
static void test_axdr_double_long(void** state) {
    (void)state;
    uint8_t buf[32];
    uint16_t offset = 0;
    int32_t val = -123456;  // 有符号 32 位

    // 编码
    int ret = dlt698_encode_double_long(buf, sizeof(buf), &offset, val);
    assert_int_equal(ret, 0);
    assert_int_equal(offset, 5);  // 1字节 tag + 4字节值
    assert_int_equal(buf[0], 5);
    assert_int_equal(buf[1], (uint8_t)((val >> 24) & 0xFF));
    assert_int_equal(buf[2], (uint8_t)((val >> 16) & 0xFF));
    assert_int_equal(buf[3], (uint8_t)((val >> 8) & 0xFF));
    assert_int_equal(buf[4], (uint8_t)(val & 0xFF));

    // 解码
    uint16_t dec_offset = 0;
    int32_t dec_val = 0;
    ret = dlt698_decode_double_long(buf, offset, &dec_offset, &dec_val);
    assert_int_equal(ret, 0);
    assert_int_equal(dec_offset, 5);
    assert_int_equal(dec_val, -123456);
}

// 3. 验证 date_time_s (Tag 28) 编解码
static void test_axdr_date_time_s(void** state) {
    (void)state;
    uint8_t buf[32];
    uint16_t offset = 0;

    uint16_t year = 2026;
    uint8_t month = 5;
    uint8_t day = 23;
    uint8_t hour = 12;
    uint8_t min = 30;
    uint8_t sec = 45;

    // 编码
    int ret =
        dlt698_encode_date_time_s(buf, sizeof(buf), &offset, year, month, day, hour, min, sec);
    assert_int_equal(ret, 0);
    assert_int_equal(offset, 8);  // 1字节 tag + 7字节值 (年2, 月1, 日1, 时1, 分1, 秒1)
    assert_int_equal(buf[0], 28);
    assert_int_equal(buf[1], (year >> 8) & 0xFF);
    assert_int_equal(buf[2], year & 0xFF);
    assert_int_equal(buf[3], month);
    assert_int_equal(buf[4], day);
    assert_int_equal(buf[5], hour);
    assert_int_equal(buf[6], min);
    assert_int_equal(buf[7], sec);

    // 解码
    uint16_t dec_offset = 0;
    uint16_t dec_year = 0;
    uint8_t dec_month = 0, dec_day = 0, dec_hour = 0, dec_min = 0, dec_sec = 0;
    ret = dlt698_decode_date_time_s(buf, offset, &dec_offset, &dec_year, &dec_month, &dec_day,
                                    &dec_hour, &dec_min, &dec_sec);
    assert_int_equal(ret, 0);
    assert_int_equal(dec_offset, 8);
    assert_int_equal(dec_year, 2026);
    assert_int_equal(dec_month, 5);
    assert_int_equal(dec_day, 23);
    assert_int_equal(dec_hour, 12);
    assert_int_equal(dec_min, 30);
    assert_int_equal(dec_sec, 45);
}

static void test_decode_rejects_truncated_tags(void** state) {
    (void)state;
    dlt698_data_t res;
    uint16_t offset;

    // 数组标签后缺少元素数量
    uint8_t arr[] = {DLT698_TAG_ARRAY};
    offset = 0;
    assert_int_equal(dlt698_decode_data(arr, sizeof(arr), &offset, &res), -1);

    // 整数标签后缺少实际值
    uint8_t i8[] = {DLT698_TAG_INTEGER};
    offset = 0;
    assert_int_equal(dlt698_decode_data(i8, sizeof(i8), &offset, &res), -1);

    // 可见字符串长度域声明 3 字节，但只给了 1 字节
    uint8_t str[] = {DLT698_TAG_VISIBLE_STRING, 3, 'a'};
    offset = 0;
    assert_int_equal(dlt698_decode_data(str, sizeof(str), &offset, &res), -1);

    // 元素数量超过 64 个静态槽
    uint8_t big[] = {DLT698_TAG_ARRAY, 65};
    offset = 0;
    assert_int_equal(dlt698_decode_data(big, sizeof(big), &offset, &res), -1);
}

static void test_decode_truncates_long_string(void** state) {
    (void)state;
    uint8_t raw[2 + 100];
    raw[0] = DLT698_TAG_VISIBLE_STRING;
    raw[1] = 100;
    for (int i = 0; i < 100; i++) raw[2 + i] = 'A';

    dlt698_data_t res;
    uint16_t offset = 0;
    assert_int_equal(dlt698_decode_data(raw, sizeof(raw), &offset, &res), 0);
    assert_int_equal(res.val.str.len, MAX_DLT698_STRING_LEN - 1);
    assert_int_equal(res.val.str.buf[res.val.str.len], '\0');
    assert_int_equal(offset, 2 + 100);
}

static void test_decode_nested_within_max_depth(void** state) {
    (void)state;
    /* 1) Mixed multi-level container: ARRAY(2) -> [STRUCTURE(2){i8(-7), ARRAY(1){u16(2200)}}, str("ok")] */
    uint8_t mixed[] = {
        DLT698_TAG_ARRAY,     2,
        DLT698_TAG_STRUCTURE, 2,
        DLT698_TAG_INTEGER,   (uint8_t)(int8_t)-7,
        DLT698_TAG_ARRAY,     1,
        DLT698_TAG_LONG_UNSIGNED, 0x08, 0x98,
        DLT698_TAG_VISIBLE_STRING, 2, 'o', 'k',
    };
    dlt698_data_t res;
    uint16_t offset = 0;
    assert_int_equal(dlt698_decode_data(mixed, sizeof(mixed), &offset, &res), 0);
    assert_int_equal(offset, sizeof(mixed));
    assert_int_equal(res.tag, DLT698_TAG_ARRAY);
    assert_int_equal(res.val.container.count, 2);
    assert_int_equal(res.val.container.items[0].tag, DLT698_TAG_STRUCTURE);
    assert_int_equal(res.val.container.items[0].val.container.count, 2);
    assert_int_equal(res.val.container.items[0].val.container.items[0].val.i8, -7);
    assert_int_equal(res.val.container.items[0].val.container.items[1].tag, DLT698_TAG_ARRAY);
    assert_int_equal(res.val.container.items[0].val.container.items[1].val.container.items[0].val.u16, 2200);
    assert_int_equal(res.val.container.items[1].tag, DLT698_TAG_VISIBLE_STRING);
    assert_string_equal((const char*)res.val.container.items[1].val.str.buf, "ok");

    /* 2) Exact boundary: 8 nested containers wrapping a scalar */
    uint8_t depth8[8 * 2 + 2];
    for (int i = 0; i < 8; i++) {
        depth8[2 * i] = (i % 2 == 0) ? DLT698_TAG_ARRAY : DLT698_TAG_STRUCTURE;
        depth8[2 * i + 1] = 1;
    }
    depth8[16] = DLT698_TAG_UNSIGNED;
    depth8[17] = 42;

    offset = 0;
    assert_int_equal(dlt698_decode_data(depth8, sizeof(depth8), &offset, &res), 0);
    assert_int_equal(offset, sizeof(depth8));
    const dlt698_data_t* cur = &res;
    for (int i = 0; i < 8; i++) {
        assert_int_equal(cur->val.container.count, 1);
        cur = &cur->val.container.items[0];
    }
    assert_int_equal(cur->tag, DLT698_TAG_UNSIGNED);
    assert_int_equal(cur->val.u8, 42);
}

static void test_decode_rejects_excessive_depth(void** state) {
    (void)state;
    dlt698_data_t res;
    uint16_t offset;

    /* 9 nested containers wrapping a scalar (> DLT698_DECODE_MAX_DEPTH = 8) */
    uint8_t depth9[9 * 2 + 2];
    for (int i = 0; i < 9; i++) {
        depth9[2 * i] = DLT698_TAG_ARRAY;
        depth9[2 * i + 1] = 1;
    }
    depth9[18] = DLT698_TAG_UNSIGNED;
    depth9[19] = 42;

    offset = 0;
    assert_int_equal(dlt698_decode_data(depth9, sizeof(depth9), &offset, &res), -1);

    /* 8 nested containers wrapping an empty container at level 9 */
    uint8_t depth9_empty[9 * 2];
    for (int i = 0; i < 8; i++) {
        depth9_empty[2 * i] = DLT698_TAG_STRUCTURE;
        depth9_empty[2 * i + 1] = 1;
    }
    depth9_empty[16] = DLT698_TAG_ARRAY;
    depth9_empty[17] = 0;

    offset = 0;
    assert_int_equal(dlt698_decode_data(depth9_empty, sizeof(depth9_empty), &offset, &res), -1);
}

static void test_decode_rejects_pool_overflow_mid_decode(void** state) {
    (void)state;
    /* Outer ARRAY(2) -> [ARRAY(33) of NULL, ARRAY(33) of NULL] => 2 + 33 + 33 = 68 > 64 */
    uint8_t buf[2 + 2 + 33 + 2 + 33];
    size_t pos = 0;
    buf[pos++] = DLT698_TAG_ARRAY;
    buf[pos++] = 2;
    for (int c = 0; c < 2; c++) {
        buf[pos++] = DLT698_TAG_ARRAY;
        buf[pos++] = 33;
        for (int i = 0; i < 33; i++) {
            buf[pos++] = DLT698_TAG_NULL;
        }
    }

    dlt698_data_t res;
    uint16_t offset = 0;
    assert_int_equal(dlt698_decode_data(buf, (uint16_t)pos, &offset, &res), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_axdr_long_unsigned),
        cmocka_unit_test(test_axdr_double_long),
        cmocka_unit_test(test_axdr_date_time_s),
        cmocka_unit_test(test_decode_rejects_truncated_tags),
        cmocka_unit_test(test_decode_truncates_long_string),
        cmocka_unit_test(test_decode_nested_within_max_depth),
        cmocka_unit_test(test_decode_rejects_excessive_depth),
        cmocka_unit_test(test_decode_rejects_pool_overflow_mid_decode),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
