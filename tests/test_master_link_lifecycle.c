#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "bus_orchestrator.h"
#include "bus_datahub_adapter.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_request.h"
#include "engines/engine_priv_internal.h"
#include "io_executor.h"
#include "net_buf.h"
#include "datahub_priv.h"

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
    driver_manager_storage_t drv_mgr_storage;
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
/* on_finish 里申请并持有的缓冲(所有权回归用例) */
static net_buf_pool_t *g_hold_pool;
static net_buf_t *g_held;
static int g_hold_on_finish;

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
    /* 在引擎回调栈里向 io 句柄同款池申请一块并持有:锁住 "回调借用的 RX 载荷
     * 在回调期间必须由 io 层持有" 这条所有权约定 */
    if (g_hold_on_finish && g_hold_pool && !g_held) {
        g_held = net_buf_alloc(g_hold_pool, 0);
        if (g_held) {
            memset(g_held->b.data, 0x5A, 16);
            g_held->b.len = 16;
        }
        g_hold_on_finish = 0;
    }
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
    uint64_t deadline = uv_hrtime() + (uint64_t)ms * (uint64_t)1000000;
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
    ctx->drv_mgr = driver_manager_create_in(&ctx->drv_mgr_storage);
    assert_non_null(ctx->drv_mgr);
    ctx->orch = bus_orchestrator_create_hub_default(&ctx->hub, ctx->arb, driver_manager_session_port_ptr(ctx->drv_mgr), 4, 16, 256);
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

    /* 第一条:把 owner 的截止期拨到过去再手动触发 arbiter 扫描,锁住
     * _on_arbiter_settled 这条路径(引擎发帧时会把 deadline 改到上线时刻,
     * 不能再靠 1ms 的提交截止期让 arbiter 扫描抢先)。超时同样是 100ms:
     * 它同时决定超时后的恢复窗口,用 5s 会让第二条在后面干等 5s。 */
    cmd_msg_t *slow = make_cmd(&req, 100);
    assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, slow), 0);
    pump(&ctx->loop, 30); /* 让请求真的发出去,引擎挂上 current_cmd */
    slow->deadline_ns = uv_hrtime() - 1;
    arbiter_check_timeouts(ctx->arb);
    pump(&ctx->loop, 30);
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

/* 排队等过超时的命令,轮到它上线时必须拿到完整的事务超时预算。
 * 修复前 arbiter 用提交时刻的 deadline:队列里等满 100ms 的命令一上线
 * 就被判超时,它之后的应答会落到下一条命令头上(串 tag)。 */
static void test_queued_command_gets_full_timeout_on_wire(void **state) {
    (void)state;
    g_done = 0;
    g_done_status = 0;
    g_finish = 0;
    g_last_status = 0;
    g_decode_mode = 1; /* 第一条只发不收,占住总线到超时 */

    link_ctx_t *ctx = make_ctx();
    edge_req_t req = {.ops = &k_fake_ops};

    assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, make_cmd(&req, 100)), 0);
    assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, make_cmd(&req, 100)), 0);

    for (int waited = 0; waited < 300 && g_done < 1; waited += 5) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        uv_sleep(5);
    }
    assert_int_equal(g_done, 1);
    assert_int_equal(g_last_status, CMD_STATUS_TIMEOUT);

    /* 第二条已在队里等了约 100ms,还要再过一条 100ms 的超时恢复窗口才上线。
     * 等对端真的收到它的请求字节,再在自己的事务预算内(40ms 后)应答 ——
     * 上线前发出去的字节会被开帧前的 DISCARD_RX 丢掉,那验不到预算。 */
    g_decode_mode = 0;
    int seen_bytes = 0;
    for (int waited = 0; waited < 400 && seen_bytes < 2; waited += 5) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        uint8_t byte;
        if (recv(ctx->peer_fd, &byte, 1, MSG_DONTWAIT) == 1) {
            seen_bytes++;
        } else {
            uv_sleep(5);
        }
    }
    assert_int_equal(seen_bytes, 2); /* 第一条 + 恢复窗口后的第二条 */
    uv_sleep(40);
    uv_run(&ctx->loop, UV_RUN_NOWAIT);
    assert_int_equal(send(ctx->peer_fd, "X", 1, 0), 1);
    for (int waited = 0; waited < 200 && g_done < 2; waited += 5) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        uv_sleep(5);
    }
    assert_int_equal(g_done, 2);
    assert_int_equal(g_last_status, CMD_STATUS_SUCCESS);

    teardown_ctx(ctx);
}

typedef struct {
    int saved_fd;
    FILE *file;
} stdout_capture_t;

static void _capture_stdout(stdout_capture_t *cap) {
    fflush(stdout);
    cap->saved_fd = dup(STDOUT_FILENO);
    assert_true(cap->saved_fd >= 0);
    cap->file = tmpfile();
    assert_non_null(cap->file);
    assert_int_equal(dup2(fileno(cap->file), STDOUT_FILENO), STDOUT_FILENO);
}

static char *_restore_stdout(stdout_capture_t *cap) {
    fflush(stdout);
    assert_int_equal(dup2(cap->saved_fd, STDOUT_FILENO), STDOUT_FILENO);
    close(cap->saved_fd);
    rewind(cap->file);
    char *buf = calloc(1, 8192);
    assert_non_null(buf);
    size_t n = fread(buf, 1, 8191, cap->file);
    buf[n] = '\0';
    fclose(cap->file);
    return buf;
}

static unsigned _count_substr(const char *haystack, const char *needle) {
    unsigned count = 0;
    size_t len = strlen(needle);
    for (const char *p = strstr(haystack, needle); p; p = strstr(p + len, needle)) count++;
    return count;
}

static void test_master_io_missing_log_once_before_attach(void **state) {
    (void)state;
    g_done = 0;

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    arbiter_t *arb = NULL;
    assert_int_equal(arbiter_init(&arb, &loop), 0);
    driver_manager_storage_t mgr_storage;
    driver_manager_t *drv_mgr = driver_manager_create_in(&mgr_storage);
    assert_non_null(drv_mgr);
    net_buf_pool_t pool;
    assert_int_equal(net_buf_pool_init(&pool, 16, 256, 0, NULL, NULL), 0);

    bus_engine_t *eng = engine_create_master(driver_manager_session_port_ptr(drv_mgr), 0,
                                             "TestProto", 1, &loop, arb, NULL, &pool);
    assert_non_null(eng);
    master_priv_t *priv = (master_priv_t *)eng->user_data;
    edge_req_t req = {.ops = &k_fake_ops};

    stdout_capture_t cap;
    _capture_stdout(&cap);
    for (int i = 0; i < 4; i++) eng->run_command(eng, make_cmd(&req, 100));
    char *logs = _restore_stdout(&cap);
    assert_int_equal(_count_substr(logs, "Run command failed - IO/Request NULL"), 1);
    assert_true(priv->io_missing_logged);
    free(logs);
    assert_int_equal(g_done, 4);

    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    io_handle_t *io = io_executor_create_from_fd(&loop, fds[0], &pool);
    assert_non_null(io);
    eng->attach(eng, io);
    assert_false(priv->io_missing_logged);

    io_handle_destroy(io);
    uv_run(&loop, UV_RUN_NOWAIT);
    close(fds[1]);
    eng->detach(eng);
    uv_run(&loop, UV_RUN_NOWAIT);
    arbiter_destroy(arb);
    driver_manager_destroy(drv_mgr);
    net_buf_pool_destroy(&pool);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/* IO_EVENT_RX 载荷所有权:io 层持有到回调返回之后才释放。
 *
 * 引擎回调里同步完成的命令会立刻派发下一条,而那条命令会从同一个 net_buf 池
 * 取 TX 缓冲。若引擎在回调里把 RX 载荷 unref 掉(旧行为),io 层回调返回后的
 * 收尾 unref 就会把回调里新申请到的缓冲一起释放 —— 排队中的 TX 帧内容随后被
 * 后续分配覆盖,同一条请求的字节会出现在多条通道的 socket 上。此处用
 * "on_finish 里持有的一块缓冲" 做哨兵:它必须活过回调返回。 */
static void test_rx_payload_outlives_engine_callback(void **state) {
    (void)state;
    g_done = 0;
    g_done_status = 0;
    g_finish = 0;
    g_last_status = 0;
    g_decode_mode = 0;
    g_held = NULL;
    g_hold_on_finish = 1;

    link_ctx_t *ctx = make_ctx();
    g_hold_pool = &ctx->pool;
    edge_req_t req = {.ops = &k_fake_ops};

    assert_int_equal(bus_orchestrator_submit_cmd(ctx->orch, 0, make_cmd(&req, 2000)), 0);
    pump(&ctx->loop, 30); /* 请求上线,等对端应答 */
    assert_int_equal(send(ctx->peer_fd, "R", 1, 0), 1);
    pump(&ctx->loop, 200);

    assert_int_equal(g_done, 1);
    assert_int_equal(g_done_status, CMD_STATUS_SUCCESS);
    assert_non_null(g_held);

    /* 再申请一块(旧行为下会拿到已被误释放的哨兵)并改写它;哨兵内容必须不变 */
    net_buf_t *probe = net_buf_alloc(&ctx->pool, 0);
    assert_non_null(probe);
    memset(probe->b.data, 0x77, 16);
    for (int i = 0; i < 16; i++) assert_int_equal(g_held->b.data[i], 0x5A);

    net_buf_unref(probe);
    net_buf_unref(g_held);
    g_held = NULL;
    g_hold_pool = NULL;
    teardown_ctx(ctx);
}

int main(void) {
    /* 对端关闭后再写会 EPIPE:不忽略 SIGPIPE 会把测试进程直接打断 */
    signal(SIGPIPE, SIG_IGN);
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_master_survives_peer_close),
        cmocka_unit_test(test_master_survives_repeated_timeout),
        cmocka_unit_test(test_queued_command_gets_full_timeout_on_wire),
        cmocka_unit_test(test_master_settles_queued_commands_on_one_bus),
        cmocka_unit_test(test_master_io_missing_log_once_before_attach),
        cmocka_unit_test(test_rx_payload_outlives_engine_callback),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
