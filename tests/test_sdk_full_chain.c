#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "arbiter.h"
#include "datahub.h"
#include "edge_service.h"
#include "stream_engine.h"

typedef struct {
    datahub_t hub;
    arbiter_t *arb;
    stream_engine_t *engine;
    service_t *svc;
    uv_loop_t *loop;
} full_chain_ctx_t;

static int setup(void **state) {
    full_chain_ctx_t *ctx = calloc(1, sizeof(full_chain_ctx_t));
    ctx->loop = uv_default_loop();
    datahub_init(&ctx->hub, ctx->loop, "tdd_full", 256, 1024, NULL, 0, NULL, NULL);
    arbiter_init(&ctx->arb, ctx->loop);
    ctx->engine = stream_engine_create(&ctx->hub, NULL);

    // 使用新版 service_new
    ctx->svc = service_new(ctx->loop, &ctx->hub, ctx->arb, NULL, ctx->engine, NULL, NULL);
    *state = ctx;
    return 0;
}

static int teardown(void **state) {
    full_chain_ctx_t *ctx = *state;
    service_delete(ctx->svc);
    stream_engine_destroy(ctx->engine);
    arbiter_destroy(ctx->arb);
    datahub_destroy(&ctx->hub);
    free(ctx);
    return 0;
}

static void test_full_chain_basic(void **state) {
    full_chain_ctx_t *ctx = *state;
    assert_non_null(ctx->svc);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_full_chain_basic),
    };
    return cmocka_run_group_tests(tests, setup, teardown);
}