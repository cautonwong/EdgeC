#pragma once
#include <uv.h>
#include "shm_ring.h"

typedef struct {
    shm_ring_t *ring;
    uv_timer_t timer;
    double sum;
} algo_sum_t;

int algo_sum_init(
    algo_sum_t *algo,
    uv_loop_t *loop,
    shm_ring_t *ring,
    uint64_t interval_ms
);

// Added for cleanup
void algo_sum_close(algo_sum_t *algo);