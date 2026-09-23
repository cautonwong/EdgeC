/**
 * @file test_examples_integration.c
 * @brief AEP-403: Integration Test for Secondary Developer Custom Driver and Algorithm Examples
 */

#include <cmocka.h>
#include <edgeos/edgeos.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../examples/custom_algo/include/custom_soc_filter.h"
#include "../examples/custom_driver/include/custom_inverter_driver.h"

/* -------------------------------------------------------------------------- */
/* Helper Functions                                                           */
/* -------------------------------------------------------------------------- */

static int example_test_map_fn(uint32_t tag_id) {
    return (int)(tag_id % 512);
}

/* -------------------------------------------------------------------------- */
/* Test Cases                                                                 */
/* -------------------------------------------------------------------------- */

static void test_custom_inverter_driver_lifecycle(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);

    // 1. Initialize DataHub
    edgeos_datahub_t hub;
    int rc = datahub_init(&hub, &loop, "test_drv_hub", 512, 512, NULL, 0, NULL, example_test_map_fn,
                          NULL);
    assert_int_equal(rc, 0);

    // 2. Driver Context Allocation & Init
    edgeos_driver_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.driver = (const edge_driver_t *)&custom_inverter_driver;

    rc = custom_inverter_driver.on_init((edge_driver_ctx_t *)&ctx);
    assert_int_equal(rc, 0);
    assert_non_null(ctx.priv);
    assert_true(custom_inverter_driver.is_ready((edge_driver_ctx_t *)&ctx));

    // 3. Driver Poll & DataHub Shadow Writeback
    rc = custom_inverter_simulate_poll(&ctx, &hub);
    assert_int_equal(rc, 0);

    // 4. Read back from DataHub
    edgeos_pipe_msg_t msg_u;
    rc = datahub_read_shadow(&hub, 1001, &msg_u);
    assert_int_equal(rc, 0);
    assert_float_equal(msg_u.payload.val.f, 220.0f, 0.001f);

    edgeos_pipe_msg_t msg_i;
    rc = datahub_read_shadow(&hub, 1002, &msg_i);
    assert_int_equal(rc, 0);
    assert_float_equal(msg_i.payload.val.f, 10.0f, 0.001f);

    edgeos_pipe_msg_t msg_p;
    rc = datahub_read_shadow(&hub, 1003, &msg_p);
    assert_int_equal(rc, 0);
    assert_float_equal(msg_p.payload.val.f, 2200.0f, 0.001f);

    edgeos_pipe_msg_t msg_f;
    rc = datahub_read_shadow(&hub, 1004, &msg_f);
    assert_int_equal(rc, 0);
    assert_float_equal(msg_f.payload.val.f, 50.0f, 0.001f);

    // 5. Cleanup
    custom_inverter_driver.on_cleanup((edge_driver_ctx_t *)&ctx);
    datahub_destroy(&hub);
    uv_loop_close(&loop);
}

static void test_custom_soc_filter_math(void **state) {
    (void)state;

    custom_soc_filter_t filter;
    int rc = custom_soc_filter_init(&filter, 0.5f, 50.0f);
    assert_int_equal(rc, 0);

    // Step 1: Input 80.0% -> 0.5*80 + 0.5*50 = 65.0%
    float val1 = custom_soc_filter_step(&filter, 80.0f);
    assert_float_equal(val1, 65.0f, 0.001f);

    // Step 2: Input 80.0% -> 0.5*80 + 0.5*65 = 72.5%
    float val2 = custom_soc_filter_step(&filter, 80.0f);
    assert_float_equal(val2, 72.5f, 0.001f);

    // Step 3: Input 80.0% -> 0.5*80 + 0.5*72.5 = 76.25%
    float val3 = custom_soc_filter_step(&filter, 80.0f);
    assert_float_equal(val3, 76.25f, 0.001f);

    // Clamping checks:
    float clamp_high = custom_soc_filter_step(&filter, 200.0f);
    assert_true(clamp_high <= 100.0f);

    custom_soc_filter_init(&filter, 1.0f, 50.0f);
    float clamp_low = custom_soc_filter_step(&filter, -50.0f);
    assert_float_equal(clamp_low, 0.0f, 0.001f);
}

static void test_driver_and_algo_pipeline(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);

    // 1. Setup DataHub
    edgeos_datahub_t hub;
    int rc = datahub_init(&hub, &loop, "test_pipe_hub", 512, 512, NULL, 0, NULL,
                          example_test_map_fn, NULL);
    assert_int_equal(rc, 0);

    // 2. Setup Filter (alpha=0.5, initial=40.0%)
    custom_soc_filter_t filter;
    custom_soc_filter_init(&filter, 0.5f, 40.0f);

    // 3. Push raw SOC reading to DataHub tag 1001
    edgeos_pipe_msg_t raw_msg;
    pipe_msg_init_scalar(&raw_msg, 1001, 80.0, 1000);
    datahub_write_shadow(&hub, &raw_msg);

    // 4. Execute filter process from DataHub (tag 1001 -> tag 4001)
    rc = custom_soc_filter_process_datahub(&filter, &hub, 1001, 4001);
    assert_int_equal(rc, 0);

    // 5. Read back filtered result from tag 4001
    edgeos_pipe_msg_t out_msg;
    rc = datahub_read_shadow(&hub, 4001, &out_msg);
    assert_int_equal(rc, 0);
    assert_float_equal(out_msg.payload.val.f, 60.0f, 0.001f);  // 0.5*80 + 0.5*40 = 60.0%

    datahub_destroy(&hub);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_custom_inverter_driver_lifecycle),
        cmocka_unit_test(test_custom_soc_filter_math),
        cmocka_unit_test(test_driver_and_algo_pipeline),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
