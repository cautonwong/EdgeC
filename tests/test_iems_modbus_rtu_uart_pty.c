#define _GNU_SOURCE

#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
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
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"
#include "iems_north_adapter.h"
#include "iems_southbound_adapter.h"
#include "io_handle_priv.h"
#include "modbus_codec.h"
#include "northbound_mgr.h"
#include "thread_port_uv.h"

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
    uint8_t expect_slave;      /* PTY 从站只应答这个地址 */
    uint16_t expect_write_value; /* FC06 合法负载的唯一写值 */
    uint16_t expect_write_regs[4]; /* FC16 合法负载的寄存器序列 */
    uint16_t expect_write_n;       /* FC16 寄存器个数 */
    uint8_t write_frame[32];   /* 首帧 FC06 写请求存证（req 缓冲会复用给后续读） */
    size_t write_frame_len;
    bool crc_ok;
    bool write_ok;
    bool frame_bad;
    bool responded;
    bool timed_out;

    int cb_count;
    int cb_status;
    uint32_t cb_tag;
    uint32_t cb_device_id;
    double cb_value;

    /* 执行端口完成回调（P4-03c1）：真实总线成功后必须回到域三态 */
    int done_count;
    int done_status;

    /* 总线事务已结算（拆 IO 前不必再等 3s 看门狗） */
    bool command_settled;
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
    if (c->frame_bad) return;

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
    /* FC16 帧长随 byte count 变化：7 字节头 + 数据 + 2 字节 CRC。
     * 等整帧到齐再判 CRC 与字段，避免把半帧当坏帧。 */
    if (c->req[1] == MODBUS_FC_WRITE_MULTIPLE_REGS) {
        if (c->req_len < 7) return;
        size_t frame_len = 7 + (size_t)c->req[6] + 2;
        if (c->req_len < frame_len) return;
    }

    c->req_frames++;
    uint16_t crc = modbus_calculate_crc(c->req, c->req_len - 2);
    c->crc_ok = _rx_crc(c, c->req, c->req_len) == crc;
    uint8_t fc = c->req[1];
    c->fc_seen = fc;
    if (c->req[0] != c->expect_slave || !c->crc_ok ||
        (fc != MODBUS_FC_READ_HOLDING_REGS && fc != MODBUS_FC_WRITE_SINGLE_REG &&
         fc != MODBUS_FC_WRITE_MULTIPLE_REGS)) {
        /* 不回坏请求：让驱动的 2000ms 超时结算，失败原因留在上面的转储里 */
        c->frame_bad = true;
        _dump_frame(c, "unexpected request frame");
        return;
    }

    if (fc == MODBUS_FC_WRITE_SINGLE_REG) {
        /* FC06 应答就是原帧回显，写值与 CRC 都原样回来 */
        c->write_ok = c->req[2] == 0x00 && c->req[3] == RTU_REG &&
                      (uint16_t)((c->req[4] << 8) | c->req[5]) == c->expect_write_value;
        if (!c->write_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected write payload");
            return;
        }
        c->write_frame_len = c->req_len;
        memcpy(c->write_frame, c->req, c->req_len);
        if (write(c->master_fd, c->req, RTU_REQ_LEN) == (ssize_t)RTU_REQ_LEN) {
            c->responded = true;
            c->req_len = 0; /* 后面可能还有一条读要应答 */
        } else {
            c->frame_bad = true;
            _dump_frame(c, "write echo failed");
        }
        return;
    }

    if (fc == MODBUS_FC_WRITE_MULTIPLE_REGS) {
        uint16_t want_n = c->expect_write_n;
        c->write_ok = c->req[2] == 0x00 && c->req[3] == RTU_REG &&
                      (uint16_t)((c->req[4] << 8) | c->req[5]) == want_n &&
                      c->req[6] == 2 * want_n;
        for (uint16_t i = 0; c->write_ok && i < want_n; i++) {
            uint16_t got = (uint16_t)((c->req[7 + 2 * i] << 8) | c->req[8 + 2 * i]);
            if (got != c->expect_write_regs[i]) c->write_ok = false;
        }
        if (!c->write_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected write payload");
            return;
        }
        /* FC16 应答只回地址与数量 */
        uint8_t ack[8] = {c->req[0], MODBUS_FC_WRITE_MULTIPLE_REGS, c->req[2],
                          c->req[3], c->req[4], c->req[5], 0, 0};
        _wire_crc(c, ack, 6);
        if (write(c->master_fd, ack, sizeof(ack)) == (ssize_t)sizeof(ack)) {
            c->responded = true;
        } else {
            c->frame_bad = true;
            _dump_frame(c, "write32 ack failed");
        }
        return;
    }

    uint8_t resp[RTU_RESP_LEN] = {c->req[0],
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
    c->command_settled = true;
    if (result) {
        c->cb_tag = (uint32_t)result->tag_id;
        c->cb_device_id = result->device_id;
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
    c->expect_slave = RTU_SLAVE;
    c->expect_write_value = RTU_RAW;
    c->expect_write_n = 2;
    c->expect_write_regs[0] = (uint16_t)(0x12345678 >> 16);
    c->expect_write_regs[1] = (uint16_t)(0x12345678 & 0xFFFF);

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
        edge_api_read_tag_ex(0, "ModbusRTU", RTU_SLAVE, 0, MODBUS_NATIVE(RTU_REG, 1), RTU_RESULT_TAG,
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

/* 写描述（u32 CDAB）：2 个寄存器按字序交换后上线，不是 ABCD。 */
static void test_modbus_rtu_write_desc_u32_cdab_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
    c->expect_write_n = 2;
    c->expect_write_regs[0] = 0x5678;
    c->expect_write_regs[1] = 0x1234;
    modbus_write_desc_t desc = {.type = MODBUS_WRITE_U32, .order = MODBUS_WORD_ORDER_CDAB};

    assert_int_equal(edge_api_write_tag_ex(0, "ModbusRTU", RTU_SLAVE, MODBUS_NATIVE(RTU_REG, 2),
                                           0x12345678, &desc, sizeof(desc), _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "write u32 cdab did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->crc_ok);
    assert_int_equal(c->req_len, 13);
    assert_int_equal(c->fc_seen, MODBUS_FC_WRITE_MULTIPLE_REGS);
    assert_true(c->write_ok);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
}

/* 4 寄存器 u64 写：描述编码 4 个寄存器，FC16 一次事务发出；
 * 从站按寄存器序列校验并回显地址+数量，主站 decode 校验 count=4。 */
static void test_modbus_rtu_write_u64_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
    c->expect_write_n = 4;
    c->expect_write_regs[0] = 0x0001;
    c->expect_write_regs[1] = 0x2345;
    c->expect_write_regs[2] = 0x6789;
    c->expect_write_regs[3] = 0xABCD;
    modbus_write_desc_t desc = {.type = MODBUS_WRITE_U64, .order = MODBUS_WORD_ORDER_ABCD};

    assert_int_equal(edge_api_write_tag_ex(0, "ModbusRTU", RTU_SLAVE, MODBUS_NATIVE(RTU_REG, 4),
                                           0x123456789ABCD, &desc, sizeof(desc), _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "write u64 did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->crc_ok);
    assert_int_equal(c->req_len, 17);
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
        edge_api_read_tag_ex(0, "ModbusRTUHiCRC", RTU_SLAVE, 0, MODBUS_NATIVE(RTU_REG, 1),
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

/* P1 回归：北向写必须落在轮询路由给出的从站地址上。从站只认 slave 3，
 * 控制写经执行适配器下发；旧实现写死 slave 1 时这条用例会红。
 * 同时钉住执行端口完成回调：真实总线成功后回到 IEMS_WRITE_DONE_OK。 */
static void _on_write_done(void *user, iems_write_done_status_t status) {
    pty_ctx_t *c = (pty_ctx_t *)user;
    c->done_count++;
    c->done_status = (int)status;
    c->command_settled = true;
}

static void test_modbus_rtu_write_slave_route_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
    c->expect_slave = 3;
    c->expect_write_value = 2201; /* 220.1 V / scaler 0.1 */
    c->done_status = -1;

    iems_execution_port_t port = iems_execution_port_create(NULL);
    assert_non_null(port.write_tag);
    /* 执行适配器按点位表翻译：1001 → 寄存器 0，1 个寄存器，raw 2201 */
    assert_int_equal(port.write_tag(port.ctx, 0, 3, IEMS_TAG_VOLTAGE_PHASE_A, 220.1, "ModbusRTU",
                                    _on_write_done, c),
                     0);
    /* 同一总线上的读作为屏障：写事务结算后才轮到它，循环在回调里停 */
    assert_int_equal(edge_api_read_tag_ex(0, "ModbusRTU", 3, 0, MODBUS_NATIVE(RTU_REG, 1),
                                          RTU_RESULT_TAG, RTU_SCALE, NULL, 0, ARBITER_PRIO_NORMAL,
                                          _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok || c->write_frame_len == 0)
        _dump_frame(c, "slave-routed write did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->write_frame_len, RTU_REQ_LEN);
    assert_int_equal(c->write_frame[0], 3);
    assert_int_equal(c->write_frame[1], MODBUS_FC_WRITE_SINGLE_REG);
    assert_int_equal((c->write_frame[4] << 8) | c->write_frame[5], 2201);
    assert_true(c->write_ok);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    /* 执行端口把总线成功映射回域状态，且只回调一次（槽已归还） */
    assert_int_equal(c->done_count, 1);
    assert_int_equal(c->done_status, IEMS_WRITE_DONE_OK);
}

/* P6-04b1 A4: 回调带回设备身份(result_device_id)并原样使用；0 是 tag 级视图，
 * 不回退从站地址。线上寻址仍只用从站地址，两者不能混。 */
static void test_modbus_rtu_result_device_id(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
    c->expect_slave = 3;

    assert_int_equal(edge_api_read_tag_ex(0, "ModbusRTU", 3, 5, MODBUS_NATIVE(RTU_REG, 1),
                                          RTU_RESULT_TAG, RTU_SCALE, NULL, 0, ARBITER_PRIO_NORMAL,
                                          _on_read_done, c),
                     0);
    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0) _dump_frame(c, "device-id read did not round-trip");
    assert_false(c->timed_out);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_device_id, 5);
    assert_int_equal(c->req[0], 3); /* 线上寻址仍是 slave 3 */

    /* result_device_id = 0：0 是合法的 tag 级视图身份，原样回传；线上仍寻址 slave 3 */
    c->cb_count = 0;
    c->cb_status = -1;
    c->cb_device_id = 0;
    c->command_settled = false;
    c->responded = false;
    c->req_len = 0;
    c->req_frames = 0;
    assert_int_equal(edge_api_read_tag_ex(0, "ModbusRTU", 3, 0, MODBUS_NATIVE(RTU_REG, 1),
                                          RTU_RESULT_TAG, RTU_SCALE, NULL, 0, ARBITER_PRIO_NORMAL,
                                          _on_read_done, c),
                     0);
    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0) _dump_frame(c, "view-identity read did not round-trip");
    assert_false(c->timed_out);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_device_id, 0);
    assert_int_equal(c->req[0], 3); /* 线上寻址仍是 slave 3 */
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

    if (c->cb_count == 0 && !c->command_settled && c->orch) {
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

/* ---- P4-03c1 全链路：真执行适配器 → 真总线/legacy 回调 → 回执队列 → 假 transport ---- */

#define NORTH_FAKE_CAP 40 /* 17 条命令各 accepted+result 也装得下 */

typedef struct {
    pthread_mutex_t mu;
    int publishes;
    char points[NORTH_FAKE_CAP][128];
    pthread_t threads[NORTH_FAKE_CAP];
} north_fake_t;

static north_fake_t g_north;
static northbound_mgr_t g_north_mgr;
static iems_north_ctx_t g_north_ctx;
static int g_north_bus_id;
static uint32_t g_north_slave_id;
static bool g_north_started;

static int _north_publish(void *ctx, const char *topic, const uint8_t *payload, size_t len) {
    (void)topic;
    north_fake_t *f = (north_fake_t *)ctx;
    pthread_mutex_lock(&f->mu);
    int i = f->publishes;
    if (i < NORTH_FAKE_CAP) {
        size_t n = len < sizeof(f->points[i]) - 1 ? len : sizeof(f->points[i]) - 1;
        if (payload && n) memcpy(f->points[i], payload, n);
        f->points[i][n] = '\0';
        f->threads[i] = pthread_self();
    }
    f->publishes++;
    pthread_mutex_unlock(&f->mu);
    return 0;
}

static bool _north_connected(void *ctx) {
    (void)ctx;
    return true;
}

static int _north_attach(void *ctx, const char *protocol, north_driver_t **out_driver) {
    (void)ctx;
    (void)protocol;
    static int marker;
    *out_driver = (north_driver_t *)(void *)&marker;
    return 0;
}

static int _north_encode(void *ctx, north_driver_t *driver, uint32_t tag_id, double value,
                         uint64_t ts_us, const char *name, const uint8_t **out_data,
                         size_t *out_len) {
    (void)ctx;
    (void)driver;
    (void)tag_id;
    (void)ts_us;
    static char frame[128];
    int n = snprintf(frame, sizeof(frame), "%s=%g", name ? name : "", value);
    if (n <= 0 || (size_t)n >= sizeof(frame)) return -1;
    *out_data = (const uint8_t *)frame;
    *out_len = (size_t)n;
    return 0;
}

static int _north_route(void *ctx, uint32_t tag_id, iems_device_route_t *route) {
    (void)ctx;
    (void)tag_id;
    memset(route, 0, sizeof(*route));
    route->bus_id = g_north_bus_id;
    route->slave_id = g_north_slave_id;
    snprintf(route->protocol, sizeof(route->protocol), "%s", "ModbusRTU");
    return 0;
}

/* 真执行适配器：把完成回调一路带到 edge_api → legacy 请求 → 总线结算。 */
static int _north_submit(void *ctx, const iems_device_route_t *route, uint32_t tag_id,
                         double value, iems_write_done_fn on_done, void *on_done_user) {
    (void)ctx;
    iems_execution_port_t port = iems_execution_port_create(NULL);
    return port.write_tag(port.ctx, route->bus_id, route->slave_id, tag_id, value,
                          route->protocol, on_done, on_done_user);
}

static void _north_setup(void) {
    memset(&g_north, 0, sizeof(g_north));
    pthread_mutex_init(&g_north.mu, NULL);
    nb_source_ops_t source = {0};
    nb_transport_ops_t transport = {
        .ctx = &g_north, .publish = _north_publish, .is_connected = _north_connected};
    assert_int_equal(northbound_mgr_init_ex(&g_north_mgr, source, "pty_e2e", NULL, transport), 0);
    north_driver_port_t port = {
        .ctx = &g_north, .attach = _north_attach, .encode_value = _north_encode};
    northbound_mgr_set_driver_port(&g_north_mgr, port);
    northbound_mgr_set_address(&g_north_mgr, "spBv1.0/{group}/{kind}/{node}", "G", "N1");

    memset(&g_north_ctx, 0, sizeof(g_north_ctx));
    iems_north_results_reset(&g_north_ctx);
    g_north_ctx.north = &g_north_mgr;
    g_north_ctx.submit_control = _north_submit;
    g_north_ctx.route_for_tag = _north_route;
    northbound_mgr_set_north_source(&g_north_mgr, iems_north_source_ops(), &g_north_ctx);

    assert_int_equal(northbound_mgr_start(&g_north_mgr), 0);
    g_north_started = true;
    bool ready = false;
    for (int i = 0; i < 5000 && !ready; i++) {
        ready = northbound_mgr_publish_result_ack_async(&g_north_mgr, 0, "READY",
                                                        NB_RESULT_STATUS_OK) == 0;
        if (!ready) usleep(1000);
    }
    assert_true(ready);
    for (int i = 0; i < 1000; i++) {
        pthread_mutex_lock(&g_north.mu);
        bool got = g_north.publishes > 0;
        pthread_mutex_unlock(&g_north.mu);
        if (got) break;
        usleep(1000);
    }
    pthread_mutex_lock(&g_north.mu);
    g_north.publishes = 0;
    memset(g_north.points, 0, sizeof(g_north.points));
    memset(g_north.threads, 0, sizeof(g_north.threads));
    pthread_mutex_unlock(&g_north.mu);
}

static void _north_teardown(void) {
    iems_north_results_reset(&g_north_ctx);
    if (g_north_started) {
        northbound_mgr_stop(&g_north_mgr);
        g_north_started = false;
    }
    northbound_mgr_destroy(&g_north_mgr);
    pthread_mutex_destroy(&g_north.mu);
}

static void _north_inject(void) {
    const char *frame = "{\"p\":\"VOLTAGE_PHASE_A\",\"v\":220.1}";
    assert_int_equal(iems_north_source_ops()->on_downlink(&g_north_ctx, "spBv1.0/G/NCMD/N1",
                                                          (const uint8_t *)frame, strlen(frame)),
                     0);
}

static bool _north_seen(const char *point) {
    bool found = false;
    pthread_mutex_lock(&g_north.mu);
    for (int i = 0; i < g_north.publishes && i < NORTH_FAKE_CAP; i++) {
        if (strcmp(g_north.points[i], point) == 0) found = true;
    }
    pthread_mutex_unlock(&g_north.mu);
    return found;
}

static int _north_count(const char *point) {
    int count = 0;
    pthread_mutex_lock(&g_north.mu);
    for (int i = 0; i < g_north.publishes && i < NORTH_FAKE_CAP; i++) {
        if (strcmp(g_north.points[i], point) == 0) count++;
    }
    pthread_mutex_unlock(&g_north.mu);
    return count;
}

/* 跑 pty loop 直到结果回执出现（结果由北向 loop 线程异步发布）。 */
static bool _north_wait(pty_ctx_t *c, const char *point, int timeout_ms) {
    for (int i = 0; i < timeout_ms; i++) {
        uv_run(&c->loop, UV_RUN_NOWAIT);
        if (_north_seen(point)) return true;
        usleep(1000);
    }
    return false;
}

static bool _north_wait_count(pty_ctx_t *c, const char *point, int want, int timeout_ms) {
    for (int i = 0; i < timeout_ms; i++) {
        uv_run(&c->loop, UV_RUN_NOWAIT);
        if (_north_count(point) >= want) return true;
        usleep(1000);
    }
    return false;
}

static bool _north_off_test_thread(void) {
    pthread_t self = pthread_self();
    bool ok = true;
    pthread_mutex_lock(&g_north.mu);
    for (int i = 0; i < g_north.publishes && i < NORTH_FAKE_CAP; i++) {
        if (pthread_equal(g_north.threads[i], self)) ok = false;
    }
    pthread_mutex_unlock(&g_north.mu);
    return ok;
}

static int setup_north(void **state) {
    int rc = setup(state);
    if (rc == 0) _north_setup();
    return rc;
}

static int teardown_north(void **state) {
    _north_teardown();
    return teardown(state);
}

/* F：成功路径经真执行适配器/legacy 回调到 `<name>.result=1`（pty 从站回显）。 */
static void test_result_receipt_success_via_real_adapter(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
    uv_timer_stop(&c->watchdog);
    c->expect_slave = 3;
    c->expect_write_value = 2201;
    g_north_bus_id = 0;
    g_north_slave_id = 3;

    _north_inject();
    assert_true(_north_wait(c, "VOLTAGE_PHASE_A.result=1", 3000));
    assert_true(_north_wait(c, "VOLTAGE_PHASE_A.ack=1", 3000));
    assert_true(_north_off_test_thread());
    assert_true(c->write_ok);
    c->command_settled = true;
}

/* F：超时路径经真执行适配器/驱动超时到 `<name>.result=2`（从站不应答）。 */
static void test_result_receipt_timeout_via_real_adapter(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
    uv_timer_stop(&c->watchdog);
    c->expect_slave = 99; /* 从站不应答：驱动 2000ms 事务超时 */
    g_north_bus_id = 0;
    g_north_slave_id = 3;

    _north_inject();
    assert_true(_north_wait(c, "VOLTAGE_PHASE_A.result=2", 4000));
    assert_true(_north_wait(c, "VOLTAGE_PHASE_A.ack=1", 3000));
    assert_true(_north_off_test_thread());
    c->command_settled = true;
}

/* F+B：失败路径——已配置但未 attach IO 的总线，引擎早退必须结算成
 * `<name>.result=0`（旧实现这里永无完成回调）。 */
static void test_result_receipt_failure_on_no_io_bus(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
    uv_timer_stop(&c->watchdog);
    assert_int_equal(bus_orchestrator_setup_master(c->orch, 1, "ModbusRTU", 1), 0);
    g_north_bus_id = 1;
    g_north_slave_id = 1;

    _north_inject();
    assert_true(_north_wait(c, "VOLTAGE_PHASE_A.result=0", 1000));
    assert_true(_north_wait(c, "VOLTAGE_PHASE_A.ack=1", 3000));
    assert_true(_north_off_test_thread());
    c->command_settled = true;
}

/* B5：无 IO 总线上连续 18 条下行，每条都必须有结果回执（完成回调释放北向
 * 结果槽）；旧实现第 17 条就因两个池耗尽而拿到 NULL 回调、永无结果。 */
static void test_no_io_burst_releases_north_result_pool(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;
    uv_timer_stop(&c->watchdog);
    assert_int_equal(bus_orchestrator_setup_master(c->orch, 1, "ModbusRTU", 1), 0);
    g_north_bus_id = 1;
    g_north_slave_id = 1;

    const int count = 18;
    for (int i = 0; i < count; i++) {
        _north_inject();
        /* 结果回执由北向 loop 异步发布；第 i+1 条的结果出现即证明它的完成
         * 回调非 NULL 且槽已归还，后面的提交不会因池满被拒。 */
        assert_true(_north_wait_count(c, "VOLTAGE_PHASE_A.result=0", i + 1, 1000));
    }
    assert_int_equal(_north_count("VOLTAGE_PHASE_A.result=0"), count);
    assert_true(_north_wait_count(c, "VOLTAGE_PHASE_A.ack=1", count, 1000));
    assert_true(_north_off_test_thread());
    c->command_settled = true;
}

int main(void) {
    edge_thread_port_install_uv();
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_modbus_rtu_read_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_write_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_write32_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_write_desc_u32_cdab_over_uart_pty, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_write_u64_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_write_slave_route_over_uart_pty, setup,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_result_device_id, setup, teardown),
        cmocka_unit_test_setup_teardown(test_modbus_rtu_hicrc_read_over_uart_pty, setup_hicrc,
                                        teardown),
        cmocka_unit_test_setup_teardown(test_result_receipt_success_via_real_adapter, setup_north,
                                        teardown_north),
        cmocka_unit_test_setup_teardown(test_result_receipt_timeout_via_real_adapter, setup_north,
                                        teardown_north),
        cmocka_unit_test_setup_teardown(test_result_receipt_failure_on_no_io_bus, setup_north,
                                        teardown_north),
        cmocka_unit_test_setup_teardown(test_no_io_burst_releases_north_result_pool, setup_north,
                                        teardown_north),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
