/**
 * @file arbiter.c
 * @brief 核心仲裁器实现 (100% 工业稳健版)
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

static void _arbiter_schedule(arbiter_t *arb, int bus_id) {
    bus_state_t *bus = &arb->buses[bus_id];
    if (bus->owner || !bus->queue) return;

    // 独占检查
    if (bus->is_exclusive && bus->queue->action != CMD_ACTION_OTA) return;

    // 弹出任务
    cmd_msg_t *job = bus->queue;
    bus->queue = job->next;
    job->next = NULL;
    bus->owner = job;

    LOG_INFO(LOG_CAT_SCHED, "Arbiter: Dispatching Priority %d Task on Bus %d", job->priority,
             bus_id);
    if (job->on_start) job->on_start(job->user_data);
}

static void _on_tick(uv_timer_t *t) {
    arbiter_t *arb = (arbiter_t *)t->data;
    if (!arb || arb->is_shutting_down) return;
    for (int i = 0; i < MAX_BUSES; i++) _arbiter_schedule(arb, i);
}

int arbiter_init(arbiter_t **out_arb, uv_loop_t *loop) {
    arbiter_t *arb = calloc(1, sizeof(struct arbiter));
    if (!arb) return -1;
    arb->loop = loop;
    for (int i = 0; i < MAX_BUSES; i++) arb->buses[i].bus_id = i;

    uv_timer_init(loop, &arb->tick);
    arb->tick.data = arb;
    uv_timer_start(&arb->tick, _on_tick, 10, 10);

    *out_arb = arb;
    return 0;
}

static void _on_arb_handle_close(uv_handle_t *h) {
    arbiter_t *arb = h->data;
    free(arb);
}

void arbiter_destroy(arbiter_t *arb) {
    if (!arb || arb->is_shutting_down) return;
    arb->is_shutting_down = true;

    for (int i = 0; i < MAX_BUSES; i++) {
        cmd_msg_t *curr = arb->buses[i].queue;
        while (curr) {
            cmd_msg_t *next = curr->next;
            if (curr->on_cleanup) curr->on_cleanup(curr);
            curr = next;
        }
        if (arb->buses[i].owner && arb->buses[i].owner->on_cleanup) {
            arb->buses[i].owner->on_cleanup(arb->buses[i].owner);
        }
    }

    uv_timer_stop(&arb->tick);
    uv_close((uv_handle_t *)&arb->tick, _on_arb_handle_close);
}

int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || arb->is_shutting_down || bus_id >= MAX_BUSES || !cmd) return -1;

    // 按优先级插入队列
    cmd_msg_t **p = &arb->buses[bus_id].queue;
    while (*p && (*p)->priority >= cmd->priority) p = &(*p)->next;
    cmd->next = *p;
    *p = cmd;

    return 0;
}

void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive) {
    if (!arb || bus_id >= MAX_BUSES) return;
    arb->buses[bus_id].is_exclusive = exclusive;
}

void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || bus_id >= MAX_BUSES || !cmd) return;

    if (arb->buses[bus_id].owner == cmd) {
        void (*cleanup)(cmd_msg_t *) = cmd->on_cleanup;
        if (cmd->on_complete) {
            cmd->on_complete(cmd, 0);
        } else if (cleanup) {
            cleanup(cmd);
        }

        arb->buses[bus_id].owner = NULL;
    }
}

int arbiter_get_statuses(arbiter_t *arb, arbiter_bus_status_t *out, int max) {
    if (!arb || !out) return 0;
    int n = (max < MAX_BUSES) ? max : MAX_BUSES;
    for (int i = 0; i < n; i++) {
        out[i].bus_id = i;
        out[i].queue_len = 0;
        cmd_msg_t *c = arb->buses[i].queue;
        while (c) {
            out[i].queue_len++;
            c = c->next;
        }
        out[i].is_busy = (arb->buses[i].owner != NULL);
    }
    return n;
}
void arbiter_shutdown(arbiter_t *arb) {
    arbiter_destroy(arb);
}