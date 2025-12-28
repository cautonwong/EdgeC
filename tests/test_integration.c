#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <uv.h>

#include "algorithm_sum.h"
#include "datahub.h"
#include "persistence_mgr.h"
#include "shmring.h"

typedef struct {
    uv_loop_t *loop;
    datahub_t data_hub;
    algo_ctx_t *algo;
    persistence_mgr_t storage;
    int result_received;
} integration_ctx_t;

static void on_algo_result(algo_ctx_t *ctx, const algo_result_t *res, void *user) {
    (void)ctx;
    integration_ctx_t *ictx = (integration_ctx_t *)user;
    ictx->result_received = 1;
    printf("[Test] Algo Result: sum=%.2f, count=%u\n", res->total_sum, res->sample_count);
}

static void test_end_to_end_flow(void **state) {
    (void)state;
    integration_ctx_t ctx = {0};
    ctx.loop = uv_default_loop();

    char ring_name[] = "/test_integration_hub";
    shm_unlink(ring_name);

    // 1. Init DataHub
    assert_int_equal(datahub_init(&ctx.data_hub, ctx.loop, ring_name, 64, NULL, NULL), 0);

    // 2. Init Algorithm
    assert_int_equal(algo_init(&ctx.algo, ctx.loop, 10, on_algo_result, &ctx), 0);

    // 3. Init Persistence (optional for this flow but tests components)
    assert_int_equal(persistence_mgr_init(&ctx.storage, ctx.loop, &ctx.data_hub.ring, 100), 0);

    // 4. Inject Data into DataHub
    for (int i = 0; i < 15; i++) {
        PipeMsg msg = {.type = PIPEMSG_SCALAR_F64,
                       .tag_id = 100,
                       .payload.val_f = 10.0,
                       .ts = uv_now(ctx.loop)};
        datahub_publish(&ctx.data_hub, &msg);

        // Manual bridge: In real app, something reads from hub and submits to algo.
        // For test, we read from hub and submit.
        PipeMsg out;
        uint64_t seq = (uint64_t)i;
        if (shm_ring_read(&ctx.data_hub.ring, &seq, &out) == 0) {
            algo_sample_t sample = {.value = out.payload.val_f, .ts = out.ts};
            algo_submit(ctx.algo, &sample);
        }
    }

    // 5. Run loop and wait for result
    for (int i = 0; i < 200; i++) {
        uv_run(ctx.loop, UV_RUN_NOWAIT);
        if (ctx.result_received) break;
        usleep(5000);
    }

    assert_int_equal(ctx.result_received, 1);

    // Cleanup
    persistence_mgr_close(&ctx.storage);
    algo_destroy(ctx.algo);
    datahub_destroy(&ctx.data_hub);
    
    // Process close callbacks
    uv_run(ctx.loop, UV_RUN_NOWAIT);
    // Maybe run a few times or loop until empty?
    // Since we don't have external events, NOWAIT should process pending closes.
    // algo_destroy uses a thread join, then close.
    // datahub_destroy uses uv_close.
    // persistence uses uv_close.
    // One tick should be enough for simple closes.
    // But uv_close might defer?
    // Let's use UV_RUN_DEFAULT but we need to ensure it stops.
    // If all handles are closed, uv_run returns 0.
    // But we need to make sure they ARE closed.
    // uv_close sets CLOSING.
    // uv_run processes them.
    
    // For robust cleanup in test:
    while (uv_loop_close(ctx.loop) == UV_EBUSY) {
        uv_run(ctx.loop, UV_RUN_ONCE);
    }
    
    uv_run(ctx.loop, UV_RUN_NOWAIT); // Final sweep
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_end_to_end_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}