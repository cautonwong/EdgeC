#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "modbus_codec.h"
#include "net_buf.h"

extern edge_driver_t modbus_rtu_driver;
extern const edge_driver_t modbus_rtu_hicrc_driver;
extern edge_driver_t modbus_tcp_driver;

typedef struct {
    driver_manager_t* mgr;
    driver_manager_storage_t mgr_storage;
} test_state_t;

static int setup(void** state) {
    test_state_t* ts = calloc(1, sizeof(test_state_t));
    ts->mgr = driver_manager_create_in(&ts->mgr_storage);
    driver_manager_register(ts->mgr, &modbus_rtu_driver);
    *state = ts;
    return 0;
}

static int teardown(void** state) {
    test_state_t* ts = *state;
    driver_manager_destroy(ts->mgr);
    free(ts);
    return 0;
}

static void test_modbus_v3_master_cycle(void** state) {
    test_state_t* ts = *state;
    edge_driver_ctx_t* ctx = driver_instance_create(ts->mgr, "ModbusRTU", 1);
    assert_non_null(ctx);

    driver_instance_destroy(ctx, "ModbusRTU");
}

/* [ADR-002 S5] 写请求必须发 FC06，不能退化成读帧。
 * 修复前 req_encode 只有一个入口且永远发 FC03——IEMS 的控制下发与 REST 写点
 * 都会静默变成一次读，控制失效且不报错。 */
static void test_modbus_rtu_write_encodes_fc06(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    /* 点位：地址 5，1 个寄存器；写 raw=1234 */
    /* 缓冲按驱动声明的 req_priv_size 开——手写一个"看起来够大"的数组就是踩栈 */
    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   true, 1234.0),
                     0);

    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(modbus_rtu_driver.req_encode(priv, NULL, out), 0);

    /* RTU 帧：addr | FC | reg_hi | reg_lo | val_hi | val_lo | crc_lo | crc_hi */
    assert_int_equal(out->b.len, 8);
    assert_int_equal(out->b.data[0], 1);      /* 从站地址 */
    assert_int_equal(out->b.data[1], 0x06);   /* FC06 = 写单寄存器 */
    assert_int_equal(out->b.data[2], 0x00);   /* 寄存器地址 5 */
    assert_int_equal(out->b.data[3], 0x05);
    assert_int_equal(out->b.data[4], 0x04);   /* 1234 = 0x04D2 */
    assert_int_equal(out->b.data[5], 0xD2);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 读请求仍然是 FC03：修复不能把读也改坏 */
static void test_modbus_rtu_read_encodes_fc03(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    /* 缓冲按驱动声明的 req_priv_size 开——手写一个"看起来够大"的数组就是踩栈 */
    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);

    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(modbus_rtu_driver.req_encode(priv, NULL, out), 0);
    assert_int_equal(out->b.len, 8);
    assert_int_equal(out->b.data[1], 0x03); /* FC03 = 读保持寄存器 */

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 超过 2 个寄存器的写显式拒绝：请求模型只带一个标量，截断比失败更危险 */
static void test_modbus_rtu_write_rejects_multi_reg(void** state) {
    (void)state;
    /* 缓冲按驱动声明的 req_priv_size 开——手写一个"看起来够大"的数组就是踩栈 */
    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_not_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 3), NULL, 0, NULL,
                                                       0, true, 1.0),
                         -0);
}

/* FC16：32 位量写必须一次发两个寄存器。iems_v3 用 modbus_write_registers
 * （src/port_transf.c:1304，注释「直接使用0x10命令码，为了支持浮点数一次写入」），
 * 拆成两次 FC06 会让从站看到中间态。 */
static void test_modbus_rtu_write32_encodes_fc16(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 2), NULL, 0, NULL, 0,
                                                   true, 0x12345678),
                     0);

    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(modbus_rtu_driver.req_encode(priv, NULL, out), 0);

    /* FC16 帧：slave | 0x10 | addr | qty | byte_count | 4 字节值 | crc */
    assert_int_equal(out->b.len, 13);
    static const uint8_t expect[11] = {1, MODBUS_FC_WRITE_MULTIPLE_REGS, 0x00, 0x05,
                                       0x00, 2, 4, 0x12, 0x34, 0x56, 0x78};
    assert_memory_equal(out->b.data, expect, sizeof(expect));
    uint16_t crc = modbus_calculate_crc(out->b.data, 11);
    assert_int_equal(out->b.data[11], (uint8_t)(crc & 0xFF));
    assert_int_equal(out->b.data[12], (uint8_t)(crc >> 8));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* MBAP length 从 unit id 起算，11 字节响应必须被接受。
 * 修过 7+len 的 off-by-one：旧代码把完整帧判成 INCOMPLETE，只能等事务超时。 */
static void test_modbus_tcp_mbap_accepts_full_frame(void** state) {
    (void)state;
    uint8_t frame[] = {0x00, 0x2A, 0x00, 0x00, 0x00, 0x05, 0x01, 0x03, 0x02, 0x12, 0x34};
    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, frame, sizeof(frame));
    nb.len = sizeof(frame);
    uint16_t tid = 0;
    uint8_t slave = 0;
    size_t pdu_len = 0;
    assert_int_equal(modbus_tcp_decode_mbap(&nb, &tid, &slave, &pdu_len), 0);
    assert_int_equal(tid, 0x2A);
    assert_int_equal(slave, 1);
    assert_int_equal((int)pdu_len, 4);

    nb.len = sizeof(frame) - 1;
    assert_int_equal(modbus_tcp_decode_mbap(&nb, &tid, &slave, &pdu_len), 1);
}

/* MBAP 坏帧必须确定性拒绝：协议号非 0、length=0、length 超过 254
 * （unit id + PDU 253）都要返回协议错误而不是 INCOMPLETE。修复前超长
 * 声明只能等 2s 事务超时才能重同步。 */
static void test_modbus_tcp_mbap_rejects_bad_frames(void** state) {
    (void)state;
    uint16_t tid = 0;
    uint8_t slave = 0;
    size_t pdu_len = 0;
    struct net_buf_simple nb;

    uint8_t proto_bad[] = {0x00, 0x2A, 0x00, 0x01, 0x00, 0x05, 0x01, 0x03, 0x02, 0x12, 0x34};
    net_buf_simple_init_with_data(&nb, proto_bad, sizeof(proto_bad));
    nb.len = sizeof(proto_bad);
    assert_int_equal(modbus_tcp_decode_mbap(&nb, &tid, &slave, &pdu_len), -1);

    uint8_t len_zero[] = {0x00, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x01};
    net_buf_simple_init_with_data(&nb, len_zero, sizeof(len_zero));
    nb.len = sizeof(len_zero);
    assert_int_equal(modbus_tcp_decode_mbap(&nb, &tid, &slave, &pdu_len), -1);

    /* 完整 261 字节帧，声明 length=255：PDU 超限，必须拒绝 */
    uint8_t overlong[MODBUS_TCP_HEADER_LEN - 1 + 255];
    memset(overlong, 0, sizeof(overlong));
    overlong[1] = 0x2A;
    overlong[5] = 255;
    overlong[6] = 1;
    net_buf_simple_init_with_data(&nb, overlong, sizeof(overlong));
    nb.len = sizeof(overlong);
    assert_int_equal(modbus_tcp_decode_mbap(&nb, &tid, &slave, &pdu_len), -1);

    /* 声明 60000 字节、实际只有 7 字节：确定性协议错误，不是 INCOMPLETE */
    uint8_t huge_len[] = {0x00, 0x2A, 0x00, 0x00, 0xEA, 0x60, 0x01};
    net_buf_simple_init_with_data(&nb, huge_len, sizeof(huge_len));
    nb.len = sizeof(huge_len);
    assert_int_equal(modbus_tcp_decode_mbap(&nb, &tid, &slave, &pdu_len), -1);
}

/* TCP 多寄存器读要像 RTU 一样按 ABCD 合包。修复前只回 raw[0]，32 位量的
 * 高字被丢掉，读回 0，写入也就无从校验。 */
static void test_modbus_tcp_read_combines_two_regs(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_tcp_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 2), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);

    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_tcp_driver.req_encode(priv, NULL, out), 0);
    /* 应答必须回显请求的事务号，否则会被当成上一笔的迟到应答 */
    uint8_t resp[] = {out->b.data[0], out->b.data[1], 0, 0, 0, 7, 1, 3, 4, 0x00, 0x01, 0x86, 0xA0};

    size_t consumed = 0;
    double value = 0.0;
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, resp, sizeof(resp), &consumed, &value),
                     0);
    assert_int_equal(consumed, sizeof(resp));
    assert_true(value == 100000.0);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 事务号对不上是上一笔的迟到应答：跳过并交回 INCOMPLETE，不能拿它结算当前请求 */
static void test_modbus_tcp_read_skips_stale_tid(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_tcp_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_tcp_driver.req_encode(priv, NULL, out), 0);
    uint8_t resp[] = {(uint8_t)(out->b.data[0] + 1), out->b.data[1], 0, 0, 0, 5, 1, 3, 2, 0x12, 0x34};

    size_t consumed = 0;
    double value = 0.0;
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, resp, sizeof(resp), &consumed, &value),
                     EDGE_ERR_INCOMPLETE);
    assert_int_equal(consumed, sizeof(resp));
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 读应答 byte_count 必须是偶数且等于 quantity*2。修复前奇数被向下取整，
 * byte_count=1 会以 raw_n=0、值 0.0 成功结算，畸形帧被当成"值 0"。 */
static void test_modbus_tcp_read_rejects_bad_byte_count(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_tcp_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_tcp_driver.req_encode(priv, NULL, out), 0);

    size_t consumed = 0;
    double value = 0.0;
    /* byte_count=1：奇数 */
    uint8_t odd[] = {out->b.data[0], out->b.data[1], 0, 0, 0, 4, 1, 3, 1, 0x12};
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, odd, sizeof(odd), &consumed, &value),
                     -1);

    /* quantity=1 却回 4 字节（2 个寄存器） */
    uint8_t wide[] = {out->b.data[0], out->b.data[1], 0, 0, 0, 7, 1, 3,
                      4,             0x00,           0x01, 0x86, 0xA0};
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, wide, sizeof(wide), &consumed, &value),
                     -1);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* TCP 是字节流：一帧可能分多次到达，也可能一次到达多帧。驱动必须用
 * consumed 告诉调用方消费了多少，INCOMPLETE 时也要消费掉已完整的前导帧，
 * 否则粘包里的迟到帧会永远卡住当前请求。 */
static void test_modbus_tcp_read_split_and_coalesced(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_tcp_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_tcp_driver.req_encode(priv, NULL, out), 0);

    uint8_t resp[] = {out->b.data[0], out->b.data[1], 0, 0, 0, 5, 1, 3, 2, 0x12, 0x34};
    size_t consumed = 0;
    double value = 0.0;

    /* 拆包：前 5 字节不足一个 MBAP 头，报 INCOMPLETE 且不消费 */
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, resp, 5, &consumed, &value),
                     EDGE_ERR_INCOMPLETE);
    assert_int_equal(consumed, 0);
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, resp, sizeof(resp), &consumed, &value),
                     0);
    assert_int_equal(consumed, sizeof(resp));
    assert_true(value == 0x1234);

    /* 粘包：匹配帧在前、迟到帧在后，只结算第一帧 */
    uint8_t two[22];
    memcpy(two, resp, sizeof(resp));
    memcpy(two + sizeof(resp), resp, sizeof(resp));
    two[sizeof(resp)] ^= 0x01; /* 第二帧换一个事务号，模拟迟到应答 */
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, two, sizeof(two), &consumed, &value),
                     0);
    assert_int_equal(consumed, sizeof(resp));
    assert_true(value == 0x1234);

    /* 粘包：迟到帧在前、匹配帧在后，先消费前导帧，再用剩下的成功 */
    uint8_t stale_first[22];
    memcpy(stale_first, resp, sizeof(resp));
    memcpy(stale_first + sizeof(resp), resp, sizeof(resp));
    stale_first[0] ^= 0x01;
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, stale_first, sizeof(stale_first),
                                                  &consumed, &value),
                     EDGE_ERR_INCOMPLETE);
    assert_int_equal(consumed, sizeof(resp));
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, stale_first + sizeof(resp),
                                                  sizeof(resp), &consumed, &value),
                     0);
    assert_true(value == 0x1234);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 事务号 uint16_t 回绕：0xFFFF 的下一笔是 0。TID 0 的应答必须结算，
 * 其余（包括 0xFFFF）都是迟到帧。 */
static void test_modbus_tcp_tid_wrap(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));

    uint16_t tid = 0;
    for (int i = 0; i < 0x10000; i++) {
        assert_int_equal(modbus_tcp_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                       0, false, 0.0),
                         0);
        net_buf_t* out = net_buf_alloc(&pool, 0);
        assert_non_null(out);
        assert_int_equal(modbus_tcp_driver.req_encode(priv, NULL, out), 0);
        tid = (uint16_t)((out->b.data[0] << 8) | out->b.data[1]);
        net_buf_unref(out);
        if (tid == 0xFFFF) break;
    }
    assert_int_equal(tid, 0xFFFF);

    assert_int_equal(modbus_tcp_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_tcp_driver.req_encode(priv, NULL, out), 0);
    assert_int_equal(out->b.data[0], 0);
    assert_int_equal(out->b.data[1], 0);

    size_t consumed = 0;
    double value = 0.0;
    uint8_t resp_ff[] = {0xFF, 0xFF, 0, 0, 0, 5, 1, 3, 2, 0x12, 0x34};
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, resp_ff, sizeof(resp_ff), &consumed,
                                                  &value),
                     EDGE_ERR_INCOMPLETE);
    uint8_t resp_0[] = {0x00, 0x00, 0, 0, 0, 5, 1, 3, 2, 0x12, 0x34};
    assert_int_equal(modbus_tcp_driver.req_decode(priv, NULL, resp_0, sizeof(resp_0), &consumed,
                                                  &value),
                     0);
    assert_true(value == 0x1234);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 写应答不能结算读请求：迟到的 FC06 回显会把读填成 0 */
static void test_modbus_rtu_read_rejects_write_echo(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);
    net_buf_t* echo = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_rtu_encode_write_single(1, 5, 0x1234, echo), 0);

    size_t consumed = 0;
    double value = 0.0;
    assert_int_equal(modbus_rtu_driver.req_decode(priv, NULL, echo->b.data, echo->b.len, &consumed,
                                                  &value),
                     -1);
    net_buf_unref(echo);
    net_buf_pool_destroy(&pool);
}

/* iems_v3 默认 modbus_rtu 的 CRC 是高字节先发；标准 Modbus 是低字节先发。
 * 同一份负载两种顺序都必须能出帧，且低序解码器不能接受高序帧。 */
static void test_modbus_rtu_hicrc_wire_order(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    net_buf_t* req = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_rtu_encode_read_ex(1, 3, 5, 1, true, req), 0);
    assert_int_equal(req->b.len, 8);
    static const uint8_t expect[6] = {1, MODBUS_FC_READ_HOLDING_REGS, 0, 5, 0, 1};
    assert_memory_equal(req->b.data, expect, sizeof(expect));
    uint16_t crc = modbus_calculate_crc(req->b.data, 6);
    assert_int_equal(req->b.data[6], (uint8_t)(crc >> 8));
    assert_int_equal(req->b.data[7], (uint8_t)(crc & 0xFF));

    /* 同负载的标准序帧，两字节顺序相反 */
    net_buf_t* std = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_rtu_encode_read_ex(1, 3, 5, 1, false, std), 0);
    assert_int_equal(std->b.data[6], (uint8_t)(crc & 0xFF));
    assert_int_equal(std->b.data[7], (uint8_t)(crc >> 8));

    /* 应答按高序封装：高序解码接受，低序解码拒绝 */
    uint8_t resp[7] = {1, MODBUS_FC_READ_HOLDING_REGS, 2, 0x12, 0x34, 0, 0};
    uint16_t rcrc = modbus_calculate_crc(resp, 5);
    resp[5] = (uint8_t)(rcrc >> 8);
    resp[6] = (uint8_t)(rcrc & 0xFF);

    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, resp, sizeof(resp));
    nb.len = sizeof(resp);
    uint8_t slave = 0, fc = 0;
    uint8_t* data = NULL;
    size_t data_len = 0;
    assert_int_equal(modbus_rtu_decode_frame_ex(&nb, true, &slave, &fc, &data, &data_len), 0);
    nb.len = sizeof(resp);
    assert_int_equal(modbus_rtu_decode_frame_ex(&nb, false, &slave, &fc, &data, &data_len), -1);

    net_buf_unref(req);
    net_buf_unref(std);
    net_buf_pool_destroy(&pool);
}

/* 驱动变体按名字可见，且写完一帧 FC06 也是高字节先发 */
static void test_modbus_rtu_hicrc_driver_encodes(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    assert_string_equal(modbus_rtu_hicrc_driver.name, "ModbusRTUHiCRC");
    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_rtu_hicrc_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                         0, true, 1234.0),
                     0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_int_equal(modbus_rtu_hicrc_driver.req_encode(priv, NULL, out), 0);
    assert_int_equal(out->b.len, 8);
    uint16_t crc = modbus_calculate_crc(out->b.data, 6);
    assert_int_equal(out->b.data[6], (uint8_t)(crc >> 8));
    assert_int_equal(out->b.data[7], (uint8_t)(crc & 0xFF));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 从站地址只有 1 字节：驱动必须在 prepare 拒绝，而不是截断后发到别的从站
 * （RTU 248..255 截成 0 广播，256 -> 0，65535 -> 255）。 */
static void test_modbus_slave_id_width(void** state) {
    (void)state;
    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));

    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 247, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                   0, false, 0.0),
                     0);
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 248, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                   0, false, 0.0),
                     -1);
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 256, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                   0, false, 0.0),
                     -1);
    assert_int_equal(modbus_rtu_hicrc_driver.req_prepare(priv, 247, MODBUS_NATIVE(5, 1), NULL, 0,
                                                         NULL, 0, false, 0.0),
                     0);
    assert_int_equal(modbus_rtu_hicrc_driver.req_prepare(priv, 248, MODBUS_NATIVE(5, 1), NULL, 0,
                                                         NULL, 0, false, 0.0),
                     -1);
    assert_int_equal(modbus_tcp_driver.req_prepare(priv, 255, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                   0, false, 0.0),
                     0);
    assert_int_equal(modbus_tcp_driver.req_prepare(priv, 256, MODBUS_NATIVE(5, 1), NULL, 0, NULL,
                                                   0, false, 0.0),
                     -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_modbus_v3_master_cycle),
        cmocka_unit_test(test_modbus_rtu_write_encodes_fc06),
        cmocka_unit_test(test_modbus_rtu_read_encodes_fc03),
        cmocka_unit_test(test_modbus_rtu_write_rejects_multi_reg),
        cmocka_unit_test(test_modbus_rtu_write32_encodes_fc16),
        cmocka_unit_test(test_modbus_tcp_mbap_accepts_full_frame),
        cmocka_unit_test(test_modbus_tcp_mbap_rejects_bad_frames),
        cmocka_unit_test(test_modbus_tcp_read_combines_two_regs),
        cmocka_unit_test(test_modbus_tcp_read_skips_stale_tid),
        cmocka_unit_test(test_modbus_tcp_read_rejects_bad_byte_count),
        cmocka_unit_test(test_modbus_tcp_read_split_and_coalesced),
        cmocka_unit_test(test_modbus_tcp_tid_wrap),
        cmocka_unit_test(test_modbus_rtu_read_rejects_write_echo),
        cmocka_unit_test(test_modbus_rtu_hicrc_wire_order),
        cmocka_unit_test(test_modbus_rtu_hicrc_driver_encodes),
        cmocka_unit_test(test_modbus_slave_id_width),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}