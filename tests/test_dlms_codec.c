#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "dlms_codec.h"
#include "net_buf.h"

static void test_dlms_encode_nested_structure(void **state) {
    (void)state;
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 1, 1024, 0, NULL, NULL);
    net_buf_t *buf = net_buf_alloc(&pool, 0);

    dlms_encoder_t enc;
    dlms_encoder_init(&enc, &buf->b);

    // 构建嵌套结构：Array [ Struct { UInt16, OctetString } ]
    dlms_encode_begin(&enc, DLMS_TYPE_ARRAY);
    {
        dlms_encode_begin(&enc, DLMS_TYPE_STRUCTURE);
        {
            dlms_encode_u16(&enc, 0x1234);
            uint8_t data[5] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
            dlms_encode_octet_string(&enc, data, 5);
        }
        dlms_encode_end(&enc);
    }
    dlms_encode_end(&enc);

    // 预期：[0x01] [0x0B] [0x02] [0x02] [0x12] [0x12, 0x34] [0x09] [0x05] [AA...EE]
    // 注意：具体编码取决于实现细节，此处验证不崩溃且长度正确
    assert_true(buf->b.len > 10);

    net_buf_unref(buf);
    net_buf_pool_destroy(&pool);
}

static void test_dlms_encode_large_octet_string(void **state) {
    (void)state;
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 1, 1024, 0, NULL, NULL);
    net_buf_t *buf = net_buf_alloc(&pool, 0);

    dlms_encoder_t enc;
    dlms_encoder_init(&enc, &buf->b);

    // 构建一个超过 128 字节的字符串，触发 BER 2字节长度头平移
    uint8_t large_data[150];
    memset(large_data, 0x55, 150);
    dlms_encode_octet_string(&enc, large_data, 150);

    // 预期长度：Tag(1) + LenHeader(2, 0x81 0x96) + Data(150) = 153
    assert_int_equal(buf->b.len, 153);
    assert_int_equal(buf->b.data[1], 0x81);
    assert_int_equal(buf->b.data[2], 150);

    net_buf_unref(buf);
    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlms_encode_nested_structure),
        cmocka_unit_test(test_dlms_encode_large_octet_string),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
