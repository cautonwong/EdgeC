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
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "edge_driver.h"
#include "iec104_apci.h"
#include "iec_asdu.h"
#include "net_buf.h"
#include "session_manager.h"

extern const edge_driver_t iec104_driver;

#define NATIVE(type, ioa) ((((uint32_t)(type) & 0xFFu) << 24) | ((uint32_t)(ioa) & 0xFFFFFFu))

/* 会话要能建起来：ctx_size 字节 + on_init */
static void* make_session_role(net_buf_pool_t* pool, protocol_role_t role) {
    edge_driver_ctx_t ctx = {0};
    ctx.pool = pool;
    ctx.role = role;
    ctx.priv = calloc(1, iec104_driver.ctx_size);
    assert_non_null(ctx.priv);
    assert_int_equal(iec104_driver.on_init(&ctx), 0);
    return ctx.priv;
}

static void* make_session(net_buf_pool_t* pool) {
    return make_session_role(pool, PROTOCOL_ROLE_MASTER);
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

/* 从站收到一个 U 帧，应答放在 out 里。 */
static void slave_exchange_u(void* session, net_buf_pool_t* pool, uint8_t c1, net_buf_t** out) {
    uint8_t frame[6] = {0x68, 0x04, c1, 0x00, 0x00, 0x00};
    size_t consumed = 0;
    *out = net_buf_alloc(pool, 0);
    assert_non_null(*out);
    assert_int_equal(iec104_driver.req_slave_exchange(NULL, session, frame, sizeof(frame), NULL,
                                                      NULL, *out, &consumed),
                     0);
    assert_int_equal(consumed, sizeof(frame));
}

/* U 帧应答的线上字节：68 | 04 | 控制域首字节 | 0 0 0 */
static void assert_u_frame(net_buf_t* out, uint8_t c1) {
    const uint8_t expect[6] = {0x68, 0x04, c1, 0x00, 0x00, 0x00};
    assert_int_equal(out->b.len, sizeof(expect));
    assert_memory_equal(out->b.data, expect, sizeof(expect));
}

/* 自发 I 帧：ASDU 从第 6 字节开始，type=M_ME_NC_1、COT=SPONT。 */
static void assert_spontaneous_frame(net_buf_t* tx) {
    assert_non_null(tx);
    assert_int_equal(tx->b.data[0], 0x68);
    assert_int_equal(tx->b.data[2] & 0x01, 0);
    assert_int_equal(tx->b.data[6], IEC_TYPE_M_ME_NC_1);
    assert_int_equal(tx->b.data[8], IEC_COT_SPONT);
}

/* 从站自发源：一个 M_ME_NC_1 点，值取 g_slave_value */
static double g_slave_value;

static int mock_slave_resolve(void* ctx, uint32_t point_id, const void* point_desc,
                              size_t point_desc_len, edge_point_value_t* out) {
    (void)ctx;
    (void)point_id;
    (void)point_desc;
    (void)point_desc_len;
    out->value = g_slave_value;
    return 0;
}

static int mock_slave_enumerate(void* ctx, void (*fn)(void* ud, uint32_t point_id), void* ud) {
    (void)ctx;
    fn(ud, NATIVE(IEC_TYPE_M_ME_NC_1, 0x4001));
    return 0;
}

static const edge_slave_source_ops_t k_slave_src = {
    .resolve = mock_slave_resolve,
    .enumerate = mock_slave_enumerate,
};

static int tick_once(void* session, net_buf_pool_t* pool, net_buf_t** out) {
    *out = NULL;
    return iec104_driver.req_tick(session, NULL, NULL, &k_slave_src, NULL, pool, out);
}

/* STOPDT ACT 必须回 STOPDT CON(0x23)；之后变化点不再产生 SPONT，重复 STOPDT 幂等。
 * TESTFR 仍回 CON：停数据传送不影响链路保活。 */
static void test_stopdt_acknowledged_and_pauses_spontaneous(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(8, 256, 0);
    assert_non_null(pool);
    void* session = make_session_role(pool, PROTOCOL_ROLE_SLAVE);

    net_buf_t* out = NULL;
    slave_exchange_u(session, pool, 0x07, &out); /* STARTDT ACT */
    assert_u_frame(out, 0x0B);
    net_buf_unref(out);

    /* 先证明自发通道是通的：首帧建基线，之后变化上送 */
    g_slave_value = 1.0;
    net_buf_t* tx = NULL;
    assert_int_equal(tick_once(session, pool, &tx), -1);
    assert_null(tx);
    g_slave_value = 2.0;
    assert_int_equal(tick_once(session, pool, &tx), 0);
    assert_spontaneous_frame(tx);
    net_buf_unref(tx);

    slave_exchange_u(session, pool, 0x13, &out); /* STOPDT ACT */
    assert_u_frame(out, 0x23);
    net_buf_unref(out);

    g_slave_value = 3.0;
    tx = NULL;
    assert_int_equal(tick_once(session, pool, &tx), -1);
    assert_null(tx);

    slave_exchange_u(session, pool, 0x13, &out); /* 重复 STOPDT 幂等 */
    assert_u_frame(out, 0x23);
    net_buf_unref(out);

    slave_exchange_u(session, pool, 0x43, &out); /* TESTFR ACT 仍回 CON */
    assert_u_frame(out, 0x83);
    net_buf_unref(out);

    free(session);
    net_buf_pool_destroy(pool);
}

/* STARTDT ACT 恢复数据传送：停顿期间攒下的变化补报一次。 */
static void test_startdt_act_resumes_spontaneous(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(8, 256, 0);
    assert_non_null(pool);
    void* session = make_session_role(pool, PROTOCOL_ROLE_SLAVE);

    net_buf_t* out = NULL;
    slave_exchange_u(session, pool, 0x07, &out); /* STARTDT ACT */
    assert_u_frame(out, 0x0B);
    net_buf_unref(out);

    g_slave_value = 1.0;
    net_buf_t* tx = NULL;
    assert_int_equal(tick_once(session, pool, &tx), -1);
    assert_null(tx);

    slave_exchange_u(session, pool, 0x13, &out); /* STOPDT ACT */
    assert_u_frame(out, 0x23);
    net_buf_unref(out);

    g_slave_value = 2.0;
    tx = NULL;
    assert_int_equal(tick_once(session, pool, &tx), -1);
    assert_null(tx);

    slave_exchange_u(session, pool, 0x07, &out); /* STARTDT ACT 恢复 */
    assert_u_frame(out, 0x0B);
    net_buf_unref(out);

    tx = NULL;
    assert_int_equal(tick_once(session, pool, &tx), 0);
    assert_spontaneous_frame(tx);

    /* 恢复后上送的是停顿期间攒下的变化值 */
    struct net_buf_simple b;
    net_buf_simple_init_with_data(&b, tx->b.data + 6, tx->b.len - 6);
    b.len = (uint16_t)(tx->b.len - 6);
    iec_type_id_t type;
    bool sq;
    uint8_t count;
    iec_cot_t cot;
    uint16_t casdu;
    assert_int_equal(
        iec_asdu_parse_header(&b, &type, &sq, &count, &cot, &casdu, &IEC_ADDR_CONF_104), 0);
    uint32_t ioa = 0;
    float f = 0.0f;
    iec_qds_t qds;
    assert_int_equal(
        iec_asdu_parse_m_me_nc_1(&b, &ioa, &f, &qds, false, false, &IEC_ADDR_CONF_104), 0);
    assert_int_equal(ioa, 0x4001);
    assert_true(f > 1.99f && f < 2.01f);
    net_buf_unref(tx);

    free(session);
    net_buf_pool_destroy(pool);
}

/* 线上 I 帧控制域：N(S) 在控制域 bit 1..15 */
static uint16_t apci_ns(const net_buf_t* tx) {
    return (uint16_t)((tx->b.data[2] | (tx->b.data[3] << 8)) >> 1);
}

/* 会话端口：显式建 ctx，让 session_manager 走真实的 create → on_init 路径 */
static edge_driver_ctx_t* fake_session_create(void* ctx, const char* protocol,
                                              uint32_t device_id) {
    (void)ctx;
    (void)protocol;
    edge_driver_ctx_t* c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    c->driver = &iec104_driver;
    c->priv = calloc(1, iec104_driver.ctx_size);
    if (!c->priv) {
        free(c);
        return NULL;
    }
    c->device_id = device_id;
    return c;
}

static void fake_session_destroy(edge_driver_ctx_t* session) {
    free(session->priv);
    free(session);
}

/* 池空闲数：只看审计统计，不读会话/APCI 私有字段 */
static uint32_t pool_free_count(net_buf_pool_t* pool) {
    net_buf_pool_stats_t stats;
    net_buf_pool_audit(pool, 60000, &stats);
    return stats.free_count;
}

static edge_driver_ctx_t* make_managed_session(net_buf_pool_t* pool, session_manager_t** out_sm) {
    edge_driver_session_port_t port = {.create = fake_session_create,
                                       .destroy = fake_session_destroy};
    session_manager_t* sm = session_manager_create(&port, 2, pool);
    assert_non_null(sm);
    assert_int_equal(session_manager_add_session(sm, "IEC104", 1, NULL), 0);
    edge_driver_ctx_t* ctx = session_manager_get(sm, "IEC104", 1);
    assert_non_null(ctx);
    session_manager_set_slave_source(sm, &k_slave_src, NULL);
    *out_sm = sm;
    return ctx;
}

/* 走驱动自发路径发 n 个 I 帧；调用方只释放自己的 tx 引用，APCI 备份仍持有 */
static void emit_unacked_i_frames(session_manager_t* sm, int n) {
    net_buf_t* tx = NULL;
    g_slave_value = 0.0;
    assert_int_equal(session_manager_tick(sm, 0, &tx, 1), 0); /* 首帧只建基线 */
    assert_null(tx);
    for (int i = 1; i <= n; i++) {
        g_slave_value = (double)i;
        tx = NULL;
        assert_int_equal(session_manager_tick(sm, 0, &tx, 1), 1);
        assert_non_null(tx);
        net_buf_unref(tx);
    }
}

/* 线上 S 帧确认到 nr：68 | 04 | 01 00 | N(R)<<1 */
static void ack_to(edge_driver_ctx_t* ctx, net_buf_pool_t* pool, uint16_t nr) {
    uint16_t c2 = (uint16_t)(nr << 1);
    uint8_t s_frame[6] = {0x68, 0x04, 0x01, 0x00, (uint8_t)(c2 & 0xFF), (uint8_t)(c2 >> 8)};
    net_buf_t* out = net_buf_alloc(pool, 0);
    assert_non_null(out);
    size_t consumed = 0;
    assert_int_equal(iec104_driver.req_slave_exchange(NULL, ctx->priv, s_frame, sizeof(s_frame),
                                                      NULL, NULL, out, &consumed),
                     0);
    assert_int_equal(consumed, sizeof(s_frame));
    net_buf_unref(out);
}

/* 新主站接管会复用同一个 session：reset 后下一帧 I 帧的 N(S) 必须回到 0 */
static void test_session_reset_restarts_sequence(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(16, 512, 0);
    assert_non_null(pool);
    edge_driver_session_port_t port = {.create = fake_session_create,
                                       .destroy = fake_session_destroy};
    session_manager_t* sm = session_manager_create(&port, 2, pool);
    assert_non_null(sm);
    assert_int_equal(session_manager_add_session(sm, "IEC104", 1, NULL), 0);
    edge_driver_ctx_t* ctx = session_manager_get(sm, "IEC104", 1);
    assert_non_null(ctx);
    session_manager_set_slave_source(sm, &k_slave_src, NULL);

    activate_link(ctx->priv, pool);
    g_slave_value = 1.0;
    net_buf_t* tx = NULL;
    assert_int_equal(session_manager_tick(sm, 0, &tx, 1), 0); /* 首帧只建基线 */
    g_slave_value = 2.0;
    assert_int_equal(session_manager_tick(sm, 0, &tx, 1), 1);
    assert_int_equal(apci_ns(tx), 0);
    net_buf_unref(tx);
    g_slave_value = 3.0;
    assert_int_equal(session_manager_tick(sm, 0, &tx, 1), 1);
    assert_int_equal(apci_ns(tx), 1);
    net_buf_unref(tx);

    assert_int_equal(session_manager_reset_sessions(sm), 0);

    /* 接管后重新 STARTDT：第一帧 I 帧从 N(S)=0 开始 */
    activate_link(ctx->priv, pool);
    g_slave_value = 4.0;
    assert_int_equal(session_manager_tick(sm, 0, &tx, 1), 0 /* 缓存已清，重新建基线 */);
    g_slave_value = 5.0;
    assert_int_equal(session_manager_tick(sm, 0, &tx, 1), 1);
    assert_int_equal(apci_ns(tx), 0);
    net_buf_unref(tx);

    session_manager_destroy(sm);
    net_buf_pool_destroy(pool);
}

/* 新主站接管会复位会话：复位丢掉旧 APCI 状态前必须释放未确认 I 帧的备份引用，
 * 否则每轮漏 2~3 个池缓冲，最终把池耗尽。 */
static void test_session_reset_releases_unacked_i_frames(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(16, 512, 0);
    assert_non_null(pool);
    session_manager_t* sm = NULL;
    edge_driver_ctx_t* ctx = make_managed_session(pool, &sm);

    activate_link(ctx->priv, pool);
    uint32_t base = pool_free_count(pool);
    emit_unacked_i_frames(sm, 3);
    assert_int_equal(pool_free_count(pool), base - 3); /* 3 个备份确实被持有 */

    assert_int_equal(session_manager_reset_sessions(sm), 0);
    assert_int_equal(pool_free_count(pool), base); /* 复位后全部归还 */

    session_manager_destroy(sm);
    net_buf_pool_destroy(pool);
}

/* 对照组：I 帧全部确认后再复位，池计数不变（确认路径已回收，复位不能再动） */
static void test_session_reset_after_ack_keeps_pool_balanced(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(16, 512, 0);
    assert_non_null(pool);
    session_manager_t* sm = NULL;
    edge_driver_ctx_t* ctx = make_managed_session(pool, &sm);

    activate_link(ctx->priv, pool);
    uint32_t base = pool_free_count(pool);
    emit_unacked_i_frames(sm, 3);
    ack_to(ctx, pool, 3);
    assert_int_equal(pool_free_count(pool), base);

    assert_int_equal(session_manager_reset_sessions(sm), 0);
    assert_int_equal(pool_free_count(pool), base);

    session_manager_destroy(sm);
    net_buf_pool_destroy(pool);
}

/* 连续两轮「未确认 + 复位」：池空闲数不能逐轮下降 */
static void test_session_reset_repeated_rounds_do_not_leak(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(16, 512, 0);
    assert_non_null(pool);
    session_manager_t* sm = NULL;
    edge_driver_ctx_t* ctx = make_managed_session(pool, &sm);

    uint32_t base = pool_free_count(pool);
    for (int round = 0; round < 2; round++) {
        activate_link(ctx->priv, pool);
        emit_unacked_i_frames(sm, 3);
        assert_int_equal(pool_free_count(pool), base - 3);
        assert_int_equal(session_manager_reset_sessions(sm), 0);
        assert_int_equal(pool_free_count(pool), base);
    }

    session_manager_destroy(sm);
    net_buf_pool_destroy(pool);
}

/* t3 从 CONNECTED 起算：没发过 I 帧也要在 20s 无活动后主动 TESTFR；
 * 发 S 帧算链路活动，基线前移。 */
static void test_apci_t3_starts_on_connected(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(4, 256, 0);
    assert_non_null(pool);
    iec104_apci_mgr_storage_t store;
    iec104_apci_mgr_t* mgr = iec104_apci_mgr_create_in(&store);
    assert_non_null(mgr);
    assert_int_equal(iec104_apci_tick(mgr, 0), IEC104_EVENT_NONE); /* 未建链不计时 */

    uint64_t t0 = uv_hrtime() / 1000000;
    iec104_apci_set_state(mgr, IEC104_STATE_CONNECTED);
    assert_int_equal(iec104_apci_tick(mgr, t0 + 15000), IEC104_EVENT_NONE);
    assert_int_equal(iec104_apci_tick(mgr, t0 + 21000), IEC104_EVENT_SEND_TESTFR);

    net_buf_t* out = net_buf_alloc(pool, 0);
    assert_non_null(out);
    assert_int_equal(iec104_apci_build_s(mgr, out), 0);
    uint64_t t1 = uv_hrtime() / 1000000;
    assert_int_equal(iec104_apci_tick(mgr, t1 + 15000), IEC104_EVENT_NONE);
    assert_int_equal(iec104_apci_tick(mgr, t1 + 21000), IEC104_EVENT_SEND_TESTFR);

    net_buf_unref(out);
    net_buf_pool_destroy(pool);
}

/* t1 优先于 t2/t3：I 帧 15s 未确认必须报 T1_TIMEOUT，不能被保活分支遮蔽。 */
static void test_apci_t1_timeout_wins_over_t2_t3(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(4, 256, 0);
    assert_non_null(pool);
    iec104_apci_mgr_storage_t store;
    iec104_apci_mgr_t* mgr = iec104_apci_mgr_create_in(&store);
    assert_non_null(mgr);
    iec104_apci_set_state(mgr, IEC104_STATE_CONNECTED);

    net_buf_t* asdu = net_buf_alloc(pool, 0);
    assert_non_null(asdu);
    uint8_t* body = net_buf_simple_add(&asdu->b, 6);
    assert_non_null(body);
    assert_int_equal(iec104_apci_wrap_i(mgr, asdu), 0); /* 第一帧 I 帧，t1 起算 */
    uint64_t t0 = uv_hrtime() / 1000000;

    assert_int_equal(iec104_apci_tick(mgr, t0 + 14000), IEC104_EVENT_NONE);
    assert_int_equal(iec104_apci_tick(mgr, t0 + 16000), IEC104_EVENT_T1_TIMEOUT);

    net_buf_unref(asdu);
    net_buf_pool_destroy(pool);
}

/* t1 到期后驱动必须经产品端口断开链路并复位会话；只断开一次，不产生帧。
 * 慢测：t1 基线来自 uv_hrtime，APCI 没有时间注入点，只能真等 15s；
 * 这条验证的是驱动消费路径，APCI 判定本身由上面那条覆盖。 */
static int g_close_calls;

static int mock_close_link(void* ctx) {
    (void)ctx;
    g_close_calls++;
    return 0;
}

static const edge_slave_source_ops_t k_slave_src_with_close = {
    .resolve = mock_slave_resolve,
    .enumerate = mock_slave_enumerate,
    .close_link = mock_close_link,
};

static void test_t1_timeout_closes_link_once(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(16, 512, 0);
    assert_non_null(pool);
    void* session = make_session_role(pool, PROTOCOL_ROLE_SLAVE);

    net_buf_t* out = NULL;
    slave_exchange_u(session, pool, 0x07, &out); /* STARTDT ACT */
    assert_u_frame(out, 0x0B);
    net_buf_unref(out);

    /* 总召唤应答产生 I 帧：t1 从第一帧起算 */
    g_slave_value = 1.0;
    const uint8_t gi[] = {0x68, 0x0e, 0x00, 0x00, 0x00, 0x00,
                          IEC_TYPE_C_IC_NA_1, 0x01, IEC_COT_ACT, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 20};
    net_buf_t* resp = net_buf_alloc(pool, 0);
    assert_non_null(resp);
    size_t consumed = 0;
    assert_int_equal(iec104_driver.req_slave_exchange(NULL, session, gi, sizeof(gi),
                                                      &k_slave_src_with_close, NULL, resp,
                                                      &consumed),
                     0);
    assert_true(resp->b.len > 0);
    net_buf_unref(resp);

    /* 刚起算：不许断链，也不许把 t1 当成待发帧 */
    g_close_calls = 0;
    net_buf_t* tx = NULL;
    assert_int_equal(iec104_driver.req_tick(session, NULL, NULL, &k_slave_src_with_close, NULL,
                                            pool, &tx),
                     -1);
    assert_int_equal(g_close_calls, 0);
    assert_null(tx);

    int rc = -1;
    uint64_t deadline = uv_hrtime() / 1000000 + 25000;
    while (g_close_calls == 0 && uv_hrtime() / 1000000 < deadline) {
        tx = NULL;
        rc = iec104_driver.req_tick(session, NULL, NULL, &k_slave_src_with_close, NULL, pool, &tx);
        assert_null(tx);
        if (g_close_calls > 0) break;
        uv_sleep(200);
    }
    assert_int_equal(g_close_calls, 1);
    assert_int_equal(rc, 0); /* t1 是状态事件，不产生帧 */

    /* 状态已复位：后续 tick 不会重复断链，链路也不再可发 */
    uv_sleep(1200);
    tx = NULL;
    assert_int_equal(iec104_driver.req_tick(session, NULL, NULL, &k_slave_src_with_close, NULL,
                                            pool, &tx),
                     -1);
    assert_null(tx);
    assert_int_equal(g_close_calls, 1);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(
        iec104_driver.req_prepare(priv, 1, NATIVE(IEC_TYPE_C_SC_NA_1, 0x4001), NULL, 0, NULL, 0,
                                  true, 1.0),
        0);
    net_buf_t* enc = net_buf_alloc(pool, 0);
    assert_non_null(enc);
    assert_int_not_equal(iec104_driver.req_encode(priv, session, enc), 0);

    net_buf_unref(enc);
    free(session);
    net_buf_pool_destroy(pool);
}

/* 持续自发（每个 tick 都有变化点）时 t1 仍要断链：不能因为自发分支每 tick
 * 都提前 return 就永远不消费 APCI 定时器。慢测：t1 基线来自 uv_hrtime，
 * APCI 没有时间注入点，只能真等 15s。 */
static void test_t1_timeout_fires_during_continuous_spontaneous(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(32, 512, 0);
    assert_non_null(pool);
    void* session = make_session_role(pool, PROTOCOL_ROLE_SLAVE);

    net_buf_t* out = NULL;
    slave_exchange_u(session, pool, 0x07, &out); /* STARTDT ACT */
    assert_u_frame(out, 0x0B);
    net_buf_unref(out);

    g_close_calls = 0;
    g_slave_value = 1.0;
    net_buf_t* tx = NULL;
    assert_int_equal(iec104_driver.req_tick(session, NULL, NULL, &k_slave_src_with_close, NULL,
                                            pool, &tx),
                     -1); /* 首帧只建基线 */
    assert_null(tx);

    int spont = 0;
    uint64_t first_ms = 0;
    uint64_t deadline = uv_hrtime() / 1000000 + 25000;
    while (uv_hrtime() / 1000000 < deadline && g_close_calls == 0) {
        g_slave_value += 1.0; /* 每 tick 都变化：自发分支一直有帧可报 */
        tx = NULL;
        int rc = iec104_driver.req_tick(session, NULL, NULL, &k_slave_src_with_close, NULL, pool,
                                        &tx);
        if (g_close_calls > 0) {
            assert_int_equal(rc, 0); /* t1 是状态事件，不产生帧 */
            assert_null(tx);
            break;
        }
        if (tx) {
            assert_int_equal(rc, 0);
            assert_spontaneous_frame(tx);
            if (spont == 0) first_ms = uv_hrtime() / 1000000;
            spont++;
            net_buf_unref(tx);
        } else {
            /* 12 帧滑窗满后组不出 I 帧，但定时器仍须继续被消费 */
            assert_int_equal(rc, -1);
        }
        uv_sleep(200);
    }
    uint64_t close_ms = uv_hrtime() / 1000000;

    assert_int_equal(g_close_calls, 1);
    assert_true(spont >= 12); /* 断链前自发帧确实一直在产出 */
    assert_true(close_ms - first_ms >= 14900);
    assert_true(close_ms - first_ms < 22000);

    /* 状态已复位：不会重复断链，也不再产生帧 */
    g_slave_value += 1.0;
    tx = NULL;
    assert_int_equal(iec104_driver.req_tick(session, NULL, NULL, &k_slave_src_with_close, NULL,
                                            pool, &tx),
                     -1);
    assert_null(tx);
    assert_int_equal(g_close_calls, 1);

    free(session);
    net_buf_pool_destroy(pool);
}

/* t3 到点与自发 I 帧落在同一个 tick 时 TESTFR ACT 也不能丢：断言线上
 * 第二个 APDU 是 68 04 43 00 00 00。慢测：t3 基线是真实时钟，空转等 21s。 */
static void test_spontaneous_frame_does_not_swallow_testfr(void** state) {
    (void)state;
    net_buf_pool_t* pool = net_buf_pool_new(16, 512, 0);
    assert_non_null(pool);
    void* session = make_session_role(pool, PROTOCOL_ROLE_SLAVE);

    net_buf_t* out = NULL;
    slave_exchange_u(session, pool, 0x07, &out); /* STARTDT ACT */
    assert_u_frame(out, 0x0B);
    net_buf_unref(out);

    /* 先建变化基线，再让链路空转超过 t3(20s) */
    g_slave_value = 1.0;
    net_buf_t* tx = NULL;
    assert_int_equal(tick_once(session, pool, &tx), -1);
    assert_null(tx);
    uv_sleep(21000);

    g_slave_value = 2.0; /* 变化与 t3 到点同时发生 */
    tx = NULL;
    assert_int_equal(tick_once(session, pool, &tx), 0);
    assert_spontaneous_frame(tx);

    size_t off = (size_t)(tx->b.data[1] + 2); /* 跳过第一个 APDU（I 帧） */
    assert_true(tx->b.len >= off + 6);
    const uint8_t testfr_act[6] = {0x68, 0x04, 0x43, 0x00, 0x00, 0x00};
    assert_memory_equal(tx->b.data + off, testfr_act, sizeof(testfr_act));
    assert_int_equal(tx->b.len, off + sizeof(testfr_act));
    net_buf_unref(tx);

    /* TESTFR 只发一次：下一 tick 没有变化也不再重复 */
    tx = NULL;
    assert_int_equal(tick_once(session, pool, &tx), -1);
    assert_null(tx);

    free(session);
    net_buf_pool_destroy(pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_control_setpoint_encodes_act_asdu),
        cmocka_unit_test(test_control_rejects_measurement_type),
        cmocka_unit_test(test_control_requires_connected_link),
        cmocka_unit_test(test_stopdt_acknowledged_and_pauses_spontaneous),
        cmocka_unit_test(test_startdt_act_resumes_spontaneous),
        cmocka_unit_test(test_session_reset_restarts_sequence),
        cmocka_unit_test(test_session_reset_releases_unacked_i_frames),
        cmocka_unit_test(test_session_reset_after_ack_keeps_pool_balanced),
        cmocka_unit_test(test_session_reset_repeated_rounds_do_not_leak),
        cmocka_unit_test(test_apci_t3_starts_on_connected),
        cmocka_unit_test(test_apci_t1_timeout_wins_over_t2_t3),
        cmocka_unit_test(test_t1_timeout_closes_link_once),
        cmocka_unit_test(test_t1_timeout_fires_during_continuous_spontaneous),
        cmocka_unit_test(test_spontaneous_frame_does_not_swallow_testfr),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
