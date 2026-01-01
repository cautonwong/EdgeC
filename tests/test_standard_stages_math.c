#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine.h"

static void test_stage_transform_math(void **state) {
    (void)state;
    pipeline_stage_t *s = stage_transform_create(2.0, 10.0);  // y = 2x + 10

    pipe_msg_t ev = {0};
    ev.type = PIPEMSG_TYPE_SCALAR;
    ev.payload.val.f = 5.0;

    stage_ret_t ret = s->process(s, &ev);
    assert_int_equal(ret, STAGE_OK);
    assert_double_equal(ev.payload.val.f, 20.0, 0.001);

    if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
    free(s);
}

static void test_stage_filter_range_boundaries(void **state) {
    (void)state;
    pipeline_stage_t *s = stage_filter_range_create(10.0, 20.0);

    pipe_msg_t ev_in = {0};
    ev_in.type = PIPEMSG_TYPE_SCALAR;
    ev_in.payload.val.f = 15.0;
    assert_int_equal(s->process(s, &ev_in), STAGE_OK);

    pipe_msg_t ev_low = {0};
    ev_low.type = PIPEMSG_TYPE_SCALAR;
    ev_low.payload.val.f = 5.0;
    assert_int_equal(s->process(s, &ev_low), STAGE_DROP);

    pipe_msg_t ev_high = {0};
    ev_high.type = PIPEMSG_TYPE_SCALAR;
    ev_high.payload.val.f = 25.0;
    assert_int_equal(s->process(s, &ev_high), STAGE_DROP);

    if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
    free(s);
}

static void test_stage_math_abnormal_values(void **state) {
    (void)state;
    pipeline_stage_t *s_trans = stage_transform_create(1.0, 0.0);
    pipeline_stage_t *s_filter = stage_filter_range_create(-100.0, 100.0);

    // 模拟非数据事件应直接通过
    pipe_msg_t tick = {0};
    tick.type = PIPEMSG_TYPE_EVENT;
    tick.payload.event.event_id = SYS_EVENT_TIME_CHANGED;

    assert_int_equal(s_trans->process(s_trans, &tick), STAGE_OK);
    assert_int_equal(s_filter->process(s_filter, &tick), STAGE_OK);

    if (s_trans->cleanup_cfg) s_trans->cleanup_cfg(s_trans->priv_cfg);
    if (s_filter->cleanup_cfg) s_filter->cleanup_cfg(s_filter->priv_cfg);
    free(s_trans);
    free(s_filter);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_stage_transform_math),
        cmocka_unit_test(test_stage_filter_range_boundaries),
        cmocka_unit_test(test_stage_math_abnormal_values),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}