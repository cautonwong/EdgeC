#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "shmring.h"

// --- Mock Contract Table ---
uint32_t edge_lookup_dlt645(uint32_t di, float *out_scale) {
    if (di == 0x00000000) {  // 有功总电能
        *out_scale = 0.01f;
        return 1001;
    }
    return 0;
}

extern int dlt645_driver_register(void);

static int setup(void **state) {
    (void)state;
    driver_manager_init();
    dlt645_driver_register();
    return 0;
}

static int teardown(void **state) {
    (void)state;
    driver_manager_fini();
    return 0;
}

/**
 * 验证 DLT645 工业级批处理解析 (Master 模式)
 */
static void test_dlt645_batch_cycle(void **state) {
    (void)state;
    edge_driver_t *drv = driver_manager_find("DLT645-2007");
    assert_non_null(drv);

    edge_poll_group_t group = {.slave_id = 12345678, .start_addr = 0x00000000, .count = 1};

    // 构造模拟响应 (6.35 kWh)
    // 68 [ADDR] 68 91 08 [DI+33] [VAL+33] CS 16
    // VAL: 635 -> BCD: 35 06 00 00 -> +33: 68 39 33 33
    uint8_t rx[] = {0x68, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00,
                    0x68, 0x91, 0x08, 0x33, 0x33, 0x33, 0x33,  // DI: 0
                    0x68, 0x39, 0x33, 0x33,                    // Val: 6.35
                    0x00, 0x16};
    uint8_t cs = 0;
    for (int i = 0; i < 18; i++) cs += rx[i];
    rx[18] = cs;

    static int found_tid = 0;
    static double found_val = 0;
    void _on_msg(pipe_msg_t * m, void *u) {
        (void)u;
        found_tid = m->tag_id;
        found_val = m->payload.val.f;
    }

    int ret = drv->parse_batch_res(&group, rx, sizeof(rx), _on_msg, NULL);
    assert_int_equal(ret, 0);
    assert_int_equal(found_tid, 1001);
    assert_double_equal(found_val, 6.35, 0.001);
}

/**
 * 验证 DLT645 从站响应周期 (Slave 模式)
 */
static void test_dlt645_slave_cycle(void **state) {
    (void)state;
    edge_driver_t *drv = driver_manager_find("DLT645-2007");

    // 1. 模拟主站请求 (Read DI:0)
    uint8_t req[] = {0x68, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x68,
                     0x11, 0x04, 0x33, 0x33, 0x33, 0x33, 0x00, 0x16};
    uint8_t cs = 0;
    for (int i = 0; i < 14; i++) cs += req[i];
    req[14] = cs;

    uint32_t sid;
    uint16_t di;
    assert_int_equal(drv->parse_request(req, sizeof(req), &sid, &di), 0);
    assert_int_equal(sid, 12345678);

    // 2. 构建响应 (123.45 kWh)
    pipe_msg_t data;
    pipe_msg_init_scalar(&data, 1001, 123.45, 0);
    uint8_t resp[256];
    size_t resp_len;
    assert_int_equal(drv->build_response(sid, 0x11, 0x00000000, &data, resp, &resp_len), 0);

    // 3. 验证响应物理内容
    // 123.45 -> *100 -> 12345 -> BCD: 45 23 01 00 -> +33: 78 56 34 33
    uint8_t *val_ptr = &resp[14];
    assert_int_equal(val_ptr[0], 0x78);
    assert_int_equal(val_ptr[1], 0x56);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dlt645_batch_cycle, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dlt645_slave_cycle, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}