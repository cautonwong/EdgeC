#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <uv.h>

#include "algorithm_sum.h"

// Test Context
typedef struct {
    uv_loop_t *loop;
    algo_ctx_t *algo;
    int result_count;
    double last_sum;
    uint32_t last_count;
} test_ctx_t;

static void on_result(algo_ctx_t *ctx, const algo_result_t *res, void *user_data) {
    (void)ctx;
    test_ctx_t *tctx = (test_ctx_t *)user_data;
    tctx->result_count++;
    tctx->last_sum = res->total_sum;
    tctx->last_count = res->sample_count;
    uv_stop(tctx->loop);  // 核心：通知 loop 退出
}

static void on_test_timeout(uv_timer_t *handle) {
    printf("[Test] !!! FATAL: Algorithm test timed out !!!\n");
    uv_stop(handle->loop);
}

static void test_algo_fusion_500(void **state) {
    test_ctx_t *tctx = *state;

    // 增加超时保护
    uv_timer_t timeout;
    uv_timer_init(tctx->loop, &timeout);
    uv_timer_start(&timeout, on_test_timeout, 5000, 0);  // 5s

    // Submit 500 samples
    for (int i = 0; i < 500; i++) {
        algo_sample_t s = {.value = 1.0, .ts = 1000 + (uint64_t)i};
        algo_submit(tctx->algo, &s);
    }

    algo_flush(tctx->algo);

    // 运行直到触发 uv_stop (由 on_result 或 on_test_timeout 触发)
    uv_run(tctx->loop, UV_RUN_DEFAULT);

    uv_timer_stop(&timeout);
    uv_close((uv_handle_t *)&timeout, NULL);

    assert_int_equal(tctx->result_count, 1);
    assert_float_equal(tctx->last_sum, 500.0, 0.001);
}

static void test_algo_auto_flush(void **state) {
    test_ctx_t *tctx = *state;

    // Submit capacity (1024) samples, should trigger auto-flush
    for (int i = 0; i < 1024; i++) {
        algo_sample_t s = {.value = 1.0};
        algo_submit(tctx->algo, &s);
    }

    // Wait for result
    for (int i = 0; i < 100; i++) {
        uv_run(tctx->loop, UV_RUN_NOWAIT);
        if (tctx->result_count > 1) break;  // Expect increment from previous test
        uv_sleep(10000);
    }

    // Note: Depends on whether we reused state. Setup/Teardown resets state.
    assert_int_equal(tctx->result_count, 1);
    assert_float_equal(tctx->last_sum, 1024.0, 0.001);
}

// Setup/Teardown
static int setup(void **state) {
    test_ctx_t *tctx = calloc(1, sizeof(test_ctx_t));
    tctx->loop = malloc(sizeof(uv_loop_t));
    uv_loop_init(tctx->loop);
    if (algo_init(&tctx->algo, tctx->loop, 1024, on_result, tctx) != 0) {
        free(tctx->loop);
        free(tctx);
        return -1;
    }
    *state = tctx;
    return 0;
}

static int teardown(void **state) {
    test_ctx_t *tctx = *state;
    algo_destroy(tctx->algo);

    uv_run(tctx->loop, UV_RUN_NOWAIT);
    uv_loop_close(tctx->loop);
    free(tctx->loop);
    free(tctx);
    return 0;
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_algo_fusion_500, setup, teardown),
        cmocka_unit_test_setup_teardown(test_algo_auto_flush, setup, teardown),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
