/**
 * @file test_dlt698_get_list.c
 * @brief GET-Response-NormalList（[2]，一次读多个 OAD）的字节级测试。
 *
 * 依据：DL/T 698.45 6.3.8.2.2 表73
 *   GetResponseNormalList = SEQUENCE { PIID-ACD, SEQUENCE OF A-ResultNormal }
 *   A-ResultNormal        = SEQUENCE { OAD(4), Get-Result }
 *   Get-Result            = CHOICE { [0] DAR, [1] Data }
 *
 * 期望字节是**按标准结构手写的**，不是拿本实现的输出反推 —— 后者只能证明
 * "代码没变"，证明不了"报文对"。附录 H 没有 [2] 的报文向量，所以这里的
 * 类型/个数宽度按项目既有 A-XDR 约定（1 字节个数）锁定；拿到现场 [2] 报文
 * 后要回头核一次，别把"自洽"当成"合标"。
 */
#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dlt698_codec.h"
#include "net_buf.h"

#define HEAD_LEN 9
#define TAIL_LEN 3

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

static size_t _apdu(const net_buf_t *out, const uint8_t **apdu) {
    const uint8_t *p = out->b.data;
    size_t len = out->b.len;
    assert_true(len > HEAD_LEN + TAIL_LEN);
    assert_int_equal(p[0], DLT698_HEAD);
    assert_int_equal(p[len - 1], DLT698_END);
    uint16_t l_field = (uint16_t)(p[1] | (p[2] << 8)) & 0x3FFF;
    assert_int_equal(l_field, len - 2);
    *apdu = p + HEAD_LEN;
    return len - HEAD_LEN - TAIL_LEN;
}

static void _expect(const uint8_t *got, size_t got_len, const uint8_t *want, size_t want_len) {
    if (got_len != want_len) {
        printf("APDU 长度不符：实际 %zu，期望 %zu\n实际: ", got_len, want_len);
        for (size_t i = 0; i < got_len; i++) printf("%02X ", got[i]);
        printf("\n");
    }
    assert_int_equal(got_len, want_len);
    for (size_t i = 0; i < want_len; i++) {
        if (got[i] != want[i]) printf("第 %zu 字节：实际 %02X 期望 %02X\n", i, got[i], want[i]);
        assert_int_equal(got[i], want[i]);
    }
}

/* 231.7f = 43 67 B3 33（IEEE754 大端）；40 = 00 28（long-unsigned） */
static const uint8_t V_FLOAT[] = {0x17, 0x43, 0x67, 0xB3, 0x33};
static const uint8_t V_COUNT[] = {0x12, 0x00, 0x28};

/**
 * `85 | 02 | PIID-ACD | n | (OAD | 01 | Data)×n | 00 | 00`
 * 两项类型不同（float32 / long-unsigned）—— 定界错一格就会整体错位，
 * 而 HCS/FCS 全对、帧合法，只有值是错的。
 */
static void test_list_response_matches_standard_layout(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    dlt698_get_item_t items[2] = {
        {.oad = 0x20000200U, .dar = 0, .data = V_FLOAT, .dlen = sizeof(V_FLOAT)},
        {.oad = 0x60000200U, .dar = 0, .data = V_COUNT, .dlen = sizeof(V_COUNT)},
    };
    assert_int_equal(dlt698_encode_get_response_list(&sa, 0x01, items, 2, out), 0);

    const uint8_t *apdu = NULL;
    size_t n = _apdu(out, &apdu);
    const uint8_t want[] = {
        0x85, 0x02, 0x01, 0x02,
        0x20, 0x00, 0x02, 0x00, 0x01, 0x17, 0x43, 0x67, 0xB3, 0x33,
        0x60, 0x00, 0x02, 0x00, 0x01, 0x12, 0x00, 0x28,
        0x00, 0x00,
    };
    _expect(apdu, n, want, sizeof(want));
    net_buf_unref(out);
}

/** 读失败的项要如实回 DAR，不能悄悄少返一项（主站会以为只请求了一个 OAD）。 */
static void test_failed_item_is_reported_as_dar(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    dlt698_get_item_t items[2] = {
        {.oad = 0x20000200U, .dar = 0, .data = V_FLOAT, .dlen = sizeof(V_FLOAT)},
        {.oad = 0x99990000U, .dar = 6, .data = NULL, .dlen = 0}, /* 对象不存在 */
    };
    assert_int_equal(dlt698_encode_get_response_list(&sa, 0x01, items, 2, out), 0);

    const uint8_t *apdu = NULL;
    size_t n = _apdu(out, &apdu);
    const uint8_t want[] = {
        0x85, 0x02, 0x01, 0x02,
        0x20, 0x00, 0x02, 0x00, 0x01, 0x17, 0x43, 0x67, 0xB3, 0x33,
        0x99, 0x99, 0x00, 0x00, 0x00, 0x06,
        0x00, 0x00,
    };
    _expect(apdu, n, want, sizeof(want));
    net_buf_unref(out);
}

/** 参数不合法时直接拒绝，不编一个"看起来对"的帧。 */
static void test_rejects_bad_arguments(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    dlt698_get_item_t one = {.oad = 0x20000200U, .data = V_FLOAT, .dlen = sizeof(V_FLOAT)};
    assert_int_not_equal(dlt698_encode_get_response_list(&sa, 1, NULL, 1, out), 0);
    assert_int_not_equal(dlt698_encode_get_response_list(&sa, 1, &one, 0, out), 0);
    assert_int_not_equal(
        dlt698_encode_get_response_list(&sa, 1, &one, DLT698_GET_LIST_MAX + 1, out), 0);
    assert_int_not_equal(dlt698_encode_get_response_list(NULL, 1, &one, 1, out), 0);
    net_buf_unref(out);
}

/** ex 解码要能吐出 CHOICE：[2] 响应帧应报 0x02，[1] 响应帧应报 0x01。 */
static void test_decode_ex_reports_choice(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();

    net_buf_t *list = net_buf_alloc(g_pool, 0);
    dlt698_get_item_t items[1] = {
        {.oad = 0x20000200U, .data = V_FLOAT, .dlen = sizeof(V_FLOAT)}};
    assert_int_equal(dlt698_encode_get_response_list(&sa, 0x01, items, 1, list), 0);

    struct net_buf_simple nb;
    uint8_t tmp[512];
    memcpy(tmp, list->b.data, list->b.len);
    net_buf_simple_init_with_data(&nb, tmp, list->b.len);
    nb.len = (uint16_t)list->b.len;

    dlt698_addr_t sa_rx;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t payload_len = 0;
    assert_int_equal(
        dlt698_decode_frame_ex(&nb, &sa_rx, &svc, &choice, &piid, &payload, &payload_len), 0);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(choice, DLT698_CHOICE_GET_NORMAL_LIST);
    assert_int_equal(piid, 0x01);
    assert_int_equal(payload[0], 1); /* 结果个数 */
    net_buf_unref(list);

    net_buf_t *single = net_buf_alloc(g_pool, 0);
    assert_non_null(single);
    assert_int_equal(
        dlt698_encode_get_response(&sa, 0x01, 0x20000200U, V_FLOAT, sizeof(V_FLOAT), single), 0);
    memcpy(tmp, single->b.data, single->b.len);
    net_buf_simple_init_with_data(&nb, tmp, single->b.len);
    nb.len = (uint16_t)single->b.len;
    assert_int_equal(
        dlt698_decode_frame_ex(&nb, &sa_rx, &svc, &choice, &piid, &payload, &payload_len), 0);
    assert_int_equal(choice, DLT698_CHOICE_GET_NORMAL);
    net_buf_unref(single);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_list_response_matches_standard_layout),
        cmocka_unit_test(test_failed_item_is_reported_as_dar),
        cmocka_unit_test(test_rejects_bad_arguments),
        cmocka_unit_test(test_decode_ex_reports_choice),
    };
    return cmocka_run_group_tests_name("dlt698_get_list", tests, setup, teardown);
}
