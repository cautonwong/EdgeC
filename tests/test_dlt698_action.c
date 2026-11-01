/**
 * @file test_dlt698_action.c
 * @brief 698 操作服务（ACTION）与"时间标签"的黄金向量测试。
 *
 * 向量全部抄自 Q/GDW《面向对象的用电信息数据交换协议》附录 H 的报文举例
 * （原文在 docs/698_parsed.md）。用标准自带的字节做期望值，而不是用本实现的
 * 输出反推 —— 后者只能证明"代码没变"，证明不了"报文对"。
 *
 * 帧头部分（68 L L 43 SA CA HCS）各家实现有自由度，这里只比对 **APDU**，
 * 并且同时断言 APDU 长度，避免"前缀对上了、尾部少一字节"这种假通过。
 */
#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dlt698_codec.h"
#include "net_buf.h"

/* 测试固定用 1 字节地址：帧头 = 68 L L 43 | SA描述(1) SA(1) | CA(1) | HCS(2) = 9 字节，
 * 帧尾 = FCS(2) + 16(1)。APDU 就是中间那一段。 */
#define HEAD_LEN 9
#define TAIL_LEN 3

/* 每个用例一个缓冲区，从同一个池里拿：用例数远小于池容量，不必逐个归还。 */
static net_buf_pool_t *g_pool;

static int setup(void **state) {
    (void)state;
    g_pool = net_buf_pool_new(16, 512, 0);
    return g_pool ? 0 : -1;
}

static int teardown(void **state) {
    (void)state;
    if (g_pool) net_buf_pool_unref(g_pool);
    g_pool = NULL;
    return 0;
}

static dlt698_addr_t _sa(void) {
    dlt698_addr_t sa = {.len = 1, .type = 0};
    sa.addr[0] = 0x01;
    return sa;
}

/** 取出 APDU 并断言帧的基本形状（68 开头、16 结尾、长度域自洽）。 */
static size_t _apdu(const net_buf_t *out, const uint8_t **apdu) {
    const uint8_t *p = out->b.data;
    size_t len = out->b.len;

    assert_true(len > HEAD_LEN + TAIL_LEN);
    assert_int_equal(p[0], DLT698_HEAD);
    assert_int_equal(p[len - 1], DLT698_END);

    /* 长度域：除起始/结束字符外的帧字节数，低 14 位有效 */
    uint16_t l_field = (uint16_t)(p[1] | (p[2] << 8)) & 0x3FFF;
    assert_int_equal(l_field, len - 2);

    *apdu = p + HEAD_LEN;
    return len - HEAD_LEN - TAIL_LEN;
}

/** 逐字节比对，并在失败时打印实际字节，避免只看到"1 != 0"。 */
static void _expect(const uint8_t *got, size_t got_len, const uint8_t *want, size_t want_len) {
    if (got_len != want_len) {
        printf("APDU 长度不符：实际 %zu，期望 %zu\n实际: ", got_len, want_len);
        for (size_t i = 0; i < got_len; i++) printf("%02X ", got[i]);
        printf("\n");
    }
    assert_int_equal(got_len, want_len);
    for (size_t i = 0; i < want_len; i++) {
        if (got[i] != want[i]) {
            printf("第 %zu 字节不符：实际 %02X，期望 %02X\n", i, got[i], want[i]);
        }
        assert_int_equal(got[i], want[i]);
    }
}

/**
 * 附录 H.5.1：执行电能量复位方法
 *   发送 07 01 05 | 00 10 01 00 | 0F 00 | 00
 *        ACTION-Request | [1] ActionRequest | PIID | OMD | Data(integer 0) | 时间标签
 */
static void test_action_request_matches_standard_example(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    const uint8_t param[] = {0x0F, 0x00}; /* integer(0) */
    assert_int_equal(dlt698_encode_action_request(&sa, 0x05, 0x0010, 0x01, 0x00, param,
                                                  sizeof(param), out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = _apdu(out, &apdu);
    const uint8_t want[] = {0x07, 0x01, 0x05, 0x00, 0x10, 0x01,
                            0x00, 0x0F, 0x00, 0x00};
    _expect(apdu, n, want, sizeof(want));
}

/** 同一份样例的响应：87 01 05 | OMD | DAR=0 | 无数据 | 无上报 | 时间标签 */
static void test_action_response_matches_standard_example(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    assert_int_equal(dlt698_encode_action_response(&sa, 0x05, 0x0010, 0x01, 0x00, 0x00, NULL, 0,
                                                   out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = _apdu(out, &apdu);
    const uint8_t want[] = {0x87, 0x01, 0x05, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    _expect(apdu, n, want, sizeof(want));
}

/**
 * 失败必须如实回 DAR。
 * 「操作失败回 0」等于告诉主站继电器动了 —— 比不支持危险得多。
 */
static void test_action_response_carries_failure_dar(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    /* DAR=15 密码错/未授权 */
    assert_int_equal(dlt698_encode_action_response(&sa, 0x07, 0x8000, 0x01, 0x00, 15, NULL, 0,
                                                   out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = _apdu(out, &apdu);
    const uint8_t want[] = {0x87, 0x01, 0x07, 0x80, 0x00, 0x01, 0x00, 0x0F, 0x00, 0x00, 0x00};
    _expect(apdu, n, want, sizeof(want));
}

/** 带返回数据时，Data OPTIONAL 标志位必须是 01，否则数据会被当成上报信息读。 */
static void test_action_response_with_data_sets_optional_flag(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    const uint8_t ret[] = {0x11, 0x01}; /* unsigned(1) */
    assert_int_equal(dlt698_encode_action_response(&sa, 0x09, 0x8000, 0x02, 0x00, 0x00, ret,
                                                   sizeof(ret), out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = _apdu(out, &apdu);
    const uint8_t want[] = {0x87, 0x01, 0x09, 0x80, 0x00, 0x02, 0x00,
                            0x00, 0x01, 0x11, 0x01, 0x00, 0x00};
    _expect(apdu, n, want, sizeof(want));
}

/** 方法标识 0 不合法（标准：1…255），不能编出一帧"看起来正常"的报文。 */
static void test_action_rejects_invalid_args(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    const uint8_t param[] = {0x0F, 0x00};
    assert_int_not_equal(
        dlt698_encode_action_request(&sa, 1, 0x0010, 0x00, 0, param, sizeof(param), out), 0);
    assert_int_not_equal(dlt698_encode_action_request(NULL, 1, 0x0010, 1, 0, param,
                                                      sizeof(param), out),
                         0);
    /* 声称有数据却没给指针 */
    assert_int_not_equal(dlt698_encode_action_request(&sa, 1, 0x0010, 1, 0, NULL, 4, out), 0);
}

/**
 * 附录 H.4.1：设置时钟
 *   06 01 02 40 00 02 00 1C 07E00114101B 0B 00
 * 末尾 00 是时间标签 —— 本实现在补它之前是漏的，这条用例就是防回退。
 */
static void test_set_request_ends_with_time_tag(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    const uint8_t when[] = {0x1C, 0x07, 0xE0, 0x01, 0x14, 0x10, 0x1B, 0x0B}; /* date_time_s */
    assert_int_equal(dlt698_encode_set_request(&sa, 0x02, 0x40000200, when, sizeof(when), out), 0);

    const uint8_t *apdu = NULL;
    size_t n = _apdu(out, &apdu);
    const uint8_t want[] = {0x06, 0x01, 0x02, 0x40, 0x00, 0x02, 0x00,
                            0x1C, 0x07, 0xE0, 0x01, 0x14, 0x10, 0x1B, 0x0B, 0x00};
    _expect(apdu, n, want, sizeof(want));
}

/**
 * 附录 H.3.6：读取通信地址的响应（明文段）
 *   85 01 01 40 01 02 00 01 09 06 201601290001 00 00
 * 尾部两个 00 = 无跟随上报 + 无时间标签。
 */
static void test_get_response_ends_with_follow_report_and_time_tag(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    const uint8_t addr[] = {0x09, 0x06, 0x20, 0x16, 0x01, 0x29, 0x00, 0x01};
    assert_int_equal(dlt698_encode_get_response(&sa, 0x01, 0x40010200, addr, sizeof(addr), out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = _apdu(out, &apdu);
    const uint8_t want[] = {0x85, 0x01, 0x01, 0x40, 0x01, 0x02, 0x00, 0x01,
                            0x09, 0x06, 0x20, 0x16, 0x01, 0x29, 0x00, 0x01,
                            0x00, 0x00};
    _expect(apdu, n, want, sizeof(want));
}


/**
 * 帧头校验 HCS 必须覆盖**长度域**（标准 5.1.5：帧头除起始字符与 HCS 之外的全部字节）。
 *
 * 编码器是在写完 APDU 才回填 L 的，如果 HCS 在回填之前算，算的是 L=0 的帧头 ——
 * 值自洽、对端一律判坏帧。这条用例专门锁这个顺序。
 */
static void test_frame_hcs_covers_length_field(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    const uint8_t data[] = {0x17, 0x43, 0x67, 0xB3, 0x33}; /* float32 = 231.7 */
    assert_int_equal(dlt698_encode_get_response(&sa, 0x01, 0x20000200, data, sizeof(data), out),
                     0);

    const uint8_t *f = out->b.data;
    size_t n = out->b.len;
    assert_int_equal(f[0], DLT698_HEAD);
    assert_int_equal(f[n - 1], DLT698_END);

    /* 帧头 = L(2) + C + SA描述(1) + SA(1) + CA(1) = 6 字节，从 f+1 起 */
    uint16_t want_hcs = dlt698_calculate_fcs(f + 1, 6);
    uint16_t got_hcs = (uint16_t)(f[7] | (f[8] << 8));
    if (want_hcs != got_hcs) {
        printf("HCS 不符：算得 %04X，帧里 %04X（HCS 是不是在回填 L 之前算的？）\n", want_hcs,
               got_hcs);
    }
    assert_int_equal(got_hcs, want_hcs);

    uint16_t want_fcs = dlt698_calculate_fcs(f + 1, n - 4);
    uint16_t got_fcs = (uint16_t)(f[n - 3] | (f[n - 2] << 8));
    assert_int_equal(got_fcs, want_fcs);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_action_request_matches_standard_example),
        cmocka_unit_test(test_action_response_matches_standard_example),
        cmocka_unit_test(test_action_response_carries_failure_dar),
        cmocka_unit_test(test_action_response_with_data_sets_optional_flag),
        cmocka_unit_test(test_action_rejects_invalid_args),
        cmocka_unit_test(test_set_request_ends_with_time_tag),
        cmocka_unit_test(test_get_response_ends_with_follow_report_and_time_tag),
        cmocka_unit_test(test_frame_hcs_covers_length_field),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
