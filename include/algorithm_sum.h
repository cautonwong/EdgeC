#pragma once
#include <uv.h>
#include "shmring.h"

typedef struct {
    shm_ring_t *ring;
    uv_timer_t timer;
    double sum;
    uint64_t last_seq; // Track position in ring
} algo_sum_t;

int algo_sum_init(
    algo_sum_t *algo,
    uv_loop_t *loop,
    shm_ring_t *ring,
    uint64_t interval_ms
);

void algo_sum_close(algo_sum_t *algo);
