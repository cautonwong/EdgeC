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
} io_test_ctx_t;

static void io_test_on_done(io_job_t *job, void *user) {
    io_test_ctx_t *ctx = (io_test_ctx_t *)user;
    ctx->done = 1;
    io_job_free(job);
}

static void test_io_executor_basic(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    
    assert_int_equal(io_executor_init(loop), 0);

    io_test_ctx_t ctx = { .done = 0 };
    
    // Create a job for RS485_1
    io_job_t *job = io_job_create(IO_BUS_RS485_1, 8, 128);
    assert_non_null(job);
    
    job->cb = io_test_on_done;
    job->user_data = &ctx;
    
    assert_int_equal(io_executor_submit(job), 0);

    // Run loop to process completion
    for (int i = 0; i < 100; i++) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (ctx.done) break;
        uv_sleep(5);
    }

    assert_int_equal(ctx.done, 1);
    
    io_executor_shutdown();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_executor_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}