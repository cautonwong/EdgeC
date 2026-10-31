#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_request.h"
#include "io_executor.h"
#include "net_buf.h"

/**
 * @file test_master_link_lifecycle.c
 * @brief 主站引擎在链路异常(对端关闭 / 只发不收)下必须活着并且每条命令都有结算。
 *
 * 这两条用例锁的是两个真实事故:南向 unix socket 对端 close() 导致 SIGSEGV、
 * 以及同一条总线上第二次超时时的 use-after-free。都不需要真驱动:请求结构
 * 由测试自己填一份最小实现(ops 全在本文件里),只验证引擎与 arbiter 的生命周期。
 */

typedef struct {
    uv_loop_t loop;
    arbiter_t *arb;
    bus_orchestrator_t *orch;
    driver_manager_t *drv_mgr;
    net_buf_pool_t pool;
    datahub_t hub;
    io_handle_t *master_io;
    int peer_fd;
} link_ctx_t;

static int g_done;
static int g_done_status;
static int g_finish;      /* 驱动侧 on_finish 被叫了几次 */
static int g_last_status;
/* 0: 收到一个字节就报 DONE;1: 永远 INCOMPLETE(只发不收) */
static int g_decode_mode;

static int fake_setup(edge_req_t *req, void *ctx) {
    (void)req;
    (void)ctx;
    return 0;
}

static int fake_encode(edge_req_t *req, net_buf_t *buf) {
    (void)req;
    net_buf_simple_add_u8(&buf->b, 0x55);
    return 0;
}

static int fake_decode(edge_req_t *req, net_buf_t *buf, size_t *consumed) {
    (void)req;
    if (g_decode_mode == 1) {
        *consumed = 0;
        return EDGE_ERR_INCOMPLETE;
    }
    *consumed = buf->b.len;
    return REQ_STATUS_DONE;
}

static void fake_teardown(edge_req_t *req) { (void)req; }

static void fake_on_finish(edge_req_t *req, int status) {
    (void)req;
    g_finish++;
    g_last_status = status;
}

static const request_ops_t k_fake_ops = {
    .setup = fake_setup,
    .encode = fake_encode,
    .decode = fake_decode,
    .teardown = fake_teardown,
    .on_finish = fake_on_finish,
};

static void on_cmd_done(cmd_msg_t *msg, int status) {
    (void)msg;
    g_done_status = status;
    g_done++;
}

static void on_cmd_cleanup(cmd_msg_t *msg) { (void)msg; }

/* 推一段事件循环:既要 uv 的 IO,也要引擎自己那条 uv_timer */
static void pump(uv_loop_t *loop, int ms) {
    uint64_t deadline = uv_hrtime() + (uint64_t)ms * 1000000ULL;
    while (uv_hrtime() < deadline) {
        uv_run(loop, UV_RUN_NOWAIT);
        uv_sleep(1);
    }
}

static link_ctx_t *make_ctx(void) {
    link_ctx_t *ctx = calloc(1, sizeof(link_ctx_t));
    assert_non_null(ctx);
    assert_int_equal(uv_loop_init(&ctx->loop), 0);
    /* shortcut: io handle 关不掉(executor 没有 close API),用例跑完靠进程退出回收 */

    assert_int_equal(datahub_init(&ctx->hub, &ctx->loop, "/test_master_link_lifecycle", 8, 256, NULL,
                                  0, NULL, NULL, NULL),
                     0);
    assert_int_equal(arbiter_init(&ctx->arb, &ctx->loop), 0);
    arbiter_resume(ctx->arb);
    assert_int_equal(net_buf_pool_init(&ctx->pool, 16, 256, 0, NULL, NULL), 0);
    ctx->drv_mgr = driver_manager_create();
    assert_non_null(ctx->drv_mgr);
    ctx->orch = bus_orchestrator_create(&ctx->hub, ctx->arb, ctx->drv_mgr, 4, 16, 256);
    assert_non_null(ctx->orch);

    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    ctx->peer_fd = fds[1];
    ctx->master_io = io_executor_create_from_fd(&ctx->loop, fds[0], &ctx->pool);
    assert_non_null(ctx->master_io);
    bus_orchestrator_attach_io(ctx->orch, 0, ctx->master_io);
    int setup_rc = bus_orchestrator_setup_master(ctx->orch, 0, "TestProto", 1);
    assert_int_equal(setup_rc, 0);
    return ctx;
}

static void teardown_ctx(link_ctx_t *ctx) {
    if (ctx->peer_fd >= 0) close(ctx->peer_fd);
    arbiter_destroy(ctx->arb);
    bus_orchestrator_destroy(ctx->orch);
    driver_manager_destroy(ctx->drv_mgr);
    /* 循环里还有没清的句柄(io handle 没有关闭 API),uv_loop_close 会返回 EBUSY,
     * 这里不假装它成功;进程退出即回收 */
    uv_run(&ctx->loop, UV_RUN_NOWAIT);
    datahub_destroy(&ctx->hub);
    net_buf_pool_destroy(&ctx->pool);
    free(ctx);
}

/* cmd 由 arbiter 负责 free,必须堆分配 */
static cmd_msg_t *make_cmd(edge_req_t *req, uint32_t timeout_ms) {
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    assert_non_null(cmd);
    cmd->action = CMD_ACTION_READ;
    cmd->tag_id = 1001;
    cmd->timeout_ms = timeout_ms;
    cmd->req = req;
    cmd->on_complete = on_cmd_done;
    cmd->on_cleanup = on_cmd_cleanup;
    strncpy(cmd->protocol, "TestProto", sizeof(cmd->protocol) - 1);
    return cmd;
}

static void test_master_survives_peer_close(void **state) {
    (void)state;
    g_done = 0;
    g_done_status = 0;
    g_finish = 0;
    g_last_status = 0;
    g_decode_mode = 0;

    link_ctx_t *ctx = make_ctx();
    edge_req_t req = {.ops = &k_fake_ops};

    assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, make_cmd(&req, 2000)), 0);
    pump(&ctx->loop, 30); /* 让请求真的发出去,引擎挂上 current_cmd */
    close(ctx->peer_fd);
    ctx->peer_fd = -1;
    pump(&ctx->loop, 300); /* 对端没了,引擎必须自己收尾,不能崩 */

    assert_int_equal(g_done, 1);
    assert_int_not_equal(g_done_status, CMD_STATUS_SUCCESS);
    assert_int_equal(g_finish, 1);
    assert_int_not_equal(g_last_status, CMD_STATUS_SUCCESS);

    /* executor 还能继续用:再发一条仍然有结算 */
    assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, make_cmd(&req, 500)), 0);
    pump(&ctx->loop, 300);
    assert_int_equal(g_done, 2);

    teardown_ctx(ctx);
}

static void test_master_survives_repeated_timeout(void **state) {
    (void)state;
    g_done = 0;
    g_done_status = 0;
    g_finish = 0;
    g_last_status = 0;
    g_decode_mode = 1; /* 只发不收 */

    link_ctx_t *ctx = make_ctx();
    edge_req_t req = {.ops = &k_fake_ops};

    /* 第一条:让 arbiter 的到期扫描先动手(引擎定时器 5s 后才到),
     * 锁住 _on_arbiter_settled 这条路径 */
    cmd_msg_t *slow = make_cmd(&req, 5000);
    slow->deadline_ns = uv_hrtime() + 1000000ULL; /* 1ms */
    assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, slow), 0);
    pump(&ctx->loop, 400);
    assert_int_equal(g_done, 1);
    assert_int_equal(g_finish, 1);
    assert_int_equal(g_last_status, CMD_STATUS_TIMEOUT);

    /* 第二圈:旧代码在这里炸(引擎定时器拿到的是 arbiter 已释放的 cmd) */
    assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, make_cmd(&req, 100)), 0);
    pump(&ctx->loop, 400);
    assert_int_equal(g_done, 2);
    assert_int_equal(g_finish, 2);

    teardown_ctx(ctx);
}


/* 同一总线并发提交 4 条,逐帧应答,4 条都必须结算 —— DCU 一次读一圈表的情景 */
static void test_master_settles_queued_commands_on_one_bus(void **state) {
    (void)state;
    g_done = 0;
    g_done_status = 0;
    g_finish = 0;
    g_last_status = 0;
    g_decode_mode = 0;

    link_ctx_t *ctx = make_ctx();
    edge_req_t req = {.ops = &k_fake_ops};

    for (int i = 0; i < 4; i++) {
        assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, make_cmd(&req, 2000)), 0);
    }

    /* 当从站:收到一帧就回一帧,直到 4 条都结算 */
    for (int round = 0; round < 200 && g_done < 4; round++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        uint8_t byte;
        ssize_t n = recv(ctx->peer_fd, &byte, 1, MSG_DONTWAIT);
        if (n == 1) {
            assert_int_equal(send(ctx->peer_fd, &byte, 1, 0), 1);
        }
        uv_sleep(5);
    }

    assert_int_equal(g_done, 4);
    assert_int_equal(g_done_status, CMD_STATUS_SUCCESS);
    assert_int_equal(g_finish, 4);

    teardown_ctx(ctx);
}

int main(void) {
    /* 对端关闭后再写会 EPIPE:不忽略 SIGPIPE 会把测试进程直接打断 */
    signal(SIGPIPE, SIG_IGN);
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_master_survives_peer_close),
        cmocka_unit_test(test_master_survives_repeated_timeout),
        cmocka_unit_test(test_master_settles_queued_commands_on_one_bus),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
