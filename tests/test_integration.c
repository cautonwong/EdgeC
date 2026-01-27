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
    datahub_t data_hub;
    arbiter_t *arbiter;
    stream_engine_t *stream_engine;
    service_t *svc;
    uv_loop_t *loop;
} test_ctx_t;

static void test_end_to_end_flow(void **state) {
    (void)state;
    test_ctx_t ctx;
    ctx.loop = uv_default_loop();

    datahub_init(&ctx.data_hub, ctx.loop, "test_integ", 128, 1024, NULL, 0, NULL, NULL, NULL);
    arbiter_init(&ctx.arbiter, ctx.loop);
    ctx.stream_engine = stream_engine_create(&ctx.data_hub, uv_default_loop(), NULL);

    // 修正：注入所有核心组件
    ctx.svc =
        service_new(ctx.loop, &ctx.data_hub, ctx.arbiter, NULL, ctx.stream_engine, NULL, NULL);
    assert_non_null(ctx.svc);

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 25.5, uv_now(ctx.loop));
    datahub_publish(&ctx.data_hub, &msg);

    uv_run(ctx.loop, UV_RUN_NOWAIT);

    service_delete(ctx.svc);
    stream_engine_destroy(ctx.stream_engine);
    arbiter_destroy(ctx.arbiter);
    datahub_destroy(&ctx.data_hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_end_to_end_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}