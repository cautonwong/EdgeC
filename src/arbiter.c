#include "arbiter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char name[32];
    arbiter_bus_t id;
    arbiter_req_t *owner;
    arbiter_req_t *waiters;
} bus_internal_t;

struct arbiter {
    uv_loop_t *loop;
    uv_timer_t tick;
    bus_internal_t buses[BUS_MAX];
    int bus_count;
};

static uint64_t now_ms(uv_loop_t *loop) {
    return uv_now(loop);
}

static void enqueue_req(bus_internal_t *bus, arbiter_req_t *req) {
    arbiter_req_t **p = &bus->waiters;
    // Keep sorted by priority (descending)
    while (*p && (*p)->priority >= req->priority) {
        p = &(*p)->next;
    }
    req->next = *p;
    *p = req;
}

static void grant_bus(arbiter_t *arb, bus_internal_t *bus, arbiter_req_t *req) {
    (void)arb;
    bus->owner = req;
    if (req->on_start) {
        req->on_start(req->user_start);
    }
}

static void try_schedule(arbiter_t *arb, bus_internal_t *bus) {
    if (!bus->waiters) return;

    if (!bus->owner) {
        arbiter_req_t *req = bus->waiters;
        bus->waiters = req->next;
        req->next = NULL;
        grant_bus(arb, bus, req);
        return;
    }

    // Preemption Logic
    arbiter_req_t *owner = bus->owner;
    arbiter_req_t *cand = bus->waiters;

    // Exclusive owners cannot be preempted easily unless EMERGENCY?
    // For Phase 3, we respect EXCLUSIVE.
    if (owner->mode == ARB_EXCLUSIVE && cand->priority < ARB_PRIO_EMERGENCY) {
        return;
    }

    if (cand->priority > owner->priority) {
        // Preempt!
        if (owner->on_preempt) {
            owner->on_preempt(owner->user_preempt);
        }

        // Put owner back to waiters or discard?
        // For simple scheduling, we discard or re-enqueue.
        // Docs suggest "preempted", so we move it back.
        bus->owner = NULL;
        enqueue_req(bus, owner);

        // Grant to candidate
        bus->waiters = cand->next;
        cand->next = NULL;
        grant_bus(arb, bus, cand);
    }
}

static void arbiter_tick(uv_timer_t *t) {
    arbiter_t *arb = (arbiter_t *)t->data;
    uint64_t now = now_ms(arb->loop);

    for (int i = 0; i < BUS_MAX; i++) {
        bus_internal_t *bus = &arb->buses[i];

        // Check timeouts in waiters
        arbiter_req_t **p = &bus->waiters;
        while (*p) {
            if ((*p)->expire_at && (*p)->expire_at < now) {
                arbiter_req_t *dead = *p;
                *p = dead->next;
                // Callback for timeout?
                if (dead->on_done) dead->on_done(dead->user_done);
                // Note: user manages memory of req or we free it?
                // In this implementation, req is provided by caller.
            } else {
                p = &(*p)->next;
            }
        }

        try_schedule(arb, bus);
    }
}

int arbiter_init(arbiter_t **out_arb, uv_loop_t *loop) {
    arbiter_t *arb = calloc(1, sizeof(struct arbiter));
    arb->loop = loop;

    for (int i = 0; i < BUS_MAX; i++) {
        arb->buses[i].id = (arbiter_bus_t)i;
    }

    uv_timer_init(loop, &arb->tick);
    arb->tick.data = arb;
    uv_timer_start(&arb->tick, arbiter_tick, 10, 10);

    *out_arb = arb;
    return 0;
}

static void on_arb_close(uv_handle_t *handle) {
    free(handle->data);
}

void arbiter_shutdown(arbiter_t *arb) {
    if (!arb) return;
    if (!uv_is_closing((uv_handle_t *)&arb->tick)) {
        arb->tick.data = arb;
        uv_close((uv_handle_t *)&arb->tick, on_arb_close);
    }
}

arbiter_bus_handle_t arbiter_bus_register(arbiter_t *arb, const char *name) {
    // For MVP, just return the first available RS485_1
    // In real app, look up by name
    (void)name;
    return (arbiter_bus_handle_t)&arb->buses[BUS_RS485_1];
}

int arbiter_submit(arbiter_bus_handle_t bus_h, arbiter_req_t *req) {
    bus_internal_t *bus = (bus_internal_t *)bus_h;
    req->expire_at = req->timeout_ms ? (uv_now(uv_default_loop()) + req->timeout_ms) : 0;
    req->next = NULL;

    enqueue_req(bus, req);
    return 0;
}

void arbiter_release(arbiter_bus_handle_t bus_h, arbiter_req_t *req) {
    bus_internal_t *bus = (bus_internal_t *)bus_h;
    if (bus->owner == req) {
        if (req->on_done) {
            req->on_done(req->user_done);
        }
        bus->owner = NULL;
    }
}