#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_engine.h"
#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "io_executor.h"
#include "net_buf.h"

extern edge_driver_t modbus_rtu_driver;

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    arbiter_t *arb;
    bus_orchestrator_t *orch;
    driver_manager_t *drv_mgr;
    net_buf_pool_t pool;

    int received_count;
} loopback_ctx_t;

static void on_data_received(datahub_t *hub, void *user_data) {
    loopback_ctx_t *ctx = (loopback_ctx_t *)user_data;
    pipe_msg_t msg;
    if (datahub_read_shadow(hub, 1001, &msg) == 0) {
        ctx->received_count++;
        uv_stop(&ctx->loop);
    }
}

static int setup(void **state) {
    loopback_ctx_t *ctx = calloc(1, sizeof(loopback_ctx_t));
    uv_loop_init(&ctx->loop);

    datahub_init(&ctx->hub, &ctx->loop, "/test_loopback", 64, 1024, NULL);
    datahub_subscribe(&ctx->hub, on_data_received, ctx);

    arbiter_init(&ctx->arb, &ctx->loop);

    net_buf_pool_init(&ctx->pool, 32, 256, 0, NULL, NULL);
    ctx->drv_mgr = driver_manager_create();
    driver_manager_register(ctx->drv_mgr, &modbus_rtu_driver);

    ctx->orch = bus_orchestrator_create(&ctx->hub, ctx->arb, ctx->drv_mgr, 8, 32, 256);

    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) return -1;

    io_handle_t *master_io = io_executor_create_from_fd(&ctx->loop, fds[0], &ctx->pool);
    io_handle_t *slave_io = io_executor_create_from_fd(&ctx->loop, fds[1], &ctx->pool);

    bus_orchestrator_attach_io(ctx->orch, 0, master_io);
    bus_orchestrator_attach_io(ctx->orch, 1, slave_io);

    bus_orchestrator_setup_master(ctx->orch, 0, "ModbusRTU", 1);
    bus_orchestrator_setup_slave(ctx->orch, 1, "ModbusRTU", 1);

    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    loopback_ctx_t *ctx = *state;

    bus_orchestrator_destroy(ctx->orch);
    arbiter_destroy(ctx->arb);

    for (int i = 0; i < 20; i++) uv_run(&ctx->loop, UV_RUN_NOWAIT);

    datahub_destroy(&ctx->hub);
    driver_manager_destroy(ctx->drv_mgr);
    net_buf_pool_destroy(&ctx->pool);

    uv_loop_close(&ctx->loop);
    free(ctx);
    return 0;
}

static void test_modbus_full_chain_flow(void **state) {
    loopback_ctx_t *ctx = *state;

    cmd_msg_t cmd = {0};
    cmd.action = CMD_ACTION_POLL;
    cmd.tag_id = 0;  // Modbus 地址 0
    strncpy(cmd.req_id, "TEST-POLL", 31);

    // 通过 Orchestrator 提交到 Arbiter
    bus_orchestrator_submit_cmd(ctx->orch, 0, &cmd);

    uv_timer_t timeout;
    uv_timer_init(&ctx->loop, &timeout);
    uv_timer_start(&timeout, (uv_timer_cb)uv_stop, 1000, 0);

    uv_run(&ctx->loop, UV_RUN_DEFAULT);

    if (!uv_is_closing((uv_handle_t *)&timeout)) {
        uv_timer_stop(&timeout);
        uv_close((uv_handle_t *)&timeout, NULL);
    }

    assert_int_not_equal(ctx->received_count, 0);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_modbus_full_chain_flow, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
