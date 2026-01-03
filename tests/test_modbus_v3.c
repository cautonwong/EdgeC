#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "shmring.h"

// --- Mock Contract ---
uint32_t edge_lookup_modbus(uint16_t reg, float *scale) {
    if (reg == 0x0100) {
        *scale = 0.1f;
        return 1001;
    }
    return 0;
}

// 重新定义 CRC 用于测试报文构造
static uint16_t test_crc16(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)buf[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

extern int modbus_rtu_driver_register(void);

static int setup(void **state) {
    (void)state;
    driver_manager_init();
    modbus_rtu_driver_register();
    return 0;
}

static int teardown(void **state) {
    (void)state;
    driver_manager_fini();
    return 0;
}

static void test_modbus_v3_master_cycle(void **state) {
    (void)state;
    edge_driver_ctx_t *ctx = driver_instance_create("ModbusRTU", 1);
    edge_driver_t *drv = driver_manager_find("ModbusRTU");

    net_buf_pool_t pool;
    memset(&pool, 0, sizeof(pool));
    net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL);

    // 1. 请求编码
    uint32_t tags[] = {0x0100};
    net_buf_t *tx = net_buf_alloc(&pool, 0);
    int ret = drv->encode_request(ctx, tags, 1, tx);
    assert_int_equal(ret, 0);

    // 2. 模拟响应 (Val: 2200)
    uint8_t rx_raw[7] = {0x01, 0x03, 0x02, 0x08, 0x98, 0x00, 0x00};
    uint16_t crc = test_crc16(rx_raw, 5);
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