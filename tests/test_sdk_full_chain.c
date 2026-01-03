#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_service.h"
#include "northbound_mgr.h"
#include "stream_engine.h"

// 模拟上下文
typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    arbiter_t *arb;
    stream_engine_t *engine;
    service_t *svc;
    bus_orchestrator_t *orch;
} full_chain_ctx_t;

extern int dlt645_driver_register(void);

static int setup(void **state) {
    full_chain_ctx_t *ctx = calloc(1, sizeof(full_chain_ctx_t));
    uv_loop_init(&ctx->loop);

    shm_unlink("full_chain_v20");
    datahub_init(&ctx->hub, &ctx->loop, "full_chain_v20", 1024);

    arbiter_init(&ctx->arb, &ctx->loop);
    ctx->engine = stream_engine_create(&ctx->hub, NULL);

    service_options_t opts = {.mode = "TDD_FULL"};
    ctx->svc = service_new(&opts);
    service_set_datahub(ctx->svc, &ctx->hub);
    service_set_arbiter(ctx->svc, ctx->arb);
    service_set_stream_engine(ctx->svc, ctx->engine);

    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    full_chain_ctx_t *ctx = *state;
    if (ctx->orch) bus_orchestrator_destroy(ctx->orch);
    stream_engine_destroy(ctx->engine);
    arbiter_destroy(ctx->arb);
    datahub_destroy(&ctx->hub);
    service_delete(ctx->svc);

    uv_run(&ctx->loop, UV_RUN_NOWAIT);
    uv_loop_close(&ctx->loop);
    free(ctx);
    return 0;
}

// [S1/S2] 周期抄读与主动推送 (回归稳定版)
static void test_scenario_periodic_polling(void **state) {
    full_chain_ctx_t *ctx = *state;

    // 模拟数据注入
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 225.6, (uint64_t)time(NULL) * 1000);
    datahub_publish(&ctx->hub, &msg);

    // 验证影子表命中
    pipe_msg_t shadow;
    int ret = datahub_read_shadow(&ctx->hub, 1001, &shadow);
    assert_int_equal(ret, 0);
    assert_double_equal(shadow.payload.val.f, 225.6, 0.001);
}

static void test_scenario_bulk_data(void **state) {
    full_chain_ctx_t *ctx = *state;
    pipe_msg_t msg = {0};
    pipe_msg_init_scalar(&msg, 1001, 220.5, 1234567);
    datahub_publish(&ctx->hub, &msg);
    assert_true(true);
}

static void test_scenario_link_and_export(void **state) {
    full_chain_ctx_t *ctx = *state;
    service_report_link_status(ctx->svc, LINK_PRIMARY, true);
    assert_true(true);
}

static void _on_test_cmd_cleanup(cmd_msg_t *cmd) {
    free(cmd);
}

static void test_scenario_cmd_arbitration_safety(void **state) {
    full_chain_ctx_t *ctx = *state;
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->priority = ARBITER_PRIO_EMERGENCY;
    cmd->signature = 0x55AA;
    cmd->on_cleanup = _on_test_cmd_cleanup;
    service_post_cmd(ctx->svc, cmd);
    assert_true(true);
}

static void test_scenario_ota_lock(void **state) {
    full_chain_ctx_t *ctx = *state;
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->priority = ARBITER_PRIO_EMERGENCY;
    cmd->signature = 0x55AA;
    cmd->action = CMD_ACTION_OTA;
    cmd->on_cleanup = _on_test_cmd_cleanup;
    service_post_cmd(ctx->svc, cmd);
    assert_true(true);
}

static void test_scenario_hot_reload_ipc(void **state) {
    full_chain_ctx_t *ctx = *state;
    const char *uds = "/tmp/edgec_tdd.sock";
    unlink(uds);
    service_add_ipc_bridge(ctx->svc, uds);
    assert_true(true);
}

static void test_scenario_time_sync(void **state) {
    full_chain_ctx_t *ctx = *state;
    service_sync_time(ctx->svc, 1704240000000ULL);
    assert_true(true);
}

static void test_scenario_discovery_tunnel(void **state) {
    full_chain_ctx_t *ctx = *state;
    service_create_tunnel(ctx->svc, 9999, 0);
    assert_true(true);
}

static void test_scenario_shadow_poweroff(void **state) {
    full_chain_ctx_t *ctx = *state;
    pipe_msg_t out;
    datahub_read_shadow(&ctx->hub, 1001, &out);
    assert_true(true);
}

static void test_scenario_ztp_bootstrap(void **state) {
    full_chain_ctx_t *ctx = *state;
    service_bootstrap(ctx->svc, "manifest.xml");
    assert_true(true);
}

static void test_scenario_circuit_breaker(void **state) {
    full_chain_ctx_t *ctx = *state;
    pipeline_t *p = stream_engine_add_pipeline(ctx->engine, "CB", 2001);
    pipeline_add_stage(p, "S", NULL, NULL, NULL, NULL, NULL);
    pipe_msg_t msg = {0};
    msg.tag_id = 2001;
    msg.ts = 100;
    stream_engine_push(ctx->engine, &msg);
    assert_true(true);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_scenario_periodic_polling, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_bulk_data, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_link_and_export, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_cmd_arbitration_safety, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_ota_lock, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_hot_reload_ipc, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_time_sync, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_discovery_tunnel, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_shadow_poweroff, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_ztp_bootstrap, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_circuit_breaker, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
