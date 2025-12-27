#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "io_executor.h"
#include "modbus_protocol.h"
#include "shmring.h"

shm_ring_t *global_ring;

typedef struct {
    int found;
    int ticks;
    shm_ring_t *ring;
    uv_timer_t *timeout_handle;
    uv_timer_t *poll_handle;
} test_ctx_t;

static void on_test_timeout(uv_timer_t *handle) {
    test_ctx_t *ctx = (test_ctx_t *)handle->data;
    printf("[Test] !!! TIMEOUT REACHED !!! (ticks: %d, found: %d)\n", ctx->ticks, ctx->found);
    uv_stop(handle->loop);
    // fail_msg 会导致直接退出，可能漏掉清理
}

static void on_poll_ring(uv_timer_t *handle) {
    test_ctx_t *ctx = (test_ctx_t *)handle->data;
    ctx->ticks++;

    PipeMsg m;
    uint64_t seq = 0;
    // 尝试读取 ring
    if (shm_ring_read(ctx->ring, &seq, &m) == 0) {
        printf("[Test] Ring data detected: val=%f\n", m.payload.val_f);
        if (m.payload.val_f > 100.0) {
            ctx->found = 1;
            uv_timer_stop(ctx->timeout_handle);
            uv_timer_stop(ctx->poll_handle);
            uv_stop(handle->loop);
        }
    }

    if (ctx->ticks % 100 == 0) {
        printf("[Test] Polling... tick %d\n", ctx->ticks);
    }
}

static void test_protocol_flow(void **state) {
    (void)state;
    static shm_ring_t ring;
    static uv_loop_t loop;
    char ring_name[] = "/test_ring_proto";

    printf("[Test] Starting ModbusProtocol Flow Test...\n");
    shm_unlink(ring_name);

    assert_int_equal(shmring_create(&ring, ring_name, 8), 0);
    global_ring = &ring;

    assert_int_equal(uv_loop_init(&loop), 0);
    io_executor_init(&loop);

    uv_timer_t timeout_handle, poll_handle;
    uv_timer_init(&loop, &timeout_handle);
    uv_timer_init(&loop, &poll_handle);

    test_ctx_t ctx = {.found = 0,
                      .ticks = 0,
                      .ring = &ring,
                      .timeout_handle = &timeout_handle,
                      .poll_handle = &poll_handle};
    poll_handle.data = &ctx;
    timeout_handle.data = &ctx;

    modbus_protocol_init(&loop);

    printf("[Test] Submitting Modbus read request...\n");
    modbus_read_request(101);

    // 设置 5 秒总超时，10ms 轮询一次
    uv_timer_start(&timeout_handle, on_test_timeout, 5000, 0);
    uv_timer_start(&poll_handle, on_poll_ring, 10, 10);

    uv_run(&loop, UV_RUN_DEFAULT);

    printf("[Test] Loop finished. Found status: %d\n", ctx.found);
    assert_int_equal(ctx.found, 1);

    io_executor_shutdown();

    // 彻底清理 loop
    uv_loop_close(&loop);
    shmring_close(&ring);
    shmring_destroy(&ring);
    printf("[Test] Test completed clean.\n");
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_protocol_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}