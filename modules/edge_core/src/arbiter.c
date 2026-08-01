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
    arb_executor_ops_t executor;  // 💡 [Consumer-Driven] 注入的执行器
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

void arbiter_set_executor(arbiter_t *arb, arb_executor_ops_t ops) {
    if (arb) arb->executor = ops;
}

void arbiter_pause(arbiter_t *arb) {
    uv_mutex_lock(&arb->lock); arb->paused = true; uv_mutex_unlock(&arb->lock);
}

void arbiter_resume(arbiter_t *arb) {
    if (!arb) return;
    uv_mutex_lock(&arb->lock);
    arb->paused = false;
    
    // 💡 [MTT 补强] 恢复时主动扫描所有总线，触发积压任务，防止“只开闸不放水”
    for (int i = 0; i < ARBITER_MAX_BUSES; i++) {
        if (!arb->buses[i].owner && arb->buses[i].queue) {
            arb->buses[i].owner = arb->buses[i].queue;
            arb->buses[i].queue = arb->buses[i].queue->next;
            cmd_msg_t *job = arb->buses[i].owner;
            
            // 释放锁后再回调，防止递归死锁
            uv_mutex_unlock(&arb->lock);
            if (arb->executor.execute) {
                arb->executor.execute(arb->executor.ctx, i, job);
            } else if (job->on_start) {
                job->on_start(job);
            }
            uv_mutex_lock(&arb->lock);
        }
    }
    uv_mutex_unlock(&arb->lock);
}

void *arbiter_bus_register(arbiter_t *arb, const char *name) {
    (void)arb; (void)name; return NULL;
}

int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || bus_id < 0 || bus_id >= ARBITER_MAX_BUSES || !cmd) return -1;
    
    uv_mutex_lock(&arb->lock);
    // 1. 优先级插入队列 (Stable Sort)
    cmd_msg_t **p = &arb->buses[bus_id].queue;
    while (*p && (*p)->priority >= cmd->priority) p = &(*p)->next;
    cmd->next = *p; *p = cmd;
    
    // 2. 如果未暂停且总线空闲，立刻触发
    if (!arb->paused && !arb->buses[bus_id].owner) {
        arb->buses[bus_id].owner = arb->buses[bus_id].queue;
        arb->buses[bus_id].queue = arb->buses[bus_id].queue->next;
        cmd_msg_t *job = arb->buses[bus_id].owner;
        uv_mutex_unlock(&arb->lock);
        if (arb->executor.execute) {
            arb->executor.execute(arb->executor.ctx, bus_id, job);
        } else if (job->on_start) {
            job->on_start(job);
        }
        return 0;
    }
    uv_mutex_unlock(&arb->lock);
    return 0;
}

void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *msg, int status) {
    if (!arb || bus_id < 0 || bus_id >= ARBITER_MAX_BUSES || !msg) return;

    // 💡 [Consumer-Driven] 优先通过 executor 回调处理
    int handled = 0;
    if (arb->executor.on_io_done) {
        handled = arb->executor.on_io_done(arb->executor.ctx, bus_id, msg, status);
    }

    if (!handled) {
        if (msg->on_complete) msg->on_complete(msg, status);
        if (msg->on_cleanup) msg->on_cleanup(msg);
        else free(msg);
    }

    uv_mutex_lock(&arb->lock);
    if (arb->buses[bus_id].owner == msg) {
        arb->buses[bus_id].owner = NULL;
    }

    if (!arb->paused && !arb->buses[bus_id].owner && arb->buses[bus_id].queue) {
        arb->buses[bus_id].owner = arb->buses[bus_id].queue;
        arb->buses[bus_id].queue = arb->buses[bus_id].queue->next;
        cmd_msg_t *job = arb->buses[bus_id].owner;
        uv_mutex_unlock(&arb->lock);
        if (arb->executor.execute) {
            arb->executor.execute(arb->executor.ctx, bus_id, job);
        } else if (job->on_start) {
            job->on_start(job);
        }
        return;
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
    uv_mutex_lock(&arb->lock);

    for (int i = 0; i < ARBITER_MAX_BUSES; i++) {
        if (arb->buses[i].owner) {
            cmd_msg_t *cmd = arb->buses[i].owner;
            arb->buses[i].owner = NULL;
            if (cmd->on_cleanup) cmd->on_cleanup(cmd);
            else free(cmd);
        }
        cmd_msg_t *curr = arb->buses[i].queue;
        arb->buses[i].queue = NULL;
        while (curr) {
            cmd_msg_t *next = curr->next;
            if (curr->on_cleanup) curr->on_cleanup(curr);
            else free(curr);
            curr = next;
        }
    }

    cmd_msg_t *z = arb->zombies;
    arb->zombies = NULL;
    while (z) {
        cmd_msg_t *next = z->next;
        if (z->on_cleanup) z->on_cleanup(z);
        else free(z);
        z = next;
    }

    uv_mutex_unlock(&arb->lock);
    uv_mutex_destroy(&arb->lock);
    free(arb);
}
