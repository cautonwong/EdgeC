#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/mman.h>
#include <uv.h>
#include "modbus_protocol.h"
#include "shmring.h"
#include <unistd.h>

shm_ring_t *global_ring;

typedef struct {
    int found;
    shm_ring_t *ring;
    uv_timer_t *timeout_handle;
    uv_timer_t *poll_handle;
} test_ctx_t;

static void on_test_timeout(uv_timer_t *handle) {
    (void)handle;
    fail_msg("Test timed out");
}

static void on_poll_ring(uv_timer_t *handle) {
    test_ctx_t *ctx = (test_ctx_t *)handle->data;
    
    PipeMsg m;
    uint64_t seq = 0;
    if (shm_ring_read(ctx->ring, &seq, &m) == 0) {
        if (m.payload.val_f > 100.0) {
            ctx->found = 1;
            uv_timer_stop(ctx->timeout_handle);
            uv_timer_stop(ctx->poll_handle);
        }
    }
}

static void test_protocol_flow(void **state) {
    (void)state;
    static shm_ring_t ring;
    static uv_loop_t loop;
    char ring_name[] = "/test_ring_proto";
    
    shm_unlink(ring_name);

    assert_int_equal(shmring_create(&ring, ring_name, 8), 0);
    global_ring = &ring;

    assert_int_equal(uv_loop_init(&loop), 0);

    io_executor_init(&loop);

    uv_timer_t timeout_handle, poll_handle;
    uv_timer_init(&loop, &timeout_handle);
    uv_timer_init(&loop, &poll_handle);
    
    test_ctx_t ctx = {
        .found = 0,
        .ring = &ring,
        .timeout_handle = &timeout_handle,
        .poll_handle = &poll_handle
    };
    poll_handle.data = &ctx;

    modbus_protocol_init(&loop);
    modbus_read_request(101); 

    uv_timer_start(&timeout_handle, on_test_timeout, 2000, 0); // 2s timeout
    uv_timer_start(&poll_handle, on_poll_ring, 10, 10); // Poll every 10ms

    uv_run(&loop, UV_RUN_DEFAULT);

    assert_int_equal(ctx.found, 1);

    io_executor_shutdown();
    uv_run(&loop, UV_RUN_ONCE); // Process close callbacks

    uv_loop_close(&loop);
    
    shmring_close(&ring);
    shmring_destroy(&ring);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_protocol_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
