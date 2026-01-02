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

/* ---------------- context ---------------- */

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
            uv_sleep(10);                        \
        }                                        \
        uv_run(&(ctx)->loop, UV_RUN_NOWAIT);     \
    } while (0)

/* ---------------- setup / teardown ---------------- */

static int setup(void **state) {
    full_chain_ctx_t *ctx = calloc(1, sizeof(full_chain_ctx_t));
    uv_loop_init(&ctx->loop);
    edge_log_init(NULL, false);

    datahub_init(&ctx->hub, &ctx->loop, "full_chain_v5", 256);
    sqlite_config_t db_cfg = {.strategy = STRATEGY_LITE, .db_path = ":memory:"};
    ctx->db = sqlite_svc_create(&ctx->loop, &db_cfg);
    arbiter_init(&ctx->arb, &ctx->loop);

    service_options_t opts = {.mode = "TDD_FULL_CHAIN"};
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

    if (ctx->orch) bus_orchestrator_destroy(ctx->orch);
    if (ctx->engine) stream_engine_destroy(ctx->engine);
    if (ctx->svc) service_delete(ctx->svc);

    for (int i = 0; i < 100; i++) uv_run(&ctx->loop, UV_RUN_NOWAIT);

    if (ctx->orch) free(ctx->orch);  // 物理隔离释放

    if (ctx->arb) {
        arbiter_shutdown(ctx->arb);
        arbiter_destroy(ctx->arb);
    }
    if (ctx->db) sqlite_svc_destroy(ctx->db);
    datahub_destroy(&ctx->hub);

    uv_run(&ctx->loop, UV_RUN_DEFAULT);
    uv_loop_close(&ctx->loop);

    edge_log_fini();
    free(ctx);
    return 0;
}

/* ---------------- Mock ---------------- */

uint32_t edge_lookup_dlt645(uint32_t di, float *out_scale) {
    if (di == 0x00000000) {
        *out_scale = 1.0f;
        return 1001;
    }
    return 0;
}
extern int dlt645_driver_register(void);

/* ---------------- Scenario Tests ---------------- */

/**
 * [S1] 周期抄读测试
 */
static void test_scenario_periodic_polling(void **state) {
    full_chain_ctx_t *ctx = *state;
    driver_manager_init();
    dlt645_driver_register();
    edge_driver_t *drv = driver_manager_find("DLT645-2007");

    ctx->orch = bus_orchestrator_create(drv, &ctx->hub, ctx->arb);
    bus_orchestrator_run(ctx->orch);

    edge_poll_group_t group = {.slave_id = 12345678, .start_addr = 0x00000000, .count = 1};
    bus_orchestrator_add_poll(ctx->orch, 50, &group);

    DRIVE_LOOP(ctx, 150);

    pipe_msg_t shadow;
    assert_int_equal(datahub_read_shadow(&ctx->hub, 1001, &shadow), 0);
    assert_double_equal(shadow.payload.val.f, 6.35, 0.001);

    driver_manager_fini();
}

static void test_scenario_shadow_service(void **state) {
    full_chain_ctx_t *ctx = *state;
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 4001, 220.5, uv_now(&ctx->loop));
    datahub_publish(&ctx->hub, &msg);
    DRIVE_LOOP(ctx, 20);
    pipe_msg_t shadow;
    assert_int_equal(datahub_read_shadow(&ctx->hub, 4001, &shadow), 0);
    assert_double_equal(shadow.payload.val.f, 220.5, 0.001);
}

static int execution_order[2];
static int exec_index = 0;
static void on_cmd_start(void *user_data) {
    int id = (int)(intptr_t)user_data;
    if (exec_index < 2) execution_order[exec_index++] = id;
}

static void test_scenario_priority_arbitration(void **state) {
    full_chain_ctx_t *ctx = *state;
    exec_index = 0;
    cmd_msg_t c1, c2;
    cmd_msg_init_write(&c1, 4003, 1.0, ARBITER_PRIO_LOW);
    c1.on_start = on_cmd_start;
    c1.user_data = (void *)1;
    c1.signature = 0x55AA;
    cmd_msg_init_write(&c2, 4003, 2.0, ARBITER_PRIO_EMERGENCY);
    c2.on_start = on_cmd_start;
    c2.user_data = (void *)2;
    c2.signature = 0x55AA;

    service_post_cmd(ctx->svc, &c1);
    service_post_cmd(ctx->svc, &c2);
    DRIVE_LOOP(ctx, 30);
    arbiter_on_io_done(ctx->arb, 0, &c2);
    DRIVE_LOOP(ctx, 30);
    arbiter_on_io_done(ctx->arb, 0, &c1);
    assert_int_equal(execution_order[0], 2);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_scenario_shadow_service, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_periodic_polling, setup, teardown),
        cmocka_unit_test_setup_teardown(test_scenario_priority_arbitration, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}