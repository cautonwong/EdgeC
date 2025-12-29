#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <uv.h>

#include "datahub.h"
#include "edge_service.h"
#include "js_engine.h"

// 这是一个 Hack，因为 service_t 是不透明的，为了在测试中手动注入 hub
struct service_s {
    uv_loop_t *loop;
    service_options_t opts;
    net_buf_pool_t *pool;
    arbiter_t *arbiter;
    datahub_t *data_hub;
};

static void test_js_eval_basic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    // 1. 创建服务容器
    service_options_t opts = {.mode = "test"};
    service_t *svc = service_new(&opts);

    // 2. 初始化 DataHub 并注入到 svc
    datahub_t hub;
    datahub_init(&hub, loop, "test_bus", 64, NULL, NULL);
    ((struct service_s *)svc)->data_hub = &hub;  // 手动关联，避开 service_run

    // 模拟注入一条数据
    PipeMsg msg;
    pipe_msg_init_scalar(&msg, 1001, 230.5, 123456);
    datahub_publish(&hub, &msg);

    // 3. 创建引擎
    js_engine_t *engine = js_engine_create(svc, 1024 * 1024);
    assert_non_null(engine);

    // Test simple math
    assert_int_equal(js_engine_eval(engine, "1 + 1", "test.js"), 0);

    // Test Bridge API: console.log
    assert_int_equal(js_engine_eval(engine, "console.log('Hello from JS');", "test.js"), 0);

    // Test Bridge API: DataHub (应该能读到 230.5)
    assert_int_equal(
        js_engine_eval(
            engine,
            "const v = DataHub.read(1001); if(v !== 230.5) throw new Error('Data mismatch: ' + v);",
            "test.js"),
        0);

    js_engine_destroy(engine);
    datahub_destroy(&hub);
    service_delete(svc);
}

static void test_js_memory_limit(void **state) {
    (void)state;
    js_engine_t *engine = js_engine_create(NULL, 100 * 1024);
    assert_non_null(engine);

    const char *oom_script =
        "const a = []; for(let i=0; i<10000; i++) a.push(new Array(100).fill(1));";

    int ret = js_engine_eval(engine, oom_script, "oom.js");
    assert_int_not_equal(ret, 0);

    js_engine_destroy(engine);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_js_eval_basic),
        cmocka_unit_test(test_js_memory_limit),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
