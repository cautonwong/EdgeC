#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "stream_engine.h"

typedef struct {
    int call_count;
    stage_ret_t return_val;
} mock_stage_state_t;

static void mock_cleanup(void *ptr) {
    free(ptr);
}

static stage_ret_t mock_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)ev;
    mock_stage_state_t *state = (mock_stage_state_t *)stage->priv_state;
    state->call_count++;
    return state->return_val;
}

static void test_engine_basic_flow(void **state) {
    (void)state;
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);

    datahub_t hub;
    datahub_init(&hub, loop, "test_hub_v1", 10);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_t *p = stream_engine_add_pipeline(engine, "test_pipe", 1001);

    mock_stage_state_t *ms = calloc(1, sizeof(mock_stage_state_t));
    pipeline_add_stage(p, "mock_stage", mock_stage_proc, ms, mock_cleanup, NULL, NULL);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = 1001;
    ev.payload.val.f = 42.0;

    stream_engine_push(engine, &ev);
    assert_int_equal(ms->call_count, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);

    // [卓越工程] 彻底排空并销毁 Loop
    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
    }
    free(loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_engine_basic_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}