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
    datahub_t hub;
    datahub_init(&hub, loop, "test_js", 64, 1024, NULL);

    // 使用新版 service_new
    service_t *svc = service_new(loop, &hub, NULL, NULL, NULL, NULL, NULL);

    // 修正：js_engine_create 需要内存限制参数
    js_engine_t *js = js_engine_create(svc, 1024 * 1024);
    assert_non_null(js);

    const char *script = "1 + 2";
    // 修正：js_engine_eval 需要文件名参数
    JSValue val = js_engine_eval(js, script, "test.js");
    (void)val;  // 此处简化

    js_engine_destroy(js);
    service_delete(svc);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_js_eval_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}