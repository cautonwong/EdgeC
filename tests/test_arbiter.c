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
    arbiter_t* arb;
    int started_flags[2];
} test_ctx_t;

static void on_start(void* user) {
    if (!user) return;
    cmd_msg_t* cmd = (cmd_msg_t*)user;
    int* flag = (int*)cmd->user_data;
    if (flag) (*flag) = 1;
}

// 物理回收指令 (Arbiter 已经负责 free(cmd)，此处仅用于清理内部数据)
static void on_cleanup(cmd_msg_t* cmd) {
    (void)cmd;
}

static void test_arbiter_preempt(void** state) {
    test_ctx_t* ctx = (test_ctx_t*)*state;

    // 物理定力：在堆上分配以适配 on_cleanup
    cmd_msg_t* low = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(low, 1001, 1.0, ARBITER_PRIO_LOW);
    low->on_start = on_start;
    low->on_cleanup = on_cleanup;
    low->user_data = &ctx->started_flags[0];

    cmd_msg_t* high = calloc(1, sizeof(cmd_msg_t));
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

static int setup(void** state) {
    test_ctx_t* ctx = calloc(1, sizeof(*ctx));
    uv_loop_init(&ctx->loop);
    arbiter_init(&ctx->arb, &ctx->loop);
    arbiter_resume(ctx->arb);  // 新增：必须恢复调度才能进行测试
    *state = ctx;
    return 0;
}

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int teardown(void** state) {
    test_ctx_t* ctx = (test_ctx_t*)*state;
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

static void* submitter_thread(void* arg) {
    arbiter_t* arb = (arbiter_t*)arg;
    for (int i = 0; i < 100; i++) {
        cmd_msg_t* cmd = calloc(1, sizeof(cmd_msg_t));
        cmd_msg_init_write(cmd, (uint32_t)(100 + i), (double)i, (uint8_t)(i % 100));
        arbiter_submit_cmd(arb, i % 8, cmd);
        if (i % 20 == 0) {
            arbiter_pause(arb);
            usleep(100);
            arbiter_resume(arb);
        }
    }
    return NULL;
}

static void test_arbiter_stress_and_destroy(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    arbiter_t* arb = NULL;
    arbiter_init(&arb, &loop);
    arbiter_resume(arb);

    pthread_t t1, t2;
    pthread_create(&t1, NULL, submitter_thread, arb);
    pthread_create(&t2, NULL, submitter_thread, arb);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    arbiter_bus_status_t stats[8];
    int count = arbiter_get_statuses(arb, stats, 8);
    assert_int_equal(count, 8);

    // Destroy arbiter with queued/active jobs; verify clean teardown
    arbiter_destroy(arb);

    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_arbiter_preempt, setup, teardown),
        cmocka_unit_test(test_arbiter_stress_and_destroy),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}