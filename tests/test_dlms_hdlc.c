#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "dlms_hdlc.h"
#include "edge_common.h"
#include "net_buf.h"

static void test_hdlc_crc_ccitt(void** state) {
    (void)state;
    // 标准 X.25 验证： "123456789" -> 0x906E
    uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint16_t crc = edge_crc16_ccitt(data, sizeof(data));
    assert_int_equal(crc, 0x906E);
}

static void test_hdlc_packetize_simple(void** state) {
    (void)state;
    dlms_hdlc_manager_t mgr;
    memset(&mgr, 0, sizeof(mgr));
    dlms_hdlc_reset(&mgr);
    mgr.server_addr = 0x03;
    mgr.client_addr = 0x01;

    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 2, 256, 0, NULL, NULL);

    net_buf_t* apdu = net_buf_alloc(&pool, 0);
    uint8_t mock_apdu[] = {0x01, 0x02, 0x03};
    net_buf_simple_add_mem(&apdu->b, mock_apdu, sizeof(mock_apdu));

    net_buf_t* out = net_buf_alloc(&pool, 0);
    int chunk = dlms_hdlc_packetize(&mgr, apdu, 0, 128, out);

    assert_int_equal(chunk, 3);
    // 检查起始和结束标志
    assert_int_equal(out->b.data[0], 0x7E);
    assert_int_equal(out->b.data[out->b.len - 1], 0x7E);

    net_buf_unref(apdu);
    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

static void _build_hdlc_frame(uint8_t *frame, uint16_t f_len, uint8_t control) {
    frame[0] = (uint8_t)(f_len >> 8);
    frame[1] = (uint8_t)(f_len & 0xFF);
    frame[2] = 0x01;
    frame[3] = 0x03;
    frame[4] = control;
    uint16_t hcs = edge_crc16_ccitt(frame, 5);
    frame[5] = (uint8_t)(hcs & 0xFF);
    frame[6] = (uint8_t)(hcs >> 8);
}

static void test_hdlc_depacketize_rejects_bad_lengths(void** state) {
    (void)state;
    dlms_hdlc_manager_t mgr;
    memset(&mgr, 0, sizeof(mgr));
    net_buf_t* out_apdu = NULL;
    struct net_buf_simple nb;

    // f_len < 9 会让 payload 长度下溢
    uint8_t too_short[9] = {0};
    _build_hdlc_frame(too_short, 8, 0x10);
    net_buf_simple_init_with_data(&nb, too_short, sizeof(too_short));
    nb.len = sizeof(too_short);
    dlms_hdlc_reset(&mgr);
    assert_int_equal(dlms_hdlc_depacketize(&mgr, &nb, NULL, &out_apdu), 1);

    // 池里没有空闲 buffer 时重组分配失败，必须报错而不是解引用
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 1, 256, 0, NULL, NULL), 0);
    net_buf_t* hold = net_buf_alloc(&pool, 0);
    assert_non_null(hold);
    uint8_t frame[19] = {0};
    _build_hdlc_frame(frame, 19, 0x10);
    net_buf_simple_init_with_data(&nb, frame, sizeof(frame));
    nb.len = sizeof(frame);
    dlms_hdlc_reset(&mgr);
    assert_int_equal(dlms_hdlc_depacketize(&mgr, &nb, &pool, &out_apdu), -1);
    net_buf_unref(hold);
    net_buf_pool_destroy(&pool);

    // 重组缓冲放不下整帧
    assert_int_equal(net_buf_pool_init(&pool, 1, 96, 0, NULL, NULL), 0);
    uint8_t big[73] = {0};  // f_len 73 -> payload 64，池尾部空间只有 32
    _build_hdlc_frame(big, 73, 0x10);
    net_buf_simple_init_with_data(&nb, big, sizeof(big));
    nb.len = sizeof(big);
    dlms_hdlc_reset(&mgr);
    assert_int_equal(dlms_hdlc_depacketize(&mgr, &nb, &pool, &out_apdu), -1);
    dlms_hdlc_reset(&mgr);
    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_hdlc_crc_ccitt),
        cmocka_unit_test(test_hdlc_packetize_simple),
        cmocka_unit_test(test_hdlc_depacketize_rejects_bad_lengths),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
