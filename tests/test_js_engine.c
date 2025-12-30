#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "edge_service.h"
#include "js_engine.h"

static void test_js_eval_basic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    // 1. 初始化容器 (仅分配内存，不启动 Loop)
    service_options_t opts = {0};
    service_t *svc = service_new(&opts);
    service_set_loop(svc, loop);

    // 2. 创建 Mock 依赖并手动注入，避开阻塞的 service_run
    datahub_t hub;
    datahub_init(&hub, loop, "test_js_bus_fixed", 64, NULL, NULL);

    // 关键修复：使用非阻塞 Setter 注入依赖
    service_set_datahub(svc, &hub);

    js_engine_t *engine = js_engine_create(svc, 1024 * 1024);
    assert_non_null(engine);

    const char *code = "console.log('[JS] Test Start'); 1 + 2;";
    int ret = js_engine_eval(engine, code, "test.js");
    assert_int_equal(ret, 0);

    // 3. 清理
    js_engine_destroy(engine);
    datahub_destroy(&hub);
    service_delete(svc);

    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

static void test_js_memory_limit(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    service_options_t opts = {0};
    service_t *svc = service_new(&opts);
    service_set_loop(svc, loop);

    js_engine_t *engine = js_engine_create(svc, 32 * 1024);
    assert_non_null(engine);

    const char *bad_code = "let s = 'a'; while(s.length < 100000) s += s;";
    js_engine_eval(engine, bad_code, "overflow.js");
    // 预期不崩溃即为成功，QuickJS 内部会抛出 InternalError

    js_engine_destroy(engine);
    service_delete(svc);
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_js_eval_basic),
        cmocka_unit_test(test_js_memory_limit),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
