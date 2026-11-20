#define _GNU_SOURCE

#include <cmocka.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <uv.h>

#include "io_executor.h"
#include "io_executor_linux.h"
#include "io_handle_priv.h"

/**
 * 验证物理通道创建 (Linux 适配层)
 */
static void test_io_create_uart(void** state) {
    (void)state;
    uv_loop_t* loop = uv_default_loop();

    // 我们无法在没有物理串口的情况下打开成功，但我们可以验证工厂函数的行为
    io_handle_t* h = io_executor_create_uart(loop, "/dev/non_existent_tty", 9600, NULL);
    assert_null(h);
}

typedef struct {
    uv_loop_t* loop;
    uint8_t rx[16];
    size_t rx_len;
    unsigned rx_count;
    unsigned tx_done;
    bool error;
} io_test_ctx_t;

static void on_io_event(io_handle_t* h, io_event_t evt, void* payload, void* user_data) {
    (void)h;
    io_test_ctx_t* ctx = (io_test_ctx_t*)user_data;

    if (evt == IO_EVENT_RX) {
        net_buf_t* nb = (net_buf_t*)payload;
        if (ctx->rx_len + nb->b.len <= sizeof(ctx->rx)) {
            memcpy(ctx->rx + ctx->rx_len, nb->b.data, nb->b.len);
            ctx->rx_len += nb->b.len;
        }
        ctx->rx_count++;
        uv_stop(ctx->loop);
    } else if (evt == IO_EVENT_TX_DONE) {
        ctx->tx_done++;
        uv_stop(ctx->loop);
    } else if (evt == IO_EVENT_ERROR) {
        ctx->error = true;
        uv_stop(ctx->loop);
    }
}

static void on_watchdog(uv_timer_t* timer) {
    uv_stop(timer->loop);
}

/* 有界跑 loop：事件到达时回调里 stop，否则 watchdog 到点 stop。 */
static void run_loop_bounded(uv_loop_t* loop, uint64_t timeout_ms) {
    uv_timer_t watchdog;
    assert_int_equal(uv_timer_init(loop, &watchdog), 0);
    assert_int_equal(uv_timer_start(&watchdog, on_watchdog, timeout_ms, 0), 0);

    uv_run(loop, UV_RUN_DEFAULT);

    uv_timer_stop(&watchdog);
    uv_close((uv_handle_t*)&watchdog, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

/**
 * PTY 回归：创建时必须应用 raw 模式与波特率，RX 与 TX 都必须走字符设备路径。
 * 这两个断言对应用户在真实串口上看到的两个缺陷：canonical 模式吞数据、
 * recv() 对 tty 返回 ENOTSOCK 导致永不收数。
 */
static void test_io_uart_pty(void** state) {
    (void)state;

    int master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(master >= 0);
    assert_int_equal(grantpt(master), 0);
    assert_int_equal(unlockpt(master), 0);
    const char* slave_name = ptsname(master);
    assert_non_null(slave_name);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    net_buf_pool_t* pool = net_buf_pool_new(32, 512, 0);
    assert_non_null(pool);

    io_handle_t* h = io_executor_create_uart(&loop, slave_name, 115200, pool);
    assert_non_null(h);

    int check_fd = open(slave_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(check_fd >= 0);
    struct termios tio;
    assert_int_equal(tcgetattr(check_fd, &tio), 0);
    assert_int_equal(tio.c_lflag & ICANON, 0);
    assert_int_equal(tio.c_lflag & ECHO, 0);
    assert_int_equal(cfgetospeed(&tio), B115200);
    close(check_fd);

    io_test_ctx_t ctx = {.loop = &loop};
    h->cb = on_io_event;
    h->user_data = &ctx;
    assert_int_equal(h->ops->enable_read(h, true), 0);

    const uint8_t rx_expect[] = {0x01, 0x03, 0x00, 0xAB};
    assert_int_equal(write(master, rx_expect, sizeof(rx_expect)), (ssize_t)sizeof(rx_expect));

    run_loop_bounded(&loop, 1000);
    assert_int_equal(ctx.rx_count, 1);
    assert_int_equal(ctx.rx_len, sizeof(rx_expect));
    assert_memory_equal(ctx.rx, rx_expect, sizeof(rx_expect));
    assert_false(ctx.error);

    net_buf_t* tx = net_buf_alloc(pool, 0);
    assert_non_null(tx);
    const uint8_t tx_expect[] = {0x10, 0x20, 0x30};
    memcpy(tx->b.data, tx_expect, sizeof(tx_expect));
    tx->b.len = sizeof(tx_expect);
    assert_int_equal(h->ops->write_async(h, tx), 0);

    run_loop_bounded(&loop, 1000);
    assert_int_equal(ctx.tx_done, 1);

    uint8_t tx_got[8];
    ssize_t n = read(master, tx_got, sizeof(tx_got));
    assert_int_equal(n, (ssize_t)sizeof(tx_expect));
    assert_memory_equal(tx_got, tx_expect, sizeof(tx_expect));

    net_buf_unref(tx);

    h->ops->destroy(h);
    uv_run(&loop, UV_RUN_DEFAULT);
    close(master);
    net_buf_pool_destroy(pool);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/**
 * 不支持的波特率必须让创建失败: 静默退回 9600 会把 wire speed 改错。
 */
static void test_io_uart_unsupported_baud(void** state) {
    (void)state;

    int master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(master >= 0);
    assert_int_equal(grantpt(master), 0);
    assert_int_equal(unlockpt(master), 0);
    const char* slave_name = ptsname(master);
    assert_non_null(slave_name);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    net_buf_pool_t* pool = net_buf_pool_new(32, 512, 0);
    assert_non_null(pool);

    assert_null(io_executor_create_uart(&loop, slave_name, 1234, pool));

    io_handle_t* h = io_executor_create_uart(&loop, slave_name, 9600, pool);
    assert_non_null(h);
    int check_fd = open(slave_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(check_fd >= 0);
    struct termios tio;
    assert_int_equal(tcgetattr(check_fd, &tio), 0);
    assert_int_equal(cfgetospeed(&tio), B9600);
    close(check_fd);

    h->ops->destroy(h);
    uv_run(&loop, UV_RUN_DEFAULT);
    close(master);
    net_buf_pool_destroy(pool);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/**
 * TCP 回环：建监听、从 127.0.0.1 连入，验证 RX 走 recv、TX 走 write。
 */
static void test_io_tcp_loopback(void** state) {
    (void)state;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(listen_fd >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert_int_equal(bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)), 0);
    assert_int_equal(listen(listen_fd, 1), 0);
    socklen_t addr_len = sizeof(addr);
    assert_int_equal(getsockname(listen_fd, (struct sockaddr*)&addr, &addr_len), 0);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    net_buf_pool_t* pool = net_buf_pool_new(32, 512, 0);
    assert_non_null(pool);

    io_handle_t* h = io_executor_create_tcp(&loop, "127.0.0.1", ntohs(addr.sin_port), pool);
    assert_non_null(h);

    int client_fd = accept(listen_fd, NULL, NULL);
    assert_true(client_fd >= 0);
    close(listen_fd);

    io_test_ctx_t ctx = {.loop = &loop};
    h->cb = on_io_event;
    h->user_data = &ctx;
    assert_int_equal(h->ops->enable_read(h, true), 0);

    const uint8_t rx_expect[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01};
    assert_int_equal(write(client_fd, rx_expect, sizeof(rx_expect)), (ssize_t)sizeof(rx_expect));

    run_loop_bounded(&loop, 1000);
    assert_int_equal(ctx.rx_count, 1);
    assert_int_equal(ctx.rx_len, sizeof(rx_expect));
    assert_memory_equal(ctx.rx, rx_expect, sizeof(rx_expect));
    assert_false(ctx.error);

    net_buf_t* tx = net_buf_alloc(pool, 0);
    assert_non_null(tx);
    const uint8_t tx_expect[] = {0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x12, 0x34};
    memcpy(tx->b.data, tx_expect, sizeof(tx_expect));
    tx->b.len = sizeof(tx_expect);
    assert_int_equal(h->ops->write_async(h, tx), 0);

    run_loop_bounded(&loop, 1000);
    assert_int_equal(ctx.tx_done, 1);

    uint8_t tx_got[16];
    ssize_t n = read(client_fd, tx_got, sizeof(tx_got));
    assert_int_equal(n, (ssize_t)sizeof(tx_expect));
    assert_memory_equal(tx_got, tx_expect, sizeof(tx_expect));

    net_buf_unref(tx);

    h->ops->destroy(h);
    uv_run(&loop, UV_RUN_DEFAULT);
    close(client_fd);
    net_buf_pool_destroy(pool);
    assert_int_equal(uv_loop_close(&loop), 0);
}

/* 链路事件端口：正常 RX 不通知 link；对端关闭时先 base.cb（引擎结算）再
 * link_cb，且 watcher 已停，不会每轮循环重复报错。 */
typedef struct {
    uv_loop_t* loop;
    int base_errors;
    int link_errors;
    unsigned first_error; /* bit1 = base 先到，bit2 = link 在其后 */
} link_probe_t;

static void on_probe_io(io_handle_t* h, io_event_t evt, void* payload, void* user_data) {
    (void)h;
    (void)payload;
    link_probe_t* p = (link_probe_t*)user_data;
    if (evt == IO_EVENT_ERROR) {
        p->base_errors++;
        if (p->first_error == 0) p->first_error |= 1;
    }
    if (evt == IO_EVENT_ERROR || evt == IO_EVENT_RX) uv_stop(p->loop);
}

static void on_probe_link(io_handle_t* h, io_event_t evt, void* user_data) {
    (void)h;
    link_probe_t* p = (link_probe_t*)user_data;
    if (evt != IO_EVENT_ERROR) return;
    p->link_errors++;
    if (p->first_error == 1) p->first_error |= 2;
    uv_stop(p->loop);
}

static void test_io_tcp_link_cb_reports_error_after_data_cb(void** state) {
    (void)state;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(listen_fd >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert_int_equal(bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)), 0);
    assert_int_equal(listen(listen_fd, 1), 0);
    socklen_t addr_len = sizeof(addr);
    assert_int_equal(getsockname(listen_fd, (struct sockaddr*)&addr, &addr_len), 0);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    net_buf_pool_t* pool = net_buf_pool_new(32, 512, 0);
    assert_non_null(pool);

    io_handle_t* h = io_executor_create_tcp(&loop, "127.0.0.1", ntohs(addr.sin_port), pool);
    assert_non_null(h);

    int client_fd = accept(listen_fd, NULL, NULL);
    assert_true(client_fd >= 0);
    close(listen_fd);

    link_probe_t p = {.loop = &loop};
    h->cb = on_probe_io;
    h->user_data = &p;
    io_handle_set_link_cb(h, on_probe_link, &p);
    assert_int_equal(h->ops->enable_read(h, true), 0);

    const uint8_t rx_expect[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01};
    assert_int_equal(write(client_fd, rx_expect, sizeof(rx_expect)), (ssize_t)sizeof(rx_expect));
    run_loop_bounded(&loop, 1000);
    assert_int_equal(p.base_errors, 0);
    assert_int_equal(p.link_errors, 0);

    close(client_fd);
    run_loop_bounded(&loop, 1000);
    assert_int_equal(p.base_errors, 1);
    assert_int_equal(p.link_errors, 1);
    assert_int_equal(p.first_error, 3); /* base 先于 link */

    /* 错误后 watcher 必须已停：再跑 100ms 不得重复回调 */
    run_loop_bounded(&loop, 100);
    assert_int_equal(p.base_errors, 1);
    assert_int_equal(p.link_errors, 1);

    h->ops->destroy(h);
    uv_run(&loop, UV_RUN_DEFAULT);
    net_buf_pool_destroy(pool);
    assert_int_equal(uv_loop_close(&loop), 0);
}

static void test_io_tcp_refused(void** state) {
    (void)state;

    int probe = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(probe >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert_int_equal(bind(probe, (struct sockaddr*)&addr, sizeof(addr)), 0);
    socklen_t len = sizeof(addr);
    assert_int_equal(getsockname(probe, (struct sockaddr*)&addr, &len), 0);
    close(probe);

    uv_loop_t* loop = uv_default_loop();
    assert_null(io_executor_create_tcp(loop, "127.0.0.1", ntohs(addr.sin_port), NULL));
}

/* 硬写错误没有 TX_DONE:引擎不会结算这一帧,pending_tx 必须在错误路径里还回池子。
 * 用 socketpair + 本地 SHUT_WR 逼出确定的 EPIPE;对端关闭不可靠 —— 第一次 write
 * 往往进内核缓冲成功,而 EOF 可读事件会先一步抢走错误路径。 */
static void test_io_write_hard_error_releases_pending_tx(void** state) {
    (void)state;

    void (*prev_sigpipe)(int) = signal(SIGPIPE, SIG_IGN);

    int sv[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
    int flags = fcntl(sv[0], F_GETFL, 0);
    assert_true(flags >= 0);
    assert_int_equal(fcntl(sv[0], F_SETFL, flags | O_NONBLOCK), 0);
    assert_int_equal(shutdown(sv[0], SHUT_WR), 0);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    net_buf_pool_t* pool = net_buf_pool_new(8, 256, 0);
    assert_non_null(pool);

    io_handle_t* h = io_executor_create_from_fd(&loop, sv[0], pool);
    assert_non_null(h);

    io_test_ctx_t ctx = {.loop = &loop};
    h->cb = on_io_event;
    h->user_data = &ctx;

    net_buf_pool_stats_t stats;
    net_buf_pool_audit(pool, 60000, &stats);
    assert_int_equal(stats.free_count, 8);

    net_buf_t* tx = net_buf_alloc(pool, 0);
    assert_non_null(tx);
    tx->b.data[0] = 0x42;
    tx->b.len = 1;
    assert_int_equal(h->ops->write_async(h, tx), 0);
    net_buf_unref(tx); /* 句柄持有 pending 的那份引用 */
    net_buf_pool_audit(pool, 60000, &stats);
    assert_int_equal(stats.free_count, 7);

    run_loop_bounded(&loop, 1000);
    assert_true(ctx.error);

    net_buf_pool_audit(pool, 60000, &stats);
    assert_int_equal(stats.free_count, 8); /* destroy 之前就已回到基线 */

    h->ops->destroy(h);
    uv_run(&loop, UV_RUN_DEFAULT);
    close(sv[1]);
    net_buf_pool_destroy(pool);
    assert_int_equal(uv_loop_close(&loop), 0);

    signal(SIGPIPE, prev_sigpipe);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_create_uart),
        cmocka_unit_test(test_io_uart_pty),
        cmocka_unit_test(test_io_uart_unsupported_baud),
        cmocka_unit_test(test_io_tcp_loopback),
        cmocka_unit_test(test_io_tcp_link_cb_reports_error_after_data_cb),
        cmocka_unit_test(test_io_tcp_refused),
        cmocka_unit_test(test_io_write_hard_error_releases_pending_tx),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
