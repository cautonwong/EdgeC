#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <stdio.h>
#include <sys/socket.h> 
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "io_executor.h"
#include "shmring.h"

shm_ring_t *global_ring = NULL;

typedef struct {
    uv_loop_t *loop;
    io_job_t job;
    int io_completed;
} test_io_ctx_t;

static void io_test_on_done(io_job_t *job, void *user) {
    test_io_ctx_t *ctx = (test_io_ctx_t *)user;
    ctx->io_completed = 1;
    // Do NOT free job->rx here, checking it in test function
}

static void test_io_executor_basic(void **state) {
    (void)state;

    uv_loop_t loop;
    uv_loop_init(&loop);

    test_io_ctx_t ctx = { .loop = &loop, .io_completed = 0 };

    int fds[2]; 
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

    io_job_t *job = &ctx.job;
    job->bus = IO_RS485;
    job->fd = fds[0]; 
    job->tx = (uint8_t*)"PING\n";
    job->tx_len = strlen((char*)job->tx);
    job->rx = (uint8_t*)malloc(128);
    job->rx_cap = 128;
    job->rx_len = 0;
    job->result = -1; 

    io_submit(&loop, job, io_test_on_done, &ctx);

    char read_buf[128] = {0};
    ssize_t n = read(fds[1], read_buf, sizeof(read_buf));
    if (n > 0) {
        assert_memory_equal(read_buf, "PING\n", 5);
        const char *mock_response = "PONG_ACK\n";
        write(fds[1], mock_response, strlen(mock_response));
    }

    for (int i=0; i<100; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        if (ctx.io_completed) break;
        usleep(1000);
    }

    assert_int_equal(ctx.io_completed, 1);
    assert_int_equal(job->result, 0); 
    
    char rx_buf[128] = {0};
    memcpy(rx_buf, job->rx, (size_t)job->rx_len);
    assert_string_equal(rx_buf, "PONG_ACK\n");

    // Cleanup
    close(fds[0]);
    close(fds[1]);
    free(job->rx); // Free here now
    
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_executor_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
