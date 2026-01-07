#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "datahub.h"
#include "stream_engine.h"

typedef struct {
    int call_count;
    stage_ret_t return_val;
} mock_state_t;

static stage_ret_t industrial_mock_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)ev;
    mock_state_t *state = (mock_state_t *)stage->priv_state;
    state->call_count++;
    return state->return_val;
}

static void test_pipeline_sequential_execution(void **state) {
    (void)state;
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    datahub_t hub;
    datahub_init(&hub, loop, "seq_bus", 64, 1024, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_t *p = stream_engine_add_pipeline(engine, "seq_pipe", 1001);

    mock_state_t *s1 = calloc(1, sizeof(mock_state_t));
    mock_state_t *s2 = calloc(1, sizeof(mock_state_t));
    pipeline_add_stage(p, "st1", industrial_mock_proc, s1, free, NULL, NULL);
    pipeline_add_stage(p, "st2", industrial_mock_proc, s2, free, NULL, NULL);

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.tag_id = 1001;
    ev.payload.val.f = 10.0;
    stream_engine_push(engine, &ev);

    assert_int_equal(s1->call_count, 1);
    assert_int_equal(s2->call_count, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);

    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
    }
    free(loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pipeline_sequential_execution),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}