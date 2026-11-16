#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "bus_datahub_adapter.h"
#include "datahub.h"
#include "datahub_priv.h"
#include "driver_manager.h"
#include "edge_api.h"
#include "edge_driver.h"
#include "io_executor.h"
#include "modbus_codec.h"
#include "net_buf.h"

extern const edge_driver_t modbus_rtu_driver;

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    arbiter_t* arb;
    bus_orchestrator_t* orch;
    driver_manager_t* drv_mgr;
    driver_manager_storage_t drv_mgr_storage;
    net_buf_pool_t pool;
    uv_timer_t watchdog;

    int received_count;
    int read_status;
    uint32_t result_tag;
    double result_value;
    bool timed_out;
} loopback_ctx_t;

static void on_data_received(datahub_t* hub, void* user_data) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)user_data;
    pipe_msg_t msg;
    if (datahub_read_shadow(hub, 1001, &msg) == 0) {
        ctx->received_count++;
        uv_stop(&ctx->loop);
    }
}

/* 产品层负责的接线（ADR-002）：驱动只回原生值，回调把它落到自己的 tag 上。 */
static void on_read_done(int status, const pipe_msg_t* result, void* user_data) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)user_data;
    ctx->read_status = status;
    if (status != 0 || !result) {
        uv_stop(&ctx->loop);
        return;
    }
    ctx->result_tag = result->tag_id;
    ctx->result_value = result->payload.val.f;
    datahub_publish(&ctx->hub, result);
}

/* 从站应答的"当前值"由组合根注入，驱动不再读影子表 */
static int mock_resolve(void* ctx, uint32_t point_id, const void* point_desc,
                        size_t point_desc_len, edge_point_value_t* out) {
    (void)ctx;
    (void)point_desc;
    (void)point_desc_len;
    if (point_id != MODBUS_NATIVE(0, 1)) return -1;
    out->raw = 2317;
    out->value = 231.7;
    out->width = 1;
    return 0;
}

/* 失败路径不能挂到 ctest 的 180s 超时：超时自己停循环，断言再报错 */
static void on_watchdog(uv_timer_t* handle) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)handle->data;
    ctx->timed_out = true;
    uv_stop(&ctx->loop);
}

static int setup(void** state) {
    loopback_ctx_t* ctx = calloc(1, sizeof(loopback_ctx_t));
    assert_non_null(ctx);
    assert_int_equal(uv_loop_init(&ctx->loop), 0);

    assert_int_equal(datahub_init(&ctx->hub, &ctx->loop, "/test_loopback", 64, 1024, NULL, 0, NULL,
                                  NULL, NULL),
                     0);
    datahub_subscribe(&ctx->hub, on_data_received, ctx);

    assert_int_equal(arbiter_init(&ctx->arb, &ctx->loop), 0);
    /* 仲裁器默认是 paused；不 resume，命令永远留在队列里，uv_run 也不会返回 */
    arbiter_resume(ctx->arb);

    assert_int_equal(net_buf_pool_init(&ctx->pool, 32, 256, 0, NULL, NULL), 0);
    ctx->drv_mgr = driver_manager_create_in(&ctx->drv_mgr_storage);
    assert_non_null(ctx->drv_mgr);
    assert_int_equal(driver_manager_register(ctx->drv_mgr, &modbus_rtu_driver), 0);

    ctx->orch = bus_orchestrator_create_hub_default(
        &ctx->hub, ctx->arb, driver_manager_session_port_ptr(ctx->drv_mgr), 8, 32, 256);
    assert_non_null(ctx->orch);

    int fds[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    io_handle_t* master_io = io_executor_create_from_fd(&ctx->loop, fds[0], &ctx->pool);
    io_handle_t* slave_io = io_executor_create_from_fd(&ctx->loop, fds[1], &ctx->pool);
    assert_non_null(master_io);
    assert_non_null(slave_io);

    assert_int_equal(bus_orchestrator_attach_io(ctx->orch, 0, master_io), 0);
    assert_int_equal(bus_orchestrator_attach_io(ctx->orch, 1, slave_io), 0);
    assert_int_equal(bus_orchestrator_setup_master(ctx->orch, 0, "ModbusRTU", 1), 0);
    assert_int_equal(bus_orchestrator_setup_slave(ctx->orch, 1, "ModbusRTU", 1), 0);

    static const edge_slave_source_ops_t s_src = {.resolve = mock_resolve, .accept = NULL};
    assert_int_equal(bus_orchestrator_set_slave_source(ctx->orch, 1, &s_src, NULL), 0);

    edge_api_init(ctx->orch);

    assert_int_equal(uv_timer_init(&ctx->loop, &ctx->watchdog), 0);
    ctx->watchdog.data = ctx;
    uv_timer_start(&ctx->watchdog, on_watchdog, 5000, 0);

    *state = ctx;
    return 0;
}

static void test_modbus_loopback_flow(void** state) {
    loopback_ctx_t* ctx = *state;

    /* 原生点位 0（寄存器 0，1 个寄存器），回调要拿回产品 tag 1001 */
    assert_int_equal(edge_api_read_tag_ex(0, "ModbusRTU", 1, MODBUS_NATIVE(0, 1), 1001, 1.0, NULL,
                                          0, ARBITER_PRIO_NORMAL, on_read_done, ctx),
                     0);

    uv_run(&ctx->loop, UV_RUN_DEFAULT);

    assert_false(ctx->timed_out);
    assert_int_equal(ctx->read_status, 0);
    assert_int_equal(ctx->result_tag, 1001);
    assert_true(ctx->result_value == 2317.0);
    assert_int_equal(ctx->received_count, 1);
}

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int teardown(void** state) {
    loopback_ctx_t* ctx = *state;

    if (ctx->orch) bus_orchestrator_destroy(ctx->orch);
    if (ctx->arb) arbiter_destroy(ctx->arb);
    if (ctx->drv_mgr) driver_manager_destroy(ctx->drv_mgr);

    uv_walk(&ctx->loop, on_walk_close, NULL);
    /* 有界排空：uv_run(DEFAULT/阻塞) 在残留句柄上会一直等，CI 就变成 180s 超时 */
    for (int i = 0; i < 200 && uv_loop_alive(&ctx->loop); i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
    }

    datahub_destroy(&ctx->hub);
    net_buf_pool_destroy(&ctx->pool);

    int ret = uv_loop_close(&ctx->loop);
    if (ret != 0) {
        fprintf(stderr, "uv_loop_close failed: %s\n", uv_strerror(ret));
    }
    free(ctx);
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_modbus_loopback_flow, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
