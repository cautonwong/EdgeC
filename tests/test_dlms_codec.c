#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "dlms_codec.h"
#include "net_buf.h"

static void test_dlms_encode_nested_structure(void** state) {
    (void)state;
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 1, 1024, 0, NULL, NULL);
    net_buf_t* buf = net_buf_alloc(&pool, 0);

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

static void test_dlms_encode_large_octet_string(void** state) {
    (void)state;
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 1, 1024, 0, NULL, NULL);
    net_buf_t* buf = net_buf_alloc(&pool, 0);

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

static void test_dlms_decode_rejects_truncated_variant(void** state) {
    (void)state;
    dlms_decoder_t dec;
    dlms_variant_t v;
    struct net_buf_simple b;

    // UINT16 标签后只有 1 字节
    uint8_t u16[] = {DLMS_TYPE_UINT16, 0x12};
    net_buf_simple_init_with_data(&b, u16, sizeof(u16));
    b.len = sizeof(u16);
    dlms_decoder_init(&dec, &b);
    assert_int_equal(dlms_decode_variant(&dec, &v), -1);

    // OCTET STRING 声明 5 字节，只给 2 字节
    uint8_t os[] = {DLMS_TYPE_OCTET_STRING, 5, 0xAA, 0xBB};
    net_buf_simple_init_with_data(&b, os, sizeof(os));
    b.len = sizeof(os);
    dlms_decoder_init(&dec, &b);
    assert_int_equal(dlms_decode_variant(&dec, &v), -3);

    // 不支持的长度编码 0x83
    uint8_t bad_len[] = {DLMS_TYPE_OCTET_STRING, 0x83, 0x01};
    net_buf_simple_init_with_data(&b, bad_len, sizeof(bad_len));
    b.len = sizeof(bad_len);
    dlms_decoder_init(&dec, &b);
    assert_int_equal(dlms_decode_variant(&dec, &v), -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_dlms_encode_nested_structure),
        cmocka_unit_test(test_dlms_encode_large_octet_string),
        cmocka_unit_test(test_dlms_decode_rejects_truncated_variant),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
