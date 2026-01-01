#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "datahub.h"
#include "stream_engine.h"

static int g_step_count = 0;

static stage_ret_t step_stage_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    (void)stage;
    (void)ev;
    g_step_count++;
    return STAGE_OK;
}

/**
 * @brief 验证 Pipeline 的顺序执行能力
 */
static void test_pipeline_sequential_execution(void **state) {
    (void)state;
    g_step_count = 0;

    datahub_t hub = {0};
    stream_engine_t *engine = stream_engine_create(&hub, NULL);

    pipeline_t *p = stream_engine_add_pipeline(engine, "seq_pipe", 1001);

    // 添加两个顺序 Stage
    pipeline_add_stage(p, "step1", step_stage_proc, NULL, NULL, NULL, NULL);
    pipeline_add_stage(p, "step2", step_stage_proc, NULL, NULL, NULL, NULL);

    pipe_msg_t ev = {.type = PIPEMSG_TYPE_SCALAR, .tag_id = 1001, .payload.val.f = 1.0};
    stream_engine_push(engine, &ev);

    // 两个 Stage 都应被执行
    assert_int_equal(g_step_count, 2);

    stream_engine_destroy(engine);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pipeline_sequential_execution),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}