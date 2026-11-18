#define _GNU_SOURCE

#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <setjmp.h>
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
    uv_timer_t slave_tick;
    uv_timer_t watchdog;

    uint8_t req[64];
    size_t req_len;
    int req_frames;
    uint8_t fc_seen;
    bool frame_ok;
    bool write_ok;
    bool rtu_frames;
    unsigned read_count;
    uint16_t read_regs[2];
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

    if (c->conn_fd < 0) {
        int fd = accept(c->listen_fd, NULL, NULL);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EINTR) return;
            c->frame_bad = true;
            _dump_frame(c, "accept failed");
            return;
        }
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
                  len == (fc == MODBUS_FC_WRITE_MULTIPLE_REGS ? 11u : 6u) &&
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
        /* FC16 写 32 位量：count=2、byte count=4、ABCD 两寄存器 */
        c->write_ok = qty == 2 && c->req[12] == 4 && c->req[13] == 0x12 && c->req[14] == 0x34 &&
                      c->req[15] == 0x56 && c->req[16] == 0x78;
        if (!c->write_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected write32 payload");
            return;
        }
        uint8_t ack[12] = {c->req[0], c->req[1], 0, 0, 0, 6,
                           TCP_SLAVE, MODBUS_FC_WRITE_MULTIPLE_REGS,
                           0,         TCP_REG,  0, 2};
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

static int setup_common(void **state, const char *protocol, bool rtu_frames) {
    tcp_ctx_t *c = calloc(1, sizeof(*c));
    assert_non_null(c);
    c->listen_fd = -1;
    c->conn_fd = -1;
    c->rtu_frames = rtu_frames;
    c->read_count = 1;
    c->read_regs[0] = TCP_RAW;

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

    bus_conn_state_t bus_state[IEMS_MAX_VBUS] = {BUS_STATE_DISCONNECTED};
    int configured = 0;
    iems_southbound_connect(&c->loop, &c->hub, c->orch, bus_state, &configured);
    unsetenv("OMNIS_BUS_0_DEV");

    assert_true(configured >= 1);
    assert_int_equal(bus_state[0], BUS_STATE_CONNECTED);

    assert_int_equal(uv_timer_init(&c->loop, &c->slave_tick), 0);
    c->slave_tick.data = c;
    assert_int_equal(uv_timer_start(&c->slave_tick, _on_slave_tick, 1, 1), 0);

    assert_int_equal(uv_timer_init(&c->loop, &c->watchdog), 0);
    c->watchdog.data = c;
    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 3000, 0), 0);

    *state = c;
    return 0;
}

static int setup(void **state) { return setup_common(state, "ModbusTCP", false); }

static int setup_rtu_over_tcp(void **state) { return setup_common(state, "ModbusRTU", true); }

static void test_modbus_tcp_read_over_loopback(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;

    assert_int_equal(
        edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 1), TCP_RESULT_TAG,
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
        edge_api_read_tag_ex(0, "ModbusRTU", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 1),
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

/* TCP 多寄存器读按 ABCD 合包：修复前只回 raw[0]，100000 会读成 0 */
static void test_modbus_tcp_read_two_regs_combined(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    c->read_count = 2;
    c->read_regs[0] = 0x0001;
    c->read_regs[1] = 0x86A0;

    assert_int_equal(edge_api_read_tag_ex(0, "ModbusTCP", TCP_SLAVE, MODBUS_NATIVE(TCP_REG, 2),
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

static void _close_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int teardown(void **state) {
    tcp_ctx_t *c = (tcp_ctx_t *)*state;
    if (!c) return 0;

    uv_timer_stop(&c->slave_tick);
    uv_timer_stop(&c->watchdog);

    if (c->cb_count == 0 && c->orch) {
        /* 用例提前停表（坏帧）时命令还挂在引擎里。先放行驱动自己的 2000ms
         * 超时完成结算，再拆 IO，避免拆完后的引擎回调踩到已释放对象。 */
        c->timed_out = false;
        uv_timer_start(&c->watchdog, _on_watchdog, 3000, 0);
        uv_run(&c->loop, UV_RUN_DEFAULT);
        uv_timer_stop(&c->watchdog);
    }

    /* orchestrator 不拥有 IO 生命周期，先自己关；close 回调要等 loop 转一圈 */
    io_handle_t *h = bus_orchestrator_get_io(c->orch, 0);
    if (h) h->ops->destroy(h);
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

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_modbus_tcp_read_over_loopback, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_write_over_loopback, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_write32_over_loopback, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_over_tcp_read, setup_rtu_over_tcp,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_modbus_tcp_read_two_regs_combined, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
