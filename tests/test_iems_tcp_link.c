#define _GNU_SOURCE

#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_datahub_adapter.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "datahub_priv.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "iems_tcp_link.h"
#include "io_executor_linux.h"
#include "modbus_codec.h"

/* 状态机单测：20/20ms 退避下走完 CONNECTED -> BACKOFF -> CONNECTED，
 * 重连后提交一笔 Modbus TCP 读必须成功；另用 prepare 计数证明错误回调后
 * 事件循环没有空转。 */

#define TCP_SLAVE 1
#define TCP_REG 0
#define TCP_RESULT_TAG 1002
#define TCP_REQ_LEN 12
#define TCP_RESP_LEN 11

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    arbiter_t *arb;
    bus_orchestrator_t *orch;
    driver_manager_t *drv_mgr;
    driver_manager_storage_t drv_mgr_storage;

    int listen_fd;
    int conn_fd;
    int accept_count;
    uv_timer_t slave_tick;
    uv_timer_t watchdog;
    uv_prepare_t prepare;

    iems_tcp_link_storage_t link_storage;
    iems_tcp_link_t *link;

    uint8_t req[32];
    size_t req_len;
    bool responded;
    bool frame_ok;
    bool frame_bad;

    bool saw_backoff;
    bool reconnected;
    bool timed_out;
    unsigned iterations;
    unsigned iterations_at_backoff;
    unsigned iterations_at_reconnect;

    int cb_count;
    int cb_status;
    double cb_value;
} link_ctx_t;

static void _on_slave_tick(uv_timer_t *handle) {
    link_ctx_t *c = (link_ctx_t *)handle->data;
    if (c->responded || c->frame_bad) return;

    if (c->conn_fd < 0) {
        int fd = accept(c->listen_fd, NULL, NULL);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EINTR) return;
            c->frame_bad = true;
            return;
        }
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(fd);
            c->frame_bad = true;
            return;
        }
        c->conn_fd = fd;
        c->req_len = 0;
        c->accept_count++;
    }

    ssize_t n = read(c->conn_fd, c->req + c->req_len, sizeof(c->req) - c->req_len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EINTR) return;
        c->frame_bad = true;
        return;
    }
    if (n == 0) return;
    c->req_len += (size_t)n;
    if (c->req_len < TCP_REQ_LEN) return;

    unsigned proto = (unsigned)((c->req[2] << 8) | c->req[3]);
    unsigned len = (unsigned)((c->req[4] << 8) | c->req[5]);
    unsigned addr = (unsigned)((c->req[8] << 8) | c->req[9]);
    unsigned qty = (unsigned)((c->req[10] << 8) | c->req[11]);
    c->frame_ok = proto == 0 && len == 6 && c->req[6] == TCP_SLAVE &&
                  c->req[7] == MODBUS_FC_READ_HOLDING_REGS && addr == TCP_REG && qty == 1;
    if (!c->frame_ok) {
        c->frame_bad = true;
        return;
    }

    uint8_t resp[TCP_RESP_LEN] = {(uint8_t)c->req[0], (uint8_t)c->req[1], 0, 0, 0, 5, TCP_SLAVE,
                                  MODBUS_FC_READ_HOLDING_REGS, 2, 0x12, 0x34};
    if (write(c->conn_fd, resp, sizeof(resp)) == (ssize_t)sizeof(resp)) {
        c->responded = true;
    } else {
        c->frame_bad = true;
    }
}

static void _on_watchdog(uv_timer_t *handle) {
    link_ctx_t *c = (link_ctx_t *)handle->data;
    c->timed_out = true;
    uv_stop(&c->loop);
}

static void _on_read_done(int status, const pipe_msg_t *result, void *user_data) {
    link_ctx_t *c = (link_ctx_t *)user_data;
    c->cb_count++;
    c->cb_status = status;
    if (result) c->cb_value = result->payload.val.f;
    uv_stop(&c->loop);
}

static void _on_prepare(uv_prepare_t *handle) {
    link_ctx_t *c = (link_ctx_t *)handle->data;
    c->iterations++;
    if (c->reconnected) return; /* 计数只服务重连窗口，别让后续 uv_run 立刻停下 */
    iems_tcp_link_state_t state = iems_tcp_link_state(c->link);
    if (state == IEMS_TCP_LINK_BACKOFF) {
        if (!c->saw_backoff) c->iterations_at_backoff = c->iterations;
        c->saw_backoff = true;
    }
    if (c->saw_backoff && state == IEMS_TCP_LINK_CONNECTED && c->accept_count >= 2) {
        c->iterations_at_reconnect = c->iterations;
        c->reconnected = true;
        uv_stop(&c->loop);
    }
}

static void _close_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int setup(void **state) {
    link_ctx_t *c = calloc(1, sizeof(*c));
    assert_non_null(c);
    c->listen_fd = -1;
    c->conn_fd = -1;

    c->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(c->listen_fd >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert_int_equal(bind(c->listen_fd, (struct sockaddr *)&addr, sizeof(addr)), 0);
    assert_int_equal(listen(c->listen_fd, 4), 0);
    socklen_t addr_len = sizeof(addr);
    assert_int_equal(getsockname(c->listen_fd, (struct sockaddr *)&addr, &addr_len), 0);
    int flags = fcntl(c->listen_fd, F_GETFL, 0);
    assert_true(flags >= 0);
    assert_int_equal(fcntl(c->listen_fd, F_SETFL, flags | O_NONBLOCK), 0);

    assert_int_equal(uv_loop_init(&c->loop), 0);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_tcp_link_%d", (int)getpid());
    assert_int_equal(datahub_init(&c->hub, &c->loop, shm_name, 16, 32, NULL, 0, NULL, NULL, NULL),
                     0);

    assert_int_equal(arbiter_init(&c->arb, &c->loop), 0);
    arbiter_resume(c->arb);

    c->drv_mgr = driver_manager_create_in(&c->drv_mgr_storage);
    assert_non_null(c->drv_mgr);
    assert_int_equal(modbus_tcp_driver_register(c->drv_mgr), 0);

    c->orch = bus_orchestrator_create_hub_default(
        &c->hub, c->arb, driver_manager_session_port_ptr(c->drv_mgr), 1, 8, 256);
    assert_non_null(c->orch);
    datahub_bind_io_pool(&c->hub, bus_orchestrator_get_pool(c->orch));
    edge_api_init(c->orch);
    assert_int_equal(bus_orchestrator_setup_master(c->orch, 0, "ModbusTCP", 0), 0);

    assert_int_equal(uv_timer_init(&c->loop, &c->slave_tick), 0);
    c->slave_tick.data = c;
    assert_int_equal(uv_timer_start(&c->slave_tick, _on_slave_tick, 1, 1), 0);

    assert_int_equal(uv_timer_init(&c->loop, &c->watchdog), 0);
    c->watchdog.data = c;

    assert_int_equal(uv_prepare_init(&c->loop, &c->prepare), 0);
    c->prepare.data = c;
    assert_int_equal(uv_prepare_start(&c->prepare, _on_prepare), 0);

    iems_tcp_link_config_t cfg = {.backoff_base_ms = 20,
                                  .backoff_cap_ms = 20,
                                  .open_io = io_executor_create_tcp,
                                  .io_pool = datahub_get_io_pool(&c->hub)};
    c->link = iems_tcp_link_create_in(&c->link_storage, &c->loop, c->orch, 0, "127.0.0.1",
                                      ntohs(addr.sin_port), &cfg);
    assert_non_null(c->link);
    assert_int_equal(iems_tcp_link_state(c->link), IEMS_TCP_LINK_CONNECTED);

    *state = c;
    return 0;
}

static int teardown(void **state) {
    link_ctx_t *c = (link_ctx_t *)*state;
    if (!c) return 0;

    uv_timer_stop(&c->slave_tick);
    uv_timer_stop(&c->watchdog);
    uv_prepare_stop(&c->prepare);

    if (c->cb_count == 0 && c->orch) {
        /* 用例提前停表时可能还有在途命令：先让引擎自己的超时结算再拆 IO */
        c->timed_out = false;
        uv_timer_start(&c->watchdog, _on_watchdog, 3000, 0);
        uv_run(&c->loop, UV_RUN_DEFAULT);
        uv_timer_stop(&c->watchdog);
    }

    iems_tcp_link_close(c->link);
    c->link = NULL;
    if (c->orch) bus_orchestrator_destroy(c->orch);
    if (c->arb) arbiter_destroy(c->arb);
    if (c->drv_mgr) driver_manager_destroy(c->drv_mgr);

    uv_walk(&c->loop, _close_handle, NULL);
    for (int i = 0; i < 200 && uv_loop_alive(&c->loop); i++) uv_run(&c->loop, UV_RUN_NOWAIT);

    datahub_destroy(&c->hub);
    if (c->conn_fd >= 0) close(c->conn_fd);
    if (c->listen_fd >= 0) close(c->listen_fd);

    int rc = uv_loop_close(&c->loop);
    if (rc != 0) fprintf(stderr, "uv_loop_close failed: %s\n", uv_strerror(rc));
    assert_int_equal(rc, 0);
    free(c);
    return 0;
}

static void test_tcp_link_reconnects_and_reads(void **state) {
    link_ctx_t *c = (link_ctx_t *)*state;

    /* 等对端接住第一次连接 */
    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 1000, 0), 0);
    while (!c->timed_out && c->accept_count < 1) uv_run(&c->loop, UV_RUN_ONCE);
    assert_false(c->timed_out);
    assert_int_equal(c->accept_count, 1);
    uv_timer_stop(&c->watchdog);

    /* 服务端断开：link 必须先进 BACKOFF，再在退避上限内重新 CONNECTED */
    assert_int_equal(close(c->conn_fd), 0);
    c->conn_fd = -1;
    c->req_len = 0;
    c->responded = false;
    c->frame_ok = false;

    c->timed_out = false;
    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 2000, 0), 0);
    uv_run(&c->loop, UV_RUN_DEFAULT);
    assert_false(c->timed_out);
    assert_true(c->saw_backoff);
    assert_true(c->reconnected);
    assert_int_equal(c->accept_count, 2);
    assert_int_equal(iems_tcp_link_state(c->link), IEMS_TCP_LINK_CONNECTED);

    /* 错误回调后不得空转：BACKOFF 窗口内的事件循环轮次必须是有限小值 */
    unsigned spin = c->iterations_at_reconnect - c->iterations_at_backoff;
    assert_true(spin < 5000);

    /* 重连后提交一笔读，必须走新句柄成功 */
    assert_int_equal(edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 1),
                                          TCP_RESULT_TAG, 0.1, NULL, 0, ARBITER_PRIO_NORMAL,
                                          _on_read_done, c),
                     0);
    c->timed_out = false;
    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 2000, 0), 0);
    uv_run(&c->loop, UV_RUN_DEFAULT);
    assert_false(c->timed_out);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_true(c->frame_ok);
    assert_true(fabs(c->cb_value - 466.0) < 1e-9);

    /* close 幂等，关闭后状态固定为 DISCONNECTED */
    iems_tcp_link_close(c->link);
    iems_tcp_link_close(c->link);
    assert_int_equal(iems_tcp_link_state(c->link), IEMS_TCP_LINK_DISCONNECTED);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_tcp_link_reconnects_and_reads, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
