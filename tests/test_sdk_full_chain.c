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
static stage_ret_t mock_chain_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    chain_mock_state_t *s = (chain_mock_state_t *)stage->priv_state;
    if (s) s->data_points++;
    return STAGE_OK;
}

static void test_full_chain_data_flow(void **state) {
    (void)state;
    uv_loop_t *loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(loop);
    datahub_t hub;
    datahub_init(&hub, loop, "full_chain_final", 64);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    pipeline_t *p = stream_engine_add_pipeline(engine, "vpp_agg", 1001);
    chain_mock_state_t *ms = calloc(1, sizeof(chain_mock_state_t));
    pipeline_add_stage(p, "agg", mock_chain_proc, ms, free, NULL, NULL);

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 500.0, 123456789);
    stream_engine_push(engine, &msg);

    assert_int_equal(ms->data_points, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);

    while (uv_loop_close(loop) == UV_EBUSY) {
        uv_run(loop, UV_RUN_ONCE);
    }
    free(loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_full_chain_data_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
