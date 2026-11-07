#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "datahub.h"
#include "js_engine.h"
#include "datahub_priv.h"

static void test_js_eval_basic(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();
    datahub_t hub;
    // 适配新签名
    datahub_init(&hub, loop, "test_js", 64, 1024, NULL, 0, NULL, NULL, NULL);

    size_t engine_size = js_engine_storage_size();
    void *engine_mem = malloc(engine_size);
    js_engine_t* engine = js_engine_create_in(engine_mem, engine_size, NULL, NULL, NULL, 1024 * 1024);
    assert_non_null(engine);

    js_engine_destroy(engine);
    free(engine_mem);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_js_eval_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
