#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "io_executor.h" // Missing include

// Context for the test
typedef struct {
    uv_loop_t *loop;
    io_job_t job;
    int io_completed;
} test_io_ctx_t;

static void io_test_on_done(io_job_t *job, void *user) {
    test_io_ctx_t *ctx = (test_io_ctx_t *)user;
    ctx->io_completed = 1;

    // The job's rx buffer was allocated by the test, so free it here
    if (job->rx != NULL) {
        free(job->rx);
        job->rx = NULL;
    }
    uv_stop(ctx->loop); // Stop the loop after the job is done
}

static void test_io_executor_basic(void **state) {
    (void)state; // Unused

    uv_loop_t loop;
    uv_loop_init(&loop);

    test_io_ctx_t ctx = { .loop = &loop, .io_completed = 0 };

    int fds[2]; // fds[0] for read, fds[1] for write
    assert_int_equal(pipe(fds), 0);

    // Make the read end of the pipe non-blocking if uv_read_start were used
    // But uv_queue_work directly uses read/write, which are blocking.
    // So the worker thread will block on these.

    io_job_t *job = &ctx.job;
    job->bus = IO_RS485;
    job->fd = fds[0]; // io_executor will read from this end
    job->tx = (uint8_t*)"PING\n";
    job->tx_len = strlen((char*)job->tx);
    job->rx = (uint8_t*)malloc(128);
    job->rx_cap = 128;
    job->rx_len = 0;
    job->result = -1; // Initial state

    // Write some data to the pipe for the io_executor to read
    const char *mock_response = "PONG_ACK\n";
    assert_int_equal(write(fds[1], mock_response, strlen(mock_response)), (ssize_t)strlen(mock_response));

    io_submit(&loop, job, io_test_on_done, &ctx);

    // Run the loop until uv_stop is called from the callback
    uv_run(&loop, UV_RUN_DEFAULT);

    // Assertions
    assert_int_equal(ctx.io_completed, 1);
    assert_int_equal(job->result, 0); // Expect success
    assert_int_equal(job->rx_len, strlen(mock_response));
    assert_string_equal((char*)job->rx, mock_response);

    // Cleanup
    close(fds[0]);
    close(fds[1]);
    // The job->rx buffer is freed in io_test_on_done
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_executor_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}