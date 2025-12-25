#include "algorithm_sum.h"
#include <string.h>
#include <stdio.h>

static void algo_tick(uv_timer_t *t) {
    algo_sum_t *a = (algo_sum_t *)t->data;
    if (!a) return;

    PipeMsg msg;
    uint64_t seq = 0; // Dummy variable for compatibility
    double sum = 0;

    // This loop reads all available messages from the ring buffer on each tick
    // and calculates their sum.
    while (shm_ring_read(a->ring, &seq, &msg) == 0) {
        if (msg.type == PIPE_SCALAR_F64) {
            sum += msg.payload.val_f;
        }
    }
    a->sum = sum;
    // For debugging:
    // printf("Algo tick: processed messages, current sum is %.2f\n", a->sum);
}

int algo_sum_init(
    algo_sum_t *algo,
    uv_loop_t *loop,
    shm_ring_t *ring,
    uint64_t interval_ms
) {
    if (!algo || !loop || !ring) {
        return -1; // Invalid arguments
    }

    memset(algo, 0, sizeof(algo_sum_t));
    algo->ring = ring;
    algo->sum = 0;

    uv_timer_init(loop, &algo->timer);
    algo->timer.data = algo;
    
    // The timer will fire first after `interval_ms`, and then repeat every `interval_ms`.
    return uv_timer_start(&algo->timer, algo_tick, interval_ms, interval_ms);
}

void algo_sum_close(algo_sum_t *algo) {
    if (algo) {
        uv_timer_stop(&algo->timer);
        // It's good practice to close handles. We need a callback to do it safely.
        // If the handle is active, uv_close will schedule the callback.
        // If it's not, it's a no-op but still good practice.
        uv_handle_t* handle = (uv_handle_t*)&algo->timer;
        if (uv_is_closing(handle) == 0) {
            uv_close(handle, NULL);
        }
    }
}