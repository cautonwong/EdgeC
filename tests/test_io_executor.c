#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "io_executor.h"

// Context for the test
typedef struct {
    uv_loop_t *loop;
    int completed_count;
    char rx_data[128];
} test_io_ctx_t;

static void on_job_complete(io_job_t *job, void *user_data) {
    test_io_ctx_t *ctx = (test_io_ctx_t *)user_data;
    ctx->completed_count++;
    
    if (job->rx_len > 0) {
        memcpy(ctx->rx_data, job->rx_buf, job->rx_len);
        ctx->rx_data[job->rx_len] = '\0';
    }
    
    io_job_free(job);
    
    // Stop loop after getting result
    if (ctx->completed_count >= 1) {
        uv_stop(ctx->loop);
    }
}

static void test_io_thread_ping_pong(void **state) {
    (void)state;
    
    uv_loop_t loop;
    uv_loop_init(&loop);
    
    test_io_ctx_t ctx = { .loop = &loop, .completed_count = 0 };
    
    // 1. Init Executor
    assert_int_equal(io_executor_init(&loop), 0);
    
    // 2. Create Job
    io_job_t *job = io_job_create(IO_BUS_RS485_1, 10, 10);
    assert_non_null(job);
    
    const char *msg = "PING";
    memcpy(job->tx_buf, msg, 4);
    job->tx_len = 4;
    
    job->cb = on_job_complete;
    job->user_data = &ctx;
    
    // 3. Submit
    assert_int_equal(io_executor_submit(job), 0);
    
    // 4. Run Loop
    // The executor thread will pick up the job, simulate IO (copy TX to RX),
    // and notify main loop via async handle.
    // uv_run will block until async event comes.
    uv_run(&loop, UV_RUN_DEFAULT);
    
    // 5. Verify
    assert_int_equal(ctx.completed_count, 1);
    assert_string_equal(ctx.rx_data, "PING"); // Loopback simulation
    
    // 6. Cleanup
    io_executor_shutdown();
    
    // Run loop once more to process close handles
    uv_run(&loop, UV_RUN_NOWAIT);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_thread_ping_pong),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}