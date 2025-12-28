#include "arbiter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUSES 8

typedef struct {
    cmd_msg_t *owner;
    cmd_msg_t *queue;  // 优先级链表
    bool is_exclusive;
    int bus_id;
} bus_state_t;

struct arbiter {
    uv_loop_t *loop;
    uv_timer_t tick;
    bus_state_t buses[MAX_BUSES];
};

static void enqueue_cmd(bus_state_t *bus, cmd_msg_t *cmd) {
    cmd_msg_t **p = &bus->queue;
    // 按优先级排序 (0 是最高优)
    while (*p && (*p)->priority <= cmd->priority) {
        p = &(*p)->next;
    }
    cmd->next = *p;
    *p = cmd;
}

static void arbiter_schedule(arbiter_t *arb, int bus_id) {
    bus_state_t *bus = &arb->buses[bus_id];

    if (bus->owner) return;
    if (!bus->queue) return;

    cmd_msg_t *job = bus->queue;

    if (bus->is_exclusive && job->action != CMD_ACTION_OTA) {
        return;
    }

    bus->queue = job->next;
    job->next = NULL;
    bus->owner = job;

    printf("[Arbiter] Starting Job %s on Bus %d\n", job->req_id, bus_id);
    if (job->on_start) {
        job->on_start(job->user_data);
    }
}

static void on_tick(uv_timer_t *t) {
    arbiter_t *arb = (arbiter_t *)t->data;
    for (int i = 0; i < MAX_BUSES; i++) {
        arbiter_schedule(arb, i);
    }
}

int arbiter_init(arbiter_t **out_arb, uv_loop_t *loop) {
    arbiter_t *arb = calloc(1, sizeof(struct arbiter));
    arb->loop = loop;
    for (int i = 0; i < MAX_BUSES; i++) arb->buses[i].bus_id = i;

    uv_timer_init(loop, &arb->tick);
    arb->tick.data = arb;
    uv_timer_start(&arb->tick, on_tick, 10, 10);

    *out_arb = arb;
    return 0;
}

arbiter_bus_handle_t arbiter_bus_register(arbiter_t *arb, const char *name) {
    (void)name;
    return (arbiter_bus_handle_t)&arb->buses[0];
}

int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (bus_id >= MAX_BUSES) return -1;

    int count = 0;
    cmd_msg_t *curr = arb->buses[bus_id].queue;
    while (curr) {
        count++;
        curr = curr->next;
    }
    if (count >= 50) return -2;

    enqueue_cmd(&arb->buses[bus_id], cmd);
    return 0;
}

void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive) {
    if (bus_id < MAX_BUSES) {
        arb->buses[bus_id].is_exclusive = exclusive;
    }
}

void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (bus_id < MAX_BUSES && arb->buses[bus_id].owner == cmd) {
        if (cmd->on_complete) cmd->on_complete(cmd, 0);
        arb->buses[bus_id].owner = NULL;
    }
}

int arbiter_get_statuses(arbiter_t *arb, arbiter_bus_status_t *out_stats, int max_count) {
    if (!arb || !out_stats) return 0;
    int limit = (max_count < MAX_BUSES) ? max_count : MAX_BUSES;
    for (int i = 0; i < limit; i++) {
        out_stats[i].bus_id = i;
        out_stats[i].is_busy = (arb->buses[i].owner != NULL);
        out_stats[i].is_exclusive = arb->buses[i].is_exclusive;

        int qlen = 0;
        cmd_msg_t *curr = arb->buses[i].queue;
        while (curr) {
            qlen++;
            curr = curr->next;
        }
        out_stats[i].queue_len = qlen;

        if (arb->buses[i].owner) {
            strncpy(out_stats[i].current_job_id, arb->buses[i].owner->req_id, 31);
        } else {
            strcpy(out_stats[i].current_job_id, "NONE");
        }
    }
    return limit;
}

static void on_arb_close(uv_handle_t *h) {
    free(h->data);
}

void arbiter_shutdown(arbiter_t *arb) {
    if (!arb) return;

    if (!uv_is_closing((uv_handle_t *)&arb->tick)) {
        arb->tick.data = arb;

        uv_close((uv_handle_t *)&arb->tick, on_arb_close);
    }
}
