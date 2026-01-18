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
    int started_flags[2];
} test_ctx_t;

static void on_start(void *user) {
    int *flag = (int *)user;
    (*flag) = 1;
}

// 物理回收指令
static void on_cleanup(cmd_msg_t *cmd) {
    if (cmd) free(cmd);
}

static void test_arbiter_preempt(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;

    // 物理定力：在堆上分配以适配 on_cleanup
    cmd_msg_t *low = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(low, 1001, 1.0, ARBITER_PRIO_LOW);
    low->on_start = on_start;
    low->on_cleanup = on_cleanup;
    low->user_data = &ctx->started_flags[0];

    cmd_msg_t *high = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(high, 1001, 2.0, ARBITER_PRIO_EMERGENCY);
    high->on_start = on_start;
    high->on_cleanup = on_cleanup;
    high->user_data = &ctx->started_flags[1];

    // 1. 提交低优先级
    arbiter_submit_cmd(ctx->arb, 0, low);

    for (int i = 0; i < 50; i++) {
        uv_run(&ctx->loop, UV_RUN_ONCE);
        if (ctx->started_flags[0]) break;
    }
    assert_int_equal(ctx->started_flags[0], 1);

    // 2. 提交高优先级
    arbiter_submit_cmd(ctx->arb, 0, high);

    // 3. 模拟低优先级完成 (注意：由于注册了 cleanup，此命令会被物理 free)
    arbiter_on_io_done(ctx->arb, 0, low, 0);

    for (int i = 0; i < 50; i++) {
        uv_run(&ctx->loop, UV_RUN_ONCE);
        if (ctx->started_flags[1]) break;
    }
    assert_int_equal(ctx->started_flags[1], 1);

    arbiter_on_io_done(ctx->arb, 0, high, 0);
}

static int setup(void **state) {
    test_ctx_t *ctx = calloc(1, sizeof(*ctx));
    uv_loop_init(&ctx->loop);
    arbiter_init(&ctx->arb, &ctx->loop);
    arbiter_resume(ctx->arb);  // 新增：必须恢复调度才能进行测试
    *state = ctx;
    return 0;
}

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int teardown(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;
    arbiter_destroy(ctx->arb);

    uv_walk(&ctx->loop, on_walk_close, NULL);
    for (int i = 0; i < 50; i++) {
        if (uv_run(&ctx->loop, UV_RUN_NOWAIT) == 0) break;
        usleep(1000);
    }

    uv_loop_close(&ctx->loop);
    free(ctx);
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_arbiter_preempt, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}