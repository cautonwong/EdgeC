#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include "io_executor.h"
#include <string.h>
#include <stdlib.h>

typedef struct {
    int done;
    uv_timer_t *timeout_handle;
} io_test_ctx_t;

static void on_test_timeout(uv_timer_t *handle) {
    (void)handle;
    fail_msg("Test timed out");
}

static void io_test_on_done(io_job_t *job, void *user) {
    io_test_ctx_t *ctx = (io_test_ctx_t *)user;
    ctx->done = 1;
    // Success! Stop the timeout timer.
    uv_timer_stop(ctx->timeout_handle);
    io_job_free(job);
}

static void test_io_executor_basic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    
    assert_int_equal(io_executor_init(loop), 0);

    // Setup timeout timer
    uv_timer_t timeout_handle;
    uv_timer_init(loop, &timeout_handle);
    
    io_test_ctx_t ctx = { .done = 0, .timeout_handle = &timeout_handle };
    
    // Create a job for RS485_1
    io_job_t *job = io_job_create(IO_BUS_RS485_1, 8, 128);
    assert_non_null(job);
    
    job->cb = io_test_on_done;
    job->user_data = &ctx;
    
    assert_int_equal(io_executor_submit(job), 0);

    // Start the timeout timer, if it fires, the test fails
    uv_timer_start(&timeout_handle, on_test_timeout, 2000, 0); // 2 second timeout

    // Run the loop until the job is done or it times out
    uv_run(loop, UV_RUN_DEFAULT);

    assert_int_equal(ctx.done, 1);
    
    io_executor_shutdown();

    // The loop is already stopped, but run it once more to process close callbacks
    uv_run(loop, UV_RUN_ONCE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_executor_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}