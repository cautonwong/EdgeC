#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "io_executor.h"

/* ---------------- test context ---------------- */

typedef struct {
    uv_loop_t loop;
    int work_count;
    int done_count;
} test_ctx_t;

/* ---------------- worker ---------------- */

static void on_io_complete(io_job_t *job, void *user_data) {
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    ctx->done_count++;

    // In real app, you'd check job->result
    io_job_free(job);
}

/* ---------------- test ---------------- */

static void test_io_executor_parallel(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;

    const int N = 10;
    for (int i = 0; i < N; i++) {
        // Create a mock job (bus 0, tx 8, rx 8)
        io_job_t *job = io_job_create(IO_BUS_RS485_1, 8, 8);
        assert_non_null(job);
        job->cb = on_io_complete;
        job->user_data = ctx;

        if (io_executor_submit(job) == 0) {
            ctx->work_count++;
        }
    }

    assert_int_equal(ctx->work_count, N);

    /* Run the loop until all jobs are done */
    while (ctx->done_count < N) {
        // uv_run will return 0 if there are no more active handles.
        // If we are still in the while loop, it means we are stuck.
        if (uv_run(&ctx->loop, UV_RUN_ONCE) == 0 && ctx->done_count < N) {
            break;
        }
    }

    assert_int_equal(ctx->done_count, N);
}

/* ---------------- setup / teardown ---------------- */

static int setup(void **state) {
    test_ctx_t *ctx = calloc(1, sizeof(*ctx));
    assert_non_null(ctx);

    ctx->work_count = 0;
    ctx->done_count = 0;

    uv_loop_init(&ctx->loop);
    io_executor_init(&ctx->loop);

    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;

    io_executor_shutdown();
    // Run the loop to ensure all close callbacks are processed
    while (uv_loop_close(&ctx->loop) == UV_EBUSY) {
        uv_run(&ctx->loop, UV_RUN_ONCE);
    }
    free(ctx);

    return 0;
}

/* ---------------- main ---------------- */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_io_executor_parallel, setup, teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}