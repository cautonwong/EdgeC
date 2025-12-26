#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "arbiter.h"
#include "io_executor.h"
#include "shmring.h"
#include "algorithm_sum.h"
#include "modbus_protocol.h"
#include <cmocka_pbc.h>

// --- Test Context and Mocking ---

typedef struct {
    uv_loop_t *loop;
    arbiter_t *arbiter;
    shm_ring_t data_hub;
    algo_sum_t algo_sum;
    uv_timer_t poll_timer;
    int io_job_submitted;
    int modbus_read_successful;
} integration_test_ctx_t;

shm_ring_t *global_ring = NULL;

// Wrapper for io_executor_submit to mock the IO operation
int __wrap_io_executor_submit(io_job_t *job) {
    function_called(); // For cmocka to track calls

    // The job's user_data is the modbus_ctx_t. Inside that, the 'user' field holds our test context.
    if (job && job->user_data) {
        modbus_ctx_t *modbus_ctx = (modbus_ctx_t *)job->user_data;
        integration_test_ctx_t *test_ctx = (integration_test_ctx_t *)modbus_ctx->user;
        if (test_ctx) {
            test_ctx->io_job_submitted = 1;
        }
    }
    
    printf("[Test Mock] __wrap_io_executor_submit called. Simulating successful Modbus response.\n");

    // Simulate a successful Modbus response: 01 03 02 00 C8 39
    uint8_t mock_rx_data[] = {0x01, 0x03, 0x02, 0x00, 0xC8, 0xB8, 0x39};
    job->rx_len = sizeof(mock_rx_data);
    memcpy(job->rx_buf, mock_rx_data, (size_t)job->rx_len);
    job->result = IO_RESULT_SUCCESS;

    // The callback needs the test context, not the modbus context.
    // The test context was passed to arbiter_request, so it's the `user` data for on_bus_granted.
    // However, the io_job doesn't have a separate field for the cb's user_data.
    // Let's retrieve it from the nested context.
    void* cb_user_data = NULL;
    if (job->user_data) {
        modbus_ctx_t *modbus_ctx = (modbus_ctx_t *)job->user_data;
        cb_user_data = modbus_ctx->user;
    }
    
    // Directly call the callback to simulate async completion
    if (job->cb) {
        job->cb(job, cb_user_data);
    }
    return 0; // Success
}

// --- Test Callbacks ---

static void integration_on_modbus_read(int status, const uint16_t *registers, uint16_t num_registers, void *user) {
    (void)num_registers;
    integration_test_ctx_t *ctx = (integration_test_ctx_t *)user;
    if (status == 0) {
        printf("[Test] Modbus read success. Register[0] = %u\n", registers[0]);
        ctx->modbus_read_successful = 1;
        PipeMsg msg = { .type = PIPE_SCALAR_F64, .payload.val_f = (double)registers[0] };
        shm_ring_write(&ctx->data_hub, &msg);
    }
}

static void integration_on_io_done(io_job_t *job, void *user_data) {
    integration_test_ctx_t *ctx = (integration_test_ctx_t *)user_data;
    modbus_protocol_handle_response(job);
    arbiter_release(ctx->arbiter, job->bus_id, ctx);
    // The job itself is freed by modbus_protocol_handle_response indirectly, let's not double free.
    // Let's trace: modbus_protocol_handle_response frees the context (job->user_data).
    // The io_job_free in the original on_io_done in modbus_protocol.c frees the job itself.
    // Our callback `integration_on_io_done` replaced that, so we are responsible for freeing the job.
    io_job_free(job);
}

static void integration_on_bus_granted(arbiter_t *arb, arbiter_bus_t bus, void *user) {
    (void)arb;
    integration_test_ctx_t *ctx = (integration_test_ctx_t *)user;
    // Create the job, passing the test context `ctx` as the final user pointer
    io_job_t *job = modbus_protocol_create_read_job(1, 100, 1, 256, integration_on_modbus_read, ctx);
    
    // Set the bus and the intermediate callback
    job->bus_id = (io_bus_id_t)bus;
    job->cb = integration_on_io_done;
    // The user_data for the callback is now correctly nested inside the modbus_ctx
    
    io_executor_submit(job);
}

static void integration_poll_task(uv_timer_t *timer) {
    integration_test_ctx_t *ctx = (integration_test_ctx_t *)timer->data;
    printf("[Test] Polling timer triggered. Requesting bus...\n");
    arbiter_request(ctx->arbiter, IO_BUS_RS485_1, ARB_PRIO_NORMAL, ARB_SHARED, 5000, integration_on_bus_granted, ctx);
    uv_timer_stop(timer);
}

// --- Test Case ---

static void test_end_to_end_flow(void **state) {
    (void)state;

    integration_test_ctx_t ctx = {0};
    uv_loop_t loop;
    uv_loop_init(&loop);
    ctx.loop = &loop;

    char ring_name[] = "/test_ring_integration";
    shm_unlink(ring_name);
    assert_int_equal(shmring_create(&ctx.data_hub, ring_name, 8), 0);
    global_ring = &ctx.data_hub;

    ctx.arbiter = arbiter_create(ctx.loop);
    assert_non_null(ctx.arbiter);
    assert_int_equal(algo_sum_init(&ctx.algo_sum, ctx.loop, &ctx.data_hub, 10), 0);

    uv_timer_init(ctx.loop, &ctx.poll_timer);
    ctx.poll_timer.data = &ctx;
    uv_timer_start(&ctx.poll_timer, integration_poll_task, 0, 0);

    expect_function_call(__wrap_io_executor_submit);

    printf("--- Running loop for integration test ---\n");
    for(int i = 0; i < 50; i++) {
        uv_run(ctx.loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    printf("--- Loop finished ---\n");

    assert_true(ctx.io_job_submitted);
    assert_true(ctx.modbus_read_successful);
    assert_float_equal(ctx.algo_sum.sum, 200.0, 0.01);

    // Cleanup
    arbiter_destroy(ctx.arbiter);
    algo_sum_close(&ctx.algo_sum);
    shmring_close(&ctx.data_hub);
    shmring_destroy(&ctx.data_hub);
    uv_close((uv_handle_t*)&ctx.poll_timer, NULL);
    
    uv_run(ctx.loop, UV_RUN_DEFAULT);
    uv_loop_close(ctx.loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_end_to_end_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
