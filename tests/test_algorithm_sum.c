#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <uv.h>

#include "shm_ring.h"
#include "algorithm_sum.h"

// Test state setup, if needed
// typedef struct {
//     uv_loop_t loop;
//     shm_ring_t ring;
//     algo_sum_t algo;
// } TestState;

static int active_uv_handles;

static void close_uv_handle_cb(uv_handle_t* handle) {
    (void)handle;
    active_uv_handles--;
}

// Modify algo_sum_close to use this callback
// Note: This requires modifying src/algorithm_sum.c or defining a local wrapper.
// For the test, we'll assume the algo_sum_close in src/algorithm_sum.c can be modified,
// or we will manually close the handle here.
// Given that the instruction is to modify the test, I will modify it to properly close the handle.
// First, I will change `algo_sum_close` in `src/algorithm_sum.c` to accept a callback.

static void test_sum_basic(void **state) {
    (void) state; /* unused */

    shm_ring_t ring;
    // The init function requires capacity to be a power of 2
    assert_int_equal(shm_ring_init(&ring, 8), 0);

    PipeMsg m = {
        .type = PIPE_SCALAR_F64,
        .payload.val_f = 10.0
    };

    // Write two messages
    shm_ring_write(&ring, &m);
    m.payload.val_f = 15.5;
    shm_ring_write(&ring, &m);

    uv_loop_t loop;
    uv_loop_init(&loop);

    algo_sum_t algo;
    active_uv_handles = 1; // For the algo.timer handle

    // Initialize the timer to fire every 1ms
    algo_sum_init(&algo, &loop, &ring, 1);

    // Run the loop once to allow the timer to fire and process messages
    uv_run(&loop, UV_RUN_ONCE);
    
    // The algo_tick function sums all messages currently in the ring.
    // 10.0 + 15.5 = 25.5
    assert_float_equal(algo.sum, 25.5, 0.01);
    
    // After the tick, the ring should be empty.
    PipeMsg test_msg;
    assert_int_equal(shm_ring_read(&ring, NULL, &test_msg), -1);

    // Write more data to test the next tick
    m.payload.val_f = 1.0;
    shm_ring_write(&ring, &m);
    m.payload.val_f = 2.0;
    shm_ring_write(&ring, &m);
    m.payload.val_f = 3.0;
    shm_ring_write(&ring, &m);
    
    // Run the loop once more for the next timer tick
    uv_run(&loop, UV_RUN_ONCE);

    // The sum should be the sum of the new messages
    assert_float_equal(algo.sum, 6.0, 0.01);

    // Cleanup
    uv_timer_stop(&algo.timer); // Stop the timer first
    uv_close((uv_handle_t*)&algo.timer, close_uv_handle_cb); // Close the handle with callback
    shm_ring_destroy(&ring);

    // Run the loop until all handles are closed
    while (active_uv_handles > 0) {
        uv_run(&loop, UV_RUN_ONCE);
    }
    uv_loop_close(&loop);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_sum_basic),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
