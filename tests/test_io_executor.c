#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "io_executor.h"

/**
 * 验证物理通道创建 (Linux 适配层)
 */
static void test_io_create_uart(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    // 我们无法在没有物理串口的情况下打开成功，但我们可以验证工厂函数的行为
    io_handle_t* h = io_executor_create_uart(loop, "/dev/non_existent_tty", 9600, NULL);
    assert_null(h);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_create_uart),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}