#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "arbiter.h"
#include "edge_log.h"

typedef struct {
    cmd_msg_t *queue;
    cmd_msg_t *owner;
    bool is_exclusive;
} bus_slot_t;

struct arbiter {
    uv_mutex_t lock;
    uv_loop_t *loop;
    bus_slot_t buses[ARBITER_MAX_BUSES];
    cmd_msg_t *zombies;
    bool paused;
    void *orchestrator;
};

int arbiter_init(arbiter_t **arb, uv_loop_t *loop) {
    *arb = calloc(1, sizeof(struct arbiter));
    (*arb)->loop = loop;
    uv_mutex_init(&(*arb)->lock);
    (*arb)->paused = true;
    return 0;
}

void arbiter_set_orchestrator(arbiter_t *arb, void *orch) {
    if (arb) arb->orchestrator = orch;
}

void *arbiter_get_orchestrator(arbiter_t *arb) {
    return arb ? arb->orchestrator : NULL;
}

void arbiter_pause(arbiter_t *arb) {
    uv_mutex_lock(&arb->lock); arb->paused = true; uv_mutex_unlock(&arb->lock);
}

void arbiter_resume(arbiter_t *arb) {
    uv_mutex_lock(&arb->lock); arb->paused = false; uv_mutex_unlock(&arb->lock);
}

void *arbiter_bus_register(arbiter_t *arb, const char *name) {
    (void)arb; (void)name; return NULL;
}

int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || bus_id < 0 || bus_id >= ARBITER_MAX_BUSES) return -1;
    uv_mutex_lock(&arb->lock);
    cmd_msg_t **p = &arb->buses[bus_id].queue;
    while (*p && (*p)->priority >= cmd->priority) p = &(*p)->next;
    cmd->next = *p; *p = cmd;
    
    if (!arb->paused && !arb->buses[bus_id].owner) {
        arb->buses[bus_id].owner = arb->buses[bus_id].queue;
        arb->buses[bus_id].queue = arb->buses[bus_id].queue->next;
        cmd_msg_t *job = arb->buses[bus_id].owner;
        uv_mutex_unlock(&arb->lock);
        if (job->on_start) job->on_start(job);
        return 0;
    }
    uv_mutex_unlock(&arb->lock);
    return 0;
}

void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *msg, int status) {
    if (!arb || bus_id < 0 || bus_id >= ARBITER_MAX_BUSES) return;
    uv_mutex_lock(&arb->lock);
    if (arb->buses[bus_id].owner == msg) {
        arb->buses[bus_id].owner = NULL;
        uv_mutex_unlock(&arb->lock);
        
        if (msg->on_complete) msg->on_complete(msg, status);
        if (msg->on_cleanup) msg->on_cleanup(msg);
        free(msg);
        
        uv_mutex_lock(&arb->lock);
        if (!arb->paused && arb->buses[bus_id].queue) {
            arb->buses[bus_id].owner = arb->buses[bus_id].queue;
            arb->buses[bus_id].queue = arb->buses[bus_id].queue->next;
            cmd_msg_t *job = arb->buses[bus_id].owner;
            uv_mutex_unlock(&arb->lock);
            if (job->on_start) job->on_start(job);
            return;
        }
    }
    uv_mutex_unlock(&arb->lock);
}

int arbiter_get_statuses(arbiter_t *arb, arbiter_bus_status_t *out_stats, int max_count) {
    if (!arb || !out_stats) return 0;
    int count = 0;
    uv_mutex_lock(&arb->lock);
    for (int i = 0; i < ARBITER_MAX_BUSES && i < max_count; i++) {
        out_stats[count].bus_id = i;
        out_stats[count].is_busy = (arb->buses[i].owner != NULL);
        int q = 0; cmd_msg_t *p = arb->buses[i].queue;
        while(p) { q++; p = p->next; }
        out_stats[count].queue_len = q;
        count++;
    }
    uv_mutex_unlock(&arb->lock);
    return count;
}

void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive) {
    if (arb && bus_id >= 0 && bus_id < ARBITER_MAX_BUSES) {
        uv_mutex_lock(&arb->lock);
        arb->buses[bus_id].is_exclusive = exclusive;
        uv_mutex_unlock(&arb->lock);
    }
}

void arbiter_destroy(arbiter_t *arb) {
    if (!arb) return;
    uv_mutex_destroy(&arb->lock);
    free(arb);
}
