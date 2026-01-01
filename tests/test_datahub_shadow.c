#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"

#define TEST_TAG_ID 1001
#define PRESSURE_COUNT 10000

typedef struct {
    datahub_t *hub;
    bool stop;
    int write_count;
} stress_ctx_t;

/**
 * @brief 高频写入线程 (模拟南向采集)
 */
static void producer_thread(void *arg) {
    stress_ctx_t *ctx = (stress_ctx_t *)arg;
    uint64_t val = 0;
    while (!ctx->stop) {
        PipeMsg msg = {0};
        msg.tag_id = TEST_TAG_ID;
        msg.payload.val.f = (double)val++;
        msg.ts = uv_now(ctx->hub->loop);

        datahub_publish(ctx->hub, &msg);
        ctx->write_count++;
        // 极短延迟，模拟极限负载
        if (val % 100 == 0) uv_sleep(1);
    }
}

/**
 * @brief 影子读取压力测试
 */
static void test_shadow_read_consistency(void **state) {
    (void)state;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub;
    datahub_init(&hub, loop, "shadow_stress", 1024);

    stress_ctx_t ctx = {.hub = &hub, .stop = false, .write_count = 0};
    uv_thread_t producer;
    uv_thread_create(&producer, producer_thread, &ctx);

    // 主线程作为观察者 (模拟 Web UI 轮询)
    double last_val = -1.0;
    for (int i = 0; i < PRESSURE_COUNT; i++) {
        PipeMsg shadow_msg;
        if (datahub_read_shadow(&hub, TEST_TAG_ID, &shadow_msg) == 0) {
            // 验证数据单调性，确证读写锁未失效
            assert_true(shadow_msg.payload.val.f >= last_val);
            last_val = shadow_msg.payload.val.f;
        }

        if (i % 1000 == 0) {
            uv_run(loop, UV_RUN_NOWAIT);
        }
    }

    ctx.stop = true;
    uv_thread_join(&producer);

    printf("[ShadowTest] Completed %d writes and %d reads.\n", ctx.write_count, PRESSURE_COUNT);

    datahub_destroy(&hub);
    while (uv_run(loop, UV_RUN_NOWAIT))
        ;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_shadow_read_consistency),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
