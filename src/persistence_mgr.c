#include "persistence_mgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "sqlite_service.h"

static void persistence_tick(uv_timer_t *t) {
    persistence_mgr_t *mgr = (persistence_mgr_t *)t->data;
    if (!mgr || !mgr->ring || !mgr->writer) return;

    PipeMsg msg;
    while (shm_ring_read(mgr->ring, &mgr->last_seq, &msg) == 0) {
        sqlite_svc_insert(mgr->writer, &msg);
    }
}

int persistence_mgr_init(persistence_mgr_t *mgr, uv_loop_t *loop, shm_ring_t *ring,
                         uint64_t interval_ms) {
    if (!mgr || !loop || !ring) return -1;

    mgr->ring = ring;
    mgr->loop = loop;
    mgr->last_seq = 0;

    uv_timer_init(loop, &mgr->timer);
    mgr->timer.data = mgr;
    return uv_timer_start(&mgr->timer, persistence_tick, interval_ms, interval_ms);
}

void persistence_mgr_close(persistence_mgr_t *mgr) {
    if (mgr && !uv_is_closing((uv_handle_t *)&mgr->timer)) {
        uv_close((uv_handle_t *)&mgr->timer, NULL);
    }
}