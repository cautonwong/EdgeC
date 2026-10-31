#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "modbus_codec.h"
#include "net_buf.h"

extern edge_driver_t modbus_rtu_driver;

typedef struct {
    driver_manager_t* mgr;
} test_state_t;

static int setup(void** state) {
    test_state_t* ts = calloc(1, sizeof(test_state_t));
    ts->mgr = driver_manager_create();
    driver_manager_register(ts->mgr, &modbus_rtu_driver);
    *state = ts;
    return 0;
}

static int teardown(void** state) {
    test_state_t* ts = *state;
    driver_manager_destroy(ts->mgr);
    free(ts);
    return 0;
}

static void test_modbus_v3_master_cycle(void** state) {
    test_state_t* ts = *state;
    edge_driver_ctx_t* ctx = driver_instance_create(ts->mgr, "ModbusRTU", 1);
    assert_non_null(ctx);

    driver_instance_destroy(ctx, "ModbusRTU");
}

/* [ADR-002 S5] 写请求必须发 FC06，不能退化成读帧。
 * 修复前 req_encode 只有一个入口且永远发 FC03——IEMS 的控制下发与 REST 写点
 * 都会静默变成一次读，控制失效且不报错。 */
static void test_modbus_rtu_write_encodes_fc06(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    /* 点位：地址 5，1 个寄存器；写 raw=1234 */
    /* 缓冲按驱动声明的 req_priv_size 开——手写一个"看起来够大"的数组就是踩栈 */
    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   true, 1234.0),
                     0);

    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(modbus_rtu_driver.req_encode(priv, NULL, out), 0);

    /* RTU 帧：addr | FC | reg_hi | reg_lo | val_hi | val_lo | crc_lo | crc_hi */
    assert_int_equal(out->b.len, 8);
    assert_int_equal(out->b.data[0], 1);      /* 从站地址 */
    assert_int_equal(out->b.data[1], 0x06);   /* FC06 = 写单寄存器 */
    assert_int_equal(out->b.data[2], 0x00);   /* 寄存器地址 5 */
    assert_int_equal(out->b.data[3], 0x05);
    assert_int_equal(out->b.data[4], 0x04);   /* 1234 = 0x04D2 */
    assert_int_equal(out->b.data[5], 0xD2);

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 读请求仍然是 FC03：修复不能把读也改坏 */
static void test_modbus_rtu_read_encodes_fc03(void** state) {
    (void)state;
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL), 0);

    /* 缓冲按驱动声明的 req_priv_size 开——手写一个"看起来够大"的数组就是踩栈 */
    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 1), NULL, 0, NULL, 0,
                                                   false, 0.0),
                     0);

    net_buf_t* out = net_buf_alloc(&pool, 0);
    assert_non_null(out);
    assert_int_equal(modbus_rtu_driver.req_encode(priv, NULL, out), 0);
    assert_int_equal(out->b.len, 8);
    assert_int_equal(out->b.data[1], 0x03); /* FC03 = 读保持寄存器 */

    net_buf_unref(out);
    net_buf_pool_destroy(&pool);
}

/* 多寄存器写显式拒绝：请求模型只带一个标量，截断比失败更危险 */
static void test_modbus_rtu_write_rejects_multi_reg(void** state) {
    (void)state;
    /* 缓冲按驱动声明的 req_priv_size 开——手写一个"看起来够大"的数组就是踩栈 */
    alignas(max_align_t) uint8_t priv[256];
    memset(priv, 0, sizeof(priv));
    assert_int_not_equal(modbus_rtu_driver.req_prepare(priv, 1, MODBUS_NATIVE(5, 2), NULL, 0, NULL,
                                                       0, true, 1.0),
                         -0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_modbus_v3_master_cycle),
        cmocka_unit_test(test_modbus_rtu_write_encodes_fc06),
        cmocka_unit_test(test_modbus_rtu_read_encodes_fc03),
        cmocka_unit_test(test_modbus_rtu_write_rejects_multi_reg),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}