#include "algorithm_sum.h"
#include <string.h>

static void algo_tick(uv_timer_t *t) {
    algo_sum_t *a = (algo_sum_t *)t->data;
    PipeMsg msg;

    while (shm_ring_read(a->ring, &a->last_seq, &msg) == 0) {
        if (msg.type == PIPE_SCALAR_F64) {
            a->sum += msg.payload.val_f;
        }
    }
}

int algo_sum_init(
    algo_sum_t *algo,
    uv_loop_t *loop,
    shm_ring_t *ring,
    uint64_t interval_ms
) {
    algo->ring = ring;
    algo->sum = 0;
    algo->last_seq = 0;

    uv_timer_init(loop, &algo->timer);
    algo->timer.data = algo;
    return uv_timer_start(&algo->timer, algo_tick, interval_ms, interval_ms);
}

void algo_sum_close(algo_sum_t *algo) {
    if (algo && !uv_is_closing((uv_handle_t*)&algo->timer)) {
        uv_close((uv_handle_t*)&algo->timer, NULL);
    }
}
