#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "stream_engine.h"

static void test_engine_basic_flow(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    // 适配新签名
    datahub_init(&hub, loop, "test_hub_v1", 10, 1024, NULL, 0, NULL, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    assert_non_null(engine);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_engine_basic_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
