#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <uv.h>

#include "stream_engine.h"
#include "datahub.h"

static uint64_t g_mock_mono_ms = 1000000;
static uint64_t g_mock_cal_ms  = 1735344000000;
static uint64_t mock_get_mono(void) { return g_mock_mono_ms; }
static uint64_t mock_get_cal(void) { return g_mock_cal_ms; }

typedef struct { int ticks; } timer_state_t;
static stage_ret_t mock_timer_proc(pipeline_stage_t *stage, pipe_event_t *ev) {
    if (ev->type == EVT_TIME_PULSE) { ((timer_state_t *)stage->priv_state)->ticks++; }
    return STAGE_OK;
}

static void test_timeguard_anti_jump(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "tg_bus_v2", 64, NULL, NULL);
    engine_time_source_t mock_src = { .get_monotonic_ms = mock_get_mono, .get_calendar_ms = mock_get_cal };

    stream_engine_t *engine = stream_engine_create(&hub, &mock_src);
    pipeline_t *p = stream_engine_add_pipeline(engine, "time_pipe", 0);
    timer_state_t *ts = calloc(1, sizeof(timer_state_t));
    
    // 适配 v2.0
    pipeline_add_stage(p, "timer_stage", mock_timer_proc, ts, free, NULL, NULL);

    g_mock_mono_ms += 60000; g_mock_cal_ms += 60000;
    pipe_event_t tick = { .type = EVT_TIME_PULSE, .calendar_ms = g_mock_cal_ms, .payload.tick_kind = 1 };
    stream_engine_push_event(engine, &tick);
    assert_int_equal(ts->ticks, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
    while (uv_run(loop, UV_RUN_NOWAIT));
}

int main(void) {
    const struct CMUnitTest tests[] = { cmocka_unit_test(test_timeguard_anti_jump) };
    return cmocka_run_group_tests(tests, NULL, NULL);
}