#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "stream_engine.h"

// Mock state
typedef struct {
    int call_count;
    stage_ret_t return_val;
} mock_stage_state_t;

static void mock_cleanup(void *ptr) {
    free(ptr);
}

static stage_ret_t mock_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    mock_stage_state_t *state = (mock_stage_state_t *)stage->priv_state;
    state->call_count++;
    return state->return_val;
}

static void test_engine_basic_flow(void **state) {
    (void)state;
    // Mock hub
    datahub_t *hub = calloc(1, sizeof(datahub_t));
    stream_engine_t *engine = stream_engine_create(hub, NULL);  // use default time

    pipeline_t *p = stream_engine_add_pipeline(engine, "test_pipe", 1001);
    assert_non_null(p);

    mock_stage_state_t *ms = calloc(1, sizeof(mock_stage_state_t));
    pipeline_add_stage(p, "mock_stage", mock_stage_proc, ms, mock_cleanup, NULL, NULL);

    // Push event
    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = 1001;
    ev.payload.val.f = 42.0;

    stream_engine_push_event(engine, &ev);

    // Check execution
    assert_int_equal(ms->call_count, 1);

    stream_engine_destroy(engine);
    free(hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_engine_basic_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
