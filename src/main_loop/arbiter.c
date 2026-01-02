/**
 * @file arbiter.c
 * @brief 控制权仲裁实现 (终极稳健版 V3)
 */

#include "arbiter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

#define MAX_BUSES 8

typedef struct {
    cmd_msg_t *owner;
    cmd_msg_t *queue;
    bool is_exclusive;
    int bus_id;
} bus_state_t;

struct arbiter {
    uv_loop_t *loop;
    uv_timer_t tick;
    bus_state_t buses[MAX_BUSES];
    bool is_shutting_down;
};

static void enqueue_cmd(bus_state_t *bus, cmd_msg_t *cmd) {
    cmd_msg_t **p = &bus->queue;
    while (*p && (*p)->priority >= cmd->priority) p = &(*p)->next;
    cmd->next = *p;
    *p = cmd;
}

static void arbiter_schedule(arbiter_t *arb, int bus_id) {
    if (!arb || arb->is_shutting_down) return;
    bus_state_t *bus = &arb->buses[bus_id];
    if (bus->owner || !bus->queue) return;
    if (bus->is_exclusive && bus->queue->action != CMD_ACTION_OTA) return;

    cmd_msg_t *job = bus->queue;
    bus->queue = job->next;
    job->next = NULL;
    bus->owner = job;

    if (job->on_start) job->on_start(job->user_data);
}

static void on_tick(uv_timer_t *t) {
    arbiter_t *arb = (arbiter_t *)t->data;
    if (!arb || arb->is_shutting_down) return;
    for (int i = 0; i < MAX_BUSES; i++) arbiter_schedule(arb, i);
}

int arbiter_init(arbiter_t **out_arb, uv_loop_t *loop) {
    arbiter_t *arb = calloc(1, sizeof(struct arbiter));
    if (!arb) return -1;
    arb->loop = loop;
    for (int i = 0; i < MAX_BUSES; i++) arb->buses[i].bus_id = i;
    uv_timer_init(loop, &arb->tick);
    arb->tick.data = arb;
    uv_timer_start(&arb->tick, on_tick, 10, 10);
    *out_arb = arb;
    return 0;
}

int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || arb->is_shutting_down || bus_id >= MAX_BUSES) return -1;
    enqueue_cmd(&arb->buses[bus_id], cmd);
    return 0;
}

void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive) {
    if (!arb || bus_id >= MAX_BUSES) return;
    arb->buses[bus_id].is_exclusive = exclusive;
}

void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || bus_id >= MAX_BUSES) return;
    if (arb->buses[bus_id].owner == cmd) {
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
    }
    return limit;
}

static void on_arbiter_close(uv_handle_t *handle) {
    arbiter_t *arb = (arbiter_t *)handle->data;
    free(arb);
}

void arbiter_shutdown(arbiter_t *arb) {
    if (!arb || arb->is_shutting_down) return;
    arb->is_shutting_down = true;

    // 清理所有总线的队列
    for (int i = 0; i < MAX_BUSES; i++) {
        bus_state_t *bus = &arb->buses[i];
        cmd_msg_t *curr = bus->queue;
        while (curr) {
            cmd_msg_t *next = curr->next;
            if (curr->on_complete) curr->on_complete(curr, -1);
            if (curr->on_cleanup) curr->on_cleanup(curr);
            curr = next;
        }
        bus->queue = NULL;
        if (bus->owner) {
            if (bus->owner->on_complete) bus->owner->on_complete(bus->owner, -1);
            if (bus->owner->on_cleanup) bus->owner->on_cleanup(bus->owner);
            bus->owner = NULL;
        }
    }

    if (!uv_is_closing((uv_handle_t *)&arb->tick)) {
        uv_close((uv_handle_t *)&arb->tick, on_arbiter_close);
    }
}

void arbiter_destroy(arbiter_t *arb) {
    arbiter_shutdown(arb);
}