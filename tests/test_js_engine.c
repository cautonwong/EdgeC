#include <cmocka.h>
#include <quickjs/quickjs.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "datahub.h"
#include "edge_service.h"
#include "js_engine.h"

static void test_js_eval_basic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    datahub_t hub;
    datahub_init(&hub, loop, "test_js_bus", 64);

    service_t *svc = service_new(NULL);
    service_set_datahub(svc, &hub);

    js_engine_t *engine = js_engine_create(svc, 1024 * 1024);
    assert_non_null(engine);

    // 分步初始化以解决 QuickJS JSValue 结构体初始化限制
    JSValue val;
    val = js_engine_eval(engine, "1 + 2", "test.js");

    int res = 0;
    JS_ToInt32(js_engine_get_context(engine), &res, val);
    assert_int_equal(res, 3);

    JS_FreeValue(js_engine_get_context(engine), val);

    js_engine_destroy(engine);
    service_delete(svc);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_js_eval_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}