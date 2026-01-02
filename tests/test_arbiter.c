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

    // 使用初始化宏确证 on_cleanup 等新成员被正确置 NULL
    cmd_msg_t low;
    cmd_msg_init_write(&low, 4001, 1.0, ARBITER_PRIO_LOW);
    low.on_start = on_start;
    low.user_data = &ctx->low_started;

    cmd_msg_t high;
    cmd_msg_init_write(&high, 4001, 2.0, ARBITER_PRIO_EMERGENCY);
    high.on_start = on_start;
    high.user_data = &ctx->high_started;

    // 1. 提交低优先级
    arbiter_submit_cmd(ctx->arb, 0, &low);

    for (int i = 0; i < 50; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        if (ctx->low_started) break;
        usleep(1000);
    }
    assert_int_equal(ctx->low_started, 1);

    // 2. 提交高优先级
    arbiter_submit_cmd(ctx->arb, 0, &high);

    // 模拟低优先级完成
    arbiter_on_io_done(ctx->arb, 0, &low);

    for (int i = 0; i < 50; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        if (ctx->high_started) break;
        usleep(1000);
    }
    assert_int_equal(ctx->high_started, 1);

    // 3. 关键：清理状态，防止 teardown 访问已销毁的栈内存
    arbiter_on_io_done(ctx->arb, 0, &high);
}

static int setup(void **state) {
    test_ctx_t *ctx = calloc(1, sizeof(*ctx));
    uv_loop_init(&ctx->loop);
    arbiter_init(&ctx->arb, &ctx->loop);
    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;

    if (ctx->arb) {
        arbiter_destroy(ctx->arb);
    }

    // 彻底排干 Loop 中的所有关闭请求
    int alive = 1;
    while (alive) {
        alive = uv_run(&ctx->loop, UV_RUN_ONCE);
        if (alive == 0) break;
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