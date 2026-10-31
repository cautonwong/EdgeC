/**
 * @file test_driver_dlt645.c
 * @brief 645 驱动层契约测试（ADR-002 之后：request-native）
 *
 * 旧版本走 legacy decode_response，断言 out[0].tag_id == DI —— 那正是 L2 泄漏
 * （要求 TagID ≡ DI）。现在驱动只回值，tag_id 由请求层贴，所以这里断言的是
 * raw 与 di，而不是 tag_id。
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_dlt645.h"
#include "driver_manager.h"
#include "edge_driver.h"

extern const edge_driver_t dlt645_driver;

static void _prepare(dlt645_req_t* priv, uint32_t di) {
    memset(priv, 0, sizeof(*priv));
    assert_int_equal(dlt645_driver.req_prepare(priv, 0, di, NULL, 0, NULL, 0), 0);
}

/* 验收台架的真实应答帧：68 010000000000 68 91 06 33343435 4a56 d8 16
 * L=0x06 = DI(4) + 数据(2)，数据 4a 56 减 0x33 得 BCD "17""23" → 2317。
 * 回归钉：此前按 L>=8 判定，L=6 的帧会被静默丢弃。 */
static void test_dlt645_acceptance_frame(void** state) {
    (void)state;
    const uint8_t frame[] = {0x68, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x91,
                             0x06, 0x33, 0x34, 0x34, 0x35, 0x4a, 0x56, 0xd8, 0x16};
    dlt645_req_t priv;
    _prepare(&priv, 0x02010100);

    size_t consumed = 0;
    double value = 0.0;
    assert_int_equal(dlt645_driver.req_decode(&priv, NULL, frame, sizeof(frame), &consumed, &value), 0);
    assert_int_equal(consumed, 18);
    assert_int_equal(priv.di, 0x02010100);   /* DI 被解析出来，但不会当 tag_id 抛出去 */
    assert_int_equal(priv.raw, 2317);        /* 未乘 scale，标定归上层 */
    assert_true(value > 2316.0 && value < 2318.0);
}

/* 四字节数据段（L=0x08，如组合有功）同样要认 */
static void test_dlt645_four_byte_payload(void** state) {
    (void)state;
    uint8_t frame[] = {0x68, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x91,
                       0x08, 0x33, 0x34, 0x34, 0x35, 0x4a, 0x56, 0x33, 0x33, 0x00, 0x16};
    uint8_t cs = 0;
    for (size_t i = 0; i < 18; i++) cs = (uint8_t)(cs + frame[i]);
    frame[18] = cs;

    dlt645_req_t priv;
    _prepare(&priv, 0x02010100);

    size_t consumed = 0;
    double value = 0.0;
    assert_int_equal(dlt645_driver.req_decode(&priv, NULL, frame, sizeof(frame), &consumed, &value), 0);
    assert_int_equal(consumed, 20);
    assert_int_equal(priv.raw, 2317);
    assert_int_equal(priv.data_len, 4);
}

/* prepare → encode → decode 全链路：驱动自己的输出自己必须能解 */
static void test_dlt645_roundtrip(void** state) {
    (void)state;
    uint8_t meta[1][6] = {{0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    dlt645_req_t req;
    memset(&req, 0, sizeof(req));
    assert_int_equal(dlt645_driver.req_prepare(&req, 0, 0x02010100, meta, 1, NULL, 0), 0);

    uint8_t mem[256];
    net_buf_t out;
    net_buf_simple_init_with_data(&out.b, mem, sizeof(mem));
    assert_int_equal(dlt645_driver.req_encode(&req, NULL, &out), 0);
    assert_int_equal(out.b.len, 16);
    assert_int_equal(out.b.data[0], 0x68);
    assert_int_equal(out.b.data[8], 0x11);
    assert_memory_equal(&out.b.data[1], meta[0], 6);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlt645_acceptance_frame),
        cmocka_unit_test(test_dlt645_four_byte_payload),
        cmocka_unit_test(test_dlt645_roundtrip),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
