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
uint32_t edge_lookup_dlt645_num(uint32_t di, float *out_scale) {
    if (di == 0x00010000) {  // 有功总电能
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
 * 验证 DLT645 V3 事务处理
 */
static void test_dlt645_v3_cycle(void **state) {
    (void)state;
    edge_driver_ctx_t *ctx = driver_instance_create("DLT645-2007", 123456789);
    assert_non_null(ctx);
    edge_driver_t *drv = driver_manager_find("DLT645-2007");

    // 1. 编码请求 (读取 DI: 00010000)
    uint32_t tags[] = {0x00010000};
    net_buf_pool_t pool;
    memset(&pool, 0, sizeof(pool));
    net_buf_pool_init(&pool, 4, 256, 0, NULL, NULL);

    net_buf_t *tx = net_buf_alloc(&pool, 0);
    assert_non_null(tx);
    int ret = drv->encode_request(ctx, tags, 1, tx);
    assert_int_equal(ret, 0);
    assert_int_equal(tx->b.data[0], 0x68);

    // 2. 模拟响应 (6.35 kWh -> DI: 00010000)
    uint8_t rx_raw[] = {0x68, 0x89, 0x67, 0x45, 0x23, 0x01, 0x00,
                        0x68, 0x91, 0x08, 0x33, 0x33, 0x34, 0x33,  // DI: 00010000 + 33
                        0x68, 0x39, 0x33, 0x33,                    // Val: 635 + 33
                        0x00, 0x16};
    // 计算校验和 (从第一个 0x68 到 CS 前)
    uint8_t cs = 0;
    for (int i = 0; i < 18; i++) cs += rx_raw[i];
    rx_raw[18] = cs;

    net_buf_t *rx = net_buf_alloc(&pool, 0);
    assert_non_null(rx);
    net_buf_simple_add_mem(&rx->b, rx_raw, sizeof(rx_raw));

    // 3. 解码响应
    pipe_msg_t msgs[4];
    int count = 0;
    ret = drv->decode_response(ctx, rx, msgs, &count);
    assert_int_equal(ret, 0);
    assert_int_equal(count, 1);
    assert_double_equal(msgs[0].payload.val.f, 6.35, 0.001);

    // 4. 清理 (防止 LeakSanitizer 报警)
    net_buf_unref(tx);
    net_buf_unref(rx);
    driver_instance_destroy(ctx, "DLT645-2007");
    net_buf_pool_destroy(&pool);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_dlt645_v3_cycle, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
