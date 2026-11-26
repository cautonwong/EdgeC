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
#include "driver_manager.h"
#include "driver_ydt1363.h"
#include "edge_api.h"
#include "edge_common.h"
#include "iems_southbound_adapter.h"
#include "io_handle_priv.h"
#include "ydt1363_codec.h"

/* PTY 对端扮演 YDT1363 设备：校验请求帧的 VER/ADR/CID1/CID2/INFO 与校验和，
 * 回一帧合法应答。驱动必须按 ydt1363_frame_len 切帧、校验并解出 AB 序的
 * 2 字节值；scale 0.1 后回调应当给出 53.0 V / 53.1 V。
 *
 * 第二个读请求的应答前面塞 3 字节垃圾，证明驱动逐字节重同步后仍解出正确值。 */

#define YDT_VER 0x22
#define YDT_ADR 0x01
#define YDT_CID1 0x46
#define YDT_CID2_READ 0x42
#define YDT_TAG 7001
#define YDT_SCALE 0.1
#define YDT_RAW_FIRST 530  /* 53.0 V */
#define YDT_RAW_SECOND 531 /* 53.1 V */

#define YDT_JUNK_LEN 3
#define YDT_MAX_FRAME 64

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

    ydt1363_point_desc_t desc;
    uint8_t req[YDT_MAX_FRAME];
    size_t req_len;
    int req_frames;
    bool req_ok;
    bool frame_bad;
    bool inject_junk;
    bool timed_out;

    int submits;
    int cb_count;
    int cb_status;
    uint32_t cb_tag;
    double cb_value;
} pty_ctx_t;

static void _dump_frame(const pty_ctx_t *c, const char *why) {
    fprintf(stderr,
            "[ydt1363-pty] %s: frames=%d req_ok=%d inject_junk=%d timed_out=%d "
            "status=%d value=%.6f\n",
            why, c->req_frames, c->req_ok, c->inject_junk, c->timed_out, c->cb_status, c->cb_value);
    fprintf(stderr, "[ydt1363-pty] rx len=%zu:", c->req_len);
    for (size_t i = 0; i < c->req_len; i++) fprintf(stderr, " %02X", c->req[i]);
    fprintf(stderr, "\n");
}

static void _mk_buf(net_buf_t *nb, uint8_t *mem, size_t cap) {
    memset(nb, 0, sizeof(*nb));
    net_buf_simple_init_with_data(&nb->b, mem, cap);
}

static ydt1363_point_desc_t _point_desc(void) {
    ydt1363_point_desc_t d;
    memset(&d, 0, sizeof(d));
    d.ver = YDT_VER;
    d.adr = YDT_ADR;
    d.cid1 = YDT_CID1;
    d.cid2 = YDT_CID2_READ;
    d.info_len = 1;
    d.info[0] = 0x01; /* 读命令的 COMMAND INFO，对端只校验原样回显 */
    d.info_offset = 0;
    d.type = YDT1363_TYPE_UINT;
    d.order = YDT1363_ORDER_AB;
    return d;
}

/* 从站：收满一帧请求，核对字节与描述一致，再按 req_frames 回值。
 * 第二个应答前面塞垃圾，其余保持合法。 */
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

    int total = ydt1363_frame_len(c->req, c->req_len);
    if (total < 0) {
        c->frame_bad = true;
        _dump_frame(c, "bad request header");
        return;
    }
    if (total == 0 || (size_t)total > c->req_len) return;
    if (c->req_len != (size_t)total) {
        c->frame_bad = true;
        _dump_frame(c, "request trailing bytes");
        return;
    }

    uint8_t expected[YDT_MAX_FRAME];
    net_buf_t eb;
    _mk_buf(&eb, expected, sizeof(expected));
    if (ydt1363_encode_read(&c->desc, &eb) != 0) {
        c->frame_bad = true;
        _dump_frame(c, "expected request encode failed");
        return;
    }

    c->req_frames++;
    c->req_ok = eb.b.len == c->req_len && memcmp(eb.b.data, c->req, c->req_len) == 0;
    if (!c->req_ok) {
        c->frame_bad = true;
        _dump_frame(c, "unexpected request frame");
        return;
    }

    uint16_t raw = (c->req_frames == 1) ? YDT_RAW_FIRST : YDT_RAW_SECOND;
    uint8_t info[2] = {(uint8_t)(raw >> 8), (uint8_t)(raw & 0xFF)};
    uint8_t rsp[YDT_MAX_FRAME];
    net_buf_t rb;
    _mk_buf(&rb, rsp, sizeof(rsp));
    if (ydt1363_encode(YDT_VER, YDT_ADR, YDT_CID1, YDT1363_RTN_NORMAL, info, sizeof(info), &rb) !=
        YDT1363_OK) {
        c->frame_bad = true;
        _dump_frame(c, "response encode failed");
        return;
    }

    static const uint8_t junk[YDT_JUNK_LEN] = {0x00, 0xAA, 0x55};
    uint8_t stream[YDT_JUNK_LEN + YDT_MAX_FRAME];
    size_t stream_len = rb.b.len;
    memcpy(stream, rb.b.data, rb.b.len);
    if (c->inject_junk && c->req_frames == 2) {
        memcpy(stream, junk, sizeof(junk));
        memcpy(stream + sizeof(junk), rb.b.data, rb.b.len);
        stream_len += sizeof(junk);
    }

    if (write(c->master_fd, stream, stream_len) != (ssize_t)stream_len) {
        c->frame_bad = true;
        _dump_frame(c, "response write failed");
        return;
    }
    c->req_len = 0; /* 后面还会来一条请求 */
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

static void _submit_read(pty_ctx_t *c) {
    c->submits++;
    assert_int_equal(edge_api_read_tag_ex(0, "YDT1363", 0, 0, 0, YDT_TAG, YDT_SCALE, &c->desc,
                                          sizeof(c->desc), ARBITER_PRIO_NORMAL, _on_read_done, c),
                     0);
}

static int setup(void **state) {
    pty_ctx_t *c = calloc(1, sizeof(*c));
    assert_non_null(c);
    c->master_fd = -1;
    c->desc = _point_desc();

    int master = posix_openpt(O_RDWR | O_NOCTTY | O_NONBLOCK);
    assert_true(master >= 0);
    assert_int_equal(grantpt(master), 0);
    assert_int_equal(unlockpt(master), 0);
    const char *slave_name = ptsname(master);
    assert_non_null(slave_name);
    c->master_fd = master;

    assert_int_equal(uv_loop_init(&c->loop), 0);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_ydt1363_pty_%d", (int)getpid());
    assert_int_equal(datahub_init(&c->hub, &c->loop, shm_name, 16, 32, NULL, 0, NULL, NULL, NULL),
                     0);

    assert_int_equal(arbiter_init(&c->arb, &c->loop), 0);
    arbiter_resume(c->arb);

    c->drv_mgr = driver_manager_create_in(&c->drv_mgr_storage);
    assert_non_null(c->drv_mgr);
    extern int ydt1363_driver_register(driver_manager_t * mgr);
    assert_int_equal(ydt1363_driver_register(c->drv_mgr), 0);
    assert_non_null(driver_manager_find(c->drv_mgr, "YDT1363"));

    c->orch = bus_orchestrator_create_hub_default(
        &c->hub, c->arb, driver_manager_session_port_ptr(c->drv_mgr), 2, 8, 256);
    assert_non_null(c->orch);
    datahub_bind_io_pool(&c->hub, bus_orchestrator_get_pool(c->orch));
    edge_api_init(c->orch);

    assert_int_equal(bus_orchestrator_setup_master(c->orch, 0, "YDT1363", 0), 0);

    assert_int_equal(setenv("OMNIS_BUS_0_DEV", slave_name, 1), 0);
    assert_int_equal(setenv("OMNIS_BUS_0_BAUD", "9600", 1), 0);

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

static void test_ydt1363_read_over_uart_pty(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    _submit_read(c);
    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->req_ok)
        _dump_frame(c, "ydt1363 read did not round-trip");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 1);
    assert_true(c->req_ok);
    assert_int_equal(c->req_len, 0);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_tag, YDT_TAG);
    assert_true(fabs(c->cb_value - (double)YDT_RAW_FIRST * YDT_SCALE) < 1e-9);
}

/* 第二个应答前塞 3 字节垃圾：驱动先整段丢弃，再按帧长切出合法帧。
 * 两次的值不同，重同步后解出的必须是新值，不能是上一次回调的残留。 */
static void test_ydt1363_resync_after_leading_garbage(void **state) {
    pty_ctx_t *c = (pty_ctx_t *)*state;

    _submit_read(c);
    uv_run(&c->loop, UV_RUN_DEFAULT);
    if (c->timed_out || c->cb_status != 0 || !c->req_ok)
        _dump_frame(c, "first ydt1363 read did not round-trip");
    assert_false(c->timed_out);
    assert_int_equal(c->cb_count, 1);
    assert_int_equal(c->cb_status, 0);
    assert_true(fabs(c->cb_value - (double)YDT_RAW_FIRST * YDT_SCALE) < 1e-9);

    c->inject_junk = true;
    uv_timer_start(&c->watchdog, _on_watchdog, 4000, 0);
    _submit_read(c);
    uv_run(&c->loop, UV_RUN_DEFAULT);

    if (c->timed_out || c->cb_status != 0 || !c->req_ok)
        _dump_frame(c, "junk-prefixed ydt1363 read did not resync");

    assert_false(c->timed_out);
    assert_int_equal(c->req_frames, 2);
    assert_true(c->req_ok);
    assert_int_equal(c->cb_count, 2);
    assert_int_equal(c->cb_status, 0);
    assert_int_equal(c->cb_tag, YDT_TAG);
    assert_true(fabs(c->cb_value - (double)YDT_RAW_SECOND * YDT_SCALE) < 1e-9);
}

/* 解析器的负例不依赖引擎超时：截断帧等下一包，坏 CHKSUM 只丢 1 字节。 */
static void test_ydt1363_decode_negative_frames(void **state) {
    (void)state;
    ydt1363_point_desc_t d = _point_desc();
    uint8_t info[2] = {(uint8_t)(YDT_RAW_FIRST >> 8), (uint8_t)(YDT_RAW_FIRST & 0xFF)};
    uint8_t frame[YDT_MAX_FRAME];
    net_buf_t nb;
    _mk_buf(&nb, frame, sizeof(frame));
    assert_int_equal(
        ydt1363_encode(YDT_VER, YDT_ADR, YDT_CID1, YDT1363_RTN_NORMAL, info, sizeof(info), &nb),
        YDT1363_OK);
    size_t total = nb.b.len;
    size_t consumed = 123;
    double value = 0.0;

    /* 合法帧 */
    assert_int_equal(ydt1363_decode_rsp(&d, frame, total, &consumed, &value), 0);
    assert_int_equal(consumed, total);
    assert_true(fabs(value - (double)YDT_RAW_FIRST) < 1e-9);

    /* 截断：不能消费，等下一包 */
    consumed = 123;
    assert_int_equal(ydt1363_decode_rsp(&d, frame, total - 1, &consumed, &value),
                     EDGE_ERR_INCOMPLETE);
    assert_int_equal(consumed, 0);

    /* CHKSUM 坏：只丢 1 字节重扫 */
    uint8_t bad[YDT_MAX_FRAME];
    memcpy(bad, frame, total);
    bad[total - 5] = (bad[total - 5] == '0') ? '1' : '0';
    consumed = 0;
    assert_int_equal(ydt1363_decode_rsp(&d, bad, total, &consumed, &value), EDGE_ERR_INCOMPLETE);
    assert_int_equal(consumed, 1);

    /* 前导垃圾 3 字节，然后是好帧 */
    uint8_t stream[YDT_JUNK_LEN + YDT_MAX_FRAME];
    static const uint8_t junk[YDT_JUNK_LEN] = {0x00, 0xAA, 0x55};
    memcpy(stream, junk, sizeof(junk));
    memcpy(stream + sizeof(junk), frame, total);
    consumed = 0;
    assert_int_equal(ydt1363_decode_rsp(&d, stream, sizeof(junk) + total, &consumed, &value),
                     EDGE_ERR_INCOMPLETE);
    assert_int_equal(consumed, YDT_JUNK_LEN);
    consumed = 0;
    assert_int_equal(ydt1363_decode_rsp(&d, stream + sizeof(junk), total, &consumed, &value), 0);
    assert_int_equal(consumed, total);

    /* ADR 不符：不能当成自己的应答 */
    net_buf_t other;
    _mk_buf(&other, bad, sizeof(bad));
    assert_int_equal(ydt1363_encode(YDT_VER, YDT_ADR + 1, YDT_CID1, YDT1363_RTN_NORMAL, info,
                                    sizeof(info), &other),
                     YDT1363_OK);
    consumed = 0;
    assert_int_equal(ydt1363_decode_rsp(&d, bad, other.b.len, &consumed, &value),
                     EDGE_ERR_INCOMPLETE);
    assert_int_equal(consumed, 1);

    /* RTN 非 0：帧完好但设备报错 */
    _mk_buf(&nb, frame, sizeof(frame));
    assert_int_equal(ydt1363_encode(YDT_VER, YDT_ADR, YDT_CID1, 0x02, info, sizeof(info), &nb),
                     YDT1363_OK);
    consumed = 0;
    assert_int_equal(ydt1363_decode_rsp(&d, frame, nb.b.len, &consumed, &value), -1);
    assert_int_equal(consumed, nb.b.len);

    /* INFO 只有 1 字节，AB 取值窗口越界：配置错，立即失败 */
    _mk_buf(&nb, frame, sizeof(frame));
    assert_int_equal(ydt1363_encode(YDT_VER, YDT_ADR, YDT_CID1, YDT1363_RTN_NORMAL, info, 1, &nb),
                     YDT1363_OK);
    consumed = 0;
    assert_int_equal(ydt1363_decode_rsp(&d, frame, nb.b.len, &consumed, &value), -1);
    assert_int_equal(consumed, nb.b.len);
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

    if (c->cb_count < c->submits && c->orch) {
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
        cmocka_unit_test_setup_teardown(test_ydt1363_read_over_uart_pty, setup, teardown),
        cmocka_unit_test_setup_teardown(test_ydt1363_resync_after_leading_garbage, setup, teardown),
        cmocka_unit_test(test_ydt1363_decode_negative_frames),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
