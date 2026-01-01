#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "stream_engine.h"

// Mock Time Source
static uint64_t g_mock_mono_ms = 1000;
static uint64_t g_mock_cal_ms = 1678888000000;

static uint64_t mock_mono(void) {
    return g_mock_mono_ms;
}
static uint64_t mock_cal(void) {
    return g_mock_cal_ms;
}

// Mock State
typedef struct {
    int call_count;
} timer_state_t;

int stream_engine_push_event(stream_engine_t *engine, pipe_msg_t *ev);

static stage_ret_t mock_timer_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)ev;
    timer_state_t *ts = (timer_state_t *)stage->priv_state;
    ts->call_count++;
    return STAGE_OK;
}

static void test_timeguard_anti_jump(void **state) {
    (void)state;
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    engine_time_source_t ts_mock = {mock_mono, mock_cal};

    stream_engine_t *engine = stream_engine_create(hub, &ts_mock);

    pipeline_t *p = stream_engine_add_pipeline(engine, "time_pipe", 0);
    timer_state_t *ts = calloc(1, sizeof(timer_state_t));
    pipeline_add_stage(p, "timer_stage", mock_timer_proc, ts, free, NULL, NULL);

    // Simulate Time Tick Event
    pipe_msg_t tick = {0};
    tick.type = PIPEMSG_TYPE_EVENT;
    tick.ts = g_mock_cal_ms;
    tick.payload.event.event_id = SYS_EVENT_TIME_CHANGED;

    stream_engine_push_event(engine, &tick);

    // Check if pipeline reacted
    assert_int_equal(ts->call_count, 1);

    stream_engine_destroy(engine);
    free(hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_timeguard_anti_jump),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
