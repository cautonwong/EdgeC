#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "../src/stream_engine/stream_engine_priv.h"
#include "datahub.h"
#include "stream_engine.h"

// --- Mocks ---

typedef struct {
    int calls;
    stage_ret_t next_ret;
} mock_stage_state_t;

static stage_ret_t industrial_mock_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    (void)ev;
    mock_stage_state_t *s = (mock_stage_state_t *)stage->priv_state;
    s->calls++;
    return s->next_ret;
}

// --- Tests ---

static void test_pipeline_sequential_execution(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "ind_bus_seq_v2", 64, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_t *p = stream_engine_add_pipeline(engine, "pipe1", 1001);

    mock_stage_state_t *s1 = calloc(1, sizeof(mock_stage_state_t));
    mock_stage_state_t *s2 = calloc(1, sizeof(mock_stage_state_t));
    s1->next_ret = STAGE_OK;
    s2->next_ret = STAGE_OK;

    pipeline_add_stage(p, "st1", industrial_mock_proc, s1, free);
    pipeline_add_stage(p, "st2", industrial_mock_proc, s2, free);

    pipe_event_t ev = {.type = EVT_DATA_POINT, .tag_id = 1001};
    stream_engine_push_event(engine, &ev);

    assert_int_equal(s1->calls, 1);
    assert_int_equal(s2->calls, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    uv_run(loop, UV_RUN_DEFAULT);
}

static void test_pipeline_async_suspension(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "ind_bus_async_v2", 64, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_t *p = stream_engine_add_pipeline(engine, "async_pipe", 1001);

    mock_stage_state_t *s1 = calloc(1, sizeof(mock_stage_state_t));
    mock_stage_state_t *s2 = calloc(1, sizeof(mock_stage_state_t));

    s1->next_ret = STAGE_ASYNC;
    s2->next_ret = STAGE_OK;

    pipeline_add_stage(p, "st1", industrial_mock_proc, s1, free);
    pipeline_add_stage(p, "st2", industrial_mock_proc, s2, free);

    // 1. Inject first event -> Suspends S1
    pipe_event_t ev1 = {.type = EVT_DATA_POINT, .tag_id = 1001, .payload.val = 1.0};
    stream_engine_push_event(engine, &ev1);

    assert_int_equal(s1->calls, 1);
    assert_int_equal(s2->calls, 0);

    // 2. Inject second event -> Queues ev2
    pipe_event_t ev2 = {.type = EVT_DATA_POINT, .tag_id = 1001, .payload.val = 2.0};
    stream_engine_push_event(engine, &ev2);

    // S1 should not be called again because pipeline is suspended
    assert_int_equal(s1->calls, 1);

    // 3. Resume pipeline
    s1->next_ret = STAGE_OK;
    pipeline_resume(p);

    // After resume:
    // - ev1 finishes (calls S2 once)
    // - ev2 is processed from queue (calls S1 and S2)
    assert_int_equal(s1->calls, 2);
    assert_int_equal(s2->calls, 2);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    uv_run(loop, UV_RUN_DEFAULT);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pipeline_sequential_execution),
        cmocka_unit_test(test_pipeline_async_suspension),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
