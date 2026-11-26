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
#include "iems_southbound_adapter.h"
#include "io_executor_linux.h"
#include "io_handle_priv.h"
#include "modbus_codec.h"

/* 第二条真实设备协议路径：南向 TCP client 主动连本机监听端，Modbus TCP 主站
 * 读请求必须真的出现在这条连接上，11 字节 MBAP 响应沿同一连接回去。
 * UDS 模拟与纯函数单测都覆盖不到启动选路、TCP 连接与请求层的组合。 */

#define TCP_REQ_LEN 12
#define TCP_RESP_LEN 11
#define TCP_SLAVE 1
#define TCP_REG 0
#define TCP_RAW 0x1234
#define TCP_SCALE 0.1
#define TCP_RESULT_TAG 1002

/* RTU over TCP：同一 TCP 链路上走无 MBAP 的 RTU 帧，v3 在 channel 含 ':' 时这样用 */
#define RTU_TCP_REQ_LEN 8
#define RTU_TCP_RESP_LEN 7
#define RTU_TCP_RESULT_TAG 1003

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
    uv_timer_t reconnect_tick;

    iems_bus_wiring_t wiring;
    bool second_sent;
    int second_rc;

    uint8_t req[64];
    size_t req_len;
    int req_frames;
    uint8_t fc_seen;
    bool frame_ok;
    bool write_ok;
    bool rtu_frames;
    bool split_resp;           /* 读响应拆成两次 tick 写，模拟 TCP 拆包 */
    uint8_t resp_tail[16];     /* 第二次 tick 补发的剩余字节 */
    size_t resp_tail_len;
    unsigned ack_addr;         /* FC16 应答回显的地址/数量，测试可改成不匹配 */
    unsigned ack_qty;
    unsigned read_count;
    uint16_t read_regs[2];
    uint16_t write_regs[4];    /* FC16 合法负载的寄存器序列 */
    unsigned write_n;
    bool frame_bad;
    bool responded;
    bool timed_out;

    int cb_count;
    int cb_status;
    uint32_t cb_tag;
    double cb_value;
} tcp_ctx_t;

static void _dump_frame(const tcp_ctx_t *c, const char *why) {
    fprintf(stderr,
            "[modbus-tcp-loopback] %s: frames=%d responded=%d timed_out=%d frame_ok=%d rx_len=%zu\n",
            why, c->req_frames, c->responded, c->timed_out, c->frame_ok, c->req_len);
    fprintf(stderr, "[modbus-tcp-loopback] rx:");
    for (size_t i = 0; i < c->req_len; i++) fprintf(stderr, " %02X", c->req[i]);
    fprintf(stderr, "\n");
    if (c->req_len >= TCP_REQ_LEN) {
        unsigned tid = (unsigned)((c->req[0] << 8) | c->req[1]);
        unsigned proto = (unsigned)((c->req[2] << 8) | c->req[3]);
        unsigned len = (unsigned)((c->req[4] << 8) | c->req[5]);
        unsigned addr = (unsigned)((c->req[8] << 8) | c->req[9]);
        unsigned qty = (unsigned)((c->req[10] << 8) | c->req[11]);
        fprintf(stderr,
                "[modbus-tcp-loopback] tid=0x%04X proto=%u len=%u unit=%u fc=0x%02X addr=%u "
                "qty=%u\n",
                tid, proto, len, c->req[6], c->req[7], addr, qty);
    }
}

/* 本测试同时扮演从站。监听 socket 非阻塞，accept 与收发都放在 1ms tick 里，
 * 不阻塞事件循环。坏请求不回，让驱动自己的 2000ms 超时结算。 */
static void _on_slave_tick(uv_timer_t *handle) {
    tcp_ctx_t *c = (tcp_ctx_t *)handle->data;
    if (c->responded || c->frame_bad) return;

    /* 拆包应答的后半段：下一 tick 补发，中间让主站先收到一次不完整帧 */
    if (c->resp_tail_len > 0) {
        ssize_t n = write(c->conn_fd, c->resp_tail, c->resp_tail_len);
        if (n != (ssize_t)c->resp_tail_len) {
            c->frame_bad = true;
            _dump_frame(c, "split response tail write failed");
            return;
        }
        c->resp_tail_len = 0;
        c->responded = true;
        return;
    }

    if (c->conn_fd < 0) {
        int fd = accept(c->listen_fd, NULL, NULL);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EINTR) return;
            c->frame_bad = true;
            _dump_frame(c, "accept failed");
            return;
        }
        c->accept_count++;
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(fd);
            c->frame_bad = true;
            _dump_frame(c, "fcntl accepted socket failed");
            return;
        }
        c->conn_fd = fd;
    }

    ssize_t n = read(c->conn_fd, c->req + c->req_len, sizeof(c->req) - c->req_len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EINTR) return;
        c->frame_bad = true;
        _dump_frame(c, "read failed");
        return;
    }
    if (n == 0) return;
    c->req_len += (size_t)n;

    if (c->rtu_frames) {
        if (c->req_len < RTU_TCP_REQ_LEN) return;
        c->req_frames++;
        uint16_t crc = modbus_calculate_crc(c->req, 6);
        c->fc_seen = c->req[1];
        c->frame_ok = c->req[0] == TCP_SLAVE && c->req[1] == MODBUS_FC_READ_HOLDING_REGS &&
                      c->req[2] == 0 && c->req[3] == TCP_REG && c->req[4] == 0 && c->req[5] == 1 &&
                      (uint16_t)(c->req[6] | (c->req[7] << 8)) == crc;
        if (!c->frame_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected rtu frame");
            return;
        }
        uint8_t resp[RTU_TCP_RESP_LEN] = {TCP_SLAVE, MODBUS_FC_READ_HOLDING_REGS, 2,
                                          (uint8_t)(TCP_RAW >> 8), (uint8_t)(TCP_RAW & 0xFF), 0, 0};
        uint16_t rcrc = modbus_calculate_crc(resp, 5);
        resp[5] = (uint8_t)(rcrc & 0xFF);
        resp[6] = (uint8_t)(rcrc >> 8);
        if (write(c->conn_fd, resp, sizeof(resp)) == (ssize_t)sizeof(resp)) {
            c->responded = true;
        } else {
            c->frame_bad = true;
            _dump_frame(c, "rtu response write failed");
        }
        return;
    }

    /* MBAP 前 7 字节够读出 length；length 只数 unit id + PDU，帧长 = 6 + length */
    if (c->req_len < 8) return;
    unsigned proto = (unsigned)((c->req[2] << 8) | c->req[3]);
    unsigned len = (unsigned)((c->req[4] << 8) | c->req[5]);
    if (len < 6 || c->req_len < 6 + len) return;

    c->req_frames++;
    unsigned addr = (unsigned)((c->req[8] << 8) | c->req[9]);
    unsigned qty = (unsigned)((c->req[10] << 8) | c->req[11]);
    uint8_t fc = c->req[7];
    c->fc_seen = fc;
    c->frame_ok = proto == MODBUS_TCP_PROTOCOL_ID &&
                  len == (fc == MODBUS_FC_WRITE_MULTIPLE_REGS ? 7u + 2u * qty : 6u) &&
                  c->req[6] == TCP_SLAVE &&
                  (fc == MODBUS_FC_READ_HOLDING_REGS || fc == MODBUS_FC_WRITE_SINGLE_REG ||
                   fc == MODBUS_FC_WRITE_MULTIPLE_REGS) &&
                  addr == TCP_REG;
    if (!c->frame_ok) {
        c->frame_bad = true;
        _dump_frame(c, "unexpected request frame");
        return;
    }

    if (fc == MODBUS_FC_WRITE_SINGLE_REG) {
        /* FC06 应答就是原帧回显；写值对不上说明请求层没把值传下来 */
        c->write_ok = qty == (TCP_RAW & 0xFFFF);
        if (!c->write_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected write payload");
            return;
        }
        if (write(c->conn_fd, c->req, TCP_REQ_LEN) == (ssize_t)TCP_REQ_LEN) {
            c->responded = true;
        } else {
            c->frame_bad = true;
            _dump_frame(c, "write echo failed");
        }
        return;
    }

    if (fc == MODBUS_FC_WRITE_MULTIPLE_REGS) {
        /* FC16 写 N 个寄存器：count=N、byte count=2N，负载按描述编码 */
        c->write_ok = qty == c->write_n && c->req[12] == (uint8_t)(2 * qty);
        for (unsigned i = 0; c->write_ok && i < qty; i++) {
            uint16_t got = (uint16_t)((c->req[13 + 2 * i] << 8) | c->req[14 + 2 * i]);
            if (got != c->write_regs[i]) c->write_ok = false;
        }
        if (!c->write_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected write payload");
            return;
        }
        uint8_t ack[12] = {c->req[0],
                           c->req[1],
                           0,
                           0,
                           0,
                           6,
                           TCP_SLAVE,
                           MODBUS_FC_WRITE_MULTIPLE_REGS,
                           (uint8_t)(c->ack_addr >> 8),
                           (uint8_t)(c->ack_addr & 0xFF),
                           (uint8_t)(c->ack_qty >> 8),
                           (uint8_t)(c->ack_qty & 0xFF)};
        if (write(c->conn_fd, ack, sizeof(ack)) == (ssize_t)sizeof(ack)) {
            c->responded = true;
        } else {
            c->frame_bad = true;
            _dump_frame(c, "write32 ack failed");
        }
        return;
    }

    if (qty != c->read_count) {
        c->frame_bad = true;
        _dump_frame(c, "unexpected read quantity");
        return;
    }

    /* 回显请求的事务号，从站只认这一帧的 MBAP */
    uint8_t resp[16] = {(uint8_t)c->req[0],
                        (uint8_t)c->req[1],
                        0,
                        0,
                        0,
                        (uint8_t)(3 + 2 * qty),
                        (uint8_t)TCP_SLAVE,
                        MODBUS_FC_READ_HOLDING_REGS,
                        (uint8_t)(2 * qty)};
    size_t resp_len = 9;
    for (unsigned i = 0; i < qty; i++) {
        resp[resp_len++] = (uint8_t)(c->read_regs[i] >> 8);
        resp[resp_len++] = (uint8_t)(c->read_regs[i] & 0xFF);
    }
    if (c->split_resp) {
        /* 先发前 5 字节（不足一个 MBAP 头），余下下一 tick 补发 */
        memcpy(c->resp_tail, resp + 5, resp_len - 5);
        c->resp_tail_len = resp_len - 5;
        if (write(c->conn_fd, resp, 5) != 5) {
            c->frame_bad = true;
            _dump_frame(c, "split response head write failed");
        }
        return;
    }
    if (write(c->conn_fd, resp, resp_len) == (ssize_t)resp_len) {
        c->responded = true;
    } else {
        c->frame_bad = true;
        _dump_frame(c, "response write failed");
    }
}

static void _on_read_done(int status, const pipe_msg_t *result, void *user_data) {
    tcp_ctx_t *c = (tcp_ctx_t *)user_data;
    c->cb_count++;
    c->cb_status = status;
    if (result) {
        c->cb_tag = (uint32_t)result->tag_id;
        c->cb_value = result->payload.val.f;
    }
    uv_stop(&c->loop);
}

/* 驱动自己的事务超时是 2000ms，看门狗必须晚于它：失败路径先让命令结算，
 * 拆 IO 与 arbiter 时引擎超时回调才不会碰到已释放对象。 */
static void _on_watchdog(uv_timer_t *handle) {
    tcp_ctx_t *c = (tcp_ctx_t *)handle->data;
    c->timed_out = true;
    uv_stop(&c->loop);
}

/* 服务端主动断开：conn_fd 复位让 tick 重新 accept，帧累积从 0 开始。 */
static void _slave_drop(tcp_ctx_t *c) {
    if (c->conn_fd >= 0) {
        close(c->conn_fd);
        c->conn_fd = -1;
    }
    c->req_len = 0;
    c->req_frames = 0;
    c->responded = false;
    c->frame_ok = false;
    c->frame_bad = false;
}

/* 重连完成后提交第二笔读；提交失败直接停表，让用例给出明确断言。 */
static void _on_reconnect_tick(uv_timer_t *handle) {
    tcp_ctx_t *c = (tcp_ctx_t *)handle->data;
    if (c->second_sent) return;
    if (c->accept_count < 2 || c->wiring.state[0] != BUS_STATE_CONNECTED) return;
    c->second_sent = true;
    c->second_rc = edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, 0,
                                        MODBUS_NATIVE(TCP_REG, 1), TCP_RESULT_TAG, TCP_SCALE, NULL,
                                        0, ARBITER_PRIO_NORMAL, _on_read_done, c);
    if (c->second_rc != 0) uv_stop(&c->loop);
}

static int setup_common(void **state, const char *protocol, bool rtu_frames) {
    tcp_ctx_t *c = calloc(1, sizeof(*c));
    assert_non_null(c);
    c->listen_fd = -1;
    c->conn_fd = -1;
    c->rtu_frames = rtu_frames;
    c->read_count = 1;
    c->read_regs[0] = TCP_RAW;
    c->ack_addr = TCP_REG;
    c->ack_qty = 2;
    c->write_n = 2;
    c->write_regs[0] = 0x1234;
    c->write_regs[1] = 0x5678;

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
    snprintf(shm_name, sizeof(shm_name), "iems_mb_tcp_loop_%d", (int)getpid());
    assert_int_equal(datahub_init(&c->hub, &c->loop, shm_name, 16, 32, NULL, 0, NULL, NULL, NULL),
                     0);

    assert_int_equal(arbiter_init(&c->arb, &c->loop), 0);
    /* arbiter 默认暂停，不 resume 命令永远留在队列里 */
    arbiter_resume(c->arb);

    c->drv_mgr = driver_manager_create_in(&c->drv_mgr_storage);
    assert_non_null(c->drv_mgr);
    assert_int_equal(modbus_rtu_driver_register(c->drv_mgr), 0);
    assert_int_equal(modbus_tcp_driver_register(c->drv_mgr), 0);

    c->orch = bus_orchestrator_create_hub_default(
        &c->hub, c->arb, driver_manager_session_port_ptr(c->drv_mgr), 2, 8, 256);
    assert_non_null(c->orch);
    /* 与 iems_app 一致：IO 池由 orchestrator 建，再交给 DataHub 的 TCP 工厂用 */
    datahub_bind_io_pool(&c->hub, bus_orchestrator_get_pool(c->orch));
    edge_api_init(c->orch);

    assert_int_equal(bus_orchestrator_setup_master(c->orch, 0, protocol, 0), 0);

    char dev[64];
    snprintf(dev, sizeof(dev), "tcp://127.0.0.1:%u", (unsigned)ntohs(addr.sin_port));
    assert_int_equal(setenv("OMNIS_BUS_0_DEV", dev, 1), 0);

    int configured = 0;
    assert_int_equal(uv_timer_init(&c->loop, &c->watchdog), 0);
    c->watchdog.data = c;
    /* 重连用例要求退避在 20/40ms 内完成，否则默认 1s 起步会让用例变慢 */
    iems_tcp_link_config_t link_cfg = {.backoff_base_ms = 20, .backoff_cap_ms = 40};
    iems_southbound_connect_in(&c->loop, &c->hub, c->orch, c->wiring.state, c->wiring.link,
                               &link_cfg, &configured);
    unsetenv("OMNIS_BUS_0_DEV");

    assert_true(configured >= 1);
    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 3000, 0), 0);
    while (!c->timed_out && c->wiring.state[0] == BUS_STATE_CONNECTING) {
        uv_run(&c->loop, UV_RUN_ONCE);
    }
    uv_timer_stop(&c->watchdog);
    c->timed_out = false;
    assert_int_equal(c->wiring.state[0], BUS_STATE_CONNECTED);

    assert_int_equal(uv_timer_init(&c->loop, &c->slave_tick), 0);
    c->slave_tick.data = c;
    assert_int_equal(uv_timer_start(&c->slave_tick, _on_slave_tick, 1, 1), 0);

    assert_int_equal(uv_timer_init(&c->loop, &c->reconnect_tick), 0);
    c->reconnect_tick.data = c;

    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 3000, 0), 0);

    *state = c;
    return 0;
}

static int setup(void **state) { return setup_common(state, "ModbusTCP", false); }

static int setup_rtu_over_tcp(void **state) { return setup_common(state, "ModbusRTU", true); }

static void test_modbus_tcp_read_over_loopback(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;

    assert_int_equal(
        edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, 0, MODBUS_NATIVE(TCP_REG, 1), TCP_RESULT_TAG,
                             TCP_SCALE, NULL, 0, ARBITER_PRIO_NORMAL, _on_read_done, c),
        0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->frame_ok)
        _dump_frame(c, "request did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->frame_ok);
    assert_int_equal(c->req_len, TCP_REQ_LEN);
    /* 事务号由驱动自增，只比 MBAP 其余字段与 PDU */
    static const uint8_t expect_req[10] = {0, 0,       0, 6, TCP_SLAVE, MODBUS_FC_READ_HOLDING_REGS,
                                           0, TCP_REG, 0, 1};
    assert_memory_equal(c->req + 2, expect_req, sizeof(expect_req));
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_tag, TCP_RESULT_TAG);
    /* 0x1234 * 0.1，期望值按协议与量纲写死，不从被测常量反推 */
    assert_true(fabs(c->cb_value - 466.0) < 1e-9);
}

/* 断线重连端到端：服务端 close 并重新 accept，link 在退避窗口内重连，
 * 第二笔读沿新连接完成且帧正确。 */
static void test_modbus_tcp_reconnects_after_peer_close(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;

    assert_int_equal(
        edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, 0, MODBUS_NATIVE(TCP_REG, 1), TCP_RESULT_TAG,
                             TCP_SCALE, NULL, 0, ARBITER_PRIO_NORMAL, _on_read_done, c),
        0);
    uv_run(&c->loop, UV_RUN_DEFAULT);
    if (c->timed_out || c->cb_status != 0 || !c->frame_ok)
        _dump_frame(c, "first read did not round-trip");
    assert_false(c->timed_out);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->accept_count, 1);

    _slave_drop(c);
    c->second_sent = false;
    c->second_rc = -1;

    /* 引擎在事务完成后会停读；对端关闭要等下一笔命令重新开读才可见
     * （写失败或 EOF 都行）。这笔探针命令预期失败，只用来触发链路错误。 */
    assert_int_equal(
        edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, 0, MODBUS_NATIVE(TCP_REG, 1), TCP_RESULT_TAG,
                             TCP_SCALE, NULL, 0, ARBITER_PRIO_NORMAL, _on_read_done, c),
        0);
    c->timed_out = false;
    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 3000, 0), 0);
    uv_run(&c->loop, UV_RUN_DEFAULT);
    assert_false(c->timed_out);
    assert_int_equal(c->cb_count, 2);

    /* 链路进入 BACKOFF 并重连；第二次 accept 后提交真正要验的读 */
    assert_int_equal(uv_timer_start(&c->reconnect_tick, _on_reconnect_tick, 1, 1), 0);
    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 3000, 0), 0);
    uv_run(&c->loop, UV_RUN_DEFAULT);
    uv_timer_stop(&c->reconnect_tick);

    if (c->timed_out || c->second_rc != 0 || c->cb_status != 0 || !c->frame_ok)
        _dump_frame(c, "second read after reconnect did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->second_rc, 0);
    assert_int_equal(c->accept_count, 2);
    assert_int_equal(c->req_frames, 1);  /* 复位后只数第二帧 */
    assert_true(c->frame_ok);
    assert_int_equal(c->cb_count, 3);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_tag, TCP_RESULT_TAG);
    assert_true(fabs(c->cb_value - 466.0) < 1e-9);
    assert_int_equal(c->wiring.state[0], BUS_STATE_CONNECTED);
}

/* 控制下发：FC06 应答原样回显，主站 req_decode 必须认它，否则写永远超时。 */
static void test_modbus_tcp_write_over_loopback(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;

    assert_int_equal(edge_api_write_tag(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 1),
                                        (double)TCP_RAW, _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "write did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->frame_ok);
    assert_int_equal(c->fc_seen, MODBUS_FC_WRITE_SINGLE_REG);
    assert_true(c->write_ok);
    assert_int_equal(c->req_len, TCP_REQ_LEN);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
}

/* RTU over TCP：驱动按协议选 ModbusRTU，传输按 tcp:// 路径选 TCP，帧里没有 MBAP。
 * iems_v3 对 channel 含 ':' 的 RTU 通道就是这个组合，任何一侧退化成 MBAP 都会红。 */
static void test_modbus_rtu_over_tcp_read(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;

    assert_int_equal(
        edge_api_read_tag_ex(0, "ModbusRTU", TCP_SLAVE, 0, MODBUS_NATIVE(TCP_REG, 1),
                             RTU_TCP_RESULT_TAG, TCP_SCALE, NULL, 0, ARBITER_PRIO_NORMAL,
                             _on_read_done, c),
        0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->frame_ok)
        _dump_frame(c, "rtu frame did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->frame_ok);
    assert_int_equal(c->req_len, RTU_TCP_REQ_LEN);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_tag, RTU_TCP_RESULT_TAG);
    assert_true(fabs(c->cb_value - 466.0) < 1e-9);
}

/* 32 位量写走 FC16：一次事务两个寄存器，MBAP length = 11。 */
static void test_modbus_tcp_write32_over_loopback(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;

    assert_int_equal(edge_api_write_tag(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 2),
                                        0x12345678, _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "write32 did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->frame_ok);
    assert_int_equal(c->fc_seen, MODBUS_FC_WRITE_MULTIPLE_REGS);
    assert_true(c->write_ok);
    assert_int_equal(c->req_len, 17);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
}

/* 写描述（u32 CDAB）：同一值按字序交换两寄存器上线，不是 ABCD。 */
static void test_modbus_tcp_write_desc_u32_cdab_over_loopback(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    c->write_n = 2;
    c->write_regs[0] = 0x5678;
    c->write_regs[1] = 0x1234;
    modbus_write_desc_t desc = {.type = MODBUS_WRITE_U32, .order = MODBUS_WORD_ORDER_CDAB};

    assert_int_equal(edge_api_write_tag_ex(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 2),
                                           0x12345678, &desc, sizeof(desc), _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "write u32 cdab did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->frame_ok);
    assert_int_equal(c->fc_seen, MODBUS_FC_WRITE_MULTIPLE_REGS);
    assert_true(c->write_ok);
    assert_int_equal(c->req_len, 17);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
}

/* 4 寄存器 u64 写：MBAP length=15、byte count=8，从站回显地址+数量 4。 */
static void test_modbus_tcp_write_u64_over_loopback(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    c->write_n = 4;
    c->write_regs[0] = 0x0001;
    c->write_regs[1] = 0x2345;
    c->write_regs[2] = 0x6789;
    c->write_regs[3] = 0xABCD;
    c->ack_qty = 4;
    modbus_write_desc_t desc = {.type = MODBUS_WRITE_U64, .order = MODBUS_WORD_ORDER_ABCD};

    assert_int_equal(edge_api_write_tag_ex(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 4),
                                           0x123456789ABCD, &desc, sizeof(desc), _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "write u64 did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->frame_ok);
    assert_int_equal(c->fc_seen, MODBUS_FC_WRITE_MULTIPLE_REGS);
    assert_true(c->write_ok);
    assert_int_equal(c->req_len, 21);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
}

/* TCP 多寄存器读按 ABCD 合包：修复前只回 raw[0]，100000 会读成 0 */
static void test_modbus_tcp_read_two_regs_combined(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    c->read_count = 2;
    c->read_regs[0] = 0x0001;
    c->read_regs[1] = 0x86A0;

    assert_int_equal(edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, 0, MODBUS_NATIVE(TCP_REG, 2),
                                         1004, 1.0, NULL, 0, ARBITER_PRIO_NORMAL, _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->frame_ok)
        _dump_frame(c, "two-reg read did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->frame_ok);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_true(fabs(c->cb_value - 100000.0) < 1e-9);
}

/* TCP 拆包：响应分两个 tick 到达。主站靠 RX 累积器重组，结果必须与整帧一致。 */
static void test_modbus_tcp_read_split_response(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    c->split_resp = true;

    assert_int_equal(
        edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, 0, MODBUS_NATIVE(TCP_REG, 1), TCP_RESULT_TAG,
                             TCP_SCALE, NULL, 0, ARBITER_PRIO_NORMAL, _on_read_done, c),
        0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->frame_ok)
        _dump_frame(c, "split response did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->frame_ok);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_true(fabs(c->cb_value - 466.0) < 1e-9);
}

/* FC16 应答数量与请求不符：从站没照做，不能当成功。
 * 回显固定用 ack_qty，默认等于请求的 2。 */
static void test_modbus_tcp_write32_ack_qty_mismatch(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    c->ack_qty = 3;

    assert_int_equal(edge_api_write_tag(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 2),
                                        0x12345678, _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out) _dump_frame(c, "mismatched ack did not settle");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, CMD_STATUS_PROTO_ERROR);
}

/* FC16 应答地址与请求不符：同样必须拒绝。 */
static void test_modbus_tcp_write32_ack_addr_mismatch(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    c->ack_addr = TCP_REG + 1;

    assert_int_equal(edge_api_write_tag(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 2),
                                        0x12345678, _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out) _dump_frame(c, "mismatched ack address did not settle");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, CMD_STATUS_PROTO_ERROR);
}

static void _close_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int teardown(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    if (!c) return 0;

    uv_timer_stop(&c->slave_tick);
    uv_timer_stop(&c->watchdog);
    uv_timer_stop(&c->reconnect_tick);

    if (c->cb_count == 0 && c->orch) {
        /* 用例提前停表（坏帧）时命令还挂在引擎里。先放行驱动自己的 2000ms
         * 超时完成结算，再拆 IO，避免拆完后的引擎回调踩到已释放对象。 */
        c->timed_out = false;
        uv_timer_start(&c->watchdog, _on_watchdog, 3000, 0);
        uv_run(&c->loop, UV_RUN_DEFAULT);
        uv_timer_stop(&c->watchdog);
    }

    /* link 拥有 IO 生命周期；close 回调要等 loop 转一圈 */
    iems_tcp_link_close(c->wiring.link[0].link);
    c->wiring.link[0].link = NULL;
    if (c->orch) bus_orchestrator_destroy(c->orch);
    if (c->arb) arbiter_destroy(c->arb);
    if (c->drv_mgr) driver_manager_destroy(c->drv_mgr);

    /* audit_timer 与引擎 timer 由 create 初始化而 destroy 不关，统一收尾 */
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

/* 空闲对端关闭的回归：修复前 _on_uv_poll 只报 ERROR 不停 watcher，libuv 每次
 * 循环都再收到一次 EOF，CPU 100% 空转到下一笔请求。这里不经过编排器，直接盯
 * io_executor 的回调计数，有界跑 100ms 后错误只允许报一次。 */
typedef struct {
    uv_loop_t *loop;
    int errors;
} eof_ctx_t;

static void _on_eof_event(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    (void)payload;
    eof_ctx_t *c = (eof_ctx_t *)user_data;
    if (evt == IO_EVENT_ERROR) c->errors++;
}

static void _on_eof_stop(uv_timer_t *handle) { uv_stop(handle->loop); }

static void test_io_tcp_eof_reports_once(void **state) {
    (void)state;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert_true(listen_fd >= 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    assert_int_equal(bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)), 0);
    assert_int_equal(listen(listen_fd, 4), 0);
    socklen_t addr_len = sizeof(addr);
    assert_int_equal(getsockname(listen_fd, (struct sockaddr *)&addr, &addr_len), 0);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    net_buf_pool_t *pool = net_buf_pool_new(4, 256, 0);
    assert_non_null(pool);

    io_handle_t *h = io_executor_create_tcp(&loop, "127.0.0.1", ntohs(addr.sin_port), pool);
    assert_non_null(h);
    int peer_fd = accept(listen_fd, NULL, NULL);
    assert_true(peer_fd >= 0);
    close(listen_fd);

    eof_ctx_t c = {.loop = &loop};
    h->cb = _on_eof_event;
    h->user_data = &c;
    assert_int_equal(io_handle_enable_read(h, true), 0);

    close(peer_fd);

    uv_timer_t stop;
    assert_int_equal(uv_timer_init(&loop, &stop), 0);
    assert_int_equal(uv_timer_start(&stop, _on_eof_stop, 100, 0), 0);
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_timer_stop(&stop);
    uv_close((uv_handle_t *)&stop, NULL);

    h->ops->destroy(h);
    uv_run(&loop, UV_RUN_DEFAULT);

    net_buf_pool_destroy(pool);
    int rc = uv_loop_close(&loop);
    if (rc != 0) fprintf(stderr, "uv_loop_close failed: %s\n", uv_strerror(rc));
    assert_int_equal(rc, 0);

    assert_int_equal(c.errors, 1);
}

/* 部分写回归：一次 write() 只发一部分时，剩余字节必须继续发、整帧发完才报
 * TX_DONE。修复前直接清 pending_tx 并报 TX_DONE，线上留下截断帧。
 * TCP 分支在这里逼不出部分写：本机客户端发送缓冲约 1.3MB，而 net_buf 的
 * len 是 uint16_t（帧上限 65535），一次 write 总能全收。tty 的缓冲只有几 KB，
 * 用 pty 在不读对端时确定性触发同一个 writable 分支。 */
#define PARTIAL_TX_BYTES 60000

typedef struct {
    uv_loop_t *loop;
    int master_fd;
    size_t got;
    int tx_done;
    int errors;
} partial_ctx_t;

static void _on_partial_event(io_handle_t *h, io_event_t evt, void *payload, void *user_data) {
    (void)h;
    (void)payload;
    partial_ctx_t *c = (partial_ctx_t *)user_data;
    if (evt == IO_EVENT_TX_DONE) c->tx_done++;
    else if (evt == IO_EVENT_ERROR) c->errors++;
}

static void _on_partial_drain(uv_timer_t *handle) {
    partial_ctx_t *c = (partial_ctx_t *)handle->data;
    uint8_t buf[8192];
    for (;;) {
        ssize_t n = read(c->master_fd, buf, sizeof(buf));
        if (n <= 0) break;
        c->got += (size_t)n;
    }
    if (c->got == PARTIAL_TX_BYTES && c->tx_done == 1) uv_stop(c->loop);
}

static void _on_partial_timeout(uv_timer_t *handle) { uv_stop(handle->loop); }

static void test_io_uart_partial_write_finishes(void **state) {
    (void)state;

    int master_fd = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(master_fd >= 0);
    assert_int_equal(grantpt(master_fd), 0);
    assert_int_equal(unlockpt(master_fd), 0);
    const char *slave_name = ptsname(master_fd);
    assert_non_null(slave_name);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);
    net_buf_pool_t *pool = net_buf_pool_new(4, PARTIAL_TX_BYTES, 0);
    assert_non_null(pool);

    io_handle_t *h = io_executor_create_uart(&loop, slave_name, 115200, pool);
    assert_non_null(h);

    partial_ctx_t c = {.loop = &loop, .master_fd = master_fd};
    h->cb = _on_partial_event;
    h->user_data = &c;

    net_buf_t *tx = net_buf_alloc(pool, 0);
    assert_non_null(tx);
    for (size_t i = 0; i < PARTIAL_TX_BYTES; i++) tx->b.data[i] = (uint8_t)(i * 7u);
    tx->b.len = PARTIAL_TX_BYTES;
    assert_int_equal(h->ops->write_async(h, tx), 0);
    net_buf_unref(tx);

    uv_timer_t drain, watchdog;
    assert_int_equal(uv_timer_init(&loop, &drain), 0);
    drain.data = &c;
    assert_int_equal(uv_timer_start(&drain, _on_partial_drain, 1, 1), 0);
    assert_int_equal(uv_timer_init(&loop, &watchdog), 0);
    watchdog.data = &c;
    assert_int_equal(uv_timer_start(&watchdog, _on_partial_timeout, 3000, 0), 0);

    uv_run(&loop, UV_RUN_DEFAULT);

    uv_timer_stop(&drain);
    uv_timer_stop(&watchdog);
    uv_close((uv_handle_t *)&drain, NULL);
    uv_close((uv_handle_t *)&watchdog, NULL);
    h->ops->destroy(h);
    uv_run(&loop, UV_RUN_DEFAULT);
    close(master_fd);
    net_buf_pool_destroy(pool);
    int rc = uv_loop_close(&loop);
    if (rc != 0) fprintf(stderr, "uv_loop_close failed: %s\n", uv_strerror(rc));
    assert_int_equal(rc, 0);

    assert_int_equal(c.errors, 0);
    assert_int_equal(c.tx_done, 1);
    assert_int_equal(c.got, PARTIAL_TX_BYTES);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_io_tcp_eof_reports_once),
        cmocka_unit_test(test_io_uart_partial_write_finishes),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_read_over_loopback, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_reconnects_after_peer_close, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_write_over_loopback, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_write32_over_loopback, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_write_desc_u32_cdab_over_loopback, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_write_u64_over_loopback, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_read_split_response, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_write32_ack_qty_mismatch, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_write32_ack_addr_mismatch, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_over_tcp_read, setup_rtu_over_tcp,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_read_two_regs_combined, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
