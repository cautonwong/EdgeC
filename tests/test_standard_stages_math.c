#include <cmocka.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine.h"

static void test_stage_transform_math(void **state) {
    (void)state;
    pipeline_stage_t *s = stage_transform_create(0.1, 5.0);
    pipe_event_t ev = {.type = EVT_DATA_POINT, .payload.val = 100.0};

    assert_int_equal(s->process(s, &ev), STAGE_OK);
    assert_double_equal(ev.payload.val, 15.0, 0.0001);

    if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
    free(s);
}

static void test_stage_filter_range_boundaries(void **state) {
    (void)state;
    pipeline_stage_t *s = stage_filter_range_create(10.0, 20.0);
    pipe_event_t ev = {.type = EVT_DATA_POINT};

    ev.payload.val = 10.0;
    assert_int_equal(s->process(s, &ev), STAGE_OK);

    ev.payload.val = 9.99;
    assert_int_equal(s->process(s, &ev), STAGE_DROP);

    if (s->cleanup_cfg) s->cleanup_cfg(s->priv_cfg);
    free(s);
}

static void test_stage_math_abnormal_values(void **state) {
    (void)state;
    pipeline_stage_t *s_trans = stage_transform_create(1.0, 0.0);
    pipeline_stage_t *s_filter = stage_filter_range_create(-100.0, 100.0);
    pipe_event_t ev = {.type = EVT_DATA_POINT};

    // 验证 NaN 拦截 (Fix: IEEE 754 logic)
    ev.payload.val = NAN;
    assert_int_equal(s_filter->process(s_filter, &ev), STAGE_DROP);

    // 验证 Inf 拦截
    ev.payload.val = INFINITY;
    assert_int_equal(s_filter->process(s_filter, &ev), STAGE_DROP);

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