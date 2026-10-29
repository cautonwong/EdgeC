/**
 * @file test_dlt645_codec.c
 * @brief DL/T 645 纯 codec 的单元测试（ADR-002 S1）
 *
 * 只测"结构体 ↔ 字节"：没有 ctx、没有 tag_id、没有 datahub。
 * 这正是这次改造的目的——驱动可独立验证，不需要起总线、不需要点位表。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "driver_dlt645.h"
#include "edge_driver.h"
#include "edge_slave_plane.h"
#include "net_buf.h"

#define TEST_BUF 256

extern const edge_driver_t dlt645_driver;

static uint8_t g_buf_mem[TEST_BUF];

static void _mk_buf(net_buf_t *nb, uint8_t *mem, size_t cap) {
    memset(nb, 0, sizeof(*nb));
    net_buf_simple_init_with_data(&nb->b, mem, cap);
}

/* 组请求：字节必须逐字节正确（这是现场能不能通的前提） */
static void test_encode_read_bytes(void** state) {
    (void)state;
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    const uint8_t addr[6] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12};
    memcpy(req.addr_bcd, addr, 6);
    req.di = 0x02010100; /* 电压，低字节先上线 */

    net_buf_t nb;
    _mk_buf(&nb, g_buf_mem, TEST_BUF);
    assert_int_equal(dlt645_encode_read(&req, &nb), 0);

    const uint8_t expect[] = {0x68, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x68,
                              0x11, 0x04, 0x33, 0x34, 0x34, 0x35, 0x6B, 0x16};
    assert_int_equal(nb.b.len, sizeof(expect));
    assert_memory_equal(nb.b.data, expect, sizeof(expect));
}

/* 从站组应答 → 主站解码，值要原样回来 */
static void test_slave_rsp_roundtrip(void** state) {
    (void)state;
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    const uint8_t addr[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    memcpy(req.addr_bcd, addr, 6);
    req.di = 0x02010100;
    req.raw = 2200;    /* 220.0V，一位小数（2 字节 BCD 上限 9999） */
    req.data_len = 2;

    net_buf_t out;
    _mk_buf(&out, g_buf_mem, TEST_BUF);
    assert_int_equal(dlt645_encode_slave_rsp(&req, &out), 0);

    dlt645_req_t dec;
    memset(&dec, 0, sizeof(dec));
    size_t consumed = 0;
    assert_int_equal(dlt645_decode_read_rsp(&dec, out.b.data, out.b.len, &consumed), 0);
    assert_int_equal(consumed, out.b.len);
    assert_int_equal(dec.di, 0x02010100);
    assert_int_equal(dec.raw, 2200);
    assert_int_equal(dec.data_len, 2);
    assert_true(dec.value > 2199.0 && dec.value < 2201.0);
}

/* 四字节量（组合有功等）也要能解 */
static void test_four_byte_quantity(void** state) {
    (void)state;
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    req.di = 0x00010000;
    req.raw = 12345678;
    req.data_len = 4;

    net_buf_t out;
    _mk_buf(&out, g_buf_mem, TEST_BUF);
    assert_int_equal(dlt645_encode_slave_rsp(&req, &out), 0);

    dlt645_req_t dec;
    memset(&dec, 0, sizeof(dec));
    size_t consumed = 0;
    assert_int_equal(dlt645_decode_read_rsp(&dec, out.b.data, out.b.len, &consumed), 0);
    assert_int_equal(dec.raw, 12345678);
    assert_int_equal(dec.data_len, 4);
}

/* 半帧必须返回 INCOMPLETE，不能当成错帧丢掉 */
static void test_incomplete_frame(void** state) {
    (void)state;
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    req.di = 0x02010100;
    req.raw = 2200;
    req.data_len = 2;

    net_buf_t out;
    _mk_buf(&out, g_buf_mem, TEST_BUF);
    assert_int_equal(dlt645_encode_slave_rsp(&req, &out), 0);

    dlt645_req_t dec;
    memset(&dec, 0, sizeof(dec));
    size_t consumed = 0;
    /* 只喂前 8 字节 */
    assert_int_equal(dlt645_decode_read_rsp(&dec, out.b.data, 8, &consumed), EDGE_ERR_INCOMPLETE);
}

/* 校验和错必须拒绝 */
static void test_bad_checksum(void** state) {
    (void)state;
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    req.di = 0x02010100;
    req.raw = 2200;
    req.data_len = 2;

    net_buf_t out;
    _mk_buf(&out, g_buf_mem, TEST_BUF);
    assert_int_equal(dlt645_encode_slave_rsp(&req, &out), 0);

    /* 篡改一个数据字节，CS 就对不上了 */
    out.b.data[14] ^= 0x01;

    dlt645_req_t dec;
    memset(&dec, 0, sizeof(dec));
    size_t consumed = 0;
    assert_int_equal(dlt645_decode_read_rsp(&dec, out.b.data, out.b.len, &consumed), -1);
}

/* 前导垃圾字节：报告跳过多少，让调用方能重新对齐 */
static void test_leading_garbage(void** state) {
    (void)state;
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    req.di = 0x02010100;
    req.raw = 2200;
    req.data_len = 2;

    net_buf_t out;
    _mk_buf(&out, g_buf_mem, TEST_BUF);
    assert_int_equal(dlt645_encode_slave_rsp(&req, &out), 0);

    uint8_t dirty[TEST_BUF];
    dirty[0] = 0xAA;
    dirty[1] = 0xBB;
    memcpy(&dirty[2], out.b.data, out.b.len);

    dlt645_req_t dec;
    memset(&dec, 0, sizeof(dec));
    size_t consumed = 0;
    assert_int_equal(dlt645_decode_read_rsp(&dec, dirty, out.b.len + 2, &consumed), 1);
    assert_int_equal(consumed, 2);

    /* 对齐后能正常解 */
    assert_int_equal(dlt645_decode_read_rsp(&dec, dirty + consumed, out.b.len, &consumed), 0);
    assert_int_equal(dec.raw, 2200);
}

/* req_prepare：点位与表地址都来自调用方，驱动不查表 */
static void test_req_prepare_uses_meta(void** state) {
    (void)state;
    uint8_t meta[3][6] = {{0}, {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}, {0}};
    dlt645_req_t priv;
    memset(&priv, 0, sizeof(priv));

    assert_int_equal(dlt645_driver.req_prepare(&priv, 1, 0x02010100, meta, 3), 0);
    assert_int_equal(priv.di, 0x02010100);
    assert_memory_equal(priv.addr_bcd, meta[1], 6);

    /* 越界或缺失 → 广播地址 */
    dlt645_req_t p2;
    memset(&p2, 0, sizeof(p2));
    assert_int_equal(dlt645_driver.req_prepare(&p2, 9, 0x02010100, meta, 3), 0);
    const uint8_t broadcast[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
    assert_memory_equal(p2.addr_bcd, broadcast, 6);
}

/* 驱动声明自己是 request-native，且私有数据尺寸在契约内 */
static void test_driver_declares_native(void** state) {
    (void)state;
    extern const edge_driver_t dlt645_driver;
    assert_non_null(dlt645_driver.req_prepare);
    assert_non_null(dlt645_driver.req_encode);
    assert_non_null(dlt645_driver.req_decode);
    assert_true(dlt645_driver.req_priv_size > 0);
    assert_true(dlt645_driver.req_priv_size <= REQ_DRV_PRIV_MAX);
    /* 不得再声明 legacy 槽位（互斥） */
    assert_null(dlt645_driver.encode_request);
    assert_null(dlt645_driver.decode_response);
    assert_null(dlt645_driver.handle_rx);
}


/* ==================== 从站面（S3-0） ====================
 * 驱动拿不到点位表、读不到影子表：值由注入的端口给。
 * 这里用 mock 端口验证"解请求 → 取数 → 组应答"这条链。 */

static int g_accept_called;
static uint32_t g_accept_di;
static double g_accept_value;

static int _mock_resolve(void* ctx, uint32_t point_id, edge_point_value_t* out) {
    (void)ctx;
    if (point_id != 0x02010100) return -1;   /* 只认电压这一点 */
    out->raw = 2200;
    out->width = 2;
    out->value = 220.0;
    return 0;
}

static int _mock_accept(void* ctx, uint32_t point_id, double value) {
    (void)ctx;
    g_accept_called++;
    g_accept_di = point_id;
    g_accept_value = value;
    return 0;
}

static void test_slave_read_uses_port(void** state) {
    (void)state;
    const uint8_t addr[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    memcpy(req.addr_bcd, addr, 6);
    req.di = 0x02010100;

    uint8_t qmem[64];
    net_buf_t q;
    net_buf_simple_init_with_data(&q.b, qmem, sizeof(qmem));
    assert_int_equal(dlt645_encode_read(&req, &q), 0);

    edge_slave_source_ops_t src = {.resolve = _mock_resolve, .accept = _mock_accept};
    uint8_t rmem[128];
    net_buf_t out;
    net_buf_simple_init_with_data(&out.b, rmem, sizeof(rmem));

    dlt645_req_t priv;
    memset(&priv, 0, sizeof(priv));
    size_t consumed = 0;
    int rc = dlt645_driver.req_slave_exchange(&priv, q.b.data, q.b.len, &src, NULL, &out, &consumed);
    assert_int_equal(rc, 0);
    assert_int_equal(consumed, q.b.len);

    /* 应答必须是 0x91，且值来自端口 */
    dlt645_req_t dec;
    memset(&dec, 0, sizeof(dec));
    size_t c2 = 0;
    assert_int_equal(dlt645_decode_read_rsp(&dec, out.b.data, out.b.len, &c2), 0);
    assert_int_equal(dec.di, 0x02010100);
    assert_int_equal(dec.raw, 2200);
    assert_int_equal(dec.data_len, 2);
}

/* 端口说没有这个点位 → 异常应答 0xD1，而不是 0 值 */
static void test_slave_read_unknown_point_is_abnormal(void** state) {
    (void)state;
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    req.di = 0x0A0B0C0D;   /* mock 不认 */

    uint8_t qmem[64];
    net_buf_t q;
    net_buf_simple_init_with_data(&q.b, qmem, sizeof(qmem));
    assert_int_equal(dlt645_encode_read(&req, &q), 0);

    edge_slave_source_ops_t src = {.resolve = _mock_resolve, .accept = _mock_accept};
    uint8_t rmem[128];
    net_buf_t out;
    net_buf_simple_init_with_data(&out.b, rmem, sizeof(rmem));

    dlt645_req_t priv;
    memset(&priv, 0, sizeof(priv));
    size_t consumed = 0;
    assert_int_equal(
        dlt645_driver.req_slave_exchange(&priv, q.b.data, q.b.len, &src, NULL, &out, &consumed), 0);

    assert_true(out.b.len >= 12);
    assert_int_equal(out.b.data[8], 0xD1);   /* 控制码 = 异常应答 */
}

/* 写请求 → 值交给端口，驱动只回 0x94 */
static void test_slave_write_goes_to_port(void** state) {
    (void)state;
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    req.di = 0x02010100;
    req.ctrl = DLT645_CTRL_WRITE;
    req.raw = 1234;
    req.data_len = 2;

    uint8_t qmem[64];
    net_buf_t q;
    net_buf_simple_init_with_data(&q.b, qmem, sizeof(qmem));
    assert_int_equal(dlt645_encode_write(&req, &q), 0);

    edge_slave_source_ops_t src = {.resolve = _mock_resolve, .accept = _mock_accept};
    uint8_t rmem[128];
    net_buf_t out;
    net_buf_simple_init_with_data(&out.b, rmem, sizeof(rmem));

    g_accept_called = 0;
    dlt645_req_t priv;
    memset(&priv, 0, sizeof(priv));
    size_t consumed = 0;
    assert_int_equal(
        dlt645_driver.req_slave_exchange(&priv, q.b.data, q.b.len, &src, NULL, &out, &consumed), 0);

    assert_int_equal(g_accept_called, 1);
    assert_int_equal(g_accept_di, 0x02010100);
    assert_true(g_accept_value > 1233.0 && g_accept_value < 1235.0);
    assert_int_equal(out.b.data[8], 0x94);   /* 写应答 */
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_encode_read_bytes),
        cmocka_unit_test(test_slave_rsp_roundtrip),
        cmocka_unit_test(test_four_byte_quantity),
        cmocka_unit_test(test_incomplete_frame),
        cmocka_unit_test(test_bad_checksum),
        cmocka_unit_test(test_leading_garbage),
        cmocka_unit_test(test_req_prepare_uses_meta),
        cmocka_unit_test(test_driver_declares_native),
        cmocka_unit_test(test_slave_read_uses_port),
        cmocka_unit_test(test_slave_read_unknown_point_is_abnormal),
        cmocka_unit_test(test_slave_write_goes_to_port),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
