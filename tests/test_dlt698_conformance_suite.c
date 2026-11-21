/**
 * @file test_dlt698_conformance_suite.c
 * @brief DL/T 698.45 报文一致性：真实帧向量逐字节对照。
 *
 * 依据（权威来源是 docs/698.pdf，docs/698_parsed.md 只用来检索）：
 *   5.1.1 图6   帧格式：68 | L(2) | C | A | HCS(2) | APDU | FCS(2) | 16
 *   5.1.2       长度域 L = 帧字节数减 2（不含起止字符），低 14 位有效
 *   5.1.3.2 表2 控制域：DIR=0 PRM=1 为客户机请求，DIR=1 PRM=1 为服务器响应
 *   5.1.5       HCS 覆盖帧头除 68 与自身之外的全部字节
 *   5.1.7       FCS 覆盖整帧除 68、16 与自身之外的全部字节
 *   附录 D      校验算法（PPP FCS-16：初值 0xFFFF，多项式 0x8408，末取反，低字节在前）
 *   H.3.1       读取一个对象属性（文档第 206 页）
 *   H.3.2       读取多个对象属性（文档第 206-207 页）
 *   H.4.1       设置一个对象属性（文档第 213 页）
 *   H.5.1       操作一个对象方法（文档第 214-215 页）
 *
 * 两条规矩：
 *   1. 期望字节抄自标准，不是拿本实现的输出反推 —— 后者只能证明"代码没变"。
 *   2. HCS/FCS 用一个独立实现（本文件里的 orc_fcs16，按附录 D 逐位算）核对，
 *      不与 codec 共用查表，避免实现与期望一起错。
 *
 * 帧头约定：本实现用 1 字节 SA、CA=0x10，因此整帧向量带的是本实现的帧头；
 * 附录 H 里出现的 "CS CS" 是校验占位符，不提供整帧字节。APDU 一律与标准一致。
 */
#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dlt698_codec.h"
#include "net_buf.h"

/* 本实现帧头长度：68 | L L | C | SA配置 | SA | CA | HCS HCS */
#define HEAD_LEN 9
/* 帧尾：FCS(2) + 16 */
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

/** 按附录 D 的 pppfcs16 逐位实现，不看 codec 的查表。 */
static uint16_t orc_fcs16(const uint8_t *p, size_t len) {
    uint16_t fcs = 0xFFFF;
    while (len--) {
        fcs = (uint16_t)(fcs ^ *p++);
        for (int bit = 0; bit < 8; bit++) {
            fcs = (uint16_t)((fcs & 1) ? ((fcs >> 1) ^ 0x8408) : (fcs >> 1));
        }
    }
    return (uint16_t)(fcs ^ 0xFFFF);
}

/** 把 APDU 包成帧，HCS 与 FCS 用独立实现算。返回整帧长度，0 表示装不下。 */
static size_t build_frame(uint8_t ctrl, const uint8_t *apdu, size_t apdu_len, uint8_t *out,
                          size_t cap) {
    size_t total = 1 + 2 + 1 + 1 + 1 + 1 + 2 + apdu_len + 2 + 1;
    if (total > cap || total > 0xFFFF) return 0;
    size_t i = 0;
    out[i++] = DLT698_HEAD;
    uint16_t l_field = (uint16_t)(total - 2);
    out[i++] = (uint8_t)(l_field & 0xFF);
    out[i++] = (uint8_t)(l_field >> 8);
    out[i++] = ctrl;
    out[i++] = 0x00; /* SA 描述：类型 0、长度 1 字节 */
    out[i++] = 0x01;
    out[i++] = 0x10;
    uint16_t hcs = orc_fcs16(out + 1, i - 1);
    out[i++] = (uint8_t)(hcs & 0xFF);
    out[i++] = (uint8_t)(hcs >> 8);
    memcpy(out + i, apdu, apdu_len);
    i += apdu_len;
    uint16_t fcs = orc_fcs16(out + 1, i - 1);
    out[i++] = (uint8_t)(fcs & 0xFF);
    out[i++] = (uint8_t)(fcs >> 8);
    out[i++] = DLT698_END;
    return i;
}

static dlt698_addr_t _sa(void) {
    dlt698_addr_t sa = {.len = 1, .type = 0};
    sa.addr[0] = 0x01;
    return sa;
}

static void _expect_bytes(const uint8_t *got, size_t got_len, const uint8_t *want,
                          size_t want_len, const char *what) {
    if (got_len != want_len) {
        printf("%s 长度不符：实际 %zu，期望 %zu\n实际: ", what, got_len, want_len);
        for (size_t i = 0; i < got_len; i++) printf("%02X ", got[i]);
        printf("\n");
    }
    assert_int_equal(got_len, want_len);
    for (size_t i = 0; i < want_len; i++) {
        if (got[i] != want[i]) printf("%s 第 %zu 字节：实际 %02X 期望 %02X\n", what, i, got[i], want[i]);
        assert_int_equal(got[i], want[i]);
    }
}

/**
 * 取 APDU 并核对帧层四件事：起止字符、长度域、HCS、FCS。
 * 这四条覆盖 5.1.1 图6、5.1.2、5.1.5、5.1.7。
 */
static size_t frame_apdu(const net_buf_t *out, uint8_t expect_ctrl, const uint8_t **apdu) {
    const uint8_t *p = out->b.data;
    size_t len = out->b.len;
    assert_true(len > HEAD_LEN + TAIL_LEN);

    assert_int_equal(p[0], DLT698_HEAD);
    assert_int_equal(p[len - 1], DLT698_END);

    uint16_t l_field = (uint16_t)(p[1] | (p[2] << 8));
    assert_int_equal(l_field & 0x3FFF, len - 2);
    assert_int_equal(p[3], expect_ctrl);

    /* HCS 覆盖 长度域..SA/CA（即 68 之后、HCS 之前） */
    uint16_t hcs_rx = (uint16_t)(p[7] | (p[8] << 8));
    assert_int_equal(orc_fcs16(p + 1, 6), hcs_rx);

    /* FCS 覆盖 68 之后、FCS 之前 */
    uint16_t fcs_rx = (uint16_t)(p[len - 3] | (p[len - 2] << 8));
    assert_int_equal(orc_fcs16(p + 1, len - 4), fcs_rx);

    *apdu = p + HEAD_LEN;
    return len - HEAD_LEN - TAIL_LEN;
}

static int decode_frame(const uint8_t *frame, size_t len, dlt698_addr_t *sa, uint8_t *svc,
                        uint8_t *choice, uint8_t *piid, uint8_t **payload, size_t *payload_len) {
    uint8_t storage[512];
    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, storage, sizeof(storage));
    net_buf_simple_add_mem(&b, frame, len);
    return dlt698_decode_frame_ex(&b, sa, svc, choice, piid, payload, payload_len);
}

/** 附录 D 的算法本身：PPP FCS-16 对 "123456789" 的检校值是 0x906E。 */
static void test_fcs_algorithm_known_answer(void **state) {
    (void)state;
    static const uint8_t ascii[] = "123456789";
    assert_int_equal(orc_fcs16(ascii, 9), 0x906E);
}

/**
 * 整帧已知答案：H.3.1 的 GET-Request 与 GET-Response 的 APDU，
 * 套本实现的帧头（68 L L 43 00 01 10 / C3 00 01 10）后 HCS/FCS 逐字节写死。
 */
static void test_frame_known_answer_full_bytes(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    /* H.3.1 发送：05 01 01 40 01 02 00 00 */
    assert_int_equal(dlt698_encode_get_request(&sa, 0x01, 0x40010200U, out), 0);
    const uint8_t want_req[] = {0x68, 0x12, 0x00, 0x43, 0x00, 0x01, 0x10, 0x4A, 0x87,
                                0x05, 0x01, 0x01, 0x40, 0x01, 0x02, 0x00, 0x00, 0xC6, 0x07,
                                0x16};
    _expect_bytes(out->b.data, out->b.len, want_req, sizeof(want_req), "GET-Request 整帧");
    net_buf_unref(out);

    /* H.3.1 响应：85 01 01 40 01 02 00 01 09 06 12 34 56 78 90 12 00 00 */
    out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    static const uint8_t addr456[] = {0x09, 0x06, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12};
    assert_int_equal(dlt698_encode_get_response(&sa, 0x01, 0x40010200U, addr456, sizeof(addr456),
                                                out),
                     0);
    const uint8_t want_res[] = {0x68, 0x1C, 0x00, 0xC3, 0x00, 0x01, 0x10, 0x86, 0x93, 0x85,
                                0x01, 0x01, 0x40, 0x01, 0x02, 0x00, 0x01, 0x09, 0x06, 0x12,
                                0x34, 0x56, 0x78, 0x90, 0x12, 0x00, 0x00, 0xCB, 0x89, 0x16};
    _expect_bytes(out->b.data, out->b.len, want_res, sizeof(want_res), "GET-Response 整帧");
    net_buf_unref(out);
}

/** H.3.1（文档第 206 页）读取一个对象属性：05 01 01 40 01 02 00 00 */
static void test_get_request_normal(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    assert_int_equal(dlt698_encode_get_request(&sa, 0x01, 0x40010200U, out), 0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0x43, &apdu);
    const uint8_t want[] = {0x05, 0x01, 0x01, 0x40, 0x01, 0x02, 0x00, 0x00};
    _expect_bytes(apdu, n, want, sizeof(want), "H.3.1 GET-Request");

    dlt698_addr_t got_sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    assert_int_equal(decode_frame(out->b.data, out->b.len, &got_sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     0);
    assert_int_equal(svc, DLT698_SVC_GET_REQ);
    assert_int_equal(choice, DLT698_CHOICE_GET_NORMAL);
    assert_int_equal(piid, 0x01);
    assert_int_equal(got_sa.len, 1);
    assert_int_equal(got_sa.addr[0], 0x01);
    const uint8_t want_payload[] = {0x40, 0x01, 0x02, 0x00, 0x00};
    _expect_bytes(payload, plen, want_payload, sizeof(want_payload), "GET-Request 载荷");
    net_buf_unref(out);
}

/** H.3.1（文档第 206 页）响应：85 01 01 40 01 02 00 01 09 06 <6 字节地址> 00 00 */
static void test_get_response_normal(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    static const uint8_t addr_bytes[] = {0x09, 0x06, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12};
    assert_int_equal(dlt698_encode_get_response(&sa, 0x01, 0x40010200U, addr_bytes,
                                                sizeof(addr_bytes), out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0xC3, &apdu);
    const uint8_t want[] = {0x85, 0x01, 0x01, 0x40, 0x01, 0x02, 0x00, 0x01, 0x09,
                            0x06, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x00, 0x00};
    _expect_bytes(apdu, n, want, sizeof(want), "H.3.1 GET-Response");

    dlt698_addr_t got_sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    assert_int_equal(decode_frame(out->b.data, out->b.len, &got_sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     0);
    assert_int_equal(svc, DLT698_SVC_GET_RES);
    assert_int_equal(choice, DLT698_CHOICE_GET_NORMAL);
    assert_int_equal(piid, 0x01);
    net_buf_unref(out);
}

/**
 * H.3.2（文档第 206-207 页）请求：05 02 02 02 <OAD1> <OAD2> 00
 *
 * 编解码层没有 GET-Request-NormalList 的构造函数，所以这里把标准的 APDU
 * 装进帧后走解析路径，断言服务、CHOICE、PIID 与载荷原样透出。
 * 构造函数缺失记在 D1 系列（见交回说明），不在本单伪造通过。
 */
static void test_get_request_normal_list_decode(void **state) {
    (void)state;
    static const uint8_t apdu[] = {0x05, 0x02, 0x02, 0x02, 0x20, 0x00, 0x02, 0x00,
                                   0x20, 0x01, 0x02, 0x00, 0x00};
    uint8_t frame[64];
    size_t flen = build_frame(0x43, apdu, sizeof(apdu), frame, sizeof(frame));
    assert_true(flen > 0);

    dlt698_addr_t sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    assert_int_equal(decode_frame(frame, flen, &sa, &svc, &choice, &piid, &payload, &plen), 0);
    assert_int_equal(svc, DLT698_SVC_GET_REQ);
    assert_int_equal(choice, DLT698_CHOICE_GET_NORMAL_LIST);
    assert_int_equal(piid, 0x02);
    const uint8_t want_payload[] = {0x02, 0x20, 0x00, 0x02, 0x00, 0x20, 0x01, 0x02, 0x00, 0x00};
    _expect_bytes(payload, plen, want_payload, sizeof(want_payload), "H.3.2 请求载荷");
}

/** H.3.2（文档第 206-207 页）响应：两项各带数组，元素类型分别是 long-unsigned 与 double-long。 */
static void test_get_response_normal_list(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    static const uint8_t volts[] = {0x01, 0x03, 0x12, 0x09, 0x6D, 0x12, 0x09, 0x6D,
                                    0x12, 0x09, 0x6D};
    static const uint8_t amps[] = {0x01, 0x03, 0x05, 0x00, 0x00, 0x03, 0xE8, 0x05,
                                   0x00, 0x00, 0x03, 0xE8, 0x05, 0x00, 0x00, 0x03, 0xE8};
    dlt698_get_item_t items[2] = {
        {.oad = 0x20000200U, .dar = 0, .data = volts, .dlen = sizeof(volts)},
        {.oad = 0x20010200U, .dar = 0, .data = amps, .dlen = sizeof(amps)},
    };
    assert_int_equal(dlt698_encode_get_response_list(&sa, 0x02, items, 2, out), 0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0xC3, &apdu);
    const uint8_t want[] = {
        0x85, 0x02, 0x02, 0x02,
        0x20, 0x00, 0x02, 0x00, 0x01, 0x01, 0x03, 0x12, 0x09, 0x6D,
        0x12, 0x09, 0x6D, 0x12, 0x09, 0x6D,
        0x20, 0x01, 0x02, 0x00, 0x01, 0x01, 0x03,
        0x05, 0x00, 0x00, 0x03, 0xE8, 0x05, 0x00, 0x00, 0x03, 0xE8,
        0x05, 0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00,
    };
    _expect_bytes(apdu, n, want, sizeof(want), "H.3.2 GET-Response");
    net_buf_unref(out);
}

/**
 * 部分失败项按 6.3.8.2.2（表73）+ 6.3.3.11 如实回 DAR。
 * 附录 H 没有失败样例，这条是按标准结构构造的（行内注明），不是抄自报文举例。
 */
static void test_get_response_list_partial_dar(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    static const uint8_t volts[] = {0x12, 0x09, 0x6D};
    dlt698_get_item_t items[2] = {
        {.oad = 0x20000200U, .dar = 0, .data = volts, .dlen = sizeof(volts)},
        {.oad = 0x20010200U, .dar = 6, .data = NULL, .dlen = 0}, /* 对象不存在（表34：6） */
    };
    assert_int_equal(dlt698_encode_get_response_list(&sa, 0x02, items, 2, out), 0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0xC3, &apdu);
    const uint8_t want[] = {0x85, 0x02, 0x02, 0x02,
                            0x20, 0x00, 0x02, 0x00, 0x01, 0x12, 0x09, 0x6D,
                            0x20, 0x01, 0x02, 0x00, 0x00, 0x06,
                            0x00, 0x00};
    _expect_bytes(apdu, n, want, sizeof(want), "GET-Response 部分失败");
    net_buf_unref(out);
}

/**
 * 记录型对象属性（GET-Request-Record，CHOICE [3]）与 RSD 尚未实现。
 * 这条不算通过：skip 掉并把缺口指向 D1-15，避免"没实现"被当成"已实现"。
 */
static void test_get_record_and_rsd_not_implemented(void **state) {
    (void)state;
    printf("SKIP: GET-Request-Record([3]) 与 RSD 未实现，见 D1-15；"
           "附录 H.3.3 的冻结记录报文在实现后补向量。\n");
    skip();
}

/** H.4.1（文档第 213 页）设置时钟：06 01 02 40 00 02 00 1C <7 字节时间> 00 */
static void test_set_request_normal(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    static const uint8_t when[] = {0x1C, 0x07, 0xE0, 0x01, 0x14, 0x10, 0x1B, 0x0B};
    assert_int_equal(dlt698_encode_set_request(&sa, 0x02, 0x40000200U, when, sizeof(when), out), 0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0x43, &apdu);
    const uint8_t want[] = {0x06, 0x01, 0x02, 0x40, 0x00, 0x02, 0x00, 0x1C,
                            0x07, 0xE0, 0x01, 0x14, 0x10, 0x1B, 0x0B, 0x00};
    _expect_bytes(apdu, n, want, sizeof(want), "H.4.1 SET-Request");
    net_buf_unref(out);
}

/**
 * H.4.1（文档第 213 页）响应：86 01 02 40 00 02 00 00 00 00
 * 编解码层没有 SET-Response 构造函数，走解析路径核对字段与载荷。
 */
static void test_set_response_normal_decode(void **state) {
    (void)state;
    static const uint8_t apdu[] = {0x86, 0x01, 0x02, 0x40, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00};
    uint8_t frame[64];
    size_t flen = build_frame(0xC3, apdu, sizeof(apdu), frame, sizeof(frame));
    assert_true(flen > 0);

    dlt698_addr_t sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    assert_int_equal(decode_frame(frame, flen, &sa, &svc, &choice, &piid, &payload, &plen), 0);
    assert_int_equal(svc, DLT698_SVC_SET_RES);
    assert_int_equal(choice, 0x01);
    assert_int_equal(piid, 0x02);
    const uint8_t want_payload[] = {0x40, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00};
    _expect_bytes(payload, plen, want_payload, sizeof(want_payload), "H.4.1 响应载荷");
}

/** H.5.1（文档第 214 页）执行电能量复位：07 01 05 00 10 01 00 0F 00 00 */
static void test_action_request_normal(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);

    static const uint8_t arg[] = {0x0F, 0x00}; /* integer 0 */
    assert_int_equal(dlt698_encode_action_request(&sa, 0x05, 0x0010, 0x01, 0x00, arg, sizeof(arg),
                                                  out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0x43, &apdu);
    const uint8_t want[] = {0x07, 0x01, 0x05, 0x00, 0x10, 0x01, 0x00, 0x0F, 0x00, 0x00};
    _expect_bytes(apdu, n, want, sizeof(want), "H.5.1 ACTION-Request");
    net_buf_unref(out);
}

/** H.5.1（文档第 215 页）响应 DAR=0：87 01 05 00 10 01 00 00 00 00 00 */
static void test_action_response_dar0(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    assert_int_equal(dlt698_encode_action_response(&sa, 0x05, 0x0010, 0x01, 0x00, 0x00, NULL, 0,
                                                   out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0xC3, &apdu);
    const uint8_t want[] = {0x87, 0x01, 0x05, 0x00, 0x10, 0x01, 0x00,
                            0x00, 0x00, 0x00, 0x00};
    _expect_bytes(apdu, n, want, sizeof(want), "H.5.1 ACTION-Response(DAR=0)");
    net_buf_unref(out);
}

/**
 * DAR=2（暂时失效）按 6.3.3.11 如实填。附录 H 没有失败样例，
 * 期望字节由标准结构推出：OMD 之后 DAR=02，Data OPTIONAL=0，FollowReport=0，时间标签=0。
 */
static void test_action_response_dar2(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    assert_int_equal(dlt698_encode_action_response(&sa, 0x05, 0x0010, 0x01, 0x00, 0x02, NULL, 0,
                                                   out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0xC3, &apdu);
    const uint8_t want[] = {0x87, 0x01, 0x05, 0x00, 0x10, 0x01, 0x00,
                            0x02, 0x00, 0x00, 0x00};
    _expect_bytes(apdu, n, want, sizeof(want), "ACTION-Response(DAR=2)");
    net_buf_unref(out);
}

/** 成功且带返回数据时，Data OPTIONAL=01 后跟 A-XDR（结构依 6.3.3.12）。 */
static void test_action_response_with_data(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    static const uint8_t ret[] = {0x12, 0x00, 0x64}; /* long-unsigned 100 */
    assert_int_equal(dlt698_encode_action_response(&sa, 0x05, 0x0010, 0x01, 0x00, 0x00, ret,
                                                   sizeof(ret), out),
                     0);

    const uint8_t *apdu = NULL;
    size_t n = frame_apdu(out, 0xC3, &apdu);
    const uint8_t want[] = {0x87, 0x01, 0x05, 0x00, 0x10, 0x01, 0x00, 0x00,
                            0x01, 0x12, 0x00, 0x64, 0x00, 0x00};
    _expect_bytes(apdu, n, want, sizeof(want), "ACTION-Response(带返回数据)");
    net_buf_unref(out);
}

static net_buf_t *valid_get_frame(void) {
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    assert_int_equal(dlt698_encode_get_request(&sa, 0x01, 0x40010200U, out), 0);
    return out;
}

/** 5.1.7：FCS 错必须拒收。 */
static void test_reject_bad_fcs(void **state) {
    (void)state;
    net_buf_t *out = valid_get_frame();
    out->b.data[out->b.len - 3] ^= 0xFF;
    dlt698_addr_t sa;
    uint8_t svc, choice, piid;
    uint8_t *payload;
    size_t plen;
    assert_int_equal(decode_frame(out->b.data, out->b.len, &sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     -1);
    net_buf_unref(out);
}

/** 5.1.2：长度域比实际帧大时必须拒收，不能按短帧解析。 */
static void test_reject_length_mismatch(void **state) {
    (void)state;
    net_buf_t *out = valid_get_frame();
    out->b.data[1] = 0x30; /* L 声称 48 字节 */
    dlt698_addr_t sa;
    uint8_t svc, choice, piid;
    uint8_t *payload;
    size_t plen;
    assert_int_equal(decode_frame(out->b.data, out->b.len, &sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     1);
    net_buf_unref(out);
}

static void test_reject_truncated_frame(void **state) {
    (void)state;
    net_buf_t *out = valid_get_frame();
    dlt698_addr_t sa;
    uint8_t svc, choice, piid;
    uint8_t *payload;
    size_t plen;
    assert_int_equal(decode_frame(out->b.data, out->b.len - 4, &sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     1);
    net_buf_unref(out);
}

/** 5.1.1：起始字符必须是 68。 */
static void test_reject_bad_head(void **state) {
    (void)state;
    net_buf_t *out = valid_get_frame();
    out->b.data[0] = 0x67;
    dlt698_addr_t sa;
    uint8_t svc, choice, piid;
    uint8_t *payload;
    size_t plen;
    assert_int_equal(decode_frame(out->b.data, out->b.len, &sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     -2);
    net_buf_unref(out);
}

/** 5.1.1：结束字符必须是 16。 */
static void test_reject_bad_end(void **state) {
    (void)state;
    net_buf_t *out = valid_get_frame();
    out->b.data[out->b.len - 1] = 0x17;
    dlt698_addr_t sa;
    uint8_t svc, choice, piid;
    uint8_t *payload;
    size_t plen;
    assert_int_equal(decode_frame(out->b.data, out->b.len, &sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     -2);
    net_buf_unref(out);
}

/**
 * APDU 的 CHOICE 被改成 [9] 时，帧校验仍成立。解析器按契约把原始 CHOICE 透出，
 * 由调用方按 5.1 与 6.3 判非法；这里锁定"透出而不是猜"的行为。
 */
static void test_passthrough_unknown_choice(void **state) {
    (void)state;
    dlt698_addr_t sa = _sa();
    net_buf_t *out = net_buf_alloc(g_pool, 0);
    assert_non_null(out);
    assert_int_equal(dlt698_encode_get_request(&sa, 0x01, 0x40010200U, out), 0);
    /* 改 CHOICE 后重算 FCS，让帧本身合法，只剩语义非法 */
    out->b.data[HEAD_LEN + 1] = 0x09;
    uint16_t fcs = orc_fcs16(out->b.data + 1, out->b.len - 4);
    out->b.data[out->b.len - 3] = (uint8_t)(fcs & 0xFF);
    out->b.data[out->b.len - 2] = (uint8_t)(fcs >> 8);

    dlt698_addr_t got_sa;
    uint8_t svc = 0, choice = 0, piid = 0;
    uint8_t *payload = NULL;
    size_t plen = 0;
    assert_int_equal(decode_frame(out->b.data, out->b.len, &got_sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     0);
    assert_int_equal(choice, 0x09);
    net_buf_unref(out);
}

/**
 * 5.1.5：HCS 错必须拒收。
 *
 * 关键在于**同时重算 FCS**：只改 HCS 会让 FCS 校验先失败，那样证明不了
 * HCS 被检查。改完 HCS 再把 FCS 按当前字节重算，帧只剩 HCS 一处错，
 * 解析器必须返回 -1。
 */
static void test_reject_bad_hcs(void **state) {
    (void)state;
    net_buf_t *out = valid_get_frame();
    out->b.data[7] ^= 0xFF; /* HCS 低字节改错 */
    uint16_t fcs = orc_fcs16(out->b.data + 1, out->b.len - 4);
    out->b.data[out->b.len - 3] = (uint8_t)(fcs & 0xFF);
    out->b.data[out->b.len - 2] = (uint8_t)(fcs >> 8);

    dlt698_addr_t sa;
    uint8_t svc, choice, piid;
    uint8_t *payload;
    size_t plen;
    assert_int_equal(decode_frame(out->b.data, out->b.len, &sa, &svc, &choice, &piid, &payload,
                                  &plen),
                     -1);
    net_buf_unref(out);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fcs_algorithm_known_answer),
        cmocka_unit_test(test_frame_known_answer_full_bytes),

        cmocka_unit_test(test_get_request_normal),
        cmocka_unit_test(test_get_response_normal),
        cmocka_unit_test(test_get_request_normal_list_decode),
        cmocka_unit_test(test_get_response_normal_list),
        cmocka_unit_test(test_get_response_list_partial_dar),
        cmocka_unit_test(test_get_record_and_rsd_not_implemented),

        cmocka_unit_test(test_set_request_normal),
        cmocka_unit_test(test_set_response_normal_decode),

        cmocka_unit_test(test_action_request_normal),
        cmocka_unit_test(test_action_response_dar0),
        cmocka_unit_test(test_action_response_dar2),
        cmocka_unit_test(test_action_response_with_data),

        cmocka_unit_test(test_reject_bad_fcs),
        cmocka_unit_test(test_reject_length_mismatch),
        cmocka_unit_test(test_reject_truncated_frame),
        cmocka_unit_test(test_reject_bad_head),
        cmocka_unit_test(test_reject_bad_end),
        cmocka_unit_test(test_passthrough_unknown_choice),
        cmocka_unit_test(test_reject_bad_hcs),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}
