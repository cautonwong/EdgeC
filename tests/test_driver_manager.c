#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"
#include "edge_driver.h"

// --- Mock Protocol for Testing ---
static int mock_init_count = 0;
static int mock_init(edge_driver_ctx_t *ctx) {
    mock_init_count++;
    ctx->user_data = (void *)0xABCD;
    return 0;
}

static edge_driver_t mock_protocol = {.name = "MOCK_V3", .ctx_size = 128, .on_init = mock_init};

static int setup(void **state) {
    (void)state;
    mock_init_count = 0;
    driver_manager_init();
    driver_manager_register(&mock_protocol);
    return 0;
}

static int teardown(void **state) {
    (void)state;
    driver_manager_fini();
    return 0;
}

/**
 * 验证 V3 驱动实例的分配与初始化
 */
static void test_driver_instance_lifecycle(void **state) {
    (void)state;

    // 1. 创建实例
    edge_driver_ctx_t *ctx = driver_instance_create("MOCK_V3", 101);
    assert_non_null(ctx);
    assert_int_equal(ctx->device_id, 101);
    assert_non_null(ctx->priv);                        // 应分配了 128 字节
    assert_ptr_equal(ctx->user_data, (void *)0xABCD);  // on_init 应被调用
    assert_int_equal(mock_init_count, 1);

    // 2. 销毁实例
    driver_instance_destroy(ctx, "MOCK_V3");
}

/**
 * 验证无效协议查找
 */
static void test_driver_instance_invalid(void **state) {
    (void)state;
    edge_driver_ctx_t *ctx = driver_instance_create("UNKNOWN_PROTO", 999);
    assert_null(ctx);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_driver_instance_lifecycle, setup, teardown),
        cmocka_unit_test_setup_teardown(test_driver_instance_invalid, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}