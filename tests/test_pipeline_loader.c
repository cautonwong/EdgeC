#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "stream_engine.h"

static void test_loader_dynamic_build(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    // 适配新签名
    datahub_init(&hub, loop, "loader_bus_v4", 64, 1024, NULL, 0, NULL, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    assert_non_null(engine);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_loader_dynamic_build),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}