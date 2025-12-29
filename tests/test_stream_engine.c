#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "stream_engine.h"

// --- Mock Stage ---
typedef struct {
    int call_count;
    double last_val;
} mock_state_t;

static stage_ret_t mock_stage_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    mock_state_t *s = (mock_state_t *)stage->priv_state;
    s->call_count++;
    if (ev->type == EVT_DATA_POINT) {
        s->last_val = ev->payload.val;
    }
    return STAGE_OK;
}

static void mock_cleanup(void *state) {
    free(state);
}

// --- Test Cases ---

static void test_engine_basic_flow(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_bus_basic", 64, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    assert_non_null(engine);

    pipeline_t *p = stream_engine_add_pipeline(engine, "test_pipe", 1001);
    mock_state_t *ms = calloc(1, sizeof(mock_state_t));
    pipeline_add_stage(p, "mock_stage", mock_stage_proc, ms, mock_cleanup);

    pipe_event_t ev = {
        .type = EVT_DATA_POINT, .tag_id = 1001, .payload.val = 123.45, .calendar_ms = 1000000};

    // 工业级：直接注入数据并验证分发
    stream_engine_push_event(engine, &ev);

    assert_int_equal(ms->call_count, 1);
    assert_float_equal(ms->last_val, 123.45, 0.001);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

static void test_engine_time_tick(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "test_bus_time", 64, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_t *p = stream_engine_add_pipeline(engine, "timer_pipe", 0);

    mock_state_t *ms = calloc(1, sizeof(mock_state_t));
    pipeline_add_stage(p, "timer_stage", mock_stage_proc, ms, mock_cleanup);

    pipe_event_t tick = {.type = EVT_TIME_PULSE,
                         .payload.tick_kind = 1,  // Minute
                         .calendar_ms = 2000000};

    stream_engine_push_event(engine, &tick);
    assert_int_equal(ms->call_count, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_engine_basic_flow),
        cmocka_unit_test(test_engine_time_tick),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
