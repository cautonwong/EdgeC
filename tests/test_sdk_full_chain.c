#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <uv.h>

#include "datahub.h"
#include "stream_engine.h"

typedef struct {
    int data_points;
} chain_mock_state_t;

static stage_ret_t mock_chain_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type != EVT_DATA_POINT) return STAGE_OK;
    chain_mock_state_t *s = (chain_mock_state_t *)stage->priv_state;
    s->data_points++;
    return STAGE_OK;
}

// Private header access for manual loop driving
#include "../src/stream_engine/stream_engine_priv.h"

static void test_full_chain_data_flow(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    datahub_t hub;
    datahub_init(&hub, loop, "full_chain_v5", 64, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    assert_non_null(engine);

    pipeline_t *p = stream_engine_add_pipeline(engine, "vpp_agg", 1001);
    chain_mock_state_t *ms = calloc(1, sizeof(chain_mock_state_t));
    pipeline_add_stage(p, "agg", mock_chain_proc, ms, free);

    PipeMsg msg = {0};
    msg.tag_id = 1001;
    msg.payload.val.f = 500.0;
    msg.ts = 123456789;

    datahub_publish(&hub, &msg);

    // 1. Process async signal on the HUB's loop (default loop)
    uv_run(loop, UV_RUN_NOWAIT);

    // 2. IMPORTANT: Since we didn't start the engine thread,
    // we must manually drive the engine's private loop to process the async notification
    uv_run(engine->loop, UV_RUN_NOWAIT);

    assert_int_equal(ms->data_points, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);

    uv_run(loop, UV_RUN_DEFAULT);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_full_chain_data_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
