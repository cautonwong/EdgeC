/**
 * @file test_fuzz_iec104.c
 * @brief IEC104 驱动的畸形帧鲁棒性（ADR-002 纯 codec 版）
 *
 * 走 request-native 的从站端口：`req_slave_exchange(NULL, session, ...)`。
 * priv（请求 scratch）从站面用不到，传 NULL；session 才是跨帧的 APCI 状态。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "iec104_apci.h"
#include "net_buf.h"

static int setup(void** state) {
    net_buf_pool_t* pool = malloc(sizeof(net_buf_pool_t));
    if (net_buf_pool_init(pool, 10, 512, 0, NULL, NULL) != 0) {
        free(pool);
        return -1;
    }
    *state = pool;
    return 0;
}

static int teardown(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_pool_destroy(pool);
    free(pool);
    return 0;
}

extern const edge_driver_t iec104_driver;

/* 建一个最小会话：只有 ctx_size 字节的持久上下文，没有点位表、没有 datahub */
static void* make_session(net_buf_pool_t* pool) {
    edge_driver_ctx_t ctx = {0};
    ctx.pool = pool;
    ctx.priv = calloc(1, iec104_driver.ctx_size);
    assert_non_null(ctx.priv);
    assert_int_equal(iec104_driver.on_init(&ctx), 0);
    return ctx.priv;
}

/* --- 1. 极短报文：只有 0x68，收不全 --- */
static void test_fuzz_short_packet(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    void* session = make_session(pool);

    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);
    net_buf_t* out = net_buf_alloc(pool, 0);

    size_t consumed = 0;
    int rc = iec104_driver.req_slave_exchange(NULL, session, in->b.data, in->b.len, NULL, NULL,
                                              out, &consumed);
    assert_int_equal(rc, EDGE_ERR_INCOMPLETE);

    net_buf_unref(in);
    net_buf_unref(out);
    free(session);
}

/* --- 2. 声明长度与实际长度不符 --- */
static void test_fuzz_mismatched_length(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    void* session = make_session(pool);

    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);
    net_buf_simple_add_u8(&in->b, 255); /* 声明 255，实际只有 6 字节 */
    net_buf_simple_add_mem(&in->b, "\x01\x00\x00\x00", 4);
    net_buf_t* out = net_buf_alloc(pool, 0);

    size_t consumed = 0;
    int rc = iec104_driver.req_slave_exchange(NULL, session, in->b.data, in->b.len, NULL, NULL,
                                              out, &consumed);
    assert_int_equal(rc, EDGE_ERR_INCOMPLETE);

    net_buf_unref(in);
    net_buf_unref(out);
    free(session);
}

/* --- 3. 序列号攻击：N(R) 给到窗口外的极大值 --- */
static void test_fuzz_invalid_ack_nr(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    void* session = make_session(pool);

    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);
    net_buf_simple_add_u8(&in->b, 4);    /* 只有 APCI，没有 ASDU */
    net_buf_simple_add_u8(&in->b, 0x00); /* N(S) = 0 */
    net_buf_simple_add_u8(&in->b, 0x00);
    net_buf_simple_add_u8(&in->b, 0xFE); /* N(R) = 0x7F，超出窗口 */
    net_buf_simple_add_u8(&in->b, 0xFF);
    net_buf_t* out = net_buf_alloc(pool, 0);

    size_t consumed = 0;
    int rc = iec104_driver.req_slave_exchange(NULL, session, in->b.data, in->b.len, NULL, NULL,
                                              out, &consumed);
    /* 不该崩：序号回收有上限，ASDU 空则直接吃掉 */
    assert_int_equal(rc, 0);

    net_buf_unref(in);
    net_buf_unref(out);
    free(session);
}

/* --- 4. 非法 TypeID --- */
static void test_fuzz_illegal_type_id(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    void* session = make_session(pool);

    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);
    net_buf_simple_add_u8(&in->b, 10);                     /* ASDU 头 6 + 数据 */
    net_buf_simple_add_mem(&in->b, "\x00\x00\x00\x00", 4); /* APCI I 帧 */
    net_buf_simple_add_u8(&in->b, 255);                    /* 非法 TypeID */
    net_buf_simple_add_u8(&in->b, 1);                      /* VSQ */
    net_buf_simple_add_mem(&in->b, "\x06\x00", 2);         /* COT = ACT */
    net_buf_simple_add_mem(&in->b, "\x01\x00", 2);         /* CASDU */
    net_buf_t* out = net_buf_alloc(pool, 0);

    size_t consumed = 0;
    int rc = iec104_driver.req_slave_exchange(NULL, session, in->b.data, in->b.len, NULL, NULL,
                                              out, &consumed);
    /* 不认识的点位类型：吃掉，不报错、不回帧 */
    assert_int_equal(rc, 0);

    net_buf_unref(in);
    net_buf_unref(out);
    free(session);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_fuzz_short_packet, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_mismatched_length, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_invalid_ack_nr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_illegal_type_id, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
