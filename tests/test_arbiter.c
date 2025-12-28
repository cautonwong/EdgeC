#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"

typedef struct {
    uv_loop_t loop;
    arbiter_t *arb;
    int low_started;
    int high_started;
} test_ctx_t;

static void on_start(void *user) {
    int *flag = (int *)user;
    (*flag)++;
}

static void test_arbiter_preempt(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;
    arbiter_bus_handle_t bus = arbiter_bus_register(ctx->arb, "RS485_1");
    (void)bus;

    cmd_msg_t low = {.req_id = "low-prio",
                     .priority = CMD_PRIO_NORMAL,
                     .on_start = on_start,
                     .user_data = &ctx->low_started};

    cmd_msg_t high = {.req_id = "high-prio",
                      .priority = CMD_PRIO_EMERGENCY,
                      .on_start = on_start,
                      .user_data = &ctx->high_started};

    // 1. 提交低优先级
    arbiter_submit_cmd(ctx->arb, 0, &low);

    for (int i = 0; i < 50; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        if (ctx->low_started) break;
        usleep(1000);
    }
    assert_int_equal(ctx->low_started, 1);

    // 2. 提交高优先级 (在当前 owner 完成前，它会在队列等待，直到触发调度)
    arbiter_submit_cmd(ctx->arb, 0, &high);

    // 模拟低优先级完成
    arbiter_on_io_done(ctx->arb, 0, &low);

    for (int i = 0; i < 50; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        if (ctx->high_started) break;
        usleep(1000);
    }
    assert_int_equal(ctx->high_started, 1);
}

static int setup(void **state) {
    test_ctx_t *ctx = calloc(1, sizeof(*ctx));
    uv_loop_init(&ctx->loop);
    arbiter_init(&ctx->arb, &ctx->loop);
    *state = ctx;
    return 0;
}

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int teardown(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;
    arbiter_shutdown(ctx->arb);

    uv_walk(&ctx->loop, on_walk_close, NULL);
    while (uv_loop_close(&ctx->loop) == UV_EBUSY) {
        uv_run(&ctx->loop, UV_RUN_ONCE);
    }

    free(ctx);
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_arbiter_preempt, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
