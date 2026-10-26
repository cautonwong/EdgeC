#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "arbiter.h"

/**
 * 断链 / 超时 时命令所有权交接的回归测试
 *
 * 现场事故(2026-10-25):南向 socket 被对端 close,以及总线只收不回,都让 iems
 * 崩在 master_engine 的 _process_req_result / _master_on_timeout 里。
 *
 * 机制:引擎的 uv_timer 与 arbiter_check_timeouts 是同一个 timeout_ms 的两个权威。
 * 后者到期会结算 owner、把它 free 掉,但**不通知引擎**;引擎随后再用 current_cmd
 * 就是 use-after-free。引擎因此在碰命令之前先问 arbiter_owns_cmd。
 *
 * 这个测试锁住的就是那句查询的语义:持有期间为真,被到期扫描结算之后必须为假。
 */

static int g_completed;
static int g_completed_status;

static uv_loop_t* g_loop;

static void on_done(cmd_msg_t* cmd, int status) {
    (void)cmd;
    g_completed++;
    g_completed_status = status;
}

static cmd_msg_t* make_cmd(void) {
    cmd_msg_t* cmd = calloc(1, sizeof(cmd_msg_t));
    assert_non_null(cmd);
    cmd->priority = ARBITER_PRIO_NORMAL;
    cmd->timeout_ms = 100;
    cmd->on_complete = on_done;
    strncpy(cmd->req_id, "OWN", sizeof(cmd->req_id) - 1);
    return cmd;
}

static int setup(void** state) {
    arbiter_t* arb = NULL;
    g_loop = calloc(1, sizeof(uv_loop_t));
    uv_loop_init(g_loop);
    assert_int_equal(arbiter_init(&arb, g_loop), 0);
    arbiter_resume(arb);
    g_completed = 0;
    g_completed_status = 0;
    *state = arb;
    return 0;
}

static int teardown(void** state) {
    arbiter_destroy((arbiter_t*)*state);
    uv_run(g_loop, UV_RUN_NOWAIT);
    uv_loop_close(g_loop);
    free(g_loop);
    return 0;
}

/** 排队里的命令不算 owner;派发出去之后才算,结算之后必须不再算 */
static void test_owns_cmd_tracks_ownership(void** state) {
    arbiter_t* arb = (arbiter_t*)*state;

    assert_false(arbiter_owns_cmd(arb, 0, NULL));
    assert_false(arbiter_owns_cmd(arb, -1, NULL));

    cmd_msg_t* cmd = make_cmd();
    assert_false(arbiter_owns_cmd(arb, 0, cmd));  // 还没交出去
    assert_false(arbiter_owns_cmd(arb, 1, cmd));  // 别的总线更不算(索引要按 bus_id 走)

    // 没有引擎注册时 arbiter 自己不会派发,这里只验证查询本身不会崩、不会误判
    assert_int_equal(arbiter_submit_cmd(arb, 0, cmd), 0);
    arbiter_on_io_done(arb, 0, cmd, CMD_STATUS_SUCCESS);
    assert_int_equal(g_completed, 1);
    assert_int_equal(g_completed_status, CMD_STATUS_SUCCESS);
}

/** 到期扫描结算 owner 之后,引擎侧那句查询必须返回 false(否则就是 UAF) */
static void test_owns_cmd_false_after_timeout_sweep(void** state) {
    arbiter_t* arb = (arbiter_t*)*state;

    cmd_msg_t* cmd = make_cmd();
    cmd->timeout_ms = 1;
    assert_int_equal(arbiter_submit_cmd(arb, 0, cmd), 0);

    // 等过 timeout_ms 再让 arbiter 自己扫描一次:它会结算并 free 掉 cmd
    uv_sleep(20);
    int swept = arbiter_check_timeouts(arb);
    assert_int_equal(swept, 1);  // 恰好结算这一条,漏掉就是"超时不再生效"
    assert_int_equal(g_completed, 1);

    // 关键断言:cmd 已经没了,引擎再问必须得到 false —— 绝不能在这之后解引用它
    assert_false(arbiter_owns_cmd(arb, 0, cmd));

    // 再发一条照常能跑:执行器/仲裁器没被这条命令的死亡带走
    cmd_msg_t* next = make_cmd();
    assert_int_equal(arbiter_submit_cmd(arb, 0, next), 0);
    arbiter_on_io_done(arb, 0, next, CMD_STATUS_SUCCESS);
    assert_int_equal(g_completed, 2);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_owns_cmd_tracks_ownership, setup, teardown),
        cmocka_unit_test_setup_teardown(test_owns_cmd_false_after_timeout_sweep, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
