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
    arbiter_t* arb;
    bus_orchestrator_t* orch;
    driver_manager_t* drv_mgr;
    net_buf_pool_t pool;

    int received_count;
} loopback_ctx_t;

static void on_data_received(datahub_t* hub, void* user_data) {
    loopback_ctx_t* ctx = (loopback_ctx_t*)user_data;
    pipe_msg_t msg;
    if (datahub_read_shadow(hub, 1001, &msg) == 0) {
        ctx->received_count++;
        uv_stop(&ctx->loop);
    }
}

static const void* mock_lookup_by_key(uint32_t key) {
    static modbus_map_t m = {.key = 0, .tag_id = 1001, .scaler = 1.0f};
    return (key == 0) ? &m : NULL;
}

static int setup(void** state) {
    loopback_ctx_t* ctx = calloc(1, sizeof(loopback_ctx_t));
    uv_loop_init(&ctx->loop);

    // 1. 初始化 DataHub (注入测试用元数据)
    datahub_init(&ctx->hub, &ctx->loop, "/test_loopback", 64, 1024, NULL, 0, NULL, NULL, NULL);
    datahub_subscribe(&ctx->hub, on_data_received, ctx);

    arbiter_init(&ctx->arb, &ctx->loop);

    net_buf_pool_init(&ctx->pool, 32, 256, 0, NULL, NULL);
    ctx->drv_mgr = driver_manager_create();
    driver_manager_register(ctx->drv_mgr, &modbus_rtu_driver);

    ctx->orch = bus_orchestrator_create(&ctx->hub, ctx->arb, ctx->drv_mgr, 8, 32, 256);

    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) return -1;

    io_handle_t* master_io = io_executor_create_from_fd(&ctx->loop, fds[0], &ctx->pool);
    io_handle_t* slave_io = io_executor_create_from_fd(&ctx->loop, fds[1], &ctx->pool);

    bus_orchestrator_attach_io(ctx->orch, 0, master_io);
    bus_orchestrator_attach_io(ctx->orch, 1, slave_io);

    // 使用新的注入 API
    driver_contract_injection_t inject = {.lookup_by_key = mock_lookup_by_key};
    bus_orchestrator_setup_master(ctx->orch, 0, "ModbusRTU", 1, &inject);
    bus_orchestrator_setup_slave(ctx->orch, 1, "ModbusRTU", 1, &inject);

    *state = ctx;
    return 0;
}

static void test_modbus_loopback_flow(void** state) {
    loopback_ctx_t* ctx = *state;

    // 构造一个读请求 (模拟上层业务)
    cmd_msg_t cmd = {0};
    cmd.action = CMD_ACTION_READ;
    cmd.tag_id = 0;  // Modbus Address 0 -> Mapped to 1001 in driver logic
    cmd.timeout_ms = 1000;
    strncpy(cmd.protocol, "ModbusRTU", 15);

    // 提交给 Arbiter (它会调度 Orchestrator -> Master Engine -> Driver)
    bus_orchestrator_submit_cmd(ctx->orch, 0, &cmd);

    // 运行循环，等待数据流转完成
    uv_run(&ctx->loop, UV_RUN_DEFAULT);

    assert_int_not_equal(ctx->received_count, 0);
}

static void on_walk_close(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int teardown(void** state) {
    loopback_ctx_t* ctx = *state;

    // 1. 业务层销毁
    if (ctx->orch) bus_orchestrator_destroy(ctx->orch);
    if (ctx->arb) arbiter_destroy(ctx->arb);
    if (ctx->drv_mgr) driver_manager_destroy(ctx->drv_mgr);

    // 2. 彻底排空所有 Handles
    uv_walk(&ctx->loop, on_walk_close, NULL);

    // 运行知道没有任何活跃或关闭中的 handle
    while (uv_run(&ctx->loop, UV_RUN_ONCE)) {
        // 继续运行直到 loop 干净
    }

    // 3. 静态资源回收
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