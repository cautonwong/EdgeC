#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <uv.h>
#include "algorithm_sum.h"
#include <unistd.h>

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
}

static void test_algo_fusion_500(void **state) {
    test_ctx_t *tctx = *state;
    
    // Submit 500 samples
    for (int i = 0; i < 500; i++) {
        algo_sample_t s = {
            .meter_id = (uint32_t)i,
            .value = 1.0,
            .ts = 1000 + i
        };
        assert_int_equal(algo_submit(tctx->algo, &s), 0);
    }
    
    // Trigger computation
    algo_flush(tctx->algo);
    
    // Run loop to process async callback
    // We run repeatedly until we get the result (threaded race)
    for (int i = 0; i < 100; i++) {
        uv_run(tctx->loop, UV_RUN_NOWAIT);
        if (tctx->result_count > 0) break;
        usleep(10000); // 10ms
    }
    
    assert_int_equal(tctx->result_count, 1);
    assert_float_equal(tctx->last_sum, 500.0, 0.001);
    assert_int_equal(tctx->last_count, 500);
}

static void test_algo_auto_flush(void **state) {
    test_ctx_t *tctx = *state;
    
    // Submit capacity (1024) samples, should trigger auto-flush
    for (int i = 0; i < 1024; i++) {
        algo_sample_t s = { .value = 1.0 };
        algo_submit(tctx->algo, &s);
    }
    
    // Wait for result
    for (int i = 0; i < 100; i++) {
        uv_run(tctx->loop, UV_RUN_NOWAIT);
        if (tctx->result_count > 1) break; // Expect increment from previous test
        usleep(10000);
    }
    
    // Note: Depends on whether we reused state. Setup/Teardown resets state.
    assert_int_equal(tctx->result_count, 1);
    assert_float_equal(tctx->last_sum, 1024.0, 0.001);
}

// Setup/Teardown
static int setup(void **state) {
    test_ctx_t *tctx = calloc(1, sizeof(test_ctx_t));
    tctx->loop = uv_default_loop();
    // Capacity 1024
    if (algo_init(&tctx->algo, tctx->loop, 1024, on_result, tctx) != 0) {
        free(tctx);
        return -1;
    }
    *state = tctx;
    return 0;
}

static int teardown(void **state) {
    test_ctx_t *tctx = *state;
    algo_destroy(tctx->algo);
    
    // Run loop to clean up async handles
    uv_run(tctx->loop, UV_RUN_NOWAIT);
    
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
