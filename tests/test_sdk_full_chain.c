#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "stream_engine.h"

typedef struct {
    int data_points;
} chain_mock_state_t;

static stage_ret_t mock_chain_proc(pipeline_stage_t *stage, pipe_msg_t *ev) {
    // 允许 scalar 类型的标量数据通过
    if (ev->type != PIPEMSG_TYPE_SCALAR) return STAGE_OK;
    chain_mock_state_t *s = (chain_mock_state_t *)stage->priv_state;
    if (s) s->data_points++;
    return STAGE_OK;
}

static void test_full_chain_data_flow(void **state) {
    (void)state;
    // 1. 基础环境搭建 (Loop 仅作为 DataHub 初始化需要，测试本身采用同步 push)
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "full_chain_final", 64, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    assert_non_null(engine);

    // 2. 构造计算管线
    pipeline_t *p = stream_engine_add_pipeline(engine, "vpp_agg", 1001);
    chain_mock_state_t *ms = calloc(1, sizeof(chain_mock_state_t));
    pipeline_add_stage(p, "agg", mock_chain_proc, ms, free, NULL, NULL);

    // 3. 模拟全链路数据流动：从采集产出到引擎处理
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, 1001, 500.0, 123456789);

    // 同步推送：确证计算平面逻辑闭环
    stream_engine_push(engine, &msg);

    // 4. 验证结果：计算平面是否准确捕获了测点
    assert_int_equal(ms->data_points, 1);

    // 5. 优雅关机：确证零泄漏
    stream_engine_destroy(engine);
    datahub_destroy(&hub);

    // 驱动 Loop 清理残留句柄
    uv_run(loop, UV_RUN_NOWAIT);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_full_chain_data_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}