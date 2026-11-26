/* 2026-11-18 IEMS 补丁审查 P2 的回归测试。
 *  1. Modbus TCP FC16/异常帧编码不得依赖未检查的临时 net_buf（小池必崩）；
 *  2. ModbusRTUHiCRC 从站的请求解码与应答 CRC 必须是高字节先发。
 */

#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "edge_driver.h"
#include "modbus_codec.h"
#include "net_buf.h"

extern const edge_driver_t modbus_rtu_driver;
extern const edge_driver_t modbus_rtu_hicrc_driver;
extern const edge_driver_t modbus_tcp_driver;

static void _reset_priv(uint8_t *priv, size_t len) {
    memset(priv, 0, len);
}

static int _prepare(const edge_driver_t *drv, uint8_t *priv, uint32_t native_id, bool is_write,
                    double value) {
    return drv->req_prepare(priv, 1, native_id, NULL, 0, NULL, 0, is_write, value);
}

/* 写值必须有限且在寄存器范围内。修前 prepare 只检查 quantity > 2，NaN/Inf
 * 及越界值的 double→uint 转换未定义，负值会回绕成 65535。 */
static void test_rtu_write_value_validation(void **state) {
    (void)state;
    alignas(max_align_t) uint8_t priv[256];
    const double bad_fc06[] = {NAN, INFINITY, -INFINITY, -1.0, 65536.0};
    const double bad_fc16[] = {-1.0, 4294967296.0};

    for (size_t i = 0; i < sizeof(bad_fc06) / sizeof(bad_fc06[0]); i++) {
        _reset_priv(priv, sizeof(priv));
        assert_int_equal(_prepare(&modbus_rtu_driver, priv, MODBUS_NATIVE(5, 1), true, bad_fc06[i]),
                         -1);
    }
    for (size_t i = 0; i < sizeof(bad_fc16) / sizeof(bad_fc16[0]); i++) {
        _reset_priv(priv, sizeof(priv));
        assert_int_equal(_prepare(&modbus_rtu_driver, priv, MODBUS_NATIVE(5, 2), true, bad_fc16[i]),
                         -1);
    }

    /* 边界值仍在合法范围；范围内小数保持既有截断语义（1.9 → 1） */
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_rtu_driver, priv, MODBUS_NATIVE(5, 1), true, 65535.0), 0);
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_rtu_driver, priv, MODBUS_NATIVE(5, 1), true, 1.9), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(modbus_rtu_driver.req_encode(priv, NULL, out), 0);
    assert_int_equal(out->b.len, 8);
    assert_int_equal(out->b.data[4], 0x00);
    assert_int_equal(out->b.data[5], 0x01);
    net_buf_unref(out);

    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_rtu_driver, priv, MODBUS_NATIVE(5, 2), true, 4294967295.0), 0);
    net_buf_pool_destroy(&pool);
}

static void test_tcp_write_value_validation(void **state) {
    (void)state;
    alignas(max_align_t) uint8_t priv[256];
    const double bad_fc06[] = {NAN, INFINITY, -INFINITY, -1.0, 65536.0};
    const double bad_fc16[] = {-1.0, 4294967296.0};

    for (size_t i = 0; i < sizeof(bad_fc06) / sizeof(bad_fc06[0]); i++) {
        _reset_priv(priv, sizeof(priv));
        assert_int_equal(_prepare(&modbus_tcp_driver, priv, MODBUS_NATIVE(5, 1), true, bad_fc06[i]),
                         -1);
    }
    for (size_t i = 0; i < sizeof(bad_fc16) / sizeof(bad_fc16[0]); i++) {
        _reset_priv(priv, sizeof(priv));
        assert_int_equal(_prepare(&modbus_tcp_driver, priv, MODBUS_NATIVE(5, 2), true, bad_fc16[i]),
                         -1);
    }

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_tcp_driver, priv, MODBUS_NATIVE(5, 1), true, 1.9), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(modbus_tcp_driver.req_encode(priv, NULL, out), 0);
    assert_int_equal(out->b.len, 12); /* MBAP 7 + FC06 PDU 5 */
    assert_int_equal(out->b.data[10], 0x00);
    assert_int_equal(out->b.data[11], 0x01);
    net_buf_unref(out);

    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_tcp_driver, priv, MODBUS_NATIVE(5, 2), true, 4294967295.0), 0);
    net_buf_pool_destroy(&pool);
}

/* HiCRC 从站：同一高字节先发的请求必须被接受，应答 CRC 也是高字节先发。
 * 修前 hicrc 驱动复用标准从站交换，按低字节先发校验请求 → 直接丢弃。 */
static void test_rtu_hicrc_slave_accepts_hi_crc(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 8, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(modbus_rtu_hicrc_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                         0, false, 0.0),
                     0);

    net_buf_t *req = net_buf_alloc(&pool, 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(req);
    assert_non_null(out);
    assert_int_equal(modbus_rtu_encode_read_ex(1, MODBUS_FC_READ_HOLDING_REGS, 5, 1, true, req), 0);

    size_t consumed = 0;
    assert_int_equal(modbus_rtu_hicrc_driver.req_slave_exchange(priv, NULL, req->b.data, req->b.len,
                                                                NULL, NULL, out, &consumed),
                     0);
    assert_int_equal(consumed, req->b.len);
    assert_int_equal(out->b.len, 7);
    assert_int_equal(out->b.data[0], 1);
    assert_int_equal(out->b.data[1], MODBUS_FC_READ_HOLDING_REGS);
    assert_int_equal(out->b.data[2], 2);
    uint16_t crc = modbus_calculate_crc(out->b.data, 5);
    assert_int_equal(out->b.data[5], (uint8_t)(crc >> 8));
    assert_int_equal(out->b.data[6], (uint8_t)(crc & 0xFF));

    net_buf_unref(req);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 标准从站不能接受高序帧，也不能按高序回帧；同一份标准帧必须照常工作。 */
static void test_rtu_std_slave_crc_semantics(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 8, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);

    net_buf_t *hi = net_buf_alloc(&pool, 0);
    net_buf_t *out_hi = net_buf_alloc(&pool, 0);
    net_buf_t *std = net_buf_alloc(&pool, 0);
    net_buf_t *out_std = net_buf_alloc(&pool, 0);
    assert_non_null(hi);
    assert_non_null(out_hi);
    assert_non_null(std);
    assert_non_null(out_std);

    assert_int_equal(modbus_rtu_encode_read_ex(1, MODBUS_FC_READ_HOLDING_REGS, 5, 1, true, hi), 0);
    size_t consumed = 0;
    assert_int_equal(modbus_rtu_driver.req_slave_exchange(priv, NULL, hi->b.data, hi->b.len, NULL,
                                                          NULL, out_hi, &consumed),
                     EDGE_ERR_INCOMPLETE);
    assert_int_equal(consumed, 1);
    assert_int_equal(out_hi->b.len, 0);

    assert_int_equal(modbus_rtu_encode_read(1, MODBUS_FC_READ_HOLDING_REGS, 5, 1, std), 0);
    assert_int_equal(modbus_rtu_driver.req_slave_exchange(priv, NULL, std->b.data, std->b.len, NULL,
                                                          NULL, out_std, &consumed),
                     0);
    assert_int_equal(out_std->b.len, 7);
    uint16_t crc = modbus_calculate_crc(out_std->b.data, 5);
    assert_int_equal(out_std->b.data[5], (uint8_t)(crc & 0xFF));
    assert_int_equal(out_std->b.data[6], (uint8_t)(crc >> 8));

    net_buf_unref(hi);
    net_buf_unref(out_hi);
    net_buf_unref(std);
    net_buf_unref(out_std);
    net_buf_pool_destroy(&pool);
}

/* 异常应答也是应答：HiCRC 从站的异常帧 CRC 必须同样是高字节先发。 */
static void test_rtu_hicrc_slave_exception_uses_hi_crc(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(modbus_rtu_hicrc_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                         0, false, 0.0),
                     0);

    net_buf_t *req = net_buf_alloc(&pool, 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(req);
    assert_non_null(out);
    /* count=0 触发 ILLEGAL_DATA_VALUE 异常应答 */
    assert_int_equal(modbus_rtu_encode_read_ex(1, MODBUS_FC_READ_HOLDING_REGS, 5, 0, true, req), 0);

    size_t consumed = 0;
    assert_int_equal(modbus_rtu_hicrc_driver.req_slave_exchange(priv, NULL, req->b.data, req->b.len,
                                                                NULL, NULL, out, &consumed),
                     0);
    assert_int_equal(out->b.len, 5);
    assert_int_equal(out->b.data[0], 1);
    assert_int_equal(out->b.data[1], (uint8_t)(MODBUS_FC_READ_HOLDING_REGS | 0x80));
    assert_int_equal(out->b.data[2], MODBUS_EX_ILLEGAL_DATA_VALUE);
    uint16_t crc = modbus_calculate_crc(out->b.data, 3);
    assert_int_equal(out->b.data[3], (uint8_t)(crc >> 8));
    assert_int_equal(out->b.data[4], (uint8_t)(crc & 0xFF));

    net_buf_unref(req);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 正常池下直接编码必须逐字节等同旧帧（MBAP length 从 unit id 起算）。 */
static void test_tcp_fc16_encodes_full_frame(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    uint16_t regs[2] = {0x1234, 0x5678};
    assert_int_equal(modbus_tcp_encode_write_multiple(0x2A, 1, 5, 2, regs, out), 0);
    static const uint8_t expect[17] = {0x00, 0x2A, 0x00, 0x00, 0x00, 0x0B, 1,
                                       0x10, 0x00, 0x05, 0x00, 0x02, 0x04, 0x12,
                                       0x34, 0x56, 0x78};
    assert_int_equal(out->b.len, sizeof(expect));
    assert_memory_equal(out->b.data, expect, sizeof(expect));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_tcp_exception_encodes_full_frame(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    assert_int_equal(modbus_tcp_encode_exception(0x2A, 1, MODBUS_FC_READ_HOLDING_REGS,
                                                 MODBUS_EX_ILLEGAL_DATA_VALUE, out),
                     0);
    static const uint8_t expect[9] = {0x00, 0x2A, 0x00, 0x00, 0x00, 0x03, 1, 0x83,
                                      MODBUS_EX_ILLEGAL_DATA_VALUE};
    assert_int_equal(out->b.len, sizeof(expect));
    assert_memory_equal(out->b.data, expect, sizeof(expect));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 1 个 buffer 的小池：out 已占满池，FC16 编码若再分配临时 PDU 会拿到 NULL
 * 并解引用。修前这里段错误；修后必须返回 -1 且不写 out。 */
static void test_tcp_fc16_tiny_pool_returns_error(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 1, 80, 0, NULL, NULL), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    uint16_t regs[2] = {0x1234, 0x5678};
    assert_int_equal(modbus_tcp_encode_write_multiple(0x2A, 1, 5, 2, regs, out), -1);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_tcp_exception_tiny_pool_returns_error(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 1, 72, 0, NULL, NULL), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    assert_int_equal(modbus_tcp_encode_exception(0x2A, 1, MODBUS_FC_READ_HOLDING_REGS,
                                                 MODBUS_EX_ILLEGAL_DATA_VALUE, out),
                     -1);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 读请求/写单寄存器/写应答旧实现都先分配临时 PDU：池里只剩 out 一个 buffer
 * 时 net_buf_alloc 返回 NULL，net_buf_add 直接解引用。修后共用一条
 * tailroom 检查，返回 -1 且不写 out。 */
static void test_tcp_read_tiny_pool_returns_error(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 1, 72, 0, NULL, NULL), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    assert_int_equal(modbus_tcp_encode_read(0x2A, 1, MODBUS_FC_READ_HOLDING_REGS, 5, 1, out), -1);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_tcp_write_single_tiny_pool_returns_error(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 1, 72, 0, NULL, NULL), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    assert_int_equal(modbus_tcp_encode_write_single(0x2A, 1, 5, 0x1234, out), -1);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_tcp_write_resp_tiny_pool_returns_error(void **state) {
    (void)state;

    /* FC06 应答就是请求回显：正常池下逐字节等于写单寄存器帧 */
    net_buf_pool_t ok_pool;
    assert_int_equal(net_buf_pool_init(&ok_pool, 2, 256, 0, NULL, NULL), 0);
    net_buf_t *ok = net_buf_alloc(&ok_pool, 0);
    assert_non_null(ok);
    assert_int_equal(modbus_tcp_encode_write_resp(0x2A, 1, 5, 0x1234, ok), 0);
    static const uint8_t expect[12] = {0x00, 0x2A, 0x00, 0x00, 0x00, 0x06,
                                       1,    0x06, 0x00, 0x05, 0x12, 0x34};
    assert_int_equal(ok->b.len, sizeof(expect));
    assert_memory_equal(ok->b.data, expect, sizeof(expect));
    net_buf_unref(ok);
    net_buf_pool_destroy(&ok_pool);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 1, 72, 0, NULL, NULL), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    assert_int_equal(modbus_tcp_encode_write_resp(0x2A, 1, 5, 0x1234, out), -1);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 1 个 buffer 的小池：out 只有 8 字节 tailroom（net_buf_alloc 预留 64 字节头），
 * FC16 写 2 个寄存器要 13 字节。修前 net_buf_add 断言/越界；修后返回 -1 且不写 out。 */
static void test_rtu_fc16_tiny_pool_returns_error(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 1, 72, 0, NULL, NULL), 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    uint16_t regs[2] = {0x1234, 0x5678};
    assert_int_equal(modbus_rtu_encode_write_multiple_ex(1, 5, 2, regs, false, out), -1);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static int _accept_ok(void *ctx, uint32_t point_id, double value) {
    (void)ctx;
    (void)point_id;
    (void)value;
    return 0;
}

static int _accept_reject(void *ctx, uint32_t point_id, double value) {
    (void)ctx;
    (void)point_id;
    (void)value;
    return -1;
}

static const edge_slave_source_ops_t k_accept_ok = {.accept = _accept_ok};
static const edge_slave_source_ops_t k_accept_reject = {.accept = _accept_reject};

/* 接受失败不能回显：FC06 的回显就是"成功"，FC16 的应答也只确认收下，
 * 主站据此会以为写生效了。端口拒绝时必须回异常应答。 */
static void test_rtu_slave_exception_when_accept_fails(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 8, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    net_buf_t *req = net_buf_alloc(&pool, 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(req);
    assert_non_null(out);
    size_t consumed = 0;

    /* FC06：端口拒绝 → 异常应答（地址不可写） */
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_rtu_driver, priv, MODBUS_NATIVE(5, 1), true, 20.0), 0);
    assert_int_equal(modbus_rtu_encode_write_single_ex(1, 5, 20, false, req), 0);
    assert_int_equal(modbus_rtu_driver.req_slave_exchange(priv, NULL, req->b.data, req->b.len,
                                                          &k_accept_reject, NULL, out, &consumed),
                     0);
    assert_int_equal(out->b.len, 5);
    assert_int_equal(out->b.data[1], (uint8_t)(MODBUS_FC_WRITE_SINGLE_REG | 0x80));
    assert_int_equal(out->b.data[2], MODBUS_EX_ILLEGAL_DATA_ADDRESS);
    uint16_t crc = modbus_calculate_crc(out->b.data, 3);
    assert_int_equal(out->b.data[3], (uint8_t)(crc & 0xFF));
    assert_int_equal(out->b.data[4], (uint8_t)(crc >> 8));

    /* 端口接受 → 仍是 FC06 回显 */
    net_buf_simple_reset(&out->b);
    assert_int_equal(modbus_rtu_driver.req_slave_exchange(priv, NULL, req->b.data, req->b.len,
                                                          &k_accept_ok, NULL, out, &consumed),
                     0);
    assert_int_equal(out->b.len, 8);
    assert_memory_equal(out->b.data, req->b.data, 8);

    /* FC16 同理 */
    net_buf_simple_reset(&req->b);
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_rtu_driver, priv, MODBUS_NATIVE(5, 2), true, 42.0), 0);
    uint16_t regs[2] = {0, 42};
    assert_int_equal(modbus_rtu_encode_write_multiple_ex(1, 5, 2, regs, false, req), 0);
    net_buf_simple_reset(&out->b);
    assert_int_equal(modbus_rtu_driver.req_slave_exchange(priv, NULL, req->b.data, req->b.len,
                                                          &k_accept_reject, NULL, out, &consumed),
                     0);
    assert_int_equal(out->b.len, 5);
    assert_int_equal(out->b.data[1], (uint8_t)(MODBUS_FC_WRITE_MULTIPLE_REGS | 0x80));
    assert_int_equal(out->b.data[2], MODBUS_EX_ILLEGAL_DATA_ADDRESS);

    net_buf_unref(req);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* TCP 从站与 RTU 同一条规则：accept 失败回 MBAP 异常帧，不再发写应答。 */
static void test_tcp_slave_exception_when_accept_fails(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 8, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    net_buf_t *req = net_buf_alloc(&pool, 0);
    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(req);
    assert_non_null(out);
    size_t consumed = 0;

    /* FC06 拒绝 */
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_tcp_driver, priv, MODBUS_NATIVE(5, 1), true, 20.0), 0);
    assert_int_equal(modbus_tcp_encode_write_single(0x2A, 1, 5, 20, req), 0);
    assert_int_equal(modbus_tcp_driver.req_slave_exchange(priv, NULL, req->b.data, req->b.len,
                                                          &k_accept_reject, NULL, out, &consumed),
                     0);
    static const uint8_t expect_fc06[9] = {0x00, 0x2A, 0x00, 0x00, 0x00, 0x03,
                                          1,    0x86, MODBUS_EX_ILLEGAL_DATA_ADDRESS};
    assert_int_equal(out->b.len, sizeof(expect_fc06));
    assert_memory_equal(out->b.data, expect_fc06, sizeof(expect_fc06));

    /* FC06 接受 → 写应答 */
    net_buf_simple_reset(&out->b);
    assert_int_equal(modbus_tcp_driver.req_slave_exchange(priv, NULL, req->b.data, req->b.len,
                                                          &k_accept_ok, NULL, out, &consumed),
                     0);
    assert_int_equal(out->b.len, 12);
    assert_int_equal(out->b.data[7], MODBUS_FC_WRITE_SINGLE_REG);

    /* FC16 拒绝 */
    net_buf_simple_reset(&req->b);
    _reset_priv(priv, sizeof(priv));
    assert_int_equal(_prepare(&modbus_tcp_driver, priv, MODBUS_NATIVE(5, 2), true, 42.0), 0);
    uint16_t regs[2] = {0, 42};
    assert_int_equal(modbus_tcp_encode_write_multiple(0x2A, 1, 5, 2, regs, req), 0);
    net_buf_simple_reset(&out->b);
    assert_int_equal(modbus_tcp_driver.req_slave_exchange(priv, NULL, req->b.data, req->b.len,
                                                          &k_accept_reject, NULL, out, &consumed),
                     0);
    static const uint8_t expect_fc16[9] = {0x00, 0x2A, 0x00, 0x00, 0x00, 0x03,
                                          1,    0x90, MODBUS_EX_ILLEGAL_DATA_ADDRESS};
    assert_int_equal(out->b.len, sizeof(expect_fc16));
    assert_memory_equal(out->b.data, expect_fc16, sizeof(expect_fc16));

    net_buf_unref(req);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_rtu_write_value_validation),
        cmocka_unit_test(test_tcp_write_value_validation),
        cmocka_unit_test(test_rtu_hicrc_slave_accepts_hi_crc),
        cmocka_unit_test(test_rtu_std_slave_crc_semantics),
        cmocka_unit_test(test_rtu_hicrc_slave_exception_uses_hi_crc),
        cmocka_unit_test(test_rtu_slave_exception_when_accept_fails),
        cmocka_unit_test(test_tcp_slave_exception_when_accept_fails),
        cmocka_unit_test(test_tcp_fc16_encodes_full_frame),
        cmocka_unit_test(test_tcp_exception_encodes_full_frame),
        cmocka_unit_test(test_tcp_fc16_tiny_pool_returns_error),
        cmocka_unit_test(test_rtu_fc16_tiny_pool_returns_error),
        cmocka_unit_test(test_tcp_exception_tiny_pool_returns_error),
        cmocka_unit_test(test_tcp_read_tiny_pool_returns_error),
        cmocka_unit_test(test_tcp_write_single_tiny_pool_returns_error),
        cmocka_unit_test(test_tcp_write_resp_tiny_pool_returns_error),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
