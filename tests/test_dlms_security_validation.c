#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "dlms_codec.h"
#include "dlms_cosem.h"
#include "edge_driver.h"
#include "net_buf.h"

// 访问驱动内部
extern const edge_driver_t dlms_driver;

static int setup(void** state) {
    net_buf_pool_t* pool = malloc(sizeof(net_buf_pool_t));
    net_buf_pool_init(pool, 10, 512, 0, NULL, NULL);
    *state = pool;
    return 0;
}

static int teardown(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_pool_destroy(pool);
    free(pool);
    return 0;
}

// --- 1. 测试：安全握手验证 (Security Handshake) ---

static void test_dlms_handshake_no_ciphering(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    dlms_cosem_ctx_t cosem_ctx = {0};
    dlms_cosem_reset(&cosem_ctx);

    // AARQ with "LN without ciphering" OID: 60 85 74 05 08 01 01
    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, DLMS_SERVICE_AARQ);
    net_buf_simple_add_mem(&in->b, "\x60\x85\x74\x05\x08\x01\x01", 7);

    int rc = dlms_cosem_parse_aarq(&cosem_ctx, &in->b);
    assert_int_equal(rc, 0);
    assert_int_equal(cosem_ctx.security_level, DLMS_SEC_NONE);

    net_buf_unref(in);
    // dlms_driver.on_cleanup 不存在
}

static void test_dlms_handshake_with_ciphering(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    dlms_cosem_ctx_t cosem_ctx = {0};
    dlms_cosem_reset(&cosem_ctx);

    // AARQ with "LN with ciphering" OID: 60 85 74 05 08 01 02
    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, DLMS_SERVICE_AARQ);
    net_buf_simple_add_mem(&in->b, "\x60\x85\x74\x05\x08\x01\x02", 7);

    int rc = dlms_cosem_parse_aarq(&cosem_ctx, &in->b);
    assert_int_equal(rc, 0);
    assert_int_equal(cosem_ctx.security_level, DLMS_SEC_AUTHENTICATED_ENCRYPTED);

    net_buf_unref(in);
    // dlms_driver.on_cleanup 不存在
}

// --- 2. 测试：对象访问越权 (Object Access Violation) ---

static void test_dlms_access_violation_blocked(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;

    edge_driver_ctx_t ctx = {0};
    ctx.pool = pool;
    ctx.priv = calloc(1, dlms_driver.ctx_size);
    dlms_driver.on_init(&ctx);

    // 1. 模拟未加密握手
    net_buf_t* aarq = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&aarq->b, DLMS_SERVICE_AARQ);
    net_buf_simple_add_mem(&aarq->b, "\x60\x85\x74\x05\x08\x01\x01", 7);

    // 构造合法的 HDLC
    net_buf_t* hdlc_aarq = net_buf_alloc(pool, 0);
    uint8_t aarq_raw[] = {
        0x7E, 0xA0, 0x12, 0x03, 0x01, 0x11, 0x00, 0x00,  // HCS
        0x60, 0x60, 0x85, 0x74, 0x05, 0x08, 0x01, 0x01,  // AARQ Tag + OID
        0x00, 0x00, 0x7E                                 // FCS
    };
    uint16_t hcs = edge_crc16_ccitt(aarq_raw + 1, 5);
    aarq_raw[6] = (uint8_t)(hcs & 0xFF);
    aarq_raw[7] = (uint8_t)(hcs >> 8);
    net_buf_simple_add_mem(&hdlc_aarq->b, aarq_raw, sizeof(aarq_raw));

    net_buf_t* resp = net_buf_alloc(pool, 0);
    size_t consumed = 0;
    /* [ADR-002] 从站面改走 req_slave_exchange：驱动不查表、不读影子表 */
    dlms_driver.req_slave_exchange(ctx.priv, hdlc_aarq->b.data, hdlc_aarq->b.len, NULL, NULL,
                                   resp, &consumed);

    // 2. 模拟请求敏感 OBIS (1.x.x.x)
    net_buf_t* get_req = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&get_req->b, DLMS_SERVICE_GET_REQUEST);
    net_buf_simple_add_mem(&get_req->b, "\x01\xC1\x00\x01", 4);
    net_buf_simple_add_mem(&get_req->b, "\x01\x01\x01\x01\x01\x01", 6);
    net_buf_simple_add_u8(&get_req->b, 0x02);

    net_buf_t* hdlc_get = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&hdlc_get->b, 0x7E);
    uint8_t get_head[] = {0xA0, 0x16, 0x03, 0x01, 0x11};
    net_buf_simple_add_mem(&hdlc_get->b, get_head, sizeof(get_head));
    uint16_t hcs_get = edge_crc16_ccitt(get_head, sizeof(get_head));
    net_buf_simple_add_le16(&hdlc_get->b, hcs_get);
    net_buf_simple_add_mem(&hdlc_get->b, get_req->b.data, get_req->b.len);
    net_buf_simple_add_mem(&hdlc_get->b, "\x00\x00\x7E", 3);

    int rc = dlms_driver.req_slave_exchange(ctx.priv, hdlc_get->b.data, hdlc_get->b.len, NULL, NULL,
                                            resp, &consumed);
    assert_int_equal(rc, -1); /* 未认证访问受保护 OBIS 必须被拒 */

    net_buf_unref(aarq);
    net_buf_unref(get_req);
    net_buf_unref(hdlc_aarq);
    net_buf_unref(hdlc_get);
    net_buf_unref(resp);
    // dlms_driver.on_cleanup 不存在
    free(ctx.priv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dlms_handshake_no_ciphering, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dlms_handshake_with_ciphering, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dlms_access_violation_blocked, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
