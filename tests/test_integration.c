#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arbiter.h"
#include "io_executor.h"
#include "shmring.h"
#include "algorithm_sum.h"
#include "bus_orchestrator.h"
#include "edge_driver.h"

extern edge_driver_t modbus_rtu_driver;

typedef struct {
    uv_loop_t *loop;
    shm_ring_t data_hub;
    algo_sum_t algo_sum;
    arbiter_t *arbiter;
    edge_point_config_t point;
} integration_test_ctx_t;

static void test_end_to_end_flow(void **state) {
    (void)state;
    integration_test_ctx_t ctx = {0};
    ctx.loop = uv_default_loop();

    // 1. Setup DataHub (shmring)
    char ring_name[] = "/test_integration_hub";
    shm_unlink(ring_name);
    assert_int_equal(shmring_create(&ctx.data_hub, ring_name, 8), 0);
    
    // 2. Setup IO Executor (Threads)
    assert_int_equal(io_executor_init(ctx.loop), 0);
    
    // 3. Setup Arbiter
    ctx.arbiter = arbiter_create(ctx.loop);
    assert_non_null(ctx.arbiter);
    
    // 4. Setup Algorithm (summing from DataHub)
    assert_int_equal(algo_sum_init(&ctx.algo_sum, ctx.loop, &ctx.data_hub, 10), 0);

    // 5. Configure a point
    ctx.point.tag_id = 2001;
    ctx.point.reg_addr = 0x0001;
    ctx.point.reg_count = 1;
    ctx.point.scale = 1.0;
    ctx.point.offset = 0.0;

    // 6. Request a read (Orchestrator flow)
    printf("[Test] Requesting Modbus Read via Orchestrator...\n");
    edge_request_read(ctx.arbiter, BUS_RS485_1, &modbus_rtu_driver, &ctx.point, 1, &ctx.data_hub);

    // 7. Run loop and wait for result
    // Flow: Orchestrator -> Arbiter -> IO Executor (Thread) -> Orchestrator Callback -> ShmRing -> Algo Sum
    for (int i = 0; i < 100; i++) {
        uv_run(ctx.loop, UV_RUN_NOWAIT);
        if (ctx.algo_sum.sum > 0) break;
        usleep(10000); 
    }

    // 8. Verify
    assert_true(ctx.algo_sum.sum > 0.0);
    printf("[Test] Success! Algo Sum detected data: %.2f\n", ctx.algo_sum.sum);

    // 9. Cleanup
    io_executor_shutdown();
    arbiter_destroy(ctx.arbiter);
    algo_sum_close(&ctx.algo_sum);
    shmring_close(&ctx.data_hub);
    shmring_destroy(&ctx.data_hub);
    
    uv_run(ctx.loop, UV_RUN_DEFAULT); // Final drain
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_end_to_end_flow),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
