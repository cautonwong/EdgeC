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

static void test_arbiter_bounded_queue_and_priority_drop(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    arbiter_t* arb = NULL;
    arbiter_init(&arb, &loop);
    /* 保持 paused 状态，使命令留在队列中不被派发 */
    arbiter_pause(arb);

    /* 设置第 0 号总线队列上限为 5 条命令 */
    arbiter_set_bus_queue_limit(arb, 0, 5);

    /* 1. 注入 5 条低优先级命令填充队列 */
    cmd_msg_t* cmds[10];

    for (int i = 0; i < 5; i++) {
        cmds[i] = calloc(1, sizeof(cmd_msg_t));
        cmd_msg_init_write(cmds[i], 1000 + i, (double)i, ARBITER_PRIO_LOW);
        cmds[i]->on_cleanup = on_cleanup;
        int rc = arbiter_submit_cmd(arb, 0, cmds[i]);
        assert_int_equal(rc, 0);
    }

    arbiter_bus_status_t stats;
    assert_int_equal(arbiter_get_statuses(arb, &stats, 1), 1);
    assert_int_equal(stats.queue_len, 5);
    assert_int_equal(stats.dropped_count, 0);

    /* 2. 注入第 6 条低/普通优先级命令：必须被准入控制拒绝 (CMD_STATUS_QUEUE_FULL) */
    cmds[5] = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmds[5], 1005, 5.0, ARBITER_PRIO_NORMAL);
    cmds[5]->on_cleanup = on_cleanup;
    int rc_rejected = arbiter_submit_cmd(arb, 0, cmds[5]);
    assert_int_equal(rc_rejected, CMD_STATUS_QUEUE_FULL);
    free(cmds[5]);

    assert_int_equal(arbiter_get_bus_dropped_count(arb, 0), 1);

    /* 3. 注入 EMERGENCY 紧急跳闸命令：必须触发 Priority-Drop 剔除队尾低优命令入队成功 */
    cmds[6] = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmds[6], 9999, 1.0, ARBITER_PRIO_EMERGENCY);
    cmds[6]->on_cleanup = on_cleanup;
    int rc_emergency = arbiter_submit_cmd(arb, 0, cmds[6]);
    assert_int_equal(rc_emergency, 0);

    /* 验证丢弃计数增加（低优被挤掉），且当前队列长度依然严格在上限 5 */
    assert_int_equal(arbiter_get_bus_dropped_count(arb, 0), 2);
    assert_int_equal(arbiter_get_statuses(arb, &stats, 1), 1);
    assert_int_equal(stats.queue_len, 5);

    arbiter_destroy(arb);
    uv_loop_close(&loop);
    for (int i = 0; i < 5; i++) {
        free(cmds[i]);
    }
    free(cmds[6]);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_arbiter_preempt, setup, teardown),
        cmocka_unit_test(test_arbiter_stress_and_destroy),
        cmocka_unit_test(test_arbiter_bounded_queue_and_priority_drop),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}