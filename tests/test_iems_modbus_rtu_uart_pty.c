#define _GNU_SOURCE

#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* 端到端约束：UART/TCP 传输、协议选择、南向 attach 都有各自单测，但没有一条
 * 真实 Modbus RTU 主站读穿过 UART 的证据。这里用 PTY 模拟从站，请求帧必须
 * 真的从 iems_southbound_connect 打开的 UART 出来，响应沿同一个 fd 回去。
 * 仲裁、驱动编码解码、请求层标定、回调任何一环断掉，本用例都会红。 */

#define RTU_REQ_LEN 8
#define RTU_RESP_LEN 7
#define RTU_SLAVE 1
#define RTU_REG 0
#define RTU_RAW 0x1234
#define RTU_SCALE 0.1
#define RTU_RESULT_TAG 1001

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    arbiter_t *arb;
    bus_orchestrator_t *orch;
    driver_manager_t *drv_mgr;
    driver_manager_storage_t drv_mgr_storage;

    int master_fd;
    uv_timer_t slave_tick;
    uv_timer_t watchdog;

    uint8_t req[32];
    size_t req_len;
    int req_frames;
    bool hi_crc; /* 驱动变体：CRC 高字节先发 */
    uint8_t fc_seen;
    bool crc_ok;
    bool write_ok;
    bool frame_bad;
    bool responded;
    bool timed_out;

    int cb_count;
    int cb_status;
    uint32_t cb_tag;
    double cb_value;
} pty_ctx_t;

static uint16_t _rx_crc(const pty_ctx_t *c, const uint8_t *p, size_t frame_len) {
    return c->hi_crc ? (uint16_t)((p[frame_len - 2] << 8) | p[frame_len - 1])
                     : (uint16_t)(p[frame_len - 2] | (p[frame_len - 1] << 8));
}

static void _wire_crc(const pty_ctx_t *c, uint8_t *p, size_t body_len) {
    uint16_t crc = modbus_calculate_crc(p, body_len);
    p[body_len] = (uint8_t)(c->hi_crc ? (crc >> 8) : (crc & 0xFF));
    p[body_len + 1] = (uint8_t)(c->hi_crc ? (crc & 0xFF) : (crc >> 8));
}

static void _dump_frame(const pty_ctx_t *c, const char *why) {
    fprintf(stderr,
            "[modbus-rtu-pty] %s: frames=%d responded=%d timed_out=%d crc_ok=%d rx_len=%zu\n", why,
            c->req_frames, c->responded, c->timed_out, c->crc_ok, c->req_len);
    fprintf(stderr, "[modbus-rtu-pty] rx:");
    for (size_t i = 0; i < c->req_len; i++) fprintf(stderr, " %02X", c->req[i]);
    fprintf(stderr, "\n");
    if (c->req_len >= RTU_REQ_LEN) {
        uint16_t got = _rx_crc(c, c->req, c->req_len);
        fprintf(stderr, "[modbus-rtu-pty] crc got=0x%04X want=0x%04X\n", got,
                modbus_calculate_crc(c->req, c->req_len - 2));
    }
}

/* 本测试同时扮演从站。PTY master 非阻塞，每次 tick 尽力收满一帧请求。 */
static void _on_slave_tick(uv_timer_t *handle) {
    pty_ctx_t *c = (pty_ctx_t *)handle->data;
    if (c->responded || c->frame_bad) return;

    ssize_t n = read(c->master_fd, c->req + c->req_len, sizeof(c->req) - c->req_len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EINTR) return;
        c->frame_bad = true;
        _dump_frame(c, "read failed");
        return;
    }
    if (n == 0) return;
    c->req_len += (size_t)n;
    if (c->req_len < RTU_REQ_LEN) return;
    /* FC16 写 32 位量共 13 字节，等整帧到齐再判 CRC 与字段 */
    if (c->req[1] == MODBUS_FC_WRITE_MULTIPLE_REGS && c->req_len < 13) return;

    c->req_frames++;
    uint16_t crc = modbus_calculate_crc(c->req, c->req_len - 2);
    c->crc_ok = _rx_crc(c, c->req, c->req_len) == crc;
    uint8_t fc = c->req[1];
    c->fc_seen = fc;
    if (c->req[0] != RTU_SLAVE || !c->crc_ok ||
        (fc != MODBUS_FC_READ_HOLDING_REGS && fc != MODBUS_FC_WRITE_SINGLE_REG &&
         fc != MODBUS_FC_WRITE_MULTIPLE_REGS)) {
        /* 不回坏请求：让驱动的 2000ms 超时结算，失败原因留在上面的转储里 */
        c->frame_bad = true;
        _dump_frame(c, "unexpected request frame");
        return;
    }

    if (fc == MODBUS_FC_WRITE_SINGLE_REG) {
        /* FC06 应答就是原帧回显，写值与 CRC 都原样回来 */
        c->write_ok = c->req[2] == 0x00 && c->req[3] == RTU_REG && c->req[4] == 0x12 &&
                      c->req[5] == 0x34;
        if (!c->write_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected write payload");
            return;
        }
        if (write(c->master_fd, c->req, RTU_REQ_LEN) == (ssize_t)RTU_REQ_LEN) {
            c->responded = true;
        } else {
            c->frame_bad = true;
            _dump_frame(c, "write echo failed");
        }
        return;
    }

    if (fc == MODBUS_FC_WRITE_MULTIPLE_REGS) {
        c->write_ok = c->req[2] == 0x00 && c->req[3] == RTU_REG && c->req[4] == 0x00 &&
                      c->req[5] == 0x02 && c->req[6] == 0x04 && c->req[7] == 0x12 &&
                      c->req[8] == 0x34 && c->req[9] == 0x56 && c->req[10] == 0x78;
        if (!c->write_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected write32 payload");
            return;
        }
        /* FC16 应答只回地址与数量 */
        uint8_t ack[8] = {RTU_SLAVE, MODBUS_FC_WRITE_MULTIPLE_REGS, 0x00, RTU_REG,
                          0x00,      0x02,                        0,    0};
        _wire_crc(c, ack, 6);
        if (write(c->master_fd, ack, sizeof(ack)) == (ssize_t)sizeof(ack)) {
            c->responded = true;
        } else {
            c->frame_bad = true;
            _dump_frame(c, "write32 ack failed");
        }
        return;
    }

    uint8_t resp[RTU_RESP_LEN] = {RTU_SLAVE,
                                  MODBUS_FC_READ_HOLDING_REGS,
                                  2,
                                  (uint8_t)(RTU_RAW >> 8),
                                  (uint8_t)(RTU_RAW & 0xFF),
                                  0,
                                  0};
    _wire_crc(c, resp, 5);
    if (write(c->master_fd, resp, sizeof(resp)) == (ssize_t)sizeof(resp)) {
        c->responded = true;
    } else {
        c->frame_bad = true;
        _dump_frame(c, "response write failed");
    }
}

static void _on_read_done(int status, const pipe_msg_t *result, void *user_data) {
    pty_ctx_t *c = (pty_ctx_t *)user_data;
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
    pty_ctx_t *c = (pty_ctx_t *)handle->data;
    c->timed_out = true;
    uv_stop(&c->loop);
}

static int setup_common(void **state, const char *protocol, bool hi_crc) {
    pty_ctx_t *c = calloc(1, sizeof(*c));
    assert_non_null(c);
    c->master_fd = -1;
    c->hi_crc = hi_crc;

    int master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(master >= 0);
    assert_int_equal(grantpt(master), 0);
    assert_int_equal(unlockpt(master), 0);
    const char *slave_name = ptsname(master);
    assert_non_null(slave_name);
    c->master_fd = master;

    assert_int_equal(uv_loop_init(&c->loop), 0);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_mb_rtu_pty_%d", (int)getpid());
    assert_int_equal(datahub_init(&c->hub, &c->loop, shm_name, 16, 32, NULL, 0, NULL, NULL, NULL),
                     0);

    assert_int_equal(arbiter_init(&c->arb, &c->loop), 0);
    /* arbiter 默认暂停，不 resume 命令永远留在队列里 */
    arbiter_resume(c->arb);

    c->drv_mgr = driver_manager_create_in(&c->drv_mgr_storage);
    assert_non_null(c->drv_mgr);
    extern int modbus_driver_register(driver_manager_t * mgr);
    extern int modbus_rtu_hicrc_driver_register(driver_manager_t * mgr);
    assert_int_equal(modbus_driver_register(c->drv_mgr), 0);
    assert_int_equal(modbus_rtu_hicrc_driver_register(c->drv_mgr), 0);

    c->orch = bus_orchestrator_create_hub_default(
        &c->hub, c->arb, driver_manager_session_port_ptr(c->drv_mgr), 2, 8, 256);
    assert_non_null(c->orch);
    /* 与 iems_app 一致：IO 池由 orchestrator 建，再交给 DataHub 的 UART 工厂用 */
    datahub_bind_io_pool(&c->hub, bus_orchestrator_get_pool(c->orch));
    edge_api_init(c->orch);

    assert_int_equal(bus_orchestrator_setup_master(c->orch, 0, protocol, 0), 0);

    assert_int_equal(setenv("OMNIS_BUS_0_DEV", slave_name, 1), 0);
    assert_int_equal(setenv("OMNIS_BUS_0_BAUD", "115200", 1), 0);

    bus_conn_state_t bus_state[IEMS_MAX_VBUS] = {BUS_STATE_DISCONNECTED};
    int configured = 0;
    iems_southbound_connect(&c->loop, &c->hub, c->orch, bus_state, &configured);
    unsetenv("OMNIS_BUS_0_DEV");
    unsetenv("OMNIS_BUS_0_BAUD");

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

static int setup(void **state) { return setup_common(state, "ModbusRTU", false); }

static int setup_hicrc(void **state) { return setup_common(state, "ModbusRTUHiCRC", true); }

static void test_modbus_rtu_read_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    assert_int_equal(
        edge_api_read_tag_ex(0, "ModbusRTU", RTU_SLAVE, MODBUS_NATIVE(RTU_REG, 1), RTU_RESULT_TAG,
                             RTU_SCALE, NULL, 0, ARBITER_PRIO_NORMAL, _on_read_done, c),
        0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->crc_ok)
        _dump_frame(c, "request did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->crc_ok);
    assert_int_equal(c->req_len, RTU_REQ_LEN);
    static const uint8_t expect_req[6] = {
        RTU_SLAVE, MODBUS_FC_READ_HOLDING_REGS, 0x00, RTU_REG, 0x00, 0x01};
    assert_memory_equal(c->req, expect_req, sizeof(expect_req));
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_tag, RTU_RESULT_TAG);
    assert_true(fabs(c->cb_value - (double)RTU_RAW * RTU_SCALE) < 1e-9);
}

/* 写路径此前只有编码单测：FC06 应答到达主站后 req_decode 只认 FC03，控制下发
 * 永远以错误结束。这里让从站按协议原样回显，断言回调成功。 */
static void test_modbus_rtu_write_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    assert_int_equal(edge_api_write_tag(0, "ModbusRTU", RTU_SLAVE, MODBUS_NATIVE(RTU_REG, 1),
                                        (double)RTU_RAW, _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "write did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->crc_ok);
    assert_int_equal(c->fc_seen, MODBUS_FC_WRITE_SINGLE_REG);
    assert_true(c->write_ok);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
}

/* 32 位量写走 FC16：一次事务写两个寄存器，从站不应看到中间态。 */
static void test_modbus_rtu_write32_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    assert_int_equal(edge_api_write_tag(0, "ModbusRTU", RTU_SLAVE, MODBUS_NATIVE(RTU_REG, 2),
                                        0x12345678, _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "write32 did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->crc_ok);
    assert_int_equal(c->req_len, 13);
    assert_int_equal(c->fc_seen, MODBUS_FC_WRITE_MULTIPLE_REGS);
    assert_true(c->write_ok);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
}

/* iems_v3 默认 modbus_rtu 的高字节先发 CRC：同一份合法负载，CRC 两字节顺序相反。
 * 这条用例与下面标准序的读用例并存，防止把两种现场混成一个。 */
static void test_modbus_rtu_hicrc_read_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    assert_int_equal(
        edge_api_read_tag_ex(0, "ModbusRTUHiCRC", RTU_SLAVE, MODBUS_NATIVE(RTU_REG, 1),
                             RTU_RESULT_TAG, RTU_SCALE, NULL, 0, ARBITER_PRIO_NORMAL, _on_read_done,
                             c),
        0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->crc_ok)
        _dump_frame(c, "hi-crc request did not round-trip");

    assert_false(c->timed_out);
    assert_true(c->hi_crc);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->crc_ok);
    assert_int_equal(c->req_len, RTU_REQ_LEN);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_true(fabs(c->cb_value - (double)RTU_RAW * RTU_SCALE) < 1e-9);
}

static void _close_handle(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) uv_close(handle, NULL);
}

static int teardown(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
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
    if (c->master_fd >= 0) close(c->master_fd);

    int rc = uv_loop_close(&c->loop);
    if (rc != 0) fprintf(stderr, "uv_loop_close failed: %s\n", uv_strerror(rc));
    assert_int_equal(rc, 0);
    free(c);
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_modbus_rtu_read_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_write_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_write32_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_hicrc_read_over_uart_pty, setup_hicrc,
                                        teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
