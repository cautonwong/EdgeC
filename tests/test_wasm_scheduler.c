#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "wasm_app_scheduler.h"
#include "wasm_mgr.h"

static void test_wasm_scheduler_lifecycle(void** state) {
    (void)state;

    // 1. Initialize DataHub
    datahub_t hub;
    uv_loop_t* loop = uv_default_loop();
    int rc = datahub_init(&hub, loop, "wasm_sched_hub", 128, 10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    // 2. Write initial value to Tag 1001 (e.g. 150.0)
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 150.0, uv_hrtime());
    rc = datahub_write_shadow(&hub, &msg);
    assert_int_equal(rc, 0);

    // 3. Initialize WASM Manager with DataHub context
    wasm_mgr_init(&hub);

    // 4. Create WASM Scheduler with 10ms period
    wasm_scheduler_t* sched = wasm_scheduler_create(&hub, loop, 10);
    assert_non_null(sched);

    // 5. Load and schedule demo.wasm
    const char* wasm_path = "../../algo/wasm/demo/demo.wasm";
    rc = wasm_scheduler_load_app(sched, "demo_app", wasm_path, 16384, 16384, 100);
    assert_int_equal(rc, 0);

    // 6. Start the scheduler
    rc = wasm_scheduler_start(sched);
    assert_int_equal(rc, 0);

    // 7. Run the event loop for 50ms to allow scheduler timer to fire
    uint64_t start_time = uv_hrtime() / 1000000;
    while (true) {
        uv_run(loop, UV_RUN_NOWAIT);
        uint64_t now = uv_hrtime() / 1000000;
        if (now - start_time >= 50) {
            break;
        }
        usleep(1000);
    }

    // 8. Stop the scheduler
    rc = wasm_scheduler_stop(sched);
    assert_int_equal(rc, 0);

    // 9. Verify that WASM scheduler periodically triggered the app, reading 1001 and writing 1002
    pipe_msg_t result;
    rc = datahub_read_shadow(&hub, 1002, &result);
    assert_int_equal(rc, 0);
    assert_float_equal(result.payload.val.f, 300.0, 0.001);  // 150.0 * 2.0 = 300.0

    printf("🔥 [SUCCESS] WASM Scheduler successfully triggered demo_app! Tag 1002 = %.2f\n",
           result.payload.val.f);

    // 10. Unload app
    rc = wasm_scheduler_unload_app(sched, "demo_app");
    assert_int_equal(rc, 0);

    // 11. Cleanup
    wasm_scheduler_destroy(sched);
    wasm_mgr_cleanup();
    datahub_destroy(&hub);

    // Drain event loop
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

static void test_wasm_scheduler_reload(void** state) {
    (void)state;

    // 1. Initialize DataHub
    datahub_t hub;
    uv_loop_t* loop = uv_default_loop();
    int rc = datahub_init(&hub, loop, "wasm_reload_hub", 128, 10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    // 2. Initialize WASM Manager
    wasm_mgr_init(&hub);

    // 3. Create WASM Scheduler
    wasm_scheduler_t* sched = wasm_scheduler_create(&hub, loop, 10);
    assert_non_null(sched);

    // 4. Load demo.wasm
    const char* wasm_path = "../../algo/wasm/demo/demo.wasm";
    rc = wasm_scheduler_load_app(sched, "reload_app", wasm_path, 16384, 16384, 100);
    assert_int_equal(rc, 0);

    // 5. Try to reload with same file
    rc = wasm_scheduler_reload_app(sched, "reload_app", wasm_path, 16384, 16384, 100);
    assert_int_equal(rc, 0);

    // 6. Try to reload non-existent app, should fail
    rc = wasm_scheduler_reload_app(sched, "non_existent_app", wasm_path, 16384, 16384, 100);
    assert_int_equal(rc, -1);

    // 7. Cleanup
    wasm_scheduler_unload_app(sched, "reload_app");
    wasm_scheduler_destroy(sched);
    wasm_mgr_cleanup();
    datahub_destroy(&hub);
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

static void test_wasm_scheduler_per_app_periods_and_budgets(void** state) {
    (void)state;

    datahub_t hub;
    uv_loop_t* loop = uv_default_loop();
    int rc = datahub_init(&hub, loop, "wasm_per_app_hub", 128, 10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    wasm_mgr_init(&hub);

    // 基准 tick 为 5ms
    wasm_scheduler_t* sched = wasm_scheduler_create(&hub, loop, 5);
    assert_non_null(sched);

    const char* wasm_path = "../../algo/wasm/demo/demo.wasm";

    // 1. 加载快周期 App (fast_app): period=10ms, budget=5ms, deadline=10ms
    rc = wasm_scheduler_load_app_ex(sched, "fast_app", wasm_path, 16384, 16384, 50, 10, 5, 10);
    assert_int_equal(rc, 0);

    // 2. 加载慢周期 App (slow_app): period=50ms, budget=30ms, deadline=50ms
    rc = wasm_scheduler_load_app_ex(sched, "slow_app", wasm_path, 16384, 16384, 100, 50, 30, 50);
    assert_int_equal(rc, 0);

    rc = wasm_scheduler_start(sched);
    assert_int_equal(rc, 0);

    // 运行 110ms
    for (int i = 0; i < 22; i++) {
        usleep(5000);
        uv_run(loop, UV_RUN_NOWAIT);
    }

    wasm_scheduler_stop(sched);

    uint64_t fast_ticks = 0, fast_overruns = 0;
    uint64_t slow_ticks = 0, slow_overruns = 0;

    assert_int_equal(wasm_scheduler_get_app_stats(sched, "fast_app", &fast_ticks, &fast_overruns),
                     0);
    assert_int_equal(wasm_scheduler_get_app_stats(sched, "slow_app", &slow_ticks, &slow_overruns),
                     0);

    // 验证：慢 App 不推迟快 App！快 App 运行频次显著高于慢 App (约 5 倍关系)
    assert_true(fast_ticks >= 5);
    assert_true(slow_ticks >= 1 && slow_ticks <= 4);
    assert_true(fast_ticks > slow_ticks);

    wasm_scheduler_unload_app(sched, "fast_app");
    wasm_scheduler_unload_app(sched, "slow_app");
    wasm_scheduler_destroy(sched);
    wasm_mgr_cleanup();
    datahub_destroy(&hub);
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wasm_scheduler_lifecycle),
        cmocka_unit_test(test_wasm_scheduler_reload),
        cmocka_unit_test(test_wasm_scheduler_per_app_periods_and_budgets),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
