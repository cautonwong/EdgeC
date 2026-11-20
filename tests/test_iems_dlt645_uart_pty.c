#define _GNU_SOURCE

#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
#include "dlt645_codec.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "iems_driver_adapter.h"
#include "iems_execution_adapter.h"
#include "iems_southbound_adapter.h"
#include "io_handle_priv.h"

/* DLT645 的电表地址来自协议元数据，DI 与量纲来自产品点表。这里用 PTY 模拟
 * 一台表计，请求帧必须带正确的 6 字节表地址与 DI，应答沿同一 fd 回去。
 * 表地址登记、DI 编码、BCD 解码、回调量纲任何一环断掉都会红。 */

#define DLT645_READ_REQ_LEN 16
#define DLT645_WRITE_REQ_LEN 18
#define METER_DI 0x02010100u
#define METER_RAW 2201 /* 220.1 V，BCD 0x01 0x22 */
#define METER_SCALE 0.1
#define METER_TAG 1001

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

    uint8_t addr_bcd[6];
    uint8_t req[32];
    size_t req_len;
    int req_frames;
    bool addr_ok;
    bool di_ok;
    bool frame_bad;
    bool responded;
    bool timed_out;
    bool submitted; /* 用例提过命令：teardown 要等引擎结算 */

    uint8_t write_frame[32];
    size_t write_frame_len;
    int write_frames;
    bool write_ok;

    int cb_count;
    int cb_status;
    uint32_t cb_tag;
    double cb_value;
} pty_ctx_t;

static void _dump_frame(const pty_ctx_t *c, const char *why) {
    fprintf(stderr,
            "[dlt645-pty] %s: frames=%d responded=%d timed_out=%d addr_ok=%d di_ok=%d "
            "status=%d value=%.6f\n",
            why, c->req_frames, c->responded, c->timed_out, c->addr_ok, c->di_ok, c->cb_status,
            c->cb_value);
    fprintf(stderr, "[dlt645-pty] rx:");
    for (size_t i = 0; i < c->req_len; i++) fprintf(stderr, " %02X", c->req[i]);
    fprintf(stderr, "\n");
}

static uint32_t _di_of(const uint8_t *frame) {
    uint32_t di = 0;
    for (int i = 0; i < 4; i++) di |= (uint32_t)(uint8_t)(frame[10 + i] - 0x33) << (8 * i);
    return di;
}

static void _put_head(uint8_t *frame, size_t *p, const uint8_t *addr, uint8_t ctrl, uint8_t l) {
    frame[(*p)++] = 0x68;
    memcpy(frame + *p, addr, 6);
    *p += 6;
    frame[(*p)++] = 0x68;
    frame[(*p)++] = ctrl;
    frame[(*p)++] = l;
}

static void _put_di(uint8_t *frame, size_t *p, uint32_t di) {
    for (int i = 0; i < 4; i++) frame[(*p)++] = (uint8_t)(((di >> (8 * i)) & 0xFF) + 0x33);
}

static void _seal(uint8_t *frame, size_t *p) {
    frame[*p] = dlt645_calculate_cs(frame, *p);
    (*p)++;
    frame[(*p)++] = 0x16;
}

/* 从站：按控制码应答读（0x11）与写（0x14）。写帧必须带正确表地址、DI 与
 * BCD 2201；任何一项不对就不回，让主站按超时失败。 */
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
    if (c->req_len < 10) return;
    if (c->req[0] != 0x68 || c->req[7] != 0x68) {
        c->frame_bad = true;
        _dump_frame(c, "bad frame head");
        return;
    }
    size_t total = (size_t)c->req[9] + 12;
    if (c->req_len < total) return;
    if (c->req[total - 1] != 0x16 ||
        dlt645_calculate_cs(c->req, total - 2) != c->req[total - 2]) {
        c->frame_bad = true;
        _dump_frame(c, "bad checksum");
        return;
    }

    uint8_t ctrl = c->req[8];
    bool addr_ok = memcmp(c->req + 1, c->addr_bcd, sizeof(c->addr_bcd)) == 0;

    if (ctrl == DLT645_CTRL_WRITE_DATA) {
        uint8_t b0 = (uint8_t)(c->req[14] - 0x33);
        uint8_t b1 = (uint8_t)(c->req[15] - 0x33);
        int32_t raw = (int32_t)((b0 >> 4) * 10 + (b0 & 0x0F)) +
                      (int32_t)((b1 >> 4) * 10 + (b1 & 0x0F)) * 100;

        c->req_frames++;
        c->write_frames++;
        c->write_frame_len = total;
        memcpy(c->write_frame, c->req, total);
        c->addr_ok = addr_ok;
        c->di_ok = _di_of(c->req) == METER_DI;
        c->write_ok = addr_ok && c->di_ok && c->req[9] == 6 && raw == METER_RAW;
        if (!c->write_ok) {
            c->frame_bad = true;
            _dump_frame(c, "unexpected write frame");
            return;
        }

        uint8_t ack[16];
        size_t p = 0;
        _put_head(ack, &p, c->addr_bcd, DLT645_CTRL_WRITE_RESPONSE, 4);
        _put_di(ack, &p, METER_DI);
        _seal(ack, &p);
        if (write(c->master_fd, ack, p) == (ssize_t)p) {
            c->responded = true;
            c->req_len = 0; /* 后面可能还有一条读要应答 */
        } else {
            c->frame_bad = true;
            _dump_frame(c, "write ack failed");
        }
        return;
    }

    if (ctrl != DLT645_CTRL_READ_DATA) {
        c->frame_bad = true;
        _dump_frame(c, "unexpected control code");
        return;
    }

    c->req_frames++;
    c->addr_ok = addr_ok;
    c->di_ok = _di_of(c->req) == METER_DI;
    if (!c->addr_ok || !c->di_ok || c->req[9] != 4) {
        c->frame_bad = true;
        _dump_frame(c, "unexpected read frame");
        return;
    }

    uint8_t resp[24];
    size_t p = 0;
    _put_head(resp, &p, c->addr_bcd, DLT645_CTRL_READ_RESPONSE, 6); /* DI 4 + 数据 2 */
    _put_di(resp, &p, METER_DI);
    /* 2201 → BCD 0x01 0x22，再按 DLT645 每字节 +0x33 */
    resp[p++] = (uint8_t)((((METER_RAW % 100) / 10) << 4 | (METER_RAW % 10)) + 0x33);
    resp[p++] = (uint8_t)(((((METER_RAW / 100) % 100) / 10) << 4 | ((METER_RAW / 100) % 10)) + 0x33);
    _seal(resp, &p);
    if (write(c->master_fd, resp, p) == (ssize_t)p) {
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

static void _on_watchdog(uv_timer_t *handle) {
    pty_ctx_t *c = (pty_ctx_t *)handle->data;
    c->timed_out = true;
    uv_stop(&c->loop);
}

static int setup(void **state) {
    pty_ctx_t *c = calloc(1, sizeof(*c));
    assert_non_null(c);
    c->master_fd = -1;
    /* 表地址 6 字节 BCD，低字节在前；测试里取一个可读的组合 */
    static const uint8_t addr[6] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12};
    memcpy(c->addr_bcd, addr, sizeof(addr));

    int master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(master >= 0);
    assert_int_equal(grantpt(master), 0);
    assert_int_equal(unlockpt(master), 0);
    const char *slave_name = ptsname(master);
    assert_non_null(slave_name);
    c->master_fd = master;

    assert_int_equal(uv_loop_init(&c->loop), 0);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_dlt645_pty_%d", (int)getpid());
    assert_int_equal(datahub_init(&c->hub, &c->loop, shm_name, 16, 32, NULL, 0, NULL, NULL, NULL),
                     0);

    assert_int_equal(arbiter_init(&c->arb, &c->loop), 0);
    arbiter_resume(c->arb);

    c->drv_mgr = driver_manager_create_in(&c->drv_mgr_storage);
    assert_non_null(c->drv_mgr);
    extern int dlt645_driver_register(driver_manager_t * mgr);
    assert_int_equal(dlt645_driver_register(c->drv_mgr), 0);

    c->orch = bus_orchestrator_create_hub_default(
        &c->hub, c->arb, driver_manager_session_port_ptr(c->drv_mgr), 2, 8, 256);
    assert_non_null(c->orch);
    datahub_bind_io_pool(&c->hub, bus_orchestrator_get_pool(c->orch));
    edge_api_init(c->orch);
    /* 表地址按 slave_id 索引；6 字节 BCD 数组只有一条，slave_id 0 */
    edge_api_set_protocol_meta("DLT645", c->addr_bcd, 1);

    assert_int_equal(bus_orchestrator_setup_master(c->orch, 0, "DLT645", 0), 0);

    assert_int_equal(setenv("OMNIS_BUS_0_DEV", slave_name, 1), 0);
    assert_int_equal(setenv("OMNIS_BUS_0_BAUD", "2400", 1), 0);

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
    assert_int_equal(uv_timer_start(&c->watchdog, _on_watchdog, 4000, 0), 0);

    *state = c;
    return 0;
}

static void test_dlt645_read_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    assert_int_equal(
        edge_api_read_tag_ex(0, "DLT645", 0, METER_DI, METER_TAG, METER_SCALE, NULL, 0,
                             ARBITER_PRIO_NORMAL, _on_read_done, c),
        0);
    c->submitted = true;

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->addr_ok || !c->di_ok)
        _dump_frame(c, "dlt645 request did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->addr_ok);
    assert_true(c->di_ok);
    assert_int_equal(c->req_len, DLT645_READ_REQ_LEN);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_tag, METER_TAG);
    assert_true(fabs(c->cb_value - (double)METER_RAW * METER_SCALE) < 1e-9);
}

/* P1 回归：北向写 220.1 V 必须落到 DI 0x02010100、BCD 2201、2 字节宽度，
 * 表计回写应答。旧实现 point_desc 为 NULL 时驱动拒绝，总线上根本没有写帧。 */
static void test_dlt645_write_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    iems_execution_port_t port = iems_execution_port_create(NULL);
    assert_non_null(port.write_tag);
    assert_int_equal(port.write_tag(port.ctx, 0, 0, METER_TAG, 220.1, "DLT645"), 0);
    c->submitted = true;
    /* 同一总线上的读作为屏障：写事务结算后才轮到它，循环在回调里停 */
    assert_int_equal(edge_api_read_tag_ex(0, "DLT645", 0, METER_DI, METER_TAG, METER_SCALE,
                                          NULL, 0, ARBITER_PRIO_NORMAL, _on_read_done, c),
                     0);

    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->write_ok)
        _dump_frame(c, "dlt645 write did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->write_frames, 1);
    assert_true(c->write_ok);
    assert_int_equal(c->write_frame_len, DLT645_WRITE_REQ_LEN);
    assert_int_equal(c->write_frame[8], DLT645_CTRL_WRITE_DATA);
    assert_int_equal(c->write_frame[9], 6);
    assert_int_equal(c->write_frame[14], (uint8_t)(0x01 + 0x33));
    assert_int_equal(c->write_frame[15], (uint8_t)(0x22 + 0x33));
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
}

/* 点表宽度 3 字节（1003 电网功率）645 驱动不支持：提交前拒绝，总线上不应出现写帧。 */
static void test_dlt645_write_rejects_unknown_width(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    iems_execution_port_t port = iems_execution_port_create(NULL);
    assert_non_null(port.write_tag);
    assert_int_equal(port.write_tag(port.ctx, 0, 0, IEMS_TAG_GRID_POWER, 12.3, "DLT645"), -1);
    assert_int_equal(c->write_frames, 0);
    assert_int_equal(c->req_frames, 0);
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

    if (c->cb_count == 0 && c->submitted && c->orch) {
        /* 用例提前停表时命令还挂在引擎里，先放行驱动自己的事务超时 */
        c->timed_out = false;
        uv_timer_start(&c->watchdog, _on_watchdog, 4000, 0);
        uv_run(&c->loop, UV_RUN_DEFAULT);
        uv_timer_stop(&c->watchdog);
    }

    io_handle_t *h = bus_orchestrator_get_io(c->orch, 0);
    if (h) h->ops->destroy(h);
    if (c->orch) bus_orchestrator_destroy(c->orch);
    if (c->arb) arbiter_destroy(c->arb);
    if (c->drv_mgr) driver_manager_destroy(c->drv_mgr);

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
        cmocka_unit_test_setup_teardown(test_dlt645_read_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dlt645_write_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_dlt645_write_rejects_unknown_width, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
