/**
 * @file test_driver_iec101.c
 * @brief IEC101 驱动 request-native 契约测试（ADR-002）
 *
 * 只测驱动自己负责的部分：点位标识编码、主站组帧、从站对链路帧的应答。
 * 可变帧的链路层封装（`iec101_link_build_variable`）在协议层还没实现——
 * 原驱动也是"此处简化"，本次迁移保持同等完整性，不假装它存在。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "edge_driver.h"
#include "edge_slave_plane.h"
#include "iec101_link.h"
#include "iec_asdu.h"
#include "net_buf.h"

#define TEST_BUF 512

extern const edge_driver_t iec101_driver;

/* 驱动内部用的编码约定：类型标识(高 8 位) + IOA(低 24 位) */
#define NATIVE(type, ioa) ((((uint32_t)(type) & 0xFFu) << 24) | ((uint32_t)(ioa) & 0xFFFFFFu))

static void test_ctx_size_fits_test_buffer(void** state) {
    (void)state;
    /* 测试用栈缓冲必须装得下驱动的持久上下文，否则 req_prepare 会写越界 */
    assert_true(iec101_driver.ctx_size <= 4096);
    assert_true(iec101_driver.ctx_size > 0);
}

static void test_native_id_encoding(void** state) {
    (void)state;
    uint32_t id = NATIVE(IEC_TYPE_M_ME_NC_1, 0x4001);
    assert_int_equal((id >> 24) & 0xFF, IEC_TYPE_M_ME_NC_1);
    assert_int_equal(id & 0xFFFFFF, 0x4001);

    /* IOA 用满 24 位也不能被类型位污染 */
    uint32_t big = NATIVE(IEC_TYPE_M_SP_NA_1, 0xFFFFFF);
    assert_int_equal((big >> 24) & 0xFF, IEC_TYPE_M_SP_NA_1);
    assert_int_equal(big & 0xFFFFFF, 0xFFFFFF);
}

/* 主站：req_prepare 只收点位标识，req_encode 组出带该 IOA 的 ASDU */
static void test_master_encode_uses_point_id(void** state) {
    (void)state;
    uint8_t priv[4096]; /* 必须 >= iec101_driver.ctx_size */
    memset(priv, 0, sizeof(priv));

    uint32_t pid = NATIVE(IEC_TYPE_M_ME_NC_1, 0x4001);
    assert_int_equal(iec101_driver.req_prepare(priv, 7, pid, NULL, 0, NULL, 0, false, 0.0), 0);

    uint8_t mem[TEST_BUF];
    net_buf_t out;
    net_buf_simple_init_with_data(&out.b, mem, sizeof(mem));
    assert_int_equal(iec101_driver.req_encode(priv, NULL, &out), 0);
    assert_true(out.b.len > 0);

    /* 类型标识必须出现在 ASDU 里（头部布局由协议层决定，不在这里钉死位置） */
    int found_type = 0;
    for (size_t i = 0; i < out.b.len; i++) {
        if (out.b.data[i] == IEC_TYPE_M_ME_NC_1) {
            found_type = 1;
            break;
        }
    }
    assert_true(found_type);
    /* ASDU 里 IOA/CASDU 的具体字节序由协议层 addr_conf 决定，
     * 不在驱动测试里钉死——这里只验"驱动把点位标识交给了协议层组帧"。
     * 字节级布局的验证属于 iec_asdu 的职责。 */
}

/* 从站：链路状态请求（0x10）→ 回链路状态帧 */
static void test_slave_answers_link_status(void** state) {
    (void)state;
    uint8_t priv[4096]; /* 必须 >= iec101_driver.ctx_size */
    memset(priv, 0, sizeof(priv));
    assert_int_equal(iec101_driver.req_prepare(priv, 1, NATIVE(IEC_TYPE_M_ME_NC_1, 1), NULL, 0, NULL, 0, false, 0.0), 0);

    /* 用协议层 helper 造一个合法的链路状态请求 */
    iec101_link_mgr_t link = {.link_addr = 1, .addr_len = 1};
    iec101_ctrl_t req = {.prm = 1, .func = 9};
    uint8_t qmem[64];
    net_buf_t q;
    net_buf_simple_init_with_data(&q.b, qmem, sizeof(qmem));
    assert_int_equal(iec101_link_build_fixed(&link, req, &q), 0);
    assert_true(q.b.len > 0);

    uint8_t rmem[TEST_BUF];
    net_buf_t out;
    net_buf_simple_init_with_data(&out.b, rmem, sizeof(rmem));

    size_t consumed = 0;
    int rc = iec101_driver.req_slave_exchange(priv, NULL, q.b.data, q.b.len, NULL, NULL, &out, &consumed);
    assert_int_equal(rc, 0);
    assert_true(consumed > 0);
    /* 必须组出应答；帧头细节由协议层 iec101_link 决定，这里只验"有输出" */
    assert_true(out.b.len > 0);
}

/* 主站：req_tick 周期请求链路状态，且不需要任何端口 */
static void test_master_tick_builds_link_request(void** state) {
    (void)state;
    uint8_t priv[4096]; /* 必须 >= iec101_driver.ctx_size */
    memset(priv, 0, sizeof(priv));
    assert_int_equal(iec101_driver.req_prepare(priv, 3, NATIVE(IEC_TYPE_M_SP_NA_1, 1), NULL, 0, NULL, 0, false, 0.0), 0);

    net_buf_pool_t* pool = net_buf_pool_new(4, 256, 0);
    assert_non_null(pool);
    net_buf_t* out_tx = NULL;
    assert_int_equal(iec101_driver.req_tick(priv, NULL, NULL, NULL, NULL, pool, &out_tx), 0);
    assert_non_null(out_tx);
    assert_true(out_tx->b.len > 0);
    net_buf_unref(out_tx);
    net_buf_pool_destroy(pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ctx_size_fits_test_buffer),
        cmocka_unit_test(test_native_id_encoding),
        cmocka_unit_test(test_master_encode_uses_point_id),
        cmocka_unit_test(test_slave_answers_link_status),
        cmocka_unit_test(test_master_tick_builds_link_request),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
