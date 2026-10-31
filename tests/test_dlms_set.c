/**
 * @file test_dlms_set.c
 * @brief DLMS 写路径：SET-Request 组帧与驱动的入参校验（ADR-002 §S5）
 *
 * 覆盖两层：
 *   1. codec —— SET-Request 的 APDU 布局、SET-Response 的解析
 *   2. 驱动 —— `req_prepare` 对 Data 类型与值域的校验
 *
 * 主站完整握手（AARQ/AARE）没有对端模拟器，不在本用例范围；驱动 encode 要求
 * 会话处于 READY，那属于端到端测试。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dlms_codec.h"
#include "dlms_cosem.h"
#include "driver_dlms.h"
#include "edge_driver.h"
#include "net_buf.h"

extern const edge_driver_t dlms_driver;

/* SET-Request：193 | 01 | invoke | class(2) | OBIS(6) | attr | 00 | value */
static void test_set_request_layout(void** state) {
    (void)state;

    dlms_cosem_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.state = COSEM_STATE_ASSOCIATED; /* codec 只要求"已关联" */

    /* Data: long-unsigned 2200 = 12 08 98 */
    const uint8_t data[] = {DLMS_TYPE_LONG_UNSIGNED, 0x08, 0x98};

    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);
    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);

    assert_int_equal(dlms_cosem_build_set_request(&ctx, 3, "1.0.32.7.0.255", 2, data,
                                                  sizeof(data), out),
                     0);

    const uint8_t* b = out->b.data;
    /* 5(头) + 6(OBIS) + 1(attr) + 1(selector) + data */
    assert_int_equal(out->b.len, 13 + sizeof(data));
    assert_int_equal(b[0], DLMS_SERVICE_SET_REQUEST); /* 193 */
    assert_int_equal(b[1], 0x01);                     /* Normal */
    assert_int_equal(b[3], 0x00);                     /* class-id 高字节 */
    assert_int_equal(b[4], 0x03);                     /* class-id = 3 */
    /* OBIS 1.0.32.7.0.255 */
    const uint8_t obis[6] = {1, 0, 32, 7, 0, 255};
    assert_memory_equal(b + 5, obis, 6);
    assert_int_equal(b[11], 2);  /* attribute index */
    assert_int_equal(b[12], 0x00); /* 无选择访问 */
    assert_memory_equal(b + 13, data, sizeof(data));

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* SET-Response：197 | 01 | invoke | result(0=成功) —— 没有值 */
static void test_set_response_decodes(void** state) {
    (void)state;

    dlms_cosem_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.state = COSEM_STATE_ASSOCIATED;

    uint8_t rsp[] = {DLMS_SERVICE_SET_RESPONSE, 0x01, 0xC1, 0x00};
    struct net_buf_simple nb;
    net_buf_simple_init_with_data(&nb, rsp, sizeof(rsp));
    nb.len = sizeof(rsp);

    dlms_variant_t var;
    assert_int_equal(dlms_cosem_decode_response(&ctx, &nb, &var), 0);

    /* 失败结果必须报错，而不是当成功 */
    uint8_t bad[] = {DLMS_SERVICE_SET_RESPONSE, 0x01, 0xC1, 0x03};
    net_buf_simple_init_with_data(&nb, bad, sizeof(bad));
    nb.len = sizeof(bad);
    assert_int_not_equal(dlms_cosem_decode_response(&ctx, &nb, &var), 0);
}

/* 驱动：写请求的类型与值域校验 */
static void test_driver_write_validation(void** state) {
    (void)state;
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];

    dlms_point_t pt = {.obis = {1, 0, 32, 7, 0, 255}, .class_id = 3, .attribute_index = 2,
                            .data_type = DLMS_WRITE_LONG_UNSIGNED};

    /* 1. 缺类型 → 拒绝 */
    memset(priv, 0, sizeof(priv));
    dlms_point_t no_type = pt;
    no_type.data_type = 0;
    assert_int_not_equal(dlms_driver.req_prepare(priv, 1, 0, NULL, 0, &no_type, sizeof(no_type),
                                                 true, 1.0),
                         0);

    /* 2. 类型不支持 → 拒绝 */
    memset(priv, 0, sizeof(priv));
    dlms_point_t bad_type = pt;
    bad_type.data_type = 0x09; /* octet-string 没有编码器 */
    assert_int_not_equal(dlms_driver.req_prepare(priv, 1, 0, NULL, 0, &bad_type, sizeof(bad_type),
                                                 true, 1.0),
                         0);

    /* 3. long-unsigned 值域 0..65535 */
    memset(priv, 0, sizeof(priv));
    assert_int_not_equal(dlms_driver.req_prepare(priv, 1, 0, NULL, 0, &pt, sizeof(pt), true,
                                                 70000.0),
                         0);
    memset(priv, 0, sizeof(priv));
    assert_int_not_equal(dlms_driver.req_prepare(priv, 1, 0, NULL, 0, &pt, sizeof(pt), true,
                                                 -1.0),
                         0);
    memset(priv, 0, sizeof(priv));
    assert_int_equal(dlms_driver.req_prepare(priv, 1, 0, NULL, 0, &pt, sizeof(pt), true, 65535.0),
                     0);

    /* 4. 读请求不看 data_type，也不受值域限制 */
    memset(priv, 0, sizeof(priv));
    assert_int_equal(dlms_driver.req_prepare(priv, 1, 0, NULL, 0, &no_type, sizeof(no_type), false,
                                             0.0),
                     0);

    /* 5. 点位描述尺寸不对 → 拒绝（void* 不校验长度就是隐患） */
    memset(priv, 0, sizeof(priv));
    assert_int_not_equal(dlms_driver.req_prepare(priv, 1, 0, NULL, 0, &pt, sizeof(pt) - 1, true,
                                                 1.0),
                         0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_set_request_layout),
        cmocka_unit_test(test_set_response_decodes),
        cmocka_unit_test(test_driver_write_validation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
