#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "iec104_apci.h"
#include "net_buf.h"

// 模拟 DataHub 和其他依赖
static int setup(void **state) {
    net_buf_pool_t *pool = malloc(sizeof(net_buf_pool_t));
    if (net_buf_pool_init(pool, 10, 512, 0, NULL, NULL) != 0) {
        free(pool);
        return -1;
    }
    *state = pool;
    return 0;
}

static int teardown(void **state) {
    net_buf_pool_t *pool = (net_buf_pool_t *)*state;
    net_buf_pool_destroy(pool);
    free(pool);
    return 0;
}

// 访问驱动内部函数 (通过 extern 或包含源文件，这里由于是驱动测试，通常通过 driver 对象访问)
extern edge_driver_t iec104_driver;

// --- 1. 测试：极短报文 (Fuzz Length) ---
static void test_fuzz_short_packet(void **state) {
    net_buf_pool_t *pool = (net_buf_pool_t *)*state;
    
    edge_driver_ctx_t ctx = {0};
    ctx.pool = pool;
    ctx.priv = malloc(iec104_driver.ctx_size);
    iec104_driver.on_init(&ctx);
    
    // 构造一个只有 0x68 的报文
    net_buf_t *in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);
    
    size_t consumed = 0;
    int rc = iec104_driver.handle_rx(&ctx, in, NULL, NULL, NULL, &consumed);
    
    // 预期：返回 EDGE_ERR_INCOMPLETE
    assert_int_equal(rc, -10); 
    
    net_buf_unref(in);
    iec104_driver.on_cleanup(&ctx);
    free(ctx.priv);
}

// --- 2. 测试：声明长度与实际长度不符 ---
static void test_fuzz_mismatched_length(void **state) {
    net_buf_pool_t *pool = (net_buf_pool_t *)*state;
    
    edge_driver_ctx_t ctx = {0};
    ctx.pool = pool;
    ctx.priv = malloc(iec104_driver.ctx_size);
    iec104_driver.on_init(&ctx);
    
    // 构造报文：头长度声明为 255，但实际只有 6 字节
    net_buf_t *in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);
    net_buf_simple_add_u8(&in->b, 255); // Length
    net_buf_simple_add_mem(&in->b, "\x01\x00\x00\x00", 4);
    
    size_t consumed = 0;
    int rc = iec104_driver.handle_rx(&ctx, in, NULL, NULL, NULL, &consumed);
    
    // 预期：返回 EDGE_ERR_INCOMPLETE，因为缓冲区不够
    assert_int_equal(rc, -10);
    
    net_buf_unref(in);
    iec104_driver.on_cleanup(&ctx);
    free(ctx.priv);
}

// --- 3. 测试：序列号攻击 (Invalid ACK NR) ---
static void test_fuzz_invalid_ack_nr(void **state) {
    net_buf_pool_t *pool = (net_buf_pool_t *)*state;
    
    edge_driver_ctx_t ctx = {0};
    ctx.pool = pool;
    ctx.priv = malloc(iec104_driver.ctx_size);
    iec104_driver.on_init(&ctx);
    
    // 模拟发送了一些包，但还没被确认
    // 注意：我们需要模拟内部状态，或者通过正常接口发送
    // 这里我们直接向 handle_rx 喂一个带有非法确认号的 I 帧
    
    net_buf_t *in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);
    net_buf_simple_add_u8(&in->b, 4); // ASDU 长度为 0，只有 APCI (4字节)
    net_buf_simple_add_u8(&in->b, 0x00); // N(S) = 0
    net_buf_simple_add_u8(&in->b, 0x00); 
    net_buf_simple_add_u8(&in->b, 0xFE); // N(R) = 0x7F (极大值，超过窗口)
    net_buf_simple_add_u8(&in->b, 0xFF); 
    
    size_t consumed = 0;
    // 虽然没有 ASDU，但 APCI 逻辑应该被处理
    int rc = iec104_driver.handle_rx(&ctx, in, NULL, NULL, NULL, &consumed);
    
    // 预期：不应该崩溃。内部 _reclaim_acked_frames 有安全上限。
    assert_int_equal(rc, 0);
    
    net_buf_unref(in);
    iec104_driver.on_cleanup(&ctx);
    free(ctx.priv);
}

// --- 4. 测试：非法 TypeID ---
static void test_fuzz_illegal_type_id(void **state) {
    net_buf_pool_t *pool = (net_buf_pool_t *)*state;
    
    edge_driver_ctx_t ctx = {0};
    ctx.pool = pool;
    ctx.priv = malloc(iec104_driver.ctx_size);
    iec104_driver.on_init(&ctx);
    
    net_buf_t *in = net_buf_alloc(pool, 0);
    net_buf_simple_add_u8(&in->b, 0x68);
    net_buf_simple_add_u8(&in->b, 10); // ASDU Header (6) + Data
    net_buf_simple_add_mem(&in->b, "\x00\x00\x00\x00", 4); // APCI I-Frame
    net_buf_simple_add_u8(&in->b, 255); // Illegal TypeID
    net_buf_simple_add_u8(&in->b, 1);   // VSQ
    net_buf_simple_add_mem(&in->b, "\x06\x00", 2); // COT (6=ACT)
    net_buf_simple_add_mem(&in->b, "\x01\x00", 2); // CASDU
    
    size_t consumed = 0;
    int rc = iec104_driver.handle_rx(&ctx, in, NULL, NULL, NULL, &consumed);
    
    // 预期：处理完成，但不应该报错 (ASDU 解析失败通常在驱动层内部消化)
    assert_int_equal(rc, 0);
    
    net_buf_unref(in);
    iec104_driver.on_cleanup(&ctx);
    free(ctx.priv);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_fuzz_short_packet, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_mismatched_length, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_invalid_ack_nr, setup, teardown),
        cmocka_unit_test_setup_teardown(test_fuzz_illegal_type_id, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
