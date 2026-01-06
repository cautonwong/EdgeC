#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "edge_log.h"

static void test_log_basic_sync(void **state) {
    (void)state;
    // [补强] 尝试在多个可能路径下查找配置文件
    if (access("config/zlog.conf", F_OK) != 0) {
        if (access("../config/zlog.conf", F_OK) == 0) {
            chdir("..");  // 回退到根目录以运行测试
        }
    }
    int ret = edge_log_init(NULL, false);
    assert_int_equal(ret, 0);
    LOG_INFO(LOG_CAT_CORE, "Synchronous log test pass.");
}

static void test_log_async_flow(void **state) {
    (void)state;
    LOG_DEBUG(LOG_CAT_CORE, "Asynchronous log flow triggered.");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_log_basic_sync),
        cmocka_unit_test(test_log_async_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}