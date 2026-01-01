#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "stream_engine.h"

static int g_mock_call_count = 0;

static stage_ret_t mock_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)stage;
    (void)ev;
    g_mock_call_count++;
    return STAGE_OK;
}

static void test_engine_basic_flow(void **state) {
    (void)state;
    g_mock_call_count = 0;

    datahub_t hub = {0};
    uv_loop_t *loop = uv_default_loop();
    datahub_init(&hub, loop, "test_hub", 10, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);

    pipeline_t *p = stream_engine_add_pipeline(engine, "test_pipe", 1001);
    assert_non_null(p);

    pipeline_add_stage(p, "mock_stage", mock_stage_proc, NULL, NULL, NULL, NULL);

    // 推送匹配 Tag 的消息
    pipe_msg_t ev = {.type = PIPEMSG_TYPE_SCALAR, .tag_id = 1001, .payload.val.f = 42.0};
    stream_engine_push(engine, &ev);
    assert_int_equal(g_mock_call_count, 1);

    // 推送不匹配 Tag 的消息
    ev.tag_id = 1002;
    stream_engine_push(engine, &ev);
    assert_int_equal(g_mock_call_count, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_engine_basic_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
