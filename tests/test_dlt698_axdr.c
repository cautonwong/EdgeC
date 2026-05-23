#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include "dlt698_codec.h"

// 1. 验证 long-unsigned (Tag 18) 编解码
static void test_axdr_long_unsigned(void **state) {
    (void)state;
    uint8_t buf[32];
    uint16_t offset = 0;
    uint16_t val = 2200; // 220.0V (电压 long-unsigned)

    // 编码
    int ret = dlt698_encode_long_unsigned(buf, sizeof(buf), &offset, val);
    assert_int_equal(ret, 0);
    assert_int_equal(offset, 3); // 1字节 tag + 2字节值
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
static void test_axdr_double_long(void **state) {
    (void)state;
    uint8_t buf[32];
    uint16_t offset = 0;
    int32_t val = -123456; // 有符号 32 位

    // 编码
    int ret = dlt698_encode_double_long(buf, sizeof(buf), &offset, val);
    assert_int_equal(ret, 0);
    assert_int_equal(offset, 5); // 1字节 tag + 4字节值
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
static void test_axdr_date_time_s(void **state) {
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
    int ret = dlt698_encode_date_time_s(buf, sizeof(buf), &offset, year, month, day, hour, min, sec);
    assert_int_equal(ret, 0);
    assert_int_equal(offset, 8); // 1字节 tag + 7字节值 (年2, 月1, 日1, 时1, 分1, 秒1)
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
    ret = dlt698_decode_date_time_s(buf, offset, &dec_offset, &dec_year, &dec_month, &dec_day, &dec_hour, &dec_min, &dec_sec);
    assert_int_equal(ret, 0);
    assert_int_equal(dec_offset, 8);
    assert_int_equal(dec_year, 2026);
    assert_int_equal(dec_month, 5);
    assert_int_equal(dec_day, 23);
    assert_int_equal(dec_hour, 12);
    assert_int_equal(dec_min, 30);
    assert_int_equal(dec_sec, 45);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_axdr_long_unsigned),
        cmocka_unit_test(test_axdr_double_long),
        cmocka_unit_test(test_axdr_date_time_s),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
