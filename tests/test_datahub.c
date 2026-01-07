#include <cmocka.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"

typedef struct {
    uv_loop_t loop;
    datahub_t hub;
    int received;
    uv_timer_t timeout;
} test_ctx_t;

static void on_timeout(uv_timer_t *handle) {
    test_ctx_t *ctx = (test_ctx_t *)handle->data;
    fprintf(stderr, "[FATAL] Test timed out! Received: %d/100\n", ctx->received);
    uv_stop(&ctx->loop);
    exit(1);  // Force fail
}

static void *producer_thread(void *arg) {
    test_ctx_t *ctx = (test_ctx_t *)arg;
    for (int i = 0; i < 100; i++) {
        pipe_msg_t msg = {0};
        msg.type = PIPEMSG_TYPE_SCALAR;
        msg.payload.val.i = (int64_t)i;
        datahub_publish(&ctx->hub, &msg);

        // 关键修复：在 QEMU/ARM64 环境下，强制让渡 CPU 时间片，
        // 否则生产者可能饿死消费者线程，导致 uv_async 无法及时处理
        uv_sleep(1);
    }
    return NULL;
}

static void on_hub_data(datahub_t *hub, void *user_data) {
    test_ctx_t *ctx = (test_ctx_t *)user_data;
    pipe_msg_t msg;
    static uint64_t my_seq = 0;
    while (shm_ring_read(&hub->ring, &my_seq, &msg) == 0) {
        ctx->received++;
    }
    if (ctx->received >= 100) {
        uv_timer_stop(&ctx->timeout);
        uv_stop(&ctx->loop);
    }
}

static void test_datahub_notify(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;

    // 安全网：5秒超时
    uv_timer_init(&ctx->loop, &ctx->timeout);
    ctx->timeout.data = ctx;
    uv_timer_start(&ctx->timeout, on_timeout, 5000, 0);

    pthread_t th;
    pthread_create(&th, NULL, producer_thread, ctx);

    uv_run(&ctx->loop, UV_RUN_DEFAULT);

    pthread_join(th, NULL);
    assert_int_equal(ctx->received, 100);
}

static int setup(void **state) {
    test_ctx_t *ctx = calloc(1, sizeof(*ctx));
    uv_loop_init(&ctx->loop);
    shm_unlink("/test_hub");

    // 增加返回值断言
    if (datahub_init(&ctx->hub, &ctx->loop, "/test_hub", 128, 1024, NULL) != 0) {
        fprintf(stderr, "[FATAL] datahub_init failed (check /dev/shm permissions?)\n");
        free(ctx);
        return -1;
    }

    datahub_subscribe(&ctx->hub, on_hub_data, ctx);

    *state = ctx;
    return 0;
}

static void on_walk_close(uv_handle_t *handle, void *arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

static int teardown(void **state) {
    test_ctx_t *ctx = (test_ctx_t *)*state;
    datahub_destroy(&ctx->hub);

    // 关键修复：关闭所有句柄 (包括 timeout timer)
    uv_walk(&ctx->loop, on_walk_close, NULL);

    // 运行循环直到所有关闭回调完成
    while (uv_run(&ctx->loop, UV_RUN_DEFAULT) != 0)
        ;

    if (uv_loop_close(&ctx->loop) != 0) {
        fprintf(stderr, "[WARN] Loop close failed (leak?)\n");
    }

    free(ctx);
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_datahub_notify, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}