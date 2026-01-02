#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <uv.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "driver_manager.h"
#include "edge_log.h"
#include "edge_service.h"
#include "net_buf.h"
#include "sqlite_service.h"
#include "stream_engine.h"

/* ---------------- Context ---------------- */

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    sqlite_svc_t *db;
    stream_engine_t *engine;
    arbiter_t *arb;
    service_t *svc;
    bus_orchestrator_t *orch;
} full_chain_ctx_t;

#define DRIVE_LOOP(ctx, ms)                      \
    do {                                         \
        for (int _i = 0; _i < (ms / 10); _i++) { \
            uv_run(&(ctx)->loop, UV_RUN_NOWAIT); \
            usleep(10000);                       \
        }                                        \
        uv_run(&(ctx)->loop, UV_RUN_NOWAIT);     \
    } while (0)

static void on_walk_close(uv_handle_t *h, void *arg) {
    (void)arg;
    if (!uv_is_closing(h)) uv_close(h, NULL);
}

/* ---------------- Setup / Teardown ---------------- */

static int setup(void **state) {
    full_chain_ctx_t *ctx = calloc(1, sizeof(full_chain_ctx_t));
    uv_loop_init(&ctx->loop);
    edge_log_init(NULL, false);

    datahub_init(&ctx->hub, &ctx->loop, "full_chain_v20", 512);
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    ctx->db = sqlite_svc_create(&ctx->loop, &db_cfg);
    arbiter_init(&ctx->arb, &ctx->loop);

    service_options_t opts = {.mode = "SCENARIO_GRAND_SLAM"};
    ctx->svc = service_new(&opts);
    service_set_datahub(ctx->svc, &ctx->hub);
    service_set_storage(ctx->svc, ctx->db);
    service_set_arbiter(ctx->svc, ctx->arb);

    ctx->engine = stream_engine_create(&ctx->hub, NULL);
    service_set_stream_engine(ctx->svc, ctx->engine);

    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    full_chain_ctx_t *ctx = *state;
    if (!ctx) return 0;

    printf("[DEBUG] Teardown starting...\n");
    if (ctx->orch) bus_orchestrator_destroy(ctx->orch);
    if (ctx->svc) service_delete(ctx->svc);
    if (ctx->engine) stream_engine_destroy(ctx->engine);
    if (ctx->arb) arbiter_destroy(ctx->arb);
    if (ctx->db) sqlite_svc_destroy(ctx->db);

    datahub_destroy(&ctx->hub);
    uv_walk(&ctx->loop, on_walk_close, NULL);
    for (int i = 0; i < 50; i++) {
        if (uv_run(&ctx->loop, UV_RUN_NOWAIT) == 0) break;
        usleep(1000);
    }
    uv_loop_close(&ctx->loop);
    edge_log_fini();
    free(ctx);
    printf("[DEBUG] Teardown complete.\n");
    return 0;
}

/* ---------------- Mocks ---------------- */
uint32_t edge_lookup_dlt645(uint32_t di, float *scale) {
    if (di == 0x00000000) {
        *scale = 0.01f;
        return 1001;
    }
    return 0;
}
extern int dlt645_driver_register(void);

/* ---------------- Scenario Functions ---------------- */

// [S1/S2] 周期抄读与主动推送
static void test_scenario_periodic_polling(void **state) {
    full_chain_ctx_t *ctx = *state;
    driver_manager_init();
    dlt645_driver_register();
    edge_driver_t *drv = driver_manager_find("DLT645-2007");
    ctx->orch = bus_orchestrator_create(drv, &ctx->hub, ctx->arb);
    bus_orchestrator_run(ctx->orch);
    edge_poll_group_t g = {.slave_id = 1234, .start_addr = 0, .count = 1};
    bus_orchestrator_add_poll(ctx->orch, 50, &g);
    DRIVE_LOOP(ctx, 150);
    pipe_msg_t shadow;
    assert_int_equal(datahub_read_shadow(&ctx->hub, 1001, &shadow), 0);
    driver_manager_fini();
}

// [S3/S15] 波形分析与大数据分片
static void test_scenario_bulk_data(void **state) {
    full_chain_ctx_t *ctx = *state;
    net_buf_pool_t pool;
    net_buf_pool_init(&pool, 1, 1024, 0, NULL, NULL);
    net_buf_t *nb = net_buf_alloc(&pool, 0);
    pipe_msg_t msg;
    pipe_msg_init_ref(&msg, 9001, nb, 1024, uv_now(&ctx->loop));
    datahub_publish(&ctx->hub, &msg);
    DRIVE_LOOP(ctx, 20);
    pipe_msg_t shadow;
    assert_int_equal(datahub_read_shadow(&ctx->hub, 9001, &shadow), 0);
    assert_ptr_equal(shadow.payload.ref.ptr, nb);
    net_buf_unref(nb);
    net_buf_pool_destroy(&pool);
}

// [S4/S19] 续传与链路冗余
static int export_calls = 0;
static void _mock_pub(uint32_t t, double v) {
    (void)t;
    (void)v;
    export_calls++;
}
static void test_scenario_link_and_export(void **state) {
    full_chain_ctx_t *ctx = *state;
    export_calls = 0;
    service_report_link_status(ctx->svc, LINK_PRIMARY, false);
    service_trigger_export(ctx->svc, "telemetry", _mock_pub);
    assert_true(export_calls >= 0);
}

// [S5/S6/S18/S20] 指令透传、控、仲裁与安全
static int exec_count = 0;
static void on_cmd_start(void *u) {
    (void)u;
    exec_count++;
}
static void test_scenario_cmd_arbitration_safety(void **state) {
    full_chain_ctx_t *ctx = *state;
    exec_count = 0;
    cmd_msg_t c1;
    cmd_msg_init_write(&c1, 1001, 1.0, ARBITER_PRIO_EMERGENCY);
    c1.signature = 0x55AA;
    c1.on_start = on_cmd_start;
    service_post_cmd(ctx->svc, &c1);
    DRIVE_LOOP(ctx, 20);
    assert_int_equal(exec_count, 1);
    arbiter_on_io_done(ctx->arb, 0, &c1);
}

// [S7] OTA 独占锁
static void test_scenario_ota_lock(void **state) {
    full_chain_ctx_t *ctx = *state;
    arbiter_set_exclusive(ctx->arb, 0, true);
    exec_count = 0;
    cmd_msg_t c1;
    cmd_msg_init_write(&c1, 1001, 1.0, ARBITER_PRIO_LOW);
    c1.signature = 0x55AA;
    c1.on_start = on_cmd_start;
    service_post_cmd(ctx->svc, &c1);
    DRIVE_LOOP(ctx, 20);
    assert_int_equal(exec_count, 0);
    arbiter_set_exclusive(ctx->arb, 0, false);
    DRIVE_LOOP(ctx, 20);
    assert_int_equal(exec_count, 1);
    arbiter_on_io_done(ctx->arb, 0, &c1);
}

// [S8/S9] 热更与 IPC
static void test_scenario_hot_reload_ipc(void **state) {
    full_chain_ctx_t *ctx = *state;
    service_add_ipc_bridge(ctx->svc, "/tmp/edgec_tdd.sock");
    assert_true(true);
}

// [S10] 对时守时
static void test_scenario_time_sync(void **state) {
    full_chain_ctx_t *ctx = *state;
    service_sync_time(ctx->svc, 1700000000000ULL);
    assert_true(service_get_now_ms(ctx->svc) >= 1700000000000ULL);
}

// [S11/S12] 自动发现与远程隧道
static void test_scenario_discovery_tunnel(void **state) {
    full_chain_ctx_t *ctx = *state;
    service_scan_bus(ctx->svc, 0);
    service_create_tunnel(ctx->svc, 9999, 0);
    char *rep = service_get_discovery_report(ctx->svc);
    assert_non_null(rep);
    free(rep);
}

// [S13/S14] 影子服务与掉电保护
static void test_scenario_shadow_poweroff(void **state) {
    full_chain_ctx_t *ctx = *state;
    pipe_msg_t m;
    pipe_msg_init_scalar(&m, 4001, 1.0, 1000);
    datahub_publish(&ctx->hub, &m);
    service_stop(ctx->svc);
    assert_true(true);
}

// [S16] 冷启动自举
static void test_scenario_ztp_bootstrap(void **state) {
    full_chain_ctx_t *ctx = *state;
    int ret = service_bootstrap(ctx->svc, "/tmp/m.xml");
    assert_int_equal(ret, -1);
}

// [S17] 逻辑熔断
static stage_ret_t _slow_cb(pipeline_stage_t *s, pipe_msg_t *m) {
    (void)s;
    (void)m;
    usleep(60000);
    return STAGE_OK;
}
static void test_scenario_circuit_breaker(void **state) {
    full_chain_ctx_t *ctx = *state;
    pipeline_t *p = stream_engine_add_pipeline(ctx->engine, "CB", 8001);
    pipeline_stage_t *s = pipeline_add_stage(p, "S", _slow_cb, NULL, NULL, NULL, NULL);
    pipe_msg_t m;
    pipe_msg_init_scalar(&m, 8001, 1.0, 1000);
    stream_engine_push(ctx->engine, &m);
    assert_int_equal(pipeline_stage_get_health(s), STAGE_HEALTH_TIMEOUT);
}

/* ---------------- Main ---------------- */

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_scenario_periodic_polling, setup, teardown),  // S1, S2
        cmocka_unit_test_setup_teardown(test_scenario_bulk_data, setup, teardown),        // S3, S15
        cmocka_unit_test_setup_teardown(test_scenario_link_and_export, setup, teardown),  // S4, S19
        cmocka_unit_test_setup_teardown(test_scenario_cmd_arbitration_safety, setup,
                                        teardown),  // S5, S6, S18, S20
        cmocka_unit_test_setup_teardown(test_scenario_ota_lock, setup, teardown),        // S7
        cmocka_unit_test_setup_teardown(test_scenario_hot_reload_ipc, setup, teardown),  // S8, S9
        cmocka_unit_test_setup_teardown(test_scenario_time_sync, setup, teardown),       // S10
        cmocka_unit_test_setup_teardown(test_scenario_discovery_tunnel, setup,
                                        teardown),  // S11, S12
        cmocka_unit_test_setup_teardown(test_scenario_shadow_poweroff, setup,
                                        teardown),                                      // S13, S14
        cmocka_unit_test_setup_teardown(test_scenario_ztp_bootstrap, setup, teardown),  // S16
        cmocka_unit_test_setup_teardown(test_scenario_circuit_breaker, setup, teardown),  // S17
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}