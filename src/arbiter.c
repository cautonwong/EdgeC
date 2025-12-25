#include "arbiter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ===== Internal Structures ===== */

typedef struct arb_req {
    arbiter_prio_t prio;
    arbiter_mode_t mode;
    arbiter_cb cb;
    void *user;
    uint64_t expire_at;
    struct arb_req *next;
} arb_req_t;

typedef struct {
    arb_req_t *owner;     /* 当前持有者 */
    arb_req_t *waiters;   /* 等待队列（按优先级） */
} arb_bus_t;

struct arbiter {
    uv_loop_t *loop;
    uv_timer_t tick;
    arb_bus_t buses[BUS_MAX];
};

/* ===== Helpers ===== */

static uint64_t now_ms(uv_loop_t *loop) {
    return uv_now(loop);
}

static void enqueue_req(arb_bus_t *bus, arb_req_t *req) {
    arb_req_t **p = &bus->waiters;
    while (*p && (*p)->prio >= req->prio) {
        p = &(*p)->next;
    }
    req->next = *p;
    *p = req;
}

static void grant_bus(arbiter_t *arb, arbiter_bus_t bus_id, arb_req_t *req) {
    arb_bus_t *bus = &arb->buses[bus_id];
    bus->owner = req;
    req->next = NULL;

    /* 回调必须异步，保证不重入 */
    arbiter_cb cb = req->cb;
    void *user = req->user;

    uv_async_t *async = malloc(sizeof(*async));
    uv_async_init(arb->loop, async, 
        (uv_async_cb) (void *) (uintptr_t) ({
            void fn(uv_async_t *a) {
                cb(arb, bus_id, user);
                uv_close((uv_handle_t*)a, (uv_close_cb)free);
            }
            fn;
        })
    );
    uv_async_send(async);
}

/* ===== Core Arbitration ===== */

static void try_schedule(arbiter_t *arb, arbiter_bus_t bus_id) {
    arb_bus_t *bus = &arb->buses[bus_id];

    if (!bus->waiters)
        return;

    if (!bus->owner) {
        arb_req_t *req = bus->waiters;
        bus->waiters = req->next;
        grant_bus(arb, bus_id, req);
        return;
    }

    /* 抢占逻辑 */
    arb_req_t *owner = bus->owner;
    arb_req_t *cand = bus->waiters;

    if (owner->mode == ARB_EXCLUSIVE)
        return;

    if (cand->prio > owner->prio) {
        /* 抢占 */
        bus->owner = NULL;
        enqueue_req(bus, owner);
        bus->waiters = cand->next;
        grant_bus(arb, bus_id, cand);
    }
}

static void arbiter_tick(uv_timer_t *t) {
    arbiter_t *arb = t->data;
    uint64_t now = now_ms(arb->loop);

    for (int i = 0; i < BUS_MAX; ++i) {
        arb_bus_t *bus = &arb->buses[i];

        /* timeout 检查 */
        arb_req_t **p = &bus->waiters;
        while (*p) {
            if ((*p)->expire_at && (*p)->expire_at < now) {
                arb_req_t *dead = *p;
                *p = dead->next;
                free(dead);
                continue;
            }
            p = &(*p)->next;
        }

        try_schedule(arb, (arbiter_bus_t)i);
    }
}

/* ===== Public API ===== */

arbiter_t *arbiter_create(uv_loop_t *loop) {
    arbiter_t *arb = calloc(1, sizeof(*arb));
    arb->loop = loop;

    uv_timer_init(loop, &arb->tick);
    arb->tick.data = arb;
    uv_timer_start(&arb->tick, arbiter_tick, 10, 10);

    return arb;
}

void arbiter_destroy(arbiter_t *arb) {
    uv_timer_stop(&arb->tick);
    uv_close((uv_handle_t*)&arb->tick, NULL);

    for (int i = 0; i < BUS_MAX; ++i) {
        arb_req_t *r = arb->buses[i].waiters;
        while (r) {
            arb_req_t *n = r->next;
            free(r);
            r = n;
        }
    }
    free(arb);
}

int arbiter_request(
    arbiter_t *arb,
    arbiter_bus_t bus,
    arbiter_prio_t prio,
    arbiter_mode_t mode,
    uint64_t timeout_ms,
    arbiter_cb on_grant,
    void *user
) {
    arb_req_t *req = calloc(1, sizeof(*req));
    req->prio = prio;
    req->mode = mode;
    req->cb = on_grant;
    req->user = user;
    req->expire_at = timeout_ms ? now_ms(arb->loop) + timeout_ms : 0;

    enqueue_req(&arb->buses[bus], req);
    return 0;
}

void arbiter_release(
    arbiter_t *arb,
    arbiter_bus_t bus,
    void *user
) {
    arb_bus_t *b = &arb->buses[bus];
    if (b->owner && b->owner->user == user) {
        free(b->owner);
        b->owner = NULL;
    }
}
