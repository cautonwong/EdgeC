#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "shmring.h"
#include "edge_common.h"

// 移除局部 edge_lookup_modbus 声明，由 contract_iems 提供

extern int modbus_rtu_driver_register(void);

static int setup(void **state) {
    (void)state;
    driver_manager_init();
    // 驱动管理器初始化时已经注册了核心驱动，此处无需重复注册
    return 0;
}

static int teardown(void **state) {
    (void)state;
    driver_manager_fini();
    return 0;
}

static void test_modbus_v3_master_cycle(void **state) {
    (void)state;
    // 使用 DeviceID 1 (对齐 IEMS 冒烟标准)
    edge_driver_ctx_t *ctx = driver_instance_create("ModbusRTU", 1);
    assert_non_null(ctx);
    edge_driver_t *drv = driver_manager_find("ModbusRTU");
    assert_non_null(drv);

    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL);

    // 1. 请求编码 (使用 IEMS 契约中存在的 0x0000 地址)
    uint32_t tags[] = {1001}; // Tag 1001 -> Voltage
    net_buf_t *tx = net_buf_alloc(&pool, 0);
    int ret = drv->encode_request(ctx, tags, 1, tx);
    assert_int_equal(ret, 0);

    // 2. 模拟响应 (Val: 2200 -> 220.0V)
    uint8_t rx_raw[7] = {0x01, 0x03, 0x02, 0x08, 0x98, 0x00, 0x00};
    uint16_t crc = edge_crc16_modbus(rx_raw, 5);
    rx_raw[5] = (uint8_t)(crc & 0xFF);
    rx_raw[6] = (uint8_t)(crc >> 8);

    net_buf_t *rx = net_buf_alloc(&pool, 0);
    net_buf_simple_add_mem(&rx->b, rx_raw, sizeof(rx_raw));

    pipe_msg_t msgs[4];
    int count = 0;
    ret = drv->decode_response(ctx, rx, msgs, &count);
    assert_int_equal(ret, 0);
    assert_double_equal(msgs[0].payload.val.f, 220.0, 0.001);

    // 3. 清理
    net_buf_unref(tx);
    net_buf_unref(rx);
    driver_instance_destroy(ctx, "ModbusRTU");
    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_modbus_v3_master_cycle, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
