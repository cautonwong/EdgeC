/**
 * @file test_omnis_integration.c
 * @brief Sprint Omnis - Full Stack Integration Test (Fixed Version)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "arbiter.h"
#include "bus_orchestrator.h"
#include "contract_data_types.h"
#include "datahub.h"
#include "stream_engine.h"

static const edge_tag_meta_t mock_metadata[] = {
    {.id = 1001, .name = "VOLTAGE_PHASE_A", .type = "float"},
    {.id = 8001, .name = "PROJECT_HUMIDITY", .type = "float"}};

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    arbiter_t* arb;
    stream_engine_t* engine;
    bus_orchestrator_t* orch;
    net_buf_pool_t pool;
    uv_timer_t timeout;
} omnis_test_ctx_t;

static void on_timeout(uv_timer_t* handle) {
    omnis_test_ctx_t* ctx = (omnis_test_ctx_t*)handle->data;
    fprintf(stderr, "[ERROR] Integration test timed out!\n");
    uv_stop(&ctx->loop);
}

static int _test_stream_submit_control(void* ctx, int bus_id, uint32_t tag_id, double value,
                                       const char* proto) {
    struct bus_orchestrator_s* orch = (struct bus_orchestrator_s*)ctx;
    if (!orch) return -1;
    extern int bus_orchestrator_submit_cmd(struct bus_orchestrator_s * ctx, int bus_id,
                                           cmd_msg_t* cmd);
    cmd_msg_t* cmd = calloc(1, sizeof(cmd_msg_t));
    if (!cmd) return -1;
    cmd->action = CMD_ACTION_WRITE;
    cmd->tag_id = tag_id;
    cmd->value = value;
    cmd->priority = ARBITER_PRIO_HIGH;
    strncpy(cmd->protocol, proto, 15);
    snprintf(cmd->req_id, sizeof(cmd->req_id), "REFLEX_%u", tag_id);
    bus_orchestrator_submit_cmd(orch, bus_id, cmd);
    return 0;
}

static int setup(void** state) {
    omnis_test_ctx_t* ctx = calloc(1, sizeof(omnis_test_ctx_t));
    uv_loop_init(&ctx->loop);

    net_buf_pool_init(&ctx->pool, 32, 256, 0, NULL, NULL);
    datahub_init(&ctx->hub, &ctx->loop, "omnis_bus", 64, 1024, mock_metadata, 2, &ctx->pool, NULL,
                 NULL);
    arbiter_init(&ctx->arb, &ctx->loop);
    ctx->orch = bus_orchestrator_create(&ctx->hub, ctx->arb, NULL, 8, 16, 256);
    ctx->engine = stream_engine_create(&ctx->hub, uv_default_loop(), NULL, NULL);

    uv_timer_init(&ctx->loop, &ctx->timeout);
    ctx->timeout.data = ctx;
    uv_timer_start(&ctx->timeout, on_timeout, 2000, 0);

    static const edge_stage_def_t stages[] = {
        {.name = "protect",
         .type = "REFLEX",
         .params = {.f1 = 260.0, .f2 = 1.0, .u1 = 8001, .s1 = "ModbusRTU"}}};
    static const edge_pipeline_def_t pipelines[] = {
        {.id = "safety_loop", .trigger_tag = 1001, .stages = stages, .stage_count = 1}};

    pipeline_loader_ctx_t l_ctx = {
        .control_ops = {.ctx = ctx->orch, .submit_control = _test_stream_submit_control}};
    stream_engine_load_static_metadata(ctx->engine, pipelines, 1, &l_ctx);

    *state = ctx;
    return 0;
}

static void on_force_close(uv_handle_t* handle, void* arg) {
    (void)arg;

    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int teardown(void** state) {
    omnis_test_ctx_t* ctx = *state;

    uv_timer_stop(&ctx->timeout);

    // 1. 业务销毁
    if (ctx->engine) stream_engine_destroy(ctx->engine);
    bus_orchestrator_destroy(ctx->orch);
    arbiter_destroy(ctx->arb);
    datahub_destroy(&ctx->hub);
    net_buf_pool_destroy(&ctx->pool);

    // 2. 彻底排空 Loop
    uv_walk(&ctx->loop, on_force_close, NULL);
    while (uv_run(&ctx->loop, UV_RUN_ONCE));
    uv_loop_close(&ctx->loop);

    free(ctx);
    return 0;
}

static void test_omnis_basic_assembly(void** state) {
    omnis_test_ctx_t* ctx = (omnis_test_ctx_t*)*state;
    assert_non_null(ctx->engine);

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 265.0, uv_now(&ctx->loop));
    stream_engine_push(ctx->engine, &msg);

    // [Fix] 显式运行几轮 Loop，给异步 Reflex 算子处理并释放堆内存的时间
    for (int i = 0; i < 5; i++) {
        uv_run(&ctx->loop, UV_RUN_NOWAIT);
        usleep(10000);
    }

    // [New Fix] 在测试函数结束前销毁引擎，确保计算线程停止
    stream_engine_destroy(ctx->engine);
    ctx->engine = NULL;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_omnis_basic_assembly, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
