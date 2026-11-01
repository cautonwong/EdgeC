/**
 * @file test_dlt645_control.c
 * @brief DLT645 跳合闸/报警/保电请求帧（控制码 1CH）的字节级测试。
 *
 * 依据是 `docs/DLT_645-2007_多功能电能表通信协议2010.pdf` 7.13 节：
 *
 *   68H A0…A5 68H 1CH L PA P0P1P2 C0…C3 N1…Nm CS 16H
 *
 *   - L = 8 + m（密码 4 + 操作者代码 4 + 命令数据 m）
 *   - N1 是命令类型：1AH 跳闸、1BH 合闸允许、2AH 报警、2BH 报警解除、3AH 保电、3BH 保电解除
 *   - N2 保留；N3～N8 是命令有效截止时间，格式 ssmmhhDDMMYY
 *   - 数据域每字节 +33H（5.2.5）
 *
 * 这些字节**不能靠"跑通"来证明**：帧自洽但字段错位时，从站要么静默丢弃、要么把
 * 密码当命令。所以这里逐字节对照标准。
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <cmocka.h>

#include "dlt645_codec.h"
#include "net_buf.h"

static net_buf_pool_t *g_pool;

static int setup(void **state) {
    (void)state;
    g_pool = net_buf_pool_new(8, 256, 0);
    return g_pool ? 0 : -1;
}

static int teardown(void **state) {
    (void)state;
    if (g_pool) net_buf_pool_destroy(g_pool);
    return 0;
}

static void assert_cs(const uint8_t *f, size_t cs_idx) {
    uint8_t want = dlt645_calculate_cs(f, cs_idx);
    if (want != f[cs_idx]) {
        printf("CS 不符：算得 %02X，帧里 %02X\n", want, f[cs_idx]);
    }
    assert_int_equal(f[cs_idx], want);
}

/** 跳闸帧：地址 000000000001、密码 02000000（明文密级）、操作者 00000000、无截止时间。 */
static void test_trip_frame_layout(void **state) {
    (void)state;
    const uint8_t addr[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00}; /* 低字节先 */
    const uint8_t pwd[4] = {0x02, 0x00, 0x00, 0x00};
    const uint8_t op[4] = {0x00, 0x00, 0x00, 0x00};

    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    assert_int_equal(
        dlt645_build_ctrl_request(addr, DLT645_CMD_TRIP, NULL, pwd, op, out), 0);

    const uint8_t *b = out->b.data;
    const uint8_t *f = b + 4; /* 跳过 FE FE FE FE */

    assert_int_equal(out->b.len, 4 + 28);
    assert_int_equal(f[0], 0x68);
    assert_memory_equal(f + 1, addr, 6);
    assert_int_equal(f[7], 0x68);
    assert_int_equal(f[8], DLT645_CTRL_TRIP_CONTROL); /* 1CH */
    assert_int_equal(f[9], 0x10);                     /* L = 8 + 8 */

    /* 数据域 +33H：密码 02 00 00 00 → 35 33 33 33；操作者全 0 → 33 33 33 33；
     * N1=1A → 4D；N2=00 → 33；N3..N8 未设截止时间 → 33×6 */
    const uint8_t want_data[16] = {0x35, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33,
                                   0x4D, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
    assert_memory_equal(f + 10, want_data, sizeof(want_data));

    assert_cs(f, 26);
    assert_int_equal(f[27], 0x16);

    net_buf_unref(out);
}

/** 合闸允许：N1 必须是 1BH（+33H 后是 4EH），与跳闸只差这一个字节。 */
static void test_allow_close_differs_only_in_n1(void **state) {
    (void)state;
    const uint8_t addr[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t pwd[4] = {0x02, 0x00, 0x00, 0x00};
    const uint8_t op[4] = {0x00, 0x00, 0x00, 0x00};

    net_buf_t *a = net_buf_alloc(g_pool, 0);
    net_buf_t *b = net_buf_alloc(g_pool, 0);
    assert_non_null(a);
    assert_non_null(b);
    assert_int_equal(dlt645_build_ctrl_request(addr, DLT645_CMD_TRIP, NULL, pwd, op, a), 0);
    assert_int_equal(dlt645_build_ctrl_request(addr, DLT645_CMD_ALLOW_CLOSE, NULL, pwd, op, b),
                     0);

    assert_int_equal(a->b.data[4 + 18], 0x4D); /* 1A + 33 */
    assert_int_equal(b->b.data[4 + 18], 0x4E); /* 1B + 33 */

    net_buf_unref(a);
    net_buf_unref(b);
}

/** 截止时间按 ss mm hh DD MM YY 顺序进 N3…N8，同样 +33H。 */
static void test_deadline_goes_into_n3_to_n8(void **state) {
    (void)state;
    const uint8_t addr[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t pwd[4] = {0x02, 0x00, 0x00, 0x00};
    const uint8_t op[4] = {0x00, 0x00, 0x00, 0x00};
    /* 2026-11-01 12:30:00 → ss=00 mm=30 hh=12 DD=01 MM=11 YY=26 */
    const uint8_t deadline[6] = {0x00, 0x30, 0x12, 0x01, 0x11, 0x26};

    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    assert_int_equal(
        dlt645_build_ctrl_request(addr, DLT645_CMD_TRIP, deadline, pwd, op, out), 0);

    const uint8_t *f = out->b.data + 4;
    for (int i = 0; i < 6; i++) {
        assert_int_equal(f[10 + 10 + i], (uint8_t)(deadline[i] + 0x33));
    }
    assert_cs(f, 26);
    net_buf_unref(out);
}

/** 缺地址/密码/操作者代码时直接拒绝，不编一个"看起来对"的帧。 */
static void test_rejects_missing_fields(void **state) {
    (void)state;
    const uint8_t addr[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t pwd[4] = {0x02, 0x00, 0x00, 0x00};
    const uint8_t op[4] = {0x00, 0x00, 0x00, 0x00};
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    assert_int_not_equal(
        dlt645_build_ctrl_request(NULL, DLT645_CMD_TRIP, NULL, pwd, op, out), 0);
    assert_int_not_equal(
        dlt645_build_ctrl_request(addr, DLT645_CMD_TRIP, NULL, NULL, op, out), 0);
    assert_int_not_equal(
        dlt645_build_ctrl_request(addr, DLT645_CMD_TRIP, NULL, pwd, NULL, out), 0);

    net_buf_unref(out);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_trip_frame_layout),
        cmocka_unit_test(test_allow_close_differs_only_in_n1),
        cmocka_unit_test(test_deadline_goes_into_n3_to_n8),
        cmocka_unit_test(test_rejects_missing_fields),
    };
    return cmocka_run_group_tests_name("dlt645_control", tests, setup, teardown);
}
