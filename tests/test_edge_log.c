#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "edge_log.h"

static void test_log_basic_sync(void **state) {
    (void)state;
    // 1. 初始化同步日志 (指向不存在的配置会失败，所以用 current config)
    int ret = edge_log_init("config/zlog.conf", false);
    assert_int_equal(ret, 0);

    // 2. 写日志
    LOG_INFO("core", "Hello EdgeOS Sync Log");
    LOG_ERROR("driver", "Simulated driver error %d", 404);

    // 3. 关闭
    edge_log_fini();
}

static void test_log_async_flow(void **state) {
    (void)state;
    // 1. 初始化异步日志
    int ret = edge_log_init("config/zlog.conf", true);
    assert_int_equal(ret, 0);

    // 2. 爆发式写入 (验证非阻塞入队)
    for (int i = 0; i < 100; i++) {
        LOG_DEBUG("stream", "Async burst message #%d", i);
    }

    // 3. 等待消费 (异步特性)
    uv_sleep(200);

    // 4. 关闭 (应等待队列排空)
    edge_log_fini();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_log_basic_sync),
        cmocka_unit_test(test_log_async_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
