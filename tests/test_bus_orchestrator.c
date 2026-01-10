#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_manager.h"

static void test_bus_orchestrator_dynamic_create(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_bus", 64, 1024, NULL, 0, NULL, NULL);

    arbiter_t *arb = NULL;
    arbiter_init(&arb, loop);

    driver_manager_t *mgr = driver_manager_create();

    // 修正：注入驱动管理器
    bus_orchestrator_t *ctx = bus_orchestrator_create(&hub, arb, mgr, 8, 16, 2048);
    assert_non_null(ctx);

    bus_orchestrator_destroy(ctx);
    driver_manager_destroy(mgr);
    arbiter_destroy(arb);

    // 关键：驱动 Loop 执行 destroy 中的 uv_close 回调
    for (int i = 0; i < 10; i++) uv_run(loop, UV_RUN_NOWAIT);

    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bus_orchestrator_dynamic_create),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
