#include "arbiter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

#define MAX_BUSES ARBITER_MAX_BUSES

struct bus_info {
    cmd_msg_t *owner;
    cmd_msg_t *queue;
    bool is_exclusive;
};

struct arbiter {
    struct bus_info buses[MAX_BUSES];
    uv_timer_t tick;
    bool is_shutting_down;
    bool is_paused;
    uv_mutex_t lock;  // [Nexus] 核心锁：保护调度队列

    // [终极补强] 延迟清理列表：解决异步回调堆栈冲突
    cmd_msg_t *zombies;
};

static void _on_arb_handle_close(uv_handle_t *h) {
    arbiter_t *arb = h->data;
    if (arb) {
        uv_mutex_destroy(&arb->lock);
        free(arb);
    }
}

/**
 * @brief 核心调度逻辑 (内部版本，必须在持锁状态下调用)
 * @return 返回需要立即启动的任务，若无则返回 NULL
 */
static cmd_msg_t *_arbiter_schedule_internal(arbiter_t *arb, int bus_id) {
    if (!arb || arb->is_paused || arb->is_shutting_down) {
        return NULL;
    }

    if (bus_id < 0 || bus_id >= MAX_BUSES) return NULL;

    struct bus_info *bus = &arb->buses[bus_id];
    if (bus->owner || bus->is_exclusive || !bus->queue) {
        return NULL;
    }

    // 摘取队列头
    cmd_msg_t *job = bus->queue;
    bus->queue = job->next;
    job->next = NULL;

    bus->owner = job;

    LOG_INFO(LOG_CAT_SCHED, "Arbiter: Dispatching Task %s (Prio %d) on Bus %d [Queued: %lu ms]",
             job->req_id[0] != '\0' ? job->req_id : "UNKNOWN", (int)job->priority, bus_id,
             (unsigned long)(uv_now(arb->tick.loop) - job->submit_ts));

    return job;
}

static void _on_tick(uv_timer_t *handle) {
    arbiter_t *arb = (arbiter_t *)handle->data;
    if (!arb || arb->is_shutting_down || arb->is_paused) return;

    // 1. 延迟清理列表 (在锁外清理，减少锁持有时间)
    uv_mutex_lock(&arb->lock);
    cmd_msg_t *z = arb->zombies;
    arb->zombies = NULL;
    uv_mutex_unlock(&arb->lock);

    while (z) {
        cmd_msg_t *next = z->next;
        if (z->on_cleanup) z->on_cleanup(z);
        free(z); // [Nexus] Fix leak
        z = next;
    }

    // 2. 逐个总线调度 (锁内摘取，锁外启动)
    for (int i = 0; i < MAX_BUSES; i++) {
        cmd_msg_t *job = NULL;
        uv_mutex_lock(&arb->lock);
        job = _arbiter_schedule_internal(arb, i);
        uv_mutex_unlock(&arb->lock);

        if (job) {
            if (job->on_start) {
                job->on_start(job->user_data);
            }
        }
    }
}

int arbiter_init(arbiter_t **arb_out, uv_loop_t *loop) {
    if (!arb_out || !loop) return -1;
    arbiter_t *arb = calloc(1, sizeof(arbiter_t));
    if (!arb) return -1;

    uv_mutex_init(&arb->lock);
    uv_timer_init(loop, &arb->tick);

    arb->tick.data = arb;
    arb->is_paused = true;

    LOG_INFO(LOG_CAT_SCHED, "Arbiter initialized in PAUSED state. (Addr: %p)", (void *)arb);

    uv_timer_start(&arb->tick, _on_tick, 5, 5);

    *arb_out = arb;
    return 0;
}

void arbiter_pause(arbiter_t *arb) {
    if (arb) {
        uv_mutex_lock(&arb->lock);
        arb->is_paused = true;
        uv_mutex_unlock(&arb->lock);
        LOG_INFO(LOG_CAT_SCHED, "Arbiter PAUSED manually.");
    }
}

void arbiter_resume(arbiter_t *arb) {
    if (arb) {
        uv_mutex_lock(&arb->lock);
        arb->is_paused = false;
        uv_mutex_unlock(&arb->lock);
        LOG_INFO(LOG_CAT_SCHED, "Arbiter RESUMED manually.");
    }
}

arbiter_bus_handle_t arbiter_bus_register(arbiter_t *arb, const char *name) {
    (void)arb;
    return (arbiter_bus_handle_t)name;
}

int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || arb->is_shutting_down || bus_id < 0 || bus_id >= MAX_BUSES || !cmd) {
        if (cmd && cmd->on_cleanup) cmd->on_cleanup(cmd);
        if (cmd) free(cmd); // [Nexus] Ensure cleanup
        return -1;
    }

    cmd_msg_t *job = NULL;
    uv_mutex_lock(&arb->lock);
    cmd->submit_ts = uv_now(arb->tick.loop);

    // 按优先级插入队列
    cmd_msg_t **p = &arb->buses[bus_id].queue;
    while (*p && (*p)->priority >= cmd->priority) p = &(*p)->next;
    cmd->next = *p;
    *p = cmd;

    // 尝试立即调度
    job = _arbiter_schedule_internal(arb, bus_id);
    uv_mutex_unlock(&arb->lock);

    // [Sprint 1] 修复 C4：在锁外执行回调，防止死锁
    if (job && job->on_start) {
        job->on_start(job->user_data);
    }

    return 0;
}

void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *cmd, int status) {
    if (!arb || bus_id < 0 || bus_id >= MAX_BUSES || !cmd) return;

    uv_mutex_lock(&arb->lock);
    if (arb->buses[bus_id].owner == cmd) {
        arb->buses[bus_id].owner = NULL;
        if (cmd->on_complete) cmd->on_complete(cmd, status);
        cmd->next = arb->zombies;
        arb->zombies = cmd;
    }
    uv_mutex_unlock(&arb->lock);
}

void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive) {
    if (arb && bus_id >= 0 && bus_id < MAX_BUSES) {
        uv_mutex_lock(&arb->lock);
        arb->buses[bus_id].is_exclusive = exclusive;
        uv_mutex_unlock(&arb->lock);
    }
}

int arbiter_get_statuses(arbiter_t *arb, arbiter_bus_status_t *out_stats, int max_count) {
    if (!arb || !out_stats) return -1;
    uv_mutex_lock(&arb->lock);
    int count = (max_count < MAX_BUSES) ? max_count : MAX_BUSES;
    for (int i = 0; i < count; i++) {
        out_stats[i].bus_id = i;
        out_stats[i].is_busy = (arb->buses[i].owner != NULL);
        out_stats[i].is_exclusive = arb->buses[i].is_exclusive;
        int qlen = 0;
        cmd_msg_t *p = arb->buses[i].queue;
        while (p) {
            qlen++;
            p = p->next;
        }
        out_stats[i].queue_len = qlen;
        if (arb->buses[i].owner) {
            strncpy(out_stats[i].current_job_id, arb->buses[i].owner->req_id, 31);
        } else {
            out_stats[i].current_job_id[0] = '\0';
        }
    }
    uv_mutex_unlock(&arb->lock);
    return count;
}

void arbiter_destroy(arbiter_t *arb) {
    if (!arb || arb->is_shutting_down) return;

    uv_mutex_lock(&arb->lock);
    arb->is_shutting_down = true;
    uv_timer_stop(&arb->tick);

    for (int i = 0; i < MAX_BUSES; i++) {
        cmd_msg_t *curr = arb->buses[i].queue;
        while (curr) {
            cmd_msg_t *next = curr->next;
            if (curr->on_cleanup) curr->on_cleanup(curr);
            free(curr); // [Nexus] Fix leak
            curr = next;
        }
        arb->buses[i].queue = NULL;

        if (arb->buses[i].owner) {
            if (arb->buses[i].owner->on_cleanup)
                arb->buses[i].owner->on_cleanup(arb->buses[i].owner);
            free(arb->buses[i].owner); // [Nexus] Fix leak
            arb->buses[i].owner = NULL;
        }
    }

    cmd_msg_t *z = arb->zombies;
    arb->zombies = NULL;
    while (z) {
        cmd_msg_t *next = z->next;
        if (z->on_cleanup) z->on_cleanup(z);
        free(z); // [Nexus] Fix leak
        z = next;
    }
    uv_mutex_unlock(&arb->lock);

    if (!uv_is_closing((uv_handle_t *)&arb->tick)) {
        uv_close((uv_handle_t *)&arb->tick, _on_arb_handle_close);
    }
}

void arbiter_shutdown(arbiter_t *arb) {
    arbiter_destroy(arb);
}
