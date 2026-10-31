/**
 * @file test_driver_dlt698.c
 * @brief DLT698 驱动的主站路径：GET 与 SET（ADR-002 §S5）
 *
 * 只测"结构体 ↔ 字节"：不起总线、不建会话、不查点位表。
 *
 * 帧格式（codec 的约定）：
 *   68 | L(2, LE) | 43 | SA(len+1) | CA | HCS(2) | APDU | FCS(2) | 16
 * 所以 APDU 起点 = 4 + (1 + sa_len) + 1 + 2 = 8 + sa_len。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dlt698_codec.h"
#include "driver_dlt698.h"
#include "edge_driver.h"
#include "net_buf.h"

extern const edge_driver_t dlt698_driver;

#define DLT698_SA_LEN 6
#define APDU_OFF (8 + DLT698_SA_LEN)

static int prepare_into(uint8_t* priv, const void* desc, size_t desc_len, uint32_t oad,
                        bool is_write, double value) {
    memset(priv, 0, REQ_DRV_PRIV_MAX);
    return dlt698_driver.req_prepare(priv, 1, oad, NULL, 0, desc, desc_len, is_write, value);
}

/* 只在 prepare 成功时使用；失败时返回 NULL 且调用方应先断言过 rc */
static net_buf_t* encode_once(net_buf_pool_t* pool, const void* desc, size_t desc_len,
                              uint32_t oad, bool is_write, double value) {
    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];
    if (prepare_into(priv, desc, desc_len, oad, is_write, value) != 0) return NULL;
    net_buf_t* out = net_buf_alloc(pool, 0);
    if (!out) return NULL;
    if (dlt698_driver.req_encode(priv, NULL, out) != 0) {
        net_buf_unref(out);
        return NULL;
    }
    return out;
}

/* FCS 覆盖 [1, len-3)，末 3 字节是 FCS(2) + 0x16 */
static void assert_frame_sealed(const net_buf_t* f) {
    assert_true(f->b.len > 6);
    assert_int_equal(f->b.data[0], DLT698_HEAD);
    assert_int_equal(f->b.data[f->b.len - 1], DLT698_END);

    /* L 字段 = 整帧长度 - 1（codec 的约定，GET/SET 一致） */
    uint16_t l = (uint16_t)(f->b.data[1] | (f->b.data[2] << 8));
    assert_int_equal(l, (uint16_t)(f->b.len - 1));

    uint16_t want = dlt698_calculate_fcs(f->b.data + 1, (size_t)(f->b.len - 4));
    uint16_t got = (uint16_t)(f->b.data[f->b.len - 3] | (f->b.data[f->b.len - 2] << 8));
    assert_int_equal(got, want);
}

/* GET：05 | 01 | PIID | OAD(4) | 00（无选择访问） */
static void test_get_request_layout(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    net_buf_t* f = encode_once(&pool, NULL, 0, 0x20000200u, false, 0.0);
    assert_non_null(f);

    const uint8_t* apdu = f->b.data + APDU_OFF;
    assert_int_equal(apdu[0], DLT698_SVC_GET_REQ);
    assert_int_equal(apdu[1], 0x01);
    assert_int_equal((apdu[3] << 24) | (apdu[4] << 16) | (apdu[5] << 8) | apdu[6], 0x20000200);
    assert_int_equal(apdu[7], 0x00);
    assert_frame_sealed(f);

    net_buf_unref(f);
    net_buf_pool_destroy(&pool);
}

/* SET long-unsigned：06 | 01 | PIID | OAD(4) | 12 | hi | lo */
static void test_set_long_unsigned(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    dlt698_write_desc_t desc = {.type = 0x12};
    net_buf_t* f = encode_once(&pool, &desc, sizeof(desc), 0x20000200u, true, 2200.0);
    assert_non_null(f);

    const uint8_t* apdu = f->b.data + APDU_OFF;
    assert_int_equal(apdu[0], DLT698_SVC_SET_REQ); /* 0x06 */
    assert_int_equal(apdu[1], 0x01);
    assert_int_equal(apdu[7], 0x12); /* long-unsigned 标签 */
    assert_int_equal(apdu[8], (2200 >> 8) & 0xFF);
    assert_int_equal(apdu[9], 2200 & 0xFF);
    /* SET 没有 GET 那个"选择访问"尾字节：数据段之后就结束了 */
    assert_int_equal(f->b.len, (size_t)APDU_OFF + 10 + 3);
    assert_frame_sealed(f);

    net_buf_unref(f);
    net_buf_pool_destroy(&pool);
}

/* SET double-long：06 | 01 | PIID | OAD(4) | 05 | 4 字节大端 */
static void test_set_double_long(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    dlt698_write_desc_t desc = {.type = 0x05};
    net_buf_t* f = encode_once(&pool, &desc, sizeof(desc), 0x20000200u, true, 123456.0);
    assert_non_null(f);

    const uint8_t* apdu = f->b.data + APDU_OFF;
    assert_int_equal(apdu[7], 0x05);
    assert_int_equal(apdu[8], 0x00);
    assert_int_equal(apdu[9], 0x01);
    assert_int_equal(apdu[10], 0xE2);
    assert_int_equal(apdu[11], 0x40);
    assert_frame_sealed(f);

    net_buf_unref(f);
    net_buf_pool_destroy(&pool);
}

/* 写请求必须带类型描述，且类型/值域合法——宁可失败也不发"看起来对"的报文 */
static void test_set_rejects_bad_input(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 512, 0, NULL, NULL), 0);

    alignas(max_align_t) uint8_t priv[REQ_DRV_PRIV_MAX];

    /* 1. 缺描述 */
    assert_int_not_equal(prepare_into(priv, NULL, 0, 0x20000200u, true, 1.0), 0);

    /* 2. 类型不支持（0x11 unsigned 没有编码器） */
    dlt698_write_desc_t bad_type = {.type = 0x11};
    assert_int_not_equal(prepare_into(priv, &bad_type, sizeof(bad_type), 0x20000200u, true, 1.0), 0);

    /* 3. long-unsigned 越界（>65535 / <0）：截断写入比失败更危险 */
    dlt698_write_desc_t lu = {.type = 0x12};
    assert_int_not_equal(prepare_into(priv, &lu, sizeof(lu), 0x20000200u, true, 70000.0), 0);
    assert_int_not_equal(prepare_into(priv, &lu, sizeof(lu), 0x20000200u, true, -1.0), 0);

    /* 3b. 合法值必须通过，证明拒绝不是"一律拒绝" */
    assert_int_equal(prepare_into(priv, &lu, sizeof(lu), 0x20000200u, true, 65535.0), 0);

    /* 4. 读请求不受影响 */
    net_buf_t* f = encode_once(&pool, NULL, 0, 0x20000200u, false, 0.0);
    assert_non_null(f);
    net_buf_unref(f);

    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_get_request_layout),
        cmocka_unit_test(test_set_long_unsigned),
        cmocka_unit_test(test_set_double_long),
        cmocka_unit_test(test_set_rejects_bad_input),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
