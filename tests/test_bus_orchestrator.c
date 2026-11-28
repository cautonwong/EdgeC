#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "bus_datahub_adapter.h"
#include "datahub.h"
#include "driver_manager.h"
#include "datahub_priv.h"

extern const edge_driver_t modbus_rtu_driver;

static void test_bus_orchestrator_dynamic_create(void** state) {
    (void)state;
    fprintf(stderr, "[DEBUG] Start test_bus_orchestrator_dynamic_create\n");

    uv_loop_t loop;
    uv_loop_init(&loop);

    datahub_t hub;
    datahub_init(&hub, &loop, "test_bus", 64, 1024, NULL, 0, NULL, NULL, NULL);

    arbiter_t* arb = NULL;
    arbiter_init(&arb, &loop);
    driver_manager_storage_t mgr_storage = {0};
    driver_manager_t* mgr = driver_manager_create_in(&mgr_storage);

    bus_orchestrator_t* ctx = bus_orchestrator_create_hub_default(&hub, arb, driver_manager_session_port_ptr(mgr), 8, 32, 2048);
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

static void test_bus_orchestrator_destroy_releases_engines_and_pools(void** state) {
    (void)state;
    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    datahub_t hub;
    assert_int_equal(datahub_init(&hub, &loop, "test_bus_teardown", 64, 1024, NULL, 0, NULL, NULL, NULL), 0);

    arbiter_t* arb = NULL;
    assert_int_equal(arbiter_init(&arb, &loop), 0);

    driver_manager_storage_t mgr_storage = {0};
    driver_manager_t* mgr = driver_manager_create_in(&mgr_storage);
    assert_non_null(mgr);
    assert_int_equal(driver_manager_register(mgr, &modbus_rtu_driver), 0);

    bus_orchestrator_storage_t orch_storage = {0};
    bus_orchestrator_t* ctx = bus_orchestrator_create_hub(
        &orch_storage, &hub, arb, driver_manager_session_port_ptr(mgr), 8, 32, 2048);
    assert_non_null(ctx);

    assert_int_equal(bus_orchestrator_setup_master(ctx, 0, "ModbusRTU", 1), 0);
    assert_int_equal(bus_orchestrator_setup_slave(ctx, 1, "ModbusRTU", 1), 0);
    assert_int_equal(bus_orchestrator_run(ctx), 0);

    bus_orchestrator_destroy(NULL);
    bus_orchestrator_destroy(ctx);
    bus_orchestrator_destroy(ctx);

    driver_manager_destroy(mgr);
    arbiter_destroy(arb);
    datahub_destroy(&hub);

    uv_run(&loop, UV_RUN_DEFAULT);
    assert_int_equal(uv_loop_close(&loop), 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bus_orchestrator_dynamic_create),
        cmocka_unit_test(test_bus_orchestrator_destroy_releases_engines_and_pools),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}