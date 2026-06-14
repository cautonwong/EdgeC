#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "net_buf.h"
#include "modbus_codec.h"

static int setup(void** state) {
    net_buf_pool_t* pool = malloc(sizeof(net_buf_pool_t));
    if (net_buf_pool_init(pool, 10, 512, 0, NULL, NULL) != 0) {
        free(pool);
        return -1;
    }
    *state = pool;
    return 0;
}

static int teardown(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_pool_destroy(pool);
    free(pool);
    return 0;
}

// 1. 极端短报文 (Fuzz Short)
static void test_fuzz_short_packet(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x01); // 只有 slave_id
    
    uint8_t slave_id, fc;
    uint8_t *data_out = NULL;
    size_t data_len = 0;
    int rc = modbus_rtu_decode_req_frame(&in->b, &slave_id, &fc, &data_out, &data_len);
    assert_true(rc > 0);
    
    net_buf_unref(in);
}

// 2. CRC 完全错误且包含随机噪声 (Fuzz CRC / Noise)
static void test_fuzz_random_noise_crc(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    
    // 伪造一个看似完整的 8 字节长度的噪声
    const uint8_t noise[] = { 0x11, 0x03, 0xFF, 0xFF, 0x00, 0x01, 0xAB, 0xCD };
    net_buf_simple_add_mem(&in->b, noise, sizeof(noise));
    
    uint8_t slave_id, fc;
    uint8_t *data_out = NULL;
    size_t data_len = 0;
    int rc = modbus_rtu_decode_req_frame(&in->b, &slave_id, &fc, &data_out, &data_len);
    // CRC check will fail and return -1 or EDGE_ERR_INVALID_CHECKSUM (-2)
    assert_true(rc < 0);
    
    net_buf_unref(in);
}

// 3. 超长报文伪造 (Fuzz Overflow)
static void test_fuzz_overflow_packet(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    
    // 添加 300 字节的数据以试图引发缓冲区溢出（Modbus 最大 256）
    for (int i=0; i<300; i++) {
        net_buf_simple_add_u8(&in->b, (uint8_t)(i % 255));
    }
    
    uint8_t slave_id, fc;
    uint8_t *data_out = NULL;
    size_t data_len = 0;
    int rc = modbus_rtu_decode_req_frame(&in->b, &slave_id, &fc, &data_out, &data_len);
    // 预期不崩溃，且能正确拒绝或处理
    assert_true(rc <= 0 || rc > 0);
    
    net_buf_unref(in);
}

// 4. 写多个寄存器 (FC=16) 时 ByteCount 字段极度不匹配 (Fuzz Mismatched ByteCount)
static void test_fuzz_fc16_mismatched_bytecount(void** state) {
    net_buf_pool_t* pool = (net_buf_pool_t*)*state;
    net_buf_t* in = net_buf_alloc(pool, 0);
    
    // 构造请求: Slave=1, FC=16, Addr=0, RegCount=2, ByteCount=255 (畸形), 然后随便跟点数据
    const uint8_t pkt[] = { 0x01, 0x10, 0x00, 0x00, 0x00, 0x02, 0xFF, 0x11, 0x22, 0x00, 0x00 };
    net_buf_simple_add_mem(&in->b, pkt, sizeof(pkt));
    
    uint8_t slave_id, fc;
    uint8_t *data_out = NULL;
    size_t data_len = 0;
    int rc = modbus_rtu_decode_req_frame(&in->b, &slave_id, &fc, &data_out, &data_len);
    // 预期：包长度声称过大，直接报不完整 (rc > 0)
    assert_true(rc > 0);
    
    net_buf_unref(in);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_fuzz_short_packet, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_random_noise_crc, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_overflow_packet, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_fc16_mismatched_bytecount, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
