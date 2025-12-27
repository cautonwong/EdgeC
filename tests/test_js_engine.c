#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include "services/js_engine.h"
#include "edge_service.h"

static void test_js_eval_basic(void **state) {
    (void)state;
    service_t svc = {0}; // Mock service
    js_engine_t *engine = js_engine_create(&svc, 1024 * 1024);
    assert_non_null(engine);

    // Test simple math
    assert_int_equal(js_engine_eval(engine, "1 + 1", "test.js"), 0);

    // Test Bridge API: Log
    assert_int_equal(js_engine_eval(engine, "Log.info('Hello from JS');", "test.js"), 0);

    // Test Bridge API: DataHub
    assert_int_equal(js_engine_eval(engine, "const v = DataHub.read(1001); if(v < 220) throw new Error('Bad Value');", "test.js"), 0);

    js_engine_destroy(engine);
}

static void test_js_memory_limit(void **state) {
    (void)state;
    service_t svc = {0};
    // Set a very low limit: 100KB
    js_engine_t *engine = js_engine_create(&svc, 100 * 1024);
    assert_non_null(engine);

    // Attempt to allocate a large array to trigger OOM
    const char *oom_script = "const a = []; for(let i=0; i<10000; i++) a.push(new Array(100).fill(1));";
    
    // This should fail due to memory limit
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
