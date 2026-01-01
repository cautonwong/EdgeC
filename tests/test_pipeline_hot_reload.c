#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../src/stream_engine/stream_engine_priv.h"
#include "stream_engine.h"

static stage_ret_t probe_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)ev;
    int *count = (int *)stage->priv_state;
    if (count) (*count)++;
    return STAGE_OK;
}

static void test_pipeline_hot_reload_logic(void **state) {
    (void)state;
    datahub_t *hub = NULL;  // Mock
    stream_engine_t *engine = stream_engine_create(hub, NULL);

    int probe = 0;
    pipeline_t *p = stream_engine_add_pipeline(engine, "reload-test", 1001);
    pipeline_add_stage(p, "probe", probe_proc, &probe, NULL, NULL, NULL);

    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 123.4, 0);
    stream_engine_push(engine, &msg);
    assert_int_equal(probe, 1);

    stream_engine_destroy(engine);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pipeline_hot_reload_logic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
