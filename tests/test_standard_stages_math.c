#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "stream_engine.h"

static void test_ema_smoothing(void** state) {
    (void)state;
    // 创建一个 alpha 为 0.5 的 EMA 算子
    pipeline_stage_t* ema = stage_ema_create(0.5);
    pipe_msg_t ev = {.type = PIPEMSG_TYPE_SCALAR, .tag_id = 1001};

    // 1. 初始值
    ev.payload.val.f = 100.0;
    ema->process(ema, &ev);
    assert_float_equal(ev.payload.val.f, 100.0, 0.001);  // 第一次输入应直接作为基准

    // 2. 阶跃输入 200.0
    ev.payload.val.f = 200.0;
    ema->process(ema, &ev);
    // Y = 0.5 * 200 + 0.5 * 100 = 150.0
    assert_float_equal(ev.payload.val.f, 150.0, 0.001);

    // 3. 再次阶跃 200.0
    ev.payload.val.f = 200.0;
    ema->process(ema, &ev);
    // Y = 0.5 * 200 + 0.5 * 150 = 175.0
    assert_float_equal(ev.payload.val.f, 175.0, 0.001);

    // 清理
    if (ema->cleanup_state) ema->cleanup_state(ema->priv_state);
    if (ema->cleanup_cfg) ema->cleanup_cfg(ema->priv_cfg);
    free(ema);
}

static void test_clamp_bounds(void** state) {
    (void)state;
    // 限幅 0.0 ~ 100.0
    pipeline_stage_t* clamp = stage_clamp_create(0.0, 100.0);
    pipe_msg_t ev = {.type = PIPEMSG_TYPE_SCALAR, .tag_id = 1001};

    // 正常值
    ev.payload.val.f = 50.5;
    clamp->process(clamp, &ev);
    assert_float_equal(ev.payload.val.f, 50.5, 0.001);

    // 低越界
    ev.payload.val.f = -10.0;
    clamp->process(clamp, &ev);
    assert_float_equal(ev.payload.val.f, 0.0, 0.001);

    // 高越界
    ev.payload.val.f = 150.0;
    clamp->process(clamp, &ev);
    assert_float_equal(ev.payload.val.f, 100.0, 0.001);

    // 清理
    if (clamp->cleanup_state) clamp->cleanup_state(clamp->priv_state);
    if (clamp->cleanup_cfg) clamp->cleanup_cfg(clamp->priv_cfg);
    free(clamp);
}

// 3. 验证削峰填谷决策算子 (V2 - 调用核心算法平面)
static void test_peak_shaving_v2(void** state) {
    (void)state;
    // 门限 5000W，控制 Tag 2001
    pipeline_stage_t* shave = stage_peak_shaving_create(5000.0, 2001);
    pipe_msg_t ev = {.type = PIPEMSG_TYPE_SCALAR, .tag_id = 1001, .payload.val.f = 6000.0};

    // 执行削峰决策
    stage_ret_t ret = shave->process(shave, &ev);

    // 预期：允许通过，且 ID 变为 2001，值为 1000.0 (Gap)
    assert_int_equal(ret, STAGE_OK);
    assert_int_equal(ev.tag_id, 2001);
    assert_float_equal(ev.payload.val.f, 1000.0, 0.001);

    // 清理
    if (shave->cleanup_state) shave->cleanup_state(shave->priv_state);
    if (shave->cleanup_cfg) shave->cleanup_cfg(shave->priv_cfg);
    free(shave);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_ema_smoothing),
        cmocka_unit_test(test_clamp_bounds),
        cmocka_unit_test(test_peak_shaving_v2),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
