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

static void test_wasm_host_api(void** state) {
    (void)state;

    // 1. Initialize DataHub
    datahub_t hub;
    uv_loop_t* loop = uv_default_loop();
    int rc = datahub_init(&hub, loop, "wasm_demo_hub", 128, 10000, NULL, 0, NULL, NULL, NULL);
    assert_int_equal(rc, 0);

    // 2. Write initial value to Tag 1001 (e.g. 240.5)
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 240.5, uv_hrtime());
    rc = datahub_write_shadow_idx(&hub, 1001, &msg);
    assert_int_equal(rc, 0);

    // 3. Initialize WASM Manager with DataHub context
    wasm_mgr_init(&hub);

    // 4. Load demo.wasm (Make sure path is relative to test executable)
    const char* wasm_path = "../../algo/wasm/demo/demo.wasm";
    wasm_app_t* app = wasm_app_create(wasm_path, 16384, 16384, 100);
    assert_non_null(app);

    // 5. Execute the "on_tick" exported function
    rc = wasm_app_exec(app, "on_tick", NULL);
    assert_int_equal(rc, 0);

    // 6. Verify that WASM properly read Tag 1001 and wrote Tag 1002
    pipe_msg_t result;
    rc = datahub_read_shadow(&hub, 1002, &result);
    assert_int_equal(rc, 0);
    assert_float_equal(result.payload.val.f, 481.0, 0.001);  // 240.5 * 2.0 = 481.0

    printf("🔥 [SUCCESS] WASM correctly interacted with physical DataHub! Tag 1002 = %.2f\n",
           result.payload.val.f);

    // 7. Cleanup
    wasm_app_destroy(app);
    wasm_mgr_cleanup();
    datahub_destroy(&hub);
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_wasm_host_api),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
