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
    pipe_event_t ev = {.type = EVT_DATA_POINT, .payload.val = 5.0};

    stage_ret_t ret = s->process(s, &ev);
    assert_int_equal(ret, STAGE_OK);
    assert_double_equal(ev.payload.val, 20.0, 0.001);

    if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
    free(s);
}

static void test_stage_filter_range_boundaries(void **state) {
    (void)state;
    pipeline_stage_t *s = stage_filter_range_create(10.0, 20.0);

    pipe_event_t ev_in = {.type = EVT_DATA_POINT, .payload.val = 15.0};
    assert_int_equal(s->process(s, &ev_in), STAGE_OK);

    pipe_event_t ev_low = {.type = EVT_DATA_POINT, .payload.val = 5.0};
    assert_int_equal(s->process(s, &ev_low), STAGE_DROP);

    pipe_event_t ev_high = {.type = EVT_DATA_POINT, .payload.val = 25.0};
    assert_int_equal(s->process(s, &ev_high), STAGE_DROP);

    if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
    free(s);
}

static void test_stage_math_abnormal_values(void **state) {
    (void)state;
    pipeline_stage_t *s_trans = stage_transform_create(1.0, 0.0);
    pipeline_stage_t *s_filter = stage_filter_range_create(-100.0, 100.0);

    // 模拟非数据事件应直接通过
    pipe_event_t tick = {.type = EVT_TIME_PULSE, .payload.tick_kind = 1};
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
