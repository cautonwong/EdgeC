#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "datahub.h"

static int setup(void **state) {
    (void)state;
    return 0;
}

static int teardown(void **state) {
    (void)state;
    return 0;
}

static void test_bus_orchestrator_dynamic_create(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_bus", 1024);

    arbiter_t *arb = NULL;
    arbiter_init(&arb, loop);

    bus_orchestrator_t *ctx = bus_orchestrator_create(&hub, arb, 8, 16, 2048);
    assert_non_null(ctx);

    // [修正] 增加销毁逻辑，释放内存
    bus_orchestrator_destroy(ctx);
    arbiter_destroy(arb);
    datahub_destroy(&hub);
}
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_bus_orchestrator_dynamic_create, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}