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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_hdlc_crc_ccitt),
        cmocka_unit_test(test_hdlc_packetize_simple),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
