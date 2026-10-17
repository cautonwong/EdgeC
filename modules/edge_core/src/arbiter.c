#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include <stdio.h>
#include "arbiter.h"
#include "edge_log.h"

typedef struct {
    cmd_msg_t *queue;
    cmd_msg_t *owner;
    bool is_exclusive;
    uint32_t queue_len;
    uint32_t max_queue_len;
    uint64_t dropped_count;
} bus_slot_t;

struct arbiter {
    uv_mutex_t lock;
    uv_loop_t *loop;
    bus_slot_t buses[ARBITER_MAX_BUSES];
    cmd_msg_t *zombies;
    bool paused;
    bool is_allocated;
    arb_executor_ops_t executor;  // 💡 [Consumer-Driven] 注入的执行器
};

static void _arbiter_setup(arbiter_t *arb, uv_loop_t *loop) {
    arb->loop = loop;
    uv_mutex_init(&arb->lock);
    arb->paused = true;
    for (int i = 0; i < ARBITER_MAX_BUSES; i++) {
        arb->buses[i].max_queue_len = ARBITER_MAX_QUEUE_PER_BUS;
    }
}

arbiter_t *arbiter_create_in(arbiter_storage_t *storage, uv_loop_t *loop) {
    if (!storage) return NULL;
    _Static_assert(sizeof(struct arbiter) <= sizeof(arbiter_storage_t), "arbiter exceeds storage");
    arbiter_t *arb = (arbiter_t *)storage;
    memset(arb, 0, sizeof(struct arbiter));
    arb->is_allocated = false;
    _arbiter_setup(arb, loop);
    return arb;
}

int arbiter_init(arbiter_t **arb, uv_loop_t *loop) {
    if (!arb) return -1;
    *arb = calloc(1, sizeof(struct arbiter));
    if (!*arb) return -1;
    (*arb)->is_allocated = true;
    _arbiter_setup(*arb, loop);
    return 0;
}

void arbiter_set_bus_queue_limit(arbiter_t *arb, int bus_id, uint32_t max_queue) {
    if (arb && bus_id >= 0 && bus_id < ARBITER_MAX_BUSES) {
        uv_mutex_lock(&arb->lock);
        arb->buses[bus_id].max_queue_len = (max_queue > 0) ? max_queue : ARBITER_MAX_QUEUE_PER_BUS;
        uv_mutex_unlock(&arb->lock);
    }
}

uint64_t arbiter_get_bus_dropped_count(arbiter_t *arb, int bus_id) {
    if (!arb || bus_id < 0 || bus_id >= ARBITER_MAX_BUSES) return 0;
    uv_mutex_lock(&arb->lock);
    uint64_t count = arb->buses[bus_id].dropped_count;
    uv_mutex_unlock(&arb->lock);
    return count;
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
    bus_slot_t *slot = &arb->buses[bus_id];

    /* 准入控制与有界背压判断 (Admission & Bounded Backpressure) */
    if (slot->queue_len >= slot->max_queue_len) {
        /* 如果新命令是普通/低优先级命令，直接拒绝 (CMD_STATUS_QUEUE_FULL) */
        if (cmd->priority < ARBITER_PRIO_EMERGENCY) {
            slot->dropped_count++;
            uv_mutex_unlock(&arb->lock);
            if (cmd->on_complete) cmd->on_complete(cmd, CMD_STATUS_QUEUE_FULL);
            return CMD_STATUS_QUEUE_FULL;
        }

        /* 如果新命令是 EMERGENCY 紧急命令，执行 Priority-Drop 剔除队尾最低优先级命令 */
        cmd_msg_t **tail_prev = &slot->queue;
        while (*tail_prev && (*tail_prev)->next) {
            tail_prev = &(*tail_prev)->next;
        }

        if (*tail_prev && (*tail_prev)->priority < cmd->priority) {
            cmd_msg_t *victim = *tail_prev;
            *tail_prev = NULL;
            slot->queue_len--;
            slot->dropped_count++;

            /* 回调被牺牲的低优先级命令通知其已被压制丢弃 */
            if (victim->on_complete) victim->on_complete(victim, CMD_STATUS_QUEUE_FULL);
            if (victim->on_cleanup) victim->on_cleanup(victim);
        } else {
            /* 即使是 EMERGENCY，若全队都是同级或高优也只能满队拒绝 */
            slot->dropped_count++;
            uv_mutex_unlock(&arb->lock);
            if (cmd->on_complete) cmd->on_complete(cmd, CMD_STATUS_QUEUE_FULL);
            return CMD_STATUS_QUEUE_FULL;
        }
    }

    // 1. 优先级插入队列 (Stable Sort)
    cmd_msg_t **p = &slot->queue;
    while (*p && (*p)->priority >= cmd->priority) p = &(*p)->next;
    cmd->next = *p; *p = cmd;
    slot->queue_len++;
    
    // 2. 如果未暂停且总线空闲，立刻触发
    if (!arb->paused && !slot->owner) {
        slot->owner = slot->queue;
        slot->queue = slot->queue->next;
        slot->queue_len--;
        cmd_msg_t *job = slot->owner;
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
        arb->buses[bus_id].queue_len--;
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
        out_stats[count].queue_len = arb->buses[i].queue_len;
        out_stats[count].dropped_count = arb->buses[i].dropped_count;
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

    cmd_msg_t *to_free_head = NULL;
    cmd_msg_t *to_free_tail = NULL;

    uv_mutex_lock(&arb->lock);

    for (int i = 0; i < ARBITER_MAX_BUSES; i++) {
        if (arb->buses[i].owner) {
            cmd_msg_t *cmd = arb->buses[i].owner;
            arb->buses[i].owner = NULL;
            cmd->next = NULL;
            if (!to_free_head) { to_free_head = cmd; to_free_tail = cmd; }
            else { to_free_tail->next = cmd; to_free_tail = cmd; }
        }
        cmd_msg_t *curr = arb->buses[i].queue;
        arb->buses[i].queue = NULL;
        while (curr) {
            cmd_msg_t *next = curr->next;
            curr->next = NULL;
            if (!to_free_head) { to_free_head = curr; to_free_tail = curr; }
            else { to_free_tail->next = curr; to_free_tail = curr; }
            curr = next;
        }
    }

    cmd_msg_t *z = arb->zombies;
    arb->zombies = NULL;
    while (z) {
        cmd_msg_t *next = z->next;
        z->next = NULL;
        if (!to_free_head) { to_free_head = z; to_free_tail = z; }
        else { to_free_tail->next = z; to_free_tail = z; }
        z = next;
    }

    uv_mutex_unlock(&arb->lock);
    uv_mutex_destroy(&arb->lock);
    if (arb->is_allocated) {
        free(arb);
    }

    // Invoke cleanups outside arbiter lock
    cmd_msg_t *curr = to_free_head;
    while (curr) {
        cmd_msg_t *next = curr->next;
        if (curr->on_cleanup) {
            curr->on_cleanup(curr);
        } else {
            free(curr);
        }
        curr = next;
    }
}
