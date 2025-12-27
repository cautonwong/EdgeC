#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <uv.h>
#include "arbiter.h"

/* ---------------- test context ---------------- */

typedef struct {
    uv_loop_t loop;
    arbiter_t *arb;

    int low_started;
    int low_preempted;
    int high_started;
    int high_done;

} test_ctx_t;

/* ---------------- callbacks ---------------- */

static void on_start(void *user)
{
    int *flag = (int *)user;
    (*flag)++;
}

static void on_preempt(void *user)
{
    int *flag = (int *)user;
    (*flag)++;
}

static void on_done(void *user)
{
    int *flag = (int *)user;
    (*flag)++;
}

/* ---------------- test case ---------------- */

/*
 * 验证：
 *  - 低优先级先运行
 *  - 高优先级到来后抢占
 */
static void test_arbiter_preempt(void **state)
{
    test_ctx_t *ctx = (test_ctx_t *)*state;

    arbiter_bus_handle_t bus = arbiter_bus_register(ctx->arb, "RS485_1");
    assert_non_null(bus);

    arbiter_req_t low = {
        .priority = ARB_PRIO_LOW,
        .on_start = on_start,
        .on_preempt = on_preempt,
        .on_done = on_done,
        .user_start = &ctx->low_started,
        .user_preempt = &ctx->low_preempted,
        .user_done = NULL,
    };

    arbiter_req_t high = {
        .priority = ARB_PRIO_HIGH,
        .on_start = on_start,
        .on_preempt = NULL,
        .on_done = on_done,
        .user_start = &ctx->high_started,
        .user_done = &ctx->high_done,
    };

    arbiter_submit(bus, &low);

    /* 让 loop 跑几圈，直到 low start. 
       注意：arbiter 使用 timer(10ms) 驱动调度 */
    for (int i=0; i<50; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        if (ctx->low_started) break;
        uv_sleep(1000);
    }
    
    assert_int_equal(ctx->low_started, 1);
    assert_int_equal(ctx->low_preempted, 0);

    /* 提交高优先级 */
    arbiter_submit(bus, &high);

    /* 让 loop 跑，触发抢占 */
    for (int i=0; i<100; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        if (ctx->high_started) break;
        uv_sleep(1000);
    }

    /* 断言抢占发生 */
    assert_int_equal(ctx->low_preempted, 1);
    assert_int_equal(ctx->high_started, 1);
    
    /* 释放高优先级 */
    arbiter_release(bus, &high);
    assert_int_equal(ctx->high_done, 1);
}

/* ---------------- setup / teardown ---------------- */

static int setup(void **state)
{
    test_ctx_t *ctx = calloc(1, sizeof(*ctx));
    assert_non_null(ctx);

    uv_loop_init(&ctx->loop);
    arbiter_init(&ctx->arb, &ctx->loop);

    *state = ctx;
    return 0;
}

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int teardown(void **state)
{
    test_ctx_t *ctx = (test_ctx_t *)*state;

    arbiter_shutdown(ctx->arb);
    
    // Close all remaining handles in the loop
    uv_walk(&ctx->loop, on_walk_close, NULL);
    uv_run(&ctx->loop, UV_RUN_DEFAULT);
    
    uv_loop_close(&ctx->loop);
    free(ctx);

    return 0;
}

/* ---------------- main ---------------- */

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(
            test_arbiter_preempt,
            setup,
            teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
