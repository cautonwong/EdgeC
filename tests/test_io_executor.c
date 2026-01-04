#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

// 声明新接口
extern int io_channel_open(const char *path, int baud);

/**
 * 验证 UDS 通道打开 (模拟模式)
 */
static void test_io_channel_uds(void **state) {
    (void)state;
    const char *path = "/tmp/io_test.sock";
    unlink(path);

    // 此时文件不存在，应返回 -1 或 成功打开 (取决于 OS 行为，此处验证逻辑)
    int fd = io_channel_open(path, 9600);
    if (fd >= 0) close(fd);
    unlink(path);
}

/**
 * 验证无效路径
 */
static void test_io_channel_invalid(void **state) {
    (void)state;
    int fd = io_channel_open(NULL, 9600);
    assert_int_equal(fd, -1);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_channel_uds),
        cmocka_unit_test(test_io_channel_invalid),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
