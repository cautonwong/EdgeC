#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"

typedef struct {
    uv_loop_t loop;
    arbiter_t* arb;
    int started_flags[2];
} test_ctx_t;

/* 签名与 cmd_msg_t.on_start 一致：靠"函数指针类型不兼容"的警告混过去就是 UB */
static void on_start(cmd_msg_t* cmd) {
    if (!cmd) return;
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

static void test_arbiter_caller_allocated(void** state) {
    (void)state;
    arbiter_storage_t store;
    uv_loop_t loop;
    uv_loop_init(&loop);

    arbiter_t* arb = arbiter_create_in(&store, &loop);
    assert_non_null(arb);
    assert_ptr_equal(arb, (arbiter_t*)&store);

    arbiter_bus_status_t stats;
    assert_int_equal(arbiter_get_statuses(arb, &stats, 1), 1);

    arbiter_destroy(arb);
    uv_loop_close(&loop);
}

typedef struct {
    int started;
    int completed;
    int status;
} timeout_test_ctx_t;

static int never_finish_execute(void* ctx, int bus_id, cmd_msg_t* cmd) {
    (void)bus_id;
    (void)cmd;
    timeout_test_ctx_t* tctx = (timeout_test_ctx_t*)ctx;
    if (tctx) tctx->started++;
    return 0;  // Hangs forever without calling arbiter_on_io_done
}

static void on_timeout_complete(cmd_msg_t* cmd, int status) {
    timeout_test_ctx_t* tctx = (timeout_test_ctx_t*)cmd->user_data;
    if (tctx) {
        tctx->completed++;
        tctx->status = status;
    }
}

static void test_arbiter_timeout_forced_release(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    arbiter_t* arb = NULL;
    arbiter_init(&arb, &loop);
    arbiter_resume(arb);

    timeout_test_ctx_t tctx = {0};
    arb_executor_ops_t exec_ops = {
        .ctx = &tctx,
        .execute = never_finish_execute,
        .on_io_done = NULL,
    };
    arbiter_set_executor(arb, exec_ops);

    cmd_msg_t* hung_cmd = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_read(hung_cmd, 1001, ARBITER_PRIO_NORMAL);
    hung_cmd->timeout_ms = 40;  // 40ms 超时
    hung_cmd->user_data = &tctx;
    hung_cmd->on_complete = on_timeout_complete;
    hung_cmd->on_cleanup = on_cleanup;

    arbiter_submit_cmd(arb, 0, hung_cmd);
    assert_int_equal(tctx.started, 1);

    arbiter_bus_status_t stats;
    arbiter_get_statuses(arb, &stats, 1);
    assert_true(stats.is_busy);

    // 运行事件循环驱动定时器检测超时
    for (int i = 0; i < 30; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        if (tctx.completed > 0) break;
        usleep(10000);  // 10ms
    }

    // 断言超时触发：收到 CMD_STATUS_TIMEOUT，总线被强制剥夺并释放
    assert_int_equal(tctx.completed, 1);
    assert_int_equal(tctx.status, CMD_STATUS_TIMEOUT);

    arbiter_get_statuses(arb, &stats, 1);
    assert_false(stats.is_busy);

    arbiter_destroy(arb);
    uv_loop_close(&loop);
}

static int s_edf_order[4] = {0};
static int s_edf_count = 0;

static int record_exec_edf(void* ctx, int bus_id, cmd_msg_t* cmd) {
    (void)bus_id;
    (void)ctx;
    if (s_edf_count < 4) {
        s_edf_order[s_edf_count++] = (int)cmd->tag_id;
    }
    return 0;
}

static void test_arbiter_edf_ordering(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    arbiter_t* arb = NULL;
    arbiter_init(&arb, &loop);
    arbiter_pause(arb);  // 暂停以构建积压队列

    s_edf_count = 0;
    arb_executor_ops_t ops = {.execute = record_exec_edf};
    arbiter_set_executor(arb, ops);

    uint64_t now = uv_hrtime();

    // 提交 3 个具有相同优先级 NORMAL 的命令，但截止期分别为 300ms, 100ms, 200ms
    cmd_msg_t* c1 = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_read(c1, 101, ARBITER_PRIO_NORMAL);
    c1->submit_ts = now;
    c1->deadline_ns = now + 300000000ULL;
    c1->on_cleanup = on_cleanup;

    cmd_msg_t* c2 = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_read(c2, 102, ARBITER_PRIO_NORMAL);
    c2->submit_ts = now;
    c2->deadline_ns = now + 100000000ULL;
    c2->on_cleanup = on_cleanup;

    cmd_msg_t* c3 = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_read(c3, 103, ARBITER_PRIO_NORMAL);
    c3->submit_ts = now;
    c3->deadline_ns = now + 200000000ULL;
    c3->on_cleanup = on_cleanup;

    arbiter_submit_cmd(arb, 0, c1);
    arbiter_submit_cmd(arb, 0, c2);
    arbiter_submit_cmd(arb, 0, c3);

    arbiter_resume(arb);

    // 验证最早截止期 (c2, 100ms) 最先出队派发
    assert_int_equal(s_edf_count, 1);
    assert_int_equal(s_edf_order[0], 102);

    // 完成第一条，派发次早截止期 (c3, 200ms)
    arbiter_on_io_done(arb, 0, c2, 0);
    assert_int_equal(s_edf_count, 2);
    assert_int_equal(s_edf_order[1], 103);

    // 完成第二条，派发最晚截止期 (c1, 300ms)
    arbiter_on_io_done(arb, 0, c3, 0);
    assert_int_equal(s_edf_count, 3);
    assert_int_equal(s_edf_order[2], 101);

    arbiter_on_io_done(arb, 0, c1, 0);

    arbiter_destroy(arb);
    uv_loop_close(&loop);
}

static int s_aging_dispatched_tag = 0;
static int aging_test_exec(void* ctx, int bus_id, cmd_msg_t* cmd) {
    (void)bus_id;
    (void)ctx;
    s_aging_dispatched_tag = (int)cmd->tag_id;
    return 0;
}

static void test_arbiter_aging_anti_starvation(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    arbiter_t* arb = NULL;
    arbiter_init(&arb, &loop);
    arbiter_pause(arb);

    s_aging_dispatched_tag = 0;
    arb_executor_ops_t ops = {.execute = aging_test_exec};
    arbiter_set_executor(arb, ops);

    uint64_t now = uv_hrtime();

    // 1. 模拟一个在 2 秒前提交的低优先级命令 (LOW = 0)
    // 2000ms / 50ms = 40 个 aging 步进，有效优先级提升至 EMERGENCY (200)
    cmd_msg_t* old_low = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_read(old_low, 888, ARBITER_PRIO_LOW);
    old_low->submit_ts = now - 2000000000ULL;
    old_low->deadline_ns = now + 1000000000ULL;
    old_low->on_cleanup = on_cleanup;
    arbiter_submit_cmd(arb, 0, old_low);

    // 2. 刚刚提交的 NORMAL 优先级命令 (NORMAL = 50)
    cmd_msg_t* fresh_normal = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_read(fresh_normal, 999, ARBITER_PRIO_NORMAL);
    fresh_normal->submit_ts = now;
    fresh_normal->deadline_ns = now + 1000000000ULL;
    fresh_normal->on_cleanup = on_cleanup;
    arbiter_submit_cmd(arb, 0, fresh_normal);

    // 3. 恢复调度：经过 aging 的 old_low 有效优先级压倒 fresh_normal，优先派发
    arbiter_resume(arb);
    assert_int_equal(s_aging_dispatched_tag, 888);

    arbiter_on_io_done(arb, 0, old_low, 0);
    assert_int_equal(s_aging_dispatched_tag, 999);
    arbiter_on_io_done(arb, 0, fresh_normal, 0);

    arbiter_destroy(arb);
    uv_loop_close(&loop);
}

static int s_exclusive_dispatched_tag = 0;
static int exclusive_test_exec(void* ctx, int bus_id, cmd_msg_t* cmd) {
    (void)bus_id;
    (void)ctx;
    s_exclusive_dispatched_tag = (int)cmd->tag_id;
    return 0;
}

static void test_arbiter_exclusive_bus(void** state) {
    (void)state;
    uv_loop_t loop;
    uv_loop_init(&loop);
    arbiter_t* arb = NULL;
    arbiter_init(&arb, &loop);
    arbiter_resume(arb);

    s_exclusive_dispatched_tag = 0;
    arb_executor_ops_t ops = {.execute = exclusive_test_exec};
    arbiter_set_executor(arb, ops);

    // 1. 开启总线 0 独占模式
    arbiter_set_exclusive(arb, 0, true);

    // 2. 提交普通命令，排他总线必须阻断其派发
    cmd_msg_t* norm = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_read(norm, 111, ARBITER_PRIO_NORMAL);
    norm->on_cleanup = on_cleanup;
    arbiter_submit_cmd(arb, 0, norm);
    assert_int_equal(s_exclusive_dispatched_tag, 0);

    arbiter_bus_status_t stats;
    arbiter_get_statuses(arb, &stats, 1);
    assert_int_equal(stats.queue_len, 1);
    assert_false(stats.is_busy);

    // 3. 提交紧急跳闸命令 (EMERGENCY)，验证允许穿透独占总线执行
    cmd_msg_t* emerg = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(emerg, 999, 1.0, ARBITER_PRIO_EMERGENCY);
    emerg->on_cleanup = on_cleanup;
    arbiter_submit_cmd(arb, 0, emerg);
    assert_int_equal(s_exclusive_dispatched_tag, 999);

    arbiter_on_io_done(arb, 0, emerg, 0);
    // 依然独占，普通命令仍留在队列
    assert_int_equal(s_exclusive_dispatched_tag, 999);

    // 4. 解除独占模式，普通命令立即被唤醒并派发
    arbiter_set_exclusive(arb, 0, false);
    assert_int_equal(s_exclusive_dispatched_tag, 111);

    arbiter_on_io_done(arb, 0, norm, 0);

    arbiter_destroy(arb);
    uv_loop_close(&loop);
}

/* ---- 帧间静默 (min_gap) ---------------------------------------------------
 * 半双工串口要求相邻两帧之间保持线路静默。间隔必须从"上一帧结束"起算，
 * 而不是从"上一帧发起"起算——否则下一帧会压在上一帧尾巴上，645 从站会粘包。
 */
typedef struct {
    uv_loop_t loop;
    uint64_t start_ts[4];
    int started;
} gap_ctx_t;

static gap_ctx_t* g_gap_ctx;

static void gap_on_start(cmd_msg_t* cmd) {
    (void)cmd;
    if (g_gap_ctx && g_gap_ctx->started < 4) {
        g_gap_ctx->start_ts[g_gap_ctx->started++] = uv_hrtime();
    }
}

static cmd_msg_t* gap_make_cmd(uint32_t tag) {
    cmd_msg_t* cmd = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmd, tag, 1.0, ARBITER_PRIO_NORMAL);
    cmd->on_start = gap_on_start;
    cmd->on_cleanup = on_cleanup;
    return cmd;
}

static void test_arbiter_min_gap(void** state) {
    (void)state;
    gap_ctx_t c;
    memset(&c, 0, sizeof(c));
    uv_loop_init(&c.loop);

    /* 用调用方分配存储：同时验证 4608 的 storage 契约成立 */
    arbiter_storage_t storage;
    arbiter_t* arb = arbiter_create_in(&storage, &c.loop);
    assert_non_null(arb);
    arbiter_resume(arb);
    arbiter_set_bus_min_gap(arb, 0, 50000); /* 50ms */
    g_gap_ctx = &c;

    /* 第一帧没有历史，立即发出 */
    cmd_msg_t* first = gap_make_cmd(1);
    assert_int_equal(arbiter_submit_cmd(arb, 0, first), 0);
    assert_int_equal(c.started, 1);

    /* 结束第一帧：静默从这里开始计时 */
    arbiter_on_io_done(arb, 0, first, CMD_STATUS_SUCCESS);

    /* 第二帧提交后不得立即发出 */
    cmd_msg_t* second = gap_make_cmd(2);
    assert_int_equal(arbiter_submit_cmd(arb, 0, second), 0);
    assert_int_equal(c.started, 1);

    /* 跑事件循环，等静默到期 */
    for (int i = 0; i < 400 && c.started < 2; i++) {
        uv_run(&c.loop, UV_RUN_ONCE);
    }
    assert_int_equal(c.started, 2);

    /* 间隔必须 >= 50ms（允许少量时钟误差） */
    uint64_t delta_ns = c.start_ts[1] - c.start_ts[0];
    assert_true(delta_ns >= 45000000ULL);

    arbiter_on_io_done(arb, 0, second, CMD_STATUS_SUCCESS);
    arbiter_destroy(arb);
    uv_run(&c.loop, UV_RUN_DEFAULT);
    uv_loop_close(&c.loop);
    g_gap_ctx = NULL;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_arbiter_preempt, setup, teardown),
        cmocka_unit_test(test_arbiter_stress_and_destroy),
        cmocka_unit_test(test_arbiter_bounded_queue_and_priority_drop),
        cmocka_unit_test(test_arbiter_caller_allocated),
        cmocka_unit_test(test_arbiter_timeout_forced_release),
        cmocka_unit_test(test_arbiter_edf_ordering),
        cmocka_unit_test(test_arbiter_aging_anti_starvation),
        cmocka_unit_test(test_arbiter_min_gap),
        cmocka_unit_test(test_arbiter_exclusive_bus),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}