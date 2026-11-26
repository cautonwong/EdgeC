/* P3-00c：Modbus 写描述（类型 + 字序）与多寄存器 FC16 的帧向量。
 *
 * 期望帧不调用被测编码器生成：类型/字序 → 寄存器序列按 Modbus 与 v3
 * port_transf.c SET_INT/ORDER_* 的定义手写，再逐字节比较。
 */

#include <cmocka.h>
#include <float.h>
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

typedef struct {
    const char *name;
    uint8_t type;
    uint8_t order;
    uint16_t quantity;
    double value;
    uint16_t regs[4];
} write_vector_t;

/* 0x123456789ABCD 与 0x12345678 都在 double 精确表示范围内（< 2^53）。 */
static const write_vector_t k_vectors[] = {
    {"u16/ABCD", MODBUS_WRITE_U16, MODBUS_WORD_ORDER_ABCD, 1, 0x1234, {0x1234}},
    {"u16/CDAB", MODBUS_WRITE_U16, MODBUS_WORD_ORDER_CDAB, 1, 0x1234, {0x1234}},
    {"u16/BADC", MODBUS_WRITE_U16, MODBUS_WORD_ORDER_BADC, 1, 0x1234, {0x3412}},
    {"u16/DCBA", MODBUS_WRITE_U16, MODBUS_WORD_ORDER_DCBA, 1, 0x1234, {0x3412}},
    {"i16/ABCD", MODBUS_WRITE_I16, MODBUS_WORD_ORDER_ABCD, 1, -2, {0xFFFE}},
    {"i16/CDAB", MODBUS_WRITE_I16, MODBUS_WORD_ORDER_CDAB, 1, -2, {0xFFFE}},
    {"i16/BADC", MODBUS_WRITE_I16, MODBUS_WORD_ORDER_BADC, 1, -2, {0xFEFF}},
    {"i16/DCBA", MODBUS_WRITE_I16, MODBUS_WORD_ORDER_DCBA, 1, -2, {0xFEFF}},

    {"u32/ABCD", MODBUS_WRITE_U32, MODBUS_WORD_ORDER_ABCD, 2, 0x12345678, {0x1234, 0x5678}},
    {"u32/CDAB", MODBUS_WRITE_U32, MODBUS_WORD_ORDER_CDAB, 2, 0x12345678, {0x5678, 0x1234}},
    {"u32/BADC", MODBUS_WRITE_U32, MODBUS_WORD_ORDER_BADC, 2, 0x12345678, {0x3412, 0x7856}},
    {"u32/DCBA", MODBUS_WRITE_U32, MODBUS_WORD_ORDER_DCBA, 2, 0x12345678, {0x7856, 0x3412}},
    {"i32/ABCD", MODBUS_WRITE_I32, MODBUS_WORD_ORDER_ABCD, 2, -2, {0xFFFF, 0xFFFE}},
    {"i32/CDAB", MODBUS_WRITE_I32, MODBUS_WORD_ORDER_CDAB, 2, -2, {0xFFFE, 0xFFFF}},
    {"i32/BADC", MODBUS_WRITE_I32, MODBUS_WORD_ORDER_BADC, 2, -2, {0xFFFF, 0xFEFF}},
    {"i32/DCBA", MODBUS_WRITE_I32, MODBUS_WORD_ORDER_DCBA, 2, -2, {0xFEFF, 0xFFFF}},
    {"f32/ABCD", MODBUS_WRITE_F32, MODBUS_WORD_ORDER_ABCD, 2, 1.5, {0x3FC0, 0x0000}},
    {"f32/CDAB", MODBUS_WRITE_F32, MODBUS_WORD_ORDER_CDAB, 2, 1.5, {0x0000, 0x3FC0}},
    {"f32/BADC", MODBUS_WRITE_F32, MODBUS_WORD_ORDER_BADC, 2, 1.5, {0xC03F, 0x0000}},
    {"f32/DCBA", MODBUS_WRITE_F32, MODBUS_WORD_ORDER_DCBA, 2, 1.5, {0x0000, 0xC03F}},

    {"u64/ABCD", MODBUS_WRITE_U64, MODBUS_WORD_ORDER_ABCD, 4, 0x123456789ABCD,
     {0x0001, 0x2345, 0x6789, 0xABCD}},
    {"u64/CDAB", MODBUS_WRITE_U64, MODBUS_WORD_ORDER_CDAB, 4, 0x123456789ABCD,
     {0xABCD, 0x6789, 0x2345, 0x0001}},
    {"u64/BADC", MODBUS_WRITE_U64, MODBUS_WORD_ORDER_BADC, 4, 0x123456789ABCD,
     {0x0100, 0x4523, 0x8967, 0xCDAB}},
    {"u64/DCBA", MODBUS_WRITE_U64, MODBUS_WORD_ORDER_DCBA, 4, 0x123456789ABCD,
     {0xCDAB, 0x8967, 0x4523, 0x0100}},
    {"i64/ABCD", MODBUS_WRITE_I64, MODBUS_WORD_ORDER_ABCD, 4, -2,
     {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFE}},
    {"i64/CDAB", MODBUS_WRITE_I64, MODBUS_WORD_ORDER_CDAB, 4, -2,
     {0xFFFE, 0xFFFF, 0xFFFF, 0xFFFF}},
    {"i64/BADC", MODBUS_WRITE_I64, MODBUS_WORD_ORDER_BADC, 4, -2,
     {0xFFFF, 0xFFFF, 0xFFFF, 0xFEFF}},
    {"i64/DCBA", MODBUS_WRITE_I64, MODBUS_WORD_ORDER_DCBA, 4, -2,
     {0xFEFF, 0xFFFF, 0xFFFF, 0xFFFF}},
    {"f64/ABCD", MODBUS_WRITE_F64, MODBUS_WORD_ORDER_ABCD, 4, 1.5, {0x3FF8, 0, 0, 0}},
    {"f64/CDAB", MODBUS_WRITE_F64, MODBUS_WORD_ORDER_CDAB, 4, 1.5, {0, 0, 0, 0x3FF8}},
    {"f64/BADC", MODBUS_WRITE_F64, MODBUS_WORD_ORDER_BADC, 4, 1.5, {0xF83F, 0, 0, 0}},
    {"f64/DCBA", MODBUS_WRITE_F64, MODBUS_WORD_ORDER_DCBA, 4, 1.5, {0, 0, 0, 0xF83F}},
};

static void _prepare(const edge_driver_t *drv, uint8_t *priv, uint16_t quantity,
                     const modbus_write_desc_t *desc, double value, int expect) {
    memset(priv, 0, REQ_DRV_PRIV_MAX);
    assert_int_equal(drv->req_prepare(priv, 1, MODBUS_NATIVE(5, quantity), NULL, 0, desc,
                                      desc ? sizeof(*desc) : 0, true, value),
                     expect);
}

static void _check_vector(const edge_driver_t *drv, bool tcp, const write_vector_t *v) {
    print_message("vector %s (%s)\n", v->name, tcp ? "TCP" : "RTU");
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    modbus_write_desc_t desc = {.type = v->type, .order = v->order};
    _prepare(drv, priv, v->quantity, &desc, v->value, 0);

    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(drv->req_encode(priv, NULL, out), 0);

    if (v->quantity == 1) {
        if (tcp) {
            assert_int_equal(out->b.len, 12); /* MBAP 7 + FC06 PDU 5 */
            assert_int_equal(out->b.data[7], MODBUS_FC_WRITE_SINGLE_REG);
            assert_int_equal(out->b.data[8], 0);
            assert_int_equal(out->b.data[9], 5);
            assert_int_equal((out->b.data[10] << 8) | out->b.data[11], v->regs[0]);
        } else {
            assert_int_equal(out->b.len, 8);
            assert_int_equal(out->b.data[1], MODBUS_FC_WRITE_SINGLE_REG);
            assert_int_equal(out->b.data[2], 0);
            assert_int_equal(out->b.data[3], 5);
            assert_int_equal((out->b.data[4] << 8) | out->b.data[5], v->regs[0]);
        }
    } else {
        size_t reg_off = 0;
        if (tcp) {
            assert_int_equal(out->b.len, 13 + 2u * v->quantity);
            assert_int_equal(out->b.data[7], MODBUS_FC_WRITE_MULTIPLE_REGS);
            assert_int_equal(out->b.data[8], 0);
            assert_int_equal(out->b.data[9], 5);
            assert_int_equal((out->b.data[10] << 8) | out->b.data[11], v->quantity);
            assert_int_equal(out->b.data[12], 2 * v->quantity);
            assert_int_equal((out->b.data[4] << 8) | out->b.data[5], 7 + 2 * v->quantity);
            reg_off = 13;
        } else {
            assert_int_equal(out->b.len, 9 + 2u * v->quantity);
            assert_int_equal(out->b.data[1], MODBUS_FC_WRITE_MULTIPLE_REGS);
            assert_int_equal(out->b.data[2], 0);
            assert_int_equal(out->b.data[3], 5);
            assert_int_equal((out->b.data[4] << 8) | out->b.data[5], v->quantity);
            assert_int_equal(out->b.data[6], 2 * v->quantity);
            uint16_t crc = modbus_calculate_crc(out->b.data, out->b.len - 2);
            assert_int_equal(out->b.data[out->b.len - 2], (uint8_t)(crc & 0xFF));
            assert_int_equal(out->b.data[out->b.len - 1], (uint8_t)(crc >> 8));
            reg_off = 7;
        }
        for (uint16_t i = 0; i < v->quantity; i++) {
            uint16_t got = (uint16_t)((out->b.data[reg_off + 2 * i] << 8) |
                                      out->b.data[reg_off + 2 * i + 1]);
            assert_int_equal(got, v->regs[i]);
        }
    }

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void test_rtu_write_desc_frame_vectors(void **state) {
    (void)state;
    for (size_t i = 0; i < sizeof(k_vectors) / sizeof(k_vectors[0]); i++) {
        _check_vector(&modbus_rtu_driver, false, &k_vectors[i]);
    }
}

static void test_tcp_write_desc_frame_vectors(void **state) {
    (void)state;
    for (size_t i = 0; i < sizeof(k_vectors) / sizeof(k_vectors[0]); i++) {
        _check_vector(&modbus_tcp_driver, true, &k_vectors[i]);
    }
}

/* HiCRC 变体只换 CRC 两字节顺序，写描述与寄存器负载不变 */
static void test_hicrc_write_desc_frame_vector(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    modbus_write_desc_t desc = {.type = MODBUS_WRITE_U64, .order = MODBUS_WORD_ORDER_CDAB};
    _prepare(&modbus_rtu_hicrc_driver, priv, 4, &desc, 0x123456789ABCD, 0);

    net_buf_t *out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(modbus_rtu_hicrc_driver.req_encode(priv, NULL, out), 0);
    assert_int_equal(out->b.len, 17);
    assert_int_equal(out->b.data[1], MODBUS_FC_WRITE_MULTIPLE_REGS);
    static const uint16_t expect[4] = {0xABCD, 0x6789, 0x2345, 0x0001};
    for (int i = 0; i < 4; i++) {
        assert_int_equal((out->b.data[7 + 2 * i] << 8) | out->b.data[8 + 2 * i], expect[i]);
    }
    uint16_t crc = modbus_calculate_crc(out->b.data, 15);
    assert_int_equal(out->b.data[15], (uint8_t)(crc >> 8));
    assert_int_equal(out->b.data[16], (uint8_t)(crc & 0xFF));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 旧默认必须原样保留：无描述时 qty1→u16 FC06、qty2→u32 ABCD FC16，其余拒绝。 */
static void test_default_contract_without_desc(void **state) {
    (void)state;
    const edge_driver_t *drivers[] = {&modbus_rtu_driver, &modbus_tcp_driver};
    for (size_t d = 0; d < 2; d++) {
        alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
        _prepare(drivers[d], priv, 1, NULL, 0x1234, 0);
        net_buf_pool_t pool;
        assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
        net_buf_t *out = net_buf_alloc(&pool, 0);
        assert_int_equal(drivers[d]->req_encode(priv, NULL, out), 0);
        assert_int_equal(drivers[d] == &modbus_rtu_driver ? out->b.data[1] : out->b.data[7],
                         MODBUS_FC_WRITE_SINGLE_REG);
        net_buf_unref(out);

        _prepare(drivers[d], priv, 2, NULL, 0x12345678, 0);
        out = net_buf_alloc(&pool, 0);
        assert_int_equal(drivers[d]->req_encode(priv, NULL, out), 0);
        assert_int_equal(drivers[d] == &modbus_rtu_driver ? out->b.data[1] : out->b.data[7],
                         MODBUS_FC_WRITE_MULTIPLE_REGS);
        net_buf_unref(out);

        _prepare(drivers[d], priv, 4, NULL, 1.0, -1);
        _prepare(drivers[d], priv, 3, NULL, 1.0, -1);
        net_buf_pool_destroy(&pool);
    }
}

/* quantity=0 由 native_id 归一为 1，走 FC06；超过 MB_MAX_REGS 的 33 在
 * 描述校验之前就拒绝；32 个寄存器只能由线级编码器产生（本轮产品写类型
 * 最宽 4 寄存器，块写留到 P0-03d 之后）。 */
static void test_quantity_boundaries(void **state) {
    (void)state;
    const edge_driver_t *drivers[] = {&modbus_rtu_driver, &modbus_tcp_driver};
    for (size_t d = 0; d < 2; d++) {
        alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
        modbus_write_desc_t u32_desc = {.type = MODBUS_WRITE_U32, .order = MODBUS_WORD_ORDER_ABCD};

        _prepare(drivers[d], priv, 0, NULL, 65535.0, 0); /* 0 → 1 个寄存器 */
        net_buf_pool_t pool;
        assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
        net_buf_t *out = net_buf_alloc(&pool, 0);
        assert_int_equal(drivers[d]->req_encode(priv, NULL, out), 0);
        assert_int_equal(drivers[d] == &modbus_rtu_driver ? out->b.data[1] : out->b.data[7],
                         MODBUS_FC_WRITE_SINGLE_REG);
        net_buf_unref(out);

        _prepare(drivers[d], priv, 33, NULL, 1.0, -1);       /* > MB_MAX_REGS */
        _prepare(drivers[d], priv, 32, &u32_desc, 1.0, -1);  /* 类型宽度不符 */
        _prepare(drivers[d], priv, 5, NULL, 1.0, -1);
        net_buf_pool_destroy(&pool);
    }

    /* 线级编码器：32 个寄存器编码不越界，0 与超过 Modbus 上限的 124 拒绝 */
    uint16_t values[32];
    for (int i = 0; i < 32; i++) values[i] = (uint16_t)(0x100 + i);

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 8, 256, 0, NULL, NULL), 0);
    net_buf_t *rtu = net_buf_alloc(&pool, 0);
    assert_non_null(rtu);
    assert_int_equal(modbus_rtu_encode_write_multiple_ex(1, 5, 32, values, false, rtu), 0);
    assert_int_equal(rtu->b.len, 9 + 64);
    assert_int_equal(rtu->b.data[4], 0);
    assert_int_equal(rtu->b.data[5], 32);
    assert_int_equal(rtu->b.data[6], 64);
    assert_int_equal(rtu->b.data[7], 0x01);
    assert_int_equal(rtu->b.data[8], 0x00);
    assert_int_equal(rtu->b.data[7 + 62], 0x01);
    assert_int_equal(rtu->b.data[7 + 63], 0x1F);
    net_buf_simple_reset(&rtu->b);
    assert_int_equal(modbus_rtu_encode_write_multiple_ex(1, 5, 0, values, false, rtu), -1);
    assert_int_equal(modbus_rtu_encode_write_multiple_ex(1, 5, 124, values, false, rtu), -1);
    net_buf_unref(rtu);

    net_buf_t *tcp = net_buf_alloc(&pool, 0);
    assert_non_null(tcp);
    assert_int_equal(modbus_tcp_encode_write_multiple(0x2A, 1, 5, 32, values, tcp), 0);
    assert_int_equal(tcp->b.len, 13 + 64);
    assert_int_equal(tcp->b.data[10], 0);
    assert_int_equal(tcp->b.data[11], 32);
    assert_int_equal(tcp->b.data[12], 64);
    assert_int_equal(tcp->b.data[13], 0x01);
    assert_int_equal(tcp->b.data[14], 0x00);
    net_buf_simple_reset(&tcp->b);
    assert_int_equal(modbus_tcp_encode_write_multiple(0x2A, 1, 5, 0, values, tcp), -1);
    assert_int_equal(modbus_tcp_encode_write_multiple(0x2A, 1, 5, 124, values, tcp), -1);
    net_buf_unref(tcp);
    net_buf_pool_destroy(&pool);
}

/* 描述尺寸、未知类型/字序、宽度不符、非有限与越界值都必须拒绝 */
static void test_write_desc_rejects_bad_input(void **state) {
    (void)state;
    const edge_driver_t *drivers[] = {&modbus_rtu_driver, &modbus_tcp_driver};
    const modbus_write_desc_t u16_desc = {.type = MODBUS_WRITE_U16, .order = MODBUS_WORD_ORDER_ABCD};
    const modbus_write_desc_t u32_desc = {.type = MODBUS_WRITE_U32, .order = MODBUS_WORD_ORDER_ABCD};
    const modbus_write_desc_t u64_desc = {.type = MODBUS_WRITE_U64, .order = MODBUS_WORD_ORDER_ABCD};
    const modbus_write_desc_t f32_desc = {.type = MODBUS_WRITE_F32, .order = MODBUS_WORD_ORDER_ABCD};
    const modbus_write_desc_t f64_desc = {.type = MODBUS_WRITE_F64, .order = MODBUS_WORD_ORDER_ABCD};
    const modbus_write_desc_t bad_type = {.type = 0xFF, .order = MODBUS_WORD_ORDER_ABCD};
    const modbus_write_desc_t bad_order = {.type = MODBUS_WRITE_U16, .order = 0xFF};

    for (size_t d = 0; d < 2; d++) {
        alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];

        /* 描述长度错误、半截描述 */
        memset(priv, 0, sizeof(priv));
        assert_int_equal(drivers[d]->req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, &u16_desc,
                                                 sizeof(u16_desc) - 1, true, 1.0),
                         -1);
        memset(priv, 0, sizeof(priv));
        assert_int_equal(drivers[d]->req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, &u16_desc,
                                                 sizeof(u16_desc) + 1, true, 1.0),
                         -1);
        memset(priv, 0, sizeof(priv));
        assert_int_equal(drivers[d]->req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                 sizeof(u16_desc), true, 1.0),
                         -1);

        /* 未知类型 / 未知字序 */
        _prepare(drivers[d], priv, 1, &bad_type, 1.0, -1);
        _prepare(drivers[d], priv, 1, &bad_order, 1.0, -1);

        /* 类型宽度与 quantity 不符 */
        _prepare(drivers[d], priv, 1, &u32_desc, 1.0, -1);
        _prepare(drivers[d], priv, 2, &u16_desc, 1.0, -1);
        _prepare(drivers[d], priv, 4, &f32_desc, 1.0, -1);
        _prepare(drivers[d], priv, 2, &u64_desc, 1.0, -1);

        /* 非有限值 */
        const double nonfinite[] = {NAN, INFINITY, -INFINITY};
        for (size_t i = 0; i < 3; i++) {
            _prepare(drivers[d], priv, 1, &u16_desc, nonfinite[i], -1);
            _prepare(drivers[d], priv, 2, &f32_desc, nonfinite[i], -1);
            _prepare(drivers[d], priv, 4, &f64_desc, nonfinite[i], -1);
        }

        /* 越界值：整数在 double 域先判边界，超出就拒绝，不回绕 */
        _prepare(drivers[d], priv, 1, &u16_desc, -1.0, -1);
        _prepare(drivers[d], priv, 1, &u16_desc, 65536.0, -1);
        _prepare(drivers[d], priv, 1, &(modbus_write_desc_t){MODBUS_WRITE_I16, MODBUS_WORD_ORDER_ABCD},
                 -32769.0, -1);
        _prepare(drivers[d], priv, 1, &(modbus_write_desc_t){MODBUS_WRITE_I16, MODBUS_WORD_ORDER_ABCD},
                 32768.0, -1);
        _prepare(drivers[d], priv, 2, &u32_desc, -1.0, -1);
        _prepare(drivers[d], priv, 2, &u32_desc, 4294967296.0, -1);
        _prepare(drivers[d], priv, 2,
                 &(modbus_write_desc_t){MODBUS_WRITE_I32, MODBUS_WORD_ORDER_ABCD}, -2147483649.0,
                 -1);
        _prepare(drivers[d], priv, 2,
                 &(modbus_write_desc_t){MODBUS_WRITE_I32, MODBUS_WORD_ORDER_ABCD}, 2147483648.0,
                 -1);
        _prepare(drivers[d], priv, 4, &u64_desc, -1.0, -1);
        _prepare(drivers[d], priv, 4, &u64_desc, 18446744073709551616.0, -1); /* 2^64 */
        _prepare(drivers[d], priv, 4,
                 &(modbus_write_desc_t){MODBUS_WRITE_I64, MODBUS_WORD_ORDER_ABCD}, -1e19, -1);
        _prepare(drivers[d], priv, 4,
                 &(modbus_write_desc_t){MODBUS_WRITE_I64, MODBUS_WORD_ORDER_ABCD},
                 9223372036854775808.0, -1); /* 2^63 */
        _prepare(drivers[d], priv, 2, &f32_desc, 1e39, -1); /* 有限但超出 float */
        _prepare(drivers[d], priv, 2, &f32_desc, -1e39, -1);
    }
}

/* 合法边界值必须接受；整数按 C 语义向零截断（与旧默认、v3 隐式转换一致） */
static void test_write_desc_accepts_boundaries(void **state) {
    (void)state;
    const edge_driver_t *drivers[] = {&modbus_rtu_driver, &modbus_tcp_driver};
    for (size_t d = 0; d < 2; d++) {
        alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
        _prepare(drivers[d], priv, 1, &(modbus_write_desc_t){MODBUS_WRITE_U16, MODBUS_WORD_ORDER_ABCD},
                 0.0, 0);
        _prepare(drivers[d], priv, 1, &(modbus_write_desc_t){MODBUS_WRITE_U16, MODBUS_WORD_ORDER_ABCD},
                 65535.0, 0);
        _prepare(drivers[d], priv, 1, &(modbus_write_desc_t){MODBUS_WRITE_I16, MODBUS_WORD_ORDER_ABCD},
                 -32768.0, 0);
        _prepare(drivers[d], priv, 1, &(modbus_write_desc_t){MODBUS_WRITE_I16, MODBUS_WORD_ORDER_ABCD},
                 32767.0, 0);
        _prepare(drivers[d], priv, 2, &(modbus_write_desc_t){MODBUS_WRITE_U32, MODBUS_WORD_ORDER_ABCD},
                 4294967295.0, 0);
        _prepare(drivers[d], priv, 2, &(modbus_write_desc_t){MODBUS_WRITE_I32, MODBUS_WORD_ORDER_ABCD},
                 -2147483648.0, 0);
        _prepare(drivers[d], priv, 2, &(modbus_write_desc_t){MODBUS_WRITE_I32, MODBUS_WORD_ORDER_ABCD},
                 2147483647.0, 0);
        _prepare(drivers[d], priv, 4, &(modbus_write_desc_t){MODBUS_WRITE_U64, MODBUS_WORD_ORDER_ABCD},
                 9007199254740992.0, 0); /* 2^53，double 仍精确 */
        _prepare(drivers[d], priv, 4, &(modbus_write_desc_t){MODBUS_WRITE_I64, MODBUS_WORD_ORDER_ABCD},
                 -9223372036854775808.0, 0); /* -2^63 */
        _prepare(drivers[d], priv, 2, &(modbus_write_desc_t){MODBUS_WRITE_F32, MODBUS_WORD_ORDER_ABCD},
                 FLT_MAX, 0);

        /* 1.9 截断为 1；-1.9 截断为 -1 */
        net_buf_pool_t pool;
        assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
        net_buf_t *out = net_buf_alloc(&pool, 0);
        _prepare(drivers[d], priv, 1,
                 &(modbus_write_desc_t){MODBUS_WRITE_U16, MODBUS_WORD_ORDER_ABCD}, 1.9, 0);
        assert_int_equal(drivers[d]->req_encode(priv, NULL, out), 0);
        const size_t val_off = drivers[d] == &modbus_rtu_driver ? 4 : 10;
        assert_int_equal((out->b.data[val_off] << 8) | out->b.data[val_off + 1], 1);
        net_buf_unref(out);

        out = net_buf_alloc(&pool, 0);
        _prepare(drivers[d], priv, 1,
                 &(modbus_write_desc_t){MODBUS_WRITE_I16, MODBUS_WORD_ORDER_ABCD}, -1.9, 0);
        assert_int_equal(drivers[d]->req_encode(priv, NULL, out), 0);
        assert_int_equal((out->b.data[val_off] << 8) | out->b.data[val_off + 1], 0xFFFF);
        net_buf_unref(out);
        net_buf_pool_destroy(&pool);
    }
}

/* 读请求忽略写描述：同一份描述（甚至尺寸不对）不能影响读帧 */
static void test_read_ignores_write_desc(void **state) {
    (void)state;
    const edge_driver_t *drivers[] = {&modbus_rtu_driver, &modbus_tcp_driver};
    for (size_t d = 0; d < 2; d++) {
        alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
        modbus_write_desc_t desc = {.type = MODBUS_WRITE_F32, .order = MODBUS_WORD_ORDER_CDAB};
        memset(priv, 0, sizeof(priv));
        assert_int_equal(drivers[d]->req_prepare(priv, 1, MODBUS_NATIVE(5, 2), NULL, 0, &desc,
                                                 sizeof(desc) - 1, false, 0.0),
                         0);
        net_buf_pool_t pool;
        assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);
        net_buf_t *out = net_buf_alloc(&pool, 0);
        assert_int_equal(drivers[d]->req_encode(priv, NULL, out), 0);
        assert_int_equal(drivers[d] == &modbus_rtu_driver ? out->b.data[1] : out->b.data[7],
                         MODBUS_FC_READ_HOLDING_REGS);
        net_buf_unref(out);
        net_buf_pool_destroy(&pool);
    }
}

/* 故意错字序的负例：同一值 ABCD 与 CDAB 的负载必须不同，用 ABCD 帧去比
 * CDAB 帧会红；未知字序直接拒绝。 */
static void test_word_order_negative(void **state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 8, 256, 0, NULL, NULL), 0);
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    modbus_write_desc_t abcd = {.type = MODBUS_WRITE_U32, .order = MODBUS_WORD_ORDER_ABCD};
    modbus_write_desc_t cdab = {.type = MODBUS_WRITE_U32, .order = MODBUS_WORD_ORDER_CDAB};

    _prepare(&modbus_rtu_driver, priv, 2, &abcd, 0x12345678, 0);
    net_buf_t *a = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_rtu_driver.req_encode(priv, NULL, a), 0);
    _prepare(&modbus_rtu_driver, priv, 2, &cdab, 0x12345678, 0);
    net_buf_t *b = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_rtu_driver.req_encode(priv, NULL, b), 0);

    /* 正确 ABCD 负载 [12 34 56 78]；错误字序的帧在这里必然不同 */
    static const uint8_t expect_abcd[4] = {0x12, 0x34, 0x56, 0x78};
    assert_memory_equal(a->b.data + 7, expect_abcd, sizeof(expect_abcd));
    assert_memory_not_equal(a->b.data + 7, b->b.data + 7, sizeof(expect_abcd));
    assert_int_equal((b->b.data[7] << 8) | b->b.data[8], 0x5678);

    net_buf_unref(a);
    net_buf_unref(b);
    net_buf_pool_destroy(&pool);

    _prepare(&modbus_rtu_driver, priv, 2,
             &(modbus_write_desc_t){MODBUS_WRITE_U32, 0x7F}, 0x12345678, -1);
}

/* v3 的 ORDER_* 名字按寄存器值语义命名，实际 wire 字节与名字交叉。
 * 下面的黄金 wire 由 v3 SET_FOR_* 宏按同值跑出：32 位用 0x11223344，
 * 64 位用 double 可精确表示的 0x0010203040506070（v3 原例 0x1122334455667788
 * 超出 2^53，无法经 double 精确进入编码器；映射性质相同）。
 * EdgeC 的 order 名就是 wire 顺序，所以同一 wire 对应的是交叉的 EdgeC 名。 */
static void test_v3_wire_order_mapping(void **state) {
    (void)state;
    static const struct {
        uint8_t type;
        uint8_t order;
        uint16_t quantity;
        double value;
        uint8_t wire[8];
    } vectors[] = {
        /* 32-bit value 0x11223344, v3 wire -> EdgeC order */
        {MODBUS_WRITE_U32, MODBUS_WORD_ORDER_BADC, 2, 0x11223344,
         {0x22, 0x11, 0x44, 0x33}}, /* v3 ORDER_ABCD */
        {MODBUS_WRITE_U32, MODBUS_WORD_ORDER_ABCD, 2, 0x11223344,
         {0x11, 0x22, 0x33, 0x44}}, /* v3 ORDER_BADC */
        {MODBUS_WRITE_U32, MODBUS_WORD_ORDER_DCBA, 2, 0x11223344,
         {0x44, 0x33, 0x22, 0x11}}, /* v3 ORDER_CDAB */
        {MODBUS_WRITE_U32, MODBUS_WORD_ORDER_CDAB, 2, 0x11223344,
         {0x33, 0x44, 0x11, 0x22}}, /* v3 ORDER_DCBA */
        /* 64-bit value 0x0010203040506070, v3 wire -> EdgeC order */
        {MODBUS_WRITE_U64, MODBUS_WORD_ORDER_BADC, 4, 0x0010203040506070,
         {0x10, 0x00, 0x30, 0x20, 0x50, 0x40, 0x70, 0x60}}, /* v3 ORDER_ABCDEFGH */
        {MODBUS_WRITE_U64, MODBUS_WORD_ORDER_ABCD, 4, 0x0010203040506070,
         {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70}}, /* v3 ORDER_BADCFEHG */
        {MODBUS_WRITE_U64, MODBUS_WORD_ORDER_DCBA, 4, 0x0010203040506070,
         {0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00}}, /* v3 ORDER_GHEFCDAB */
        {MODBUS_WRITE_U64, MODBUS_WORD_ORDER_CDAB, 4, 0x0010203040506070,
         {0x60, 0x70, 0x40, 0x50, 0x20, 0x30, 0x00, 0x10}}, /* v3 ORDER_HGFEDCBA */
    };
    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        modbus_write_desc_t desc = {.type = vectors[i].type, .order = vectors[i].order};
        uint16_t regs[4] = {0};
        assert_int_equal(modbus_encode_write_regs(vectors[i].value, &desc, vectors[i].quantity,
                                                  regs),
                         0);
        for (uint16_t j = 0; j < vectors[i].quantity; j++) {
            assert_int_equal((regs[j] >> 8) & 0xFF, vectors[i].wire[2 * j]);
            assert_int_equal(regs[j] & 0xFF, vectors[i].wire[2 * j + 1]);
        }
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_rtu_write_desc_frame_vectors),
        cmocka_unit_test(test_tcp_write_desc_frame_vectors),
        cmocka_unit_test(test_hicrc_write_desc_frame_vector),
        cmocka_unit_test(test_default_contract_without_desc),
        cmocka_unit_test(test_quantity_boundaries),
        cmocka_unit_test(test_write_desc_rejects_bad_input),
        cmocka_unit_test(test_write_desc_accepts_boundaries),
        cmocka_unit_test(test_read_ignores_write_desc),
        cmocka_unit_test(test_word_order_negative),
        cmocka_unit_test(test_v3_wire_order_mapping),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
