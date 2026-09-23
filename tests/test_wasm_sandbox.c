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
#include "wasm_mgr.h"

static void test_wasm_sandbox_divide_by_zero(void** state) {
    (void)state;

    // 1. Initialize DataHub & WASM Manager
    datahub_t hub;
    uv_loop_t* loop = uv_default_loop();
    int rc = datahub_init(&hub, loop, "wasm_sandbox_hub1", 128, 10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);
    wasm_mgr_init(&hub);

    // 2. Load the division-by-zero app
    const char* wasm_path = "../../tests/virus_divzero.wasm";
    wasm_app_t* app = wasm_app_create(wasm_path, 8192, 8192, 100);
    assert_non_null(app);

    // 3. Execute the function and verify it returns -1 (trap caught) without crashing the host
    rc = wasm_app_exec(app, "on_tick", NULL);
    assert_int_equal(rc, -1);

    printf("🔥 [SUCCESS] WASM Sandbox caught divide-by-zero trap safely!\n");

    // 4. Cleanup
    wasm_app_destroy(app);
    wasm_mgr_cleanup();
    datahub_destroy(&hub);
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

static void test_wasm_sandbox_infinite_loop_timeout(void** state) {
    (void)state;

    // 1. Initialize DataHub & WASM Manager
    datahub_t hub;
    uv_loop_t* loop = uv_default_loop();
    int rc = datahub_init(&hub, loop, "wasm_sandbox_hub2", 128, 10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);
    wasm_mgr_init(&hub);

    // 2. Load the infinite-loop app with 50ms timeout
    const char* wasm_path = "../../tests/virus_loop.wasm";
    wasm_app_t* app = wasm_app_create(wasm_path, 8192, 8192, 50);
    assert_non_null(app);

    // 3. Measure execution duration
    uint64_t start = uv_hrtime() / 1000000;
    rc = wasm_app_exec(app, "on_tick", NULL);
    uint64_t end = uv_hrtime() / 1000000;
    uint64_t duration = end - start;

    // 4. Verify it was interrupted and returned -1
    assert_int_equal(rc, -1);
    // Timeout is 50ms, so duration should be at least 50ms but less than 150ms
    assert_true(duration >= 50);
    assert_true(duration < 250);

    printf("🔥 [SUCCESS] WASM Sandbox terminated infinite loop watchdog in %lu ms!\n", duration);

    // 5. Cleanup
    wasm_app_destroy(app);
    wasm_mgr_cleanup();
    datahub_destroy(&hub);
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wasm_sandbox_divide_by_zero),
        cmocka_unit_test(test_wasm_sandbox_infinite_loop_timeout),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
