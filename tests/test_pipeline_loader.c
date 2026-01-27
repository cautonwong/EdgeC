#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "stream_engine.h"

static void test_loader_dynamic_build(void **state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    datahub_t hub;
    // 适配新签名
    datahub_init(&hub, &loop, "test_bus", 64, 1024, NULL, 0, NULL, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, &loop, NULL);
    stream_engine_start(engine);
    assert_non_null(engine);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    while (uv_run(&loop, UV_RUN_NOWAIT))
        ;
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_loader_dynamic_build),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}