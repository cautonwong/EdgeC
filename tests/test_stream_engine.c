#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "stream_engine.h"
#include "datahub.h"

typedef struct {
    int call_count;
    double last_val;
} mock_stage_state_t;

static stage_ret_t mock_stage_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    mock_stage_state_t *state = (mock_stage_state_t *)stage->priv_state;
    state->call_count++;
    if (ev->type == EVT_DATA_POINT) {
        state->last_val = ev->payload.val;
    }
    return STAGE_OK;
}

static void mock_cleanup(void *state) {
    free(state);
}

static void test_engine_basic_flow(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_bus_basic", 64, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_t *p = stream_engine_add_pipeline(engine, "p1", 1001);
    
    mock_stage_state_t *ms = calloc(1, sizeof(mock_stage_state_t));
    pipeline_add_stage(p, "mock_stage", mock_stage_proc, ms, mock_cleanup, NULL, NULL);

    pipe_event_t ev = {.type = EVT_DATA_POINT, .tag_id = 1001, .payload.val = 42.0};
    stream_engine_push_event(engine, &ev);

    assert_int_equal(ms->call_count, 1);
    assert_double_equal(ms->last_val, 42.0, 0.001);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    while (uv_run(loop, UV_RUN_NOWAIT));
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_engine_basic_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}