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
    s->data_points++;
    return STAGE_OK;
}

// Private header access for manual loop driving
#include "../src/stream_engine/stream_engine_priv.h"

static void test_full_chain_data_flow(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();

    datahub_t hub;
    datahub_init(&hub, loop, "full_chain_v6", 64, NULL, NULL);

    stream_engine_t *engine = stream_engine_create(&hub, NULL);
    assert_non_null(engine);

    pipeline_t *p = stream_engine_add_pipeline(engine, "vpp_agg", 1001);
    chain_mock_state_t *ms = calloc(1, sizeof(chain_mock_state_t));

    // 适配 v2.0：注入清理回调 (cleanup_state = free)
    pipeline_add_stage(p, "agg", mock_chain_proc, ms, free, NULL, NULL);

    pipe_msg_t msg = {0};
    msg.tag_id = 1001;
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.payload.val.f = 500.0;
    msg.ts = 123456789;

    datahub_publish(&hub, &msg);

    // 驱动 Loop 处理异步信号
    uv_run(loop, UV_RUN_NOWAIT);
    // 注意：engine 内部有自己的 thread 和 loop。但在测试中我们可能没有启动线程（手动 create），
    // 或者我们想测试单线程行为。stream_engine_create 会启动线程。
    // 为了同步测试，我们可以稍微 sleep 等待，或者更优雅地使用 uv_async。
    // 这里的原始逻辑是手动 uv_run engine->loop，但这在多线程模式下不安全。
    // 鉴于这是一个 Mock 测试，我们假设 engine 的线程在跑。

    // 给一点时间让 worker 线程处理
    usleep(100000);

    // 由于是在另一个线程处理，这里直接断言可能会有竞态。
    // 但 ms 是在堆上分配的，两个线程共享。
    // 为了稳妥，我们可以加个简单自旋。
    int retries = 10;
    while (ms->data_points == 0 && retries-- > 0) {
        usleep(10000);
    }

    assert_int_equal(ms->data_points, 1);

    stream_engine_destroy(engine);
    datahub_destroy(&hub);

    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_full_chain_data_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
