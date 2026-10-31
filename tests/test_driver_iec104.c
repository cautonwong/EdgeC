/**
 * @file test_driver_iec104.c
 * @brief IEC104 驱动的控制方向（ADR-002 §S5）
 *
 * 只测驱动自己负责的部分：命令点位标识、控制 ASDU 组帧、以及"没建链就不发帧"。
 * APCI 拆包/序号那套由 test_fuzz_iec104 覆盖。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_driver.h"
#include "iec104_apci.h"
#include "iec_asdu.h"
#include "net_buf.h"

extern const edge_driver_t iec104_driver;

#define NATIVE(type, ioa) ((((uint32_t)(type) & 0xFFu) << 24) | ((uint32_t)(ioa) & 0xFFFFFFu))

/* 会话要能建起来：ctx_size 字节 + on_init */
static void* make_session(net_buf_pool_t* pool) {
    edge_driver_ctx_t ctx = {0};
    ctx.pool = pool;
    ctx.priv = calloc(1, iec104_driver.ctx_size);
    assert_non_null(ctx.priv);
    assert_int_equal(iec104_driver.on_init(&ctx), 0);
    return ctx.priv;
}

/* 104 是面向连接的：encode 之前链路必须是 CONNECTED，
 * 用真实的 STARTDT CON 帧把状态推过去，而不是去戳驱动的私有字段。 */
static void activate_link(void* session, net_buf_pool_t* pool) {
    net_buf_t* out = net_buf_alloc(pool, 0);
    assert_non_null(out);
    const uint8_t startdt_con[] = {0x68, 0x04, 0x0B, 0x00, 0x00, 0x00};
    size_t consumed = 0;
    assert_int_equal(iec104_driver.req_slave_exchange(NULL, session, startdt_con,
                                                      sizeof(startdt_con), NULL, NULL, out,
                                                      &consumed),
                     0);
    net_buf_unref(out);
}

/* 控制 ASDU：类型=命令类型、COT=ACT、I 帧封装 */
static void test_control_setpoint_encodes_act_asdu(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(4, 256, 0);
    assert_non_null(pool);
    void* session = make_session(pool);
    activate_link(session, pool);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(
        iec104_driver.req_prepare(priv, 1, NATIVE(IEC_TYPE_C_SE_NC_1, 0x4001), NULL, 0, NULL, 0,
                                  true, 12.5),
        0);

    net_buf_t* out = net_buf_alloc(pool, 0);
    assert_non_null(out);
    assert_int_equal(iec104_driver.req_encode(priv, session, out), 0);
    assert_true(out->b.len > 6);

    /* APCI：68 | len | ctrl(4)。I 帧的控制域最低位是 0 */
    assert_int_equal(out->b.data[0], 0x68);
    assert_int_equal(out->b.data[2] & 0x01, 0);

    /* ASDU 从第 6 字节开始 */
    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, out->b.data + 6, out->b.len - 6);
    b.len = (uint16_t)(out->b.len - 6);

    iec_type_id_t type;
    bool sq;
    uint8_t count;
    iec_cot_t cot;
    uint16_t casdu;
    assert_int_equal(iec_asdu_parse_header(&b, &type, &sq, &count, &cot, &casdu,
                                           &IEC_ADDR_CONF_104),
                     0);
    assert_int_equal(type, IEC_TYPE_C_SE_NC_1); /* 50 */
    assert_int_equal(cot, IEC_COT_ACT);         /* 控制方向是 ACT，不是 REQ */
    /* 剩下 IOA(3) + 短浮点(4) + QOS(1) */
    assert_int_equal(b.len, 8);

    net_buf_unref(out);
    free(session);
    net_buf_pool_destroy(pool);
}

/* 写请求必须带命令类型：测量类型去"写"是协议错误 */
static void test_control_rejects_measurement_type(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(4, 256, 0);
    assert_non_null(pool);
    void* session = make_session(pool);
    activate_link(session, pool);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));

    /* 类型 0（没给）也不行：读路径才允许缺省成 M_ME_NC_1 */
    assert_int_not_equal(
        iec104_driver.req_prepare(priv, 1, NATIVE(0, 0x4001), NULL, 0, NULL, 0, true, 1.0), 0);
    assert_int_not_equal(
        iec104_driver.req_prepare(priv, 1, NATIVE(IEC_TYPE_M_ME_NC_1, 0x4001), NULL, 0, NULL, 0,
                                  true, 1.0),
        0);

    /* 读请求不受影响 */
    assert_int_equal(
        iec104_driver.req_prepare(priv, 1, NATIVE(IEC_TYPE_M_ME_NC_1, 0x4001), NULL, 0, NULL, 0,
                                  false, 0.0),
        0);

    free(session);
    net_buf_pool_destroy(pool);
}

/* 没建链就不发帧：宁可失败，也不能把控制帧塞进一个未激活的连接 */
static void test_control_requires_connected_link(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(4, 256, 0);
    assert_non_null(pool);
    void* session = make_session(pool);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(
        iec104_driver.req_prepare(priv, 1, NATIVE(IEC_TYPE_C_SC_NA_1, 0x4001), NULL, 0, NULL, 0,
                                  true, 1.0),
        0);

    net_buf_t* out = net_buf_alloc(pool, 0);
    assert_non_null(out);
    assert_int_not_equal(iec104_driver.req_encode(priv, session, out), 0);
    assert_int_equal(out->b.len, 0);

    net_buf_unref(out);
    free(session);
    net_buf_pool_destroy(pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_control_setpoint_encodes_act_asdu),
        cmocka_unit_test(test_control_rejects_measurement_type),
        cmocka_unit_test(test_control_requires_connected_link),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
