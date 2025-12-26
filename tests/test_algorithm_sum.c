#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "algorithm_sum.h"
#include <unistd.h>
#include <sys/mman.h>

// A mock ticker to simulate time passing if needed, 
// or we just rely on usleep + uv_run.

static void test_sum_basic(void **state) {
    (void)state;
    shm_ring_t ring;
    char ring_name[] = "/test_ring_algo";
    
    // Cleanup first in case previous run crashed
    shm_unlink(ring_name);

    assert_int_equal(shmring_create(&ring, ring_name, 8), 0);

    PipeMsg m = {
        .type = PIPE_SCALAR_F64,
        .payload.val_f = 10.0
    };

    shm_ring_write(&ring, &m);
    shm_ring_write(&ring, &m);

    uv_loop_t loop;
    assert_int_equal(uv_loop_init(&loop), 0);

    algo_sum_t algo;
    assert_int_equal(algo_sum_init(&algo, &loop, &ring, 10), 0);

    // Run loop for a bit to let timer fire
    // We expect sum to be 20.0 after processing
    int i;
    for (i=0; i<50; i++) {
        uv_run(&loop, UV_RUN_NOWAIT);
        if (algo.sum > 10.0) break;
        usleep(2000);
    }

    assert_float_equal(algo.sum, 20.0, 0.01);

    // --- Graceful Shutdown Sequence ---
    // 1. Close the handle (timer)
    if (!uv_is_closing((uv_handle_t*)&algo.timer)) {
        uv_close((uv_handle_t*)&algo.timer, NULL);
    }
    
    // 2. Run loop to process the close event
    uv_run(&loop, UV_RUN_DEFAULT);

    // 3. Close the loop
    uv_loop_close(&loop);
    
    // 4. Cleanup SHM
    shmring_close(&ring);
    shmring_destroy(&ring);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sum_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
