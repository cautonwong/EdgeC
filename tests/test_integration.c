#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "io_executor.h"
#include "shmring.h"
#include "algorithm_sum.h"
#include "modbus_protocol.h"
#include <cmocka_pbc.h> // For mock_function_call, etc.

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

// Define global_ring for modbus protocol
shm_ring_t *global_ring = NULL;

// The wrapped function for io_submit
void __wrap_io_submit(uv_loop_t *loop, io_job_t *job, io_done_cb cb, void *user);

typedef struct {
    uv_idle_t idler;
    io_job_t *job;
    io_done_cb cb;
    void *user;
} mock_io_ctx_t;

// This callback will run on the next loop iteration, simulating async completion
static void run_io_done_cb(uv_idle_t* handle) {
    mock_io_ctx_t *mock_ctx = (mock_io_ctx_t *)handle->data;
    if (mock_ctx->cb) {
        mock_ctx->cb(mock_ctx->job, mock_ctx->user);
    }
    uv_close((uv_handle_t*)handle, free); // Clean up the idle handle and its context
}

// Wrapper for io_submit to mock the IO operation
void __wrap_io_submit(uv_loop_t *loop, io_job_t *job, io_done_cb cb, void *user) {
    function_called(); // For cmocka to track calls

    integration_test_ctx_t *test_ctx = (integration_test_ctx_t *)user;
    test_ctx->io_job_submitted = 1;
    
    printf("[Test Mock] __wrap_io_submit called. Simulating successful Modbus response.\n");

    // Simulate a successful Modbus response: 01 03 02 00 C8 39
    // Slave ID (1) + FC (3) + Byte Count (2) + Data (0x00C8 = 200) + CRC (B839)
    uint8_t mock_rx_data[] = {0x01, 0x03, 0x02, 0x00, 0xC8, 0xB8, 0x39};
    job->rx_len = sizeof(mock_rx_data);
    memcpy(job->rx, mock_rx_data, (size_t)job->rx_len);
    job->result = 0; // Success

    // To simulate async behavior, queue the callback to run on the next loop iteration
    mock_io_ctx_t *mock_ctx = malloc(sizeof(mock_io_ctx_t));
    mock_ctx->job = job;
    mock_ctx->cb = cb;
    mock_ctx->user = user;
    mock_ctx->idler.data = mock_ctx;

    uv_idle_init(loop, &mock_ctx->idler);
    uv_idle_start(&mock_ctx->idler, run_io_done_cb);
}

// --- Test Callbacks ---

static void integration_on_modbus_read(int status, const uint16_t *registers, uint16_t num_registers, void *user) {
    (void)num_registers;
    integration_test_ctx_t *ctx = (integration_test_ctx_t *)user;
    if (status == 0) {
        printf("[Test] Modbus read success. Register[0] = %u\n", registers[0]);
        ctx->modbus_read_successful = 1;
        // Publish to DataHub
        PipeMsg msg = { .type = PIPE_SCALAR_F64, .payload.val_f = (double)registers[0] };
        shm_ring_write(&ctx->data_hub, &msg);
    }
}

static void integration_on_io_done(io_job_t *job, void *user) {
    integration_test_ctx_t *ctx = (integration_test_ctx_t *)user;
    modbus_protocol_handle_response(job);
    arbiter_release(ctx->arbiter, job->bus, ctx);
    if (job) { free(job->tx); free(job->rx); free(job); }
}

static void integration_on_bus_granted(arbiter_t *arb, arbiter_bus_t bus, void *user) {
    (void)arb;
    integration_test_ctx_t *ctx = (integration_test_ctx_t *)user;
    io_job_t *job = modbus_protocol_create_read_job(1, 100, 1, 256, integration_on_modbus_read, ctx);
    job->bus = (io_bus_t)bus; // Explicit cast
    job->fd = -1; // Not used by mock
    __wrap_io_submit(ctx->loop, job, integration_on_io_done, ctx);
}

static void integration_poll_task(uv_timer_t *timer) {
    integration_test_ctx_t *ctx = (integration_test_ctx_t *)timer->data;
    printf("[Test] Polling timer triggered. Requesting bus...\n");
    arbiter_request(ctx->arbiter, BUS_RS485_1, ARB_PRIO_NORMAL, ARB_SHARED, 5000, integration_on_bus_granted, ctx);
    uv_timer_stop(timer); // Run only once for the test
}

// --- Test Case ---

static void test_end_to_end_flow(void **state) {
    (void)state;

    integration_test_ctx_t ctx = {0};
    uv_loop_t loop;
    uv_loop_init(&loop);
    ctx.loop = &loop;

    // Init components
    char ring_name[] = "/test_ring_integration";
    shm_unlink(ring_name);
    assert_int_equal(shmring_create(&ctx.data_hub, ring_name, 8), 0);
    global_ring = &ctx.data_hub;

    ctx.arbiter = arbiter_create(ctx.loop);
    assert_non_null(ctx.arbiter);
    assert_int_equal(algo_sum_init(&ctx.algo_sum, ctx.loop, &ctx.data_hub, 10), 0);

    // Setup the polling timer to fire immediately
    uv_timer_init(ctx.loop, &ctx.poll_timer);
    ctx.poll_timer.data = &ctx;
    uv_timer_start(&ctx.poll_timer, integration_poll_task, 0, 0); // Fire once, immediately

    // Expect io_submit to be called once by our flow
    expect_function_call(__wrap_io_submit);

    // Run the loop. It should process: poll timer -> arbiter -> io (mock) -> datahub -> algo timer
    printf("--- Running loop for integration test ---\
");
    for(int i = 0; i < 50; i++) { // Run a few times to ensure all callbacks are processed
        uv_run(ctx.loop, UV_RUN_NOWAIT);
        usleep(1000);
    }
    printf("--- Loop finished ---\
");

    // Assertions
    assert_true(ctx.io_job_submitted);
    assert_true(ctx.modbus_read_successful);
    // The mock response value was 200 (0x00C8)
    assert_float_equal(ctx.algo_sum.sum, 200.0, 0.01);

    // Cleanup
    arbiter_destroy(ctx.arbiter);
    algo_sum_close(&ctx.algo_sum);
    shmring_close(&ctx.data_hub);
    shmring_destroy(&ctx.data_hub);
    uv_close((uv_handle_t*)&ctx.poll_timer, NULL);
    
    // Run loop until all close callbacks are done
    uv_run(ctx.loop, UV_RUN_DEFAULT); // Process closes
    uv_loop_close(ctx.loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_end_to_end_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
