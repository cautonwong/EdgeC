#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_manager.h"

static void on_walk_print(uv_handle_t* handle, void* arg) {
    (void)arg;
    fprintf(stderr, "[DEBUG] Pending handle: %p, type: %d, closing: %d\n", (void*)handle,
            handle->type, uv_is_closing(handle));
}

static void test_bus_orchestrator_dynamic_create(void** state) {
    (void)state;
    fprintf(stderr, "[DEBUG] Start test_bus_orchestrator_dynamic_create\n");

    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_t hub;
    datahub_init(&hub, &loop, "test_bus", 64, 1024, NULL, 0, NULL, NULL, NULL);

    arbiter_t* arb = NULL;
    arbiter_init(&arb, &loop);
    driver_manager_t* mgr = driver_manager_create();

    bus_orchestrator_t* ctx = bus_orchestrator_create(&hub, arb, mgr, 8, 32, 2048);
    assert_non_null(ctx);

    fprintf(stderr, "[DEBUG] Created Orchestrator, now destroying...\n");
    bus_orchestrator_destroy(ctx);
    driver_manager_destroy(mgr);
    arbiter_destroy(arb);

    fprintf(stderr, "[DEBUG] Entering cleanup loop...\n");

    // 先销毁 DataHub，释放它的异步句柄

    datahub_destroy(&hub);

    // 使用 DEFAULT 模式跑，直到没有任何活跃句柄

    // Libuv 保证在没有活跃 handle 时 uv_run 会返回 0

    uv_run(&loop, UV_RUN_DEFAULT);

    fprintf(stderr, "[DEBUG] Cleanup loop done. Remaining alive: %d\n", uv_loop_alive(&loop));

    uv_loop_close(&loop);

    fprintf(stderr, "[DEBUG] Test finished successfully\n");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bus_orchestrator_dynamic_create),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}