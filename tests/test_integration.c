#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "edge_service.h"
#include "stream_engine.h"

typedef struct {
    uv_loop_t *loop;
    datahub_t data_hub;
    stream_engine_t *stream_engine;
    service_t *svc;
} integration_ctx_t;

static void test_end_to_end_flow(void **state) {
    (void)state;
    integration_ctx_t ctx;
    ctx.loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(ctx.loop);

    const char *ring_name = "/test_integration_bus";
    shm_unlink(ring_name);

    datahub_init(&ctx.data_hub, ctx.loop, ring_name, 64);
    ctx.stream_engine = stream_engine_create(&ctx.data_hub, NULL);

    service_options_t opts = {.mode = "INTEGRATION_TEST"};
    ctx.svc = service_new(&opts);
    service_set_datahub(ctx.svc, &ctx.data_hub);
    service_set_stream_engine(ctx.svc, ctx.stream_engine);

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 123.45, 1678888000000);
    datahub_publish(&ctx.data_hub, &msg);

    uv_run(ctx.loop, UV_RUN_NOWAIT);

    stream_engine_destroy(ctx.stream_engine);
    service_delete(ctx.svc);
    datahub_destroy(&ctx.data_hub);

    while (uv_loop_close(ctx.loop) == UV_EBUSY) {
        uv_run(ctx.loop, UV_RUN_ONCE);
    }
    free(ctx.loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_end_to_end_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}