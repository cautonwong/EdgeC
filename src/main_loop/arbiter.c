#include "arbiter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

#define MAX_BUSES BUS_MAX

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

    // [终极补强] 延迟清理列表：解决异步回调堆栈冲突
    cmd_msg_t *zombies;
};

static void _on_arb_handle_close(uv_handle_t *h) {
    arbiter_t *arb = h->data;
    free(arb);
}

// 核心调度算法 (O(1) 优先级分发)
static void _arbiter_schedule(arbiter_t *arb, int bus_id) {
    // [终极防御] 显式禁止在暂停状态下进行任何分发动作
    if (!arb || arb->is_paused || arb->is_shutting_down) {
        return;
    }

    struct bus_info *bus = &arb->buses[bus_id];
    if (bus->owner) {
        // printf(" ⏳ [Arbiter] Bus %d Busy (Owner: %s)\n", bus_id, bus->owner->req_id);
        return;
    }
    if (!bus->queue) return;

    // 摘取队列头
    cmd_msg_t *job = bus->queue;
    bus->queue = job->next;
    job->next = NULL;

    bus->owner = job;

    LOG_INFO(LOG_CAT_SCHED, "Arbiter: Dispatching Task %s (Prio %d) on Bus %d [Queued: %lu ms]",
             job->req_id[0] != '\0' ? job->req_id : "UNKNOWN", (int)job->priority, bus_id,
             (unsigned long)(uv_now(arb->tick.loop) - job->submit_ts));

    if (job->on_start) {
        job->on_start(job->user_data);
    }
}

static void _on_tick(uv_timer_t *handle) {
    arbiter_t *arb = (arbiter_t *)handle->data;
    if (arb->is_shutting_down || arb->is_paused) return;

    // 1. [核心补强] 延迟物理释放：清理上一轮产生的完成任务
    cmd_msg_t *z = arb->zombies;
    arb->zombies = NULL;
    while (z) {
        cmd_msg_t *next = z->next;
        if (z->on_cleanup) z->on_cleanup(z);
        z = next;
    }

    // 2. 执行常规调度
    for (int i = 0; i < MAX_BUSES; i++) {
        _arbiter_schedule(arb, i);
    }
}

int arbiter_init(arbiter_t **arb_out, uv_loop_t *loop) {
    arbiter_t *arb = calloc(1, sizeof(arbiter_t));

    uv_timer_init(loop, &arb->tick);

    arb->tick.data = arb;

    arb->is_paused = true;

    LOG_INFO(LOG_CAT_SCHED, "Arbiter initialized in PAUSED state. (Addr: %p)", (void *)arb);

    uv_timer_start(&arb->tick, _on_tick, 10, 10);

    *arb_out = arb;

    return 0;
}

void arbiter_pause(arbiter_t *arb) {
    if (arb) {
        arb->is_paused = true;

        LOG_INFO(LOG_CAT_SCHED, "Arbiter PAUSED manually.");
    }
}

void arbiter_resume(arbiter_t *arb) {
    if (arb) {
        arb->is_paused = false;

        LOG_INFO(LOG_CAT_SCHED, "Arbiter RESUMED manually.");
    }
}

arbiter_bus_handle_t arbiter_bus_register(arbiter_t *arb, const char *name) {
    (void)arb;
    (void)name;
    return (arbiter_bus_handle_t)name;
}

int arbiter_submit_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || arb->is_shutting_down || bus_id >= MAX_BUSES || !cmd) return -1;

    cmd->submit_ts = uv_now(arb->tick.loop);

    // 按优先级插入队列
    cmd_msg_t **p = &arb->buses[bus_id].queue;
    while (*p && (*p)->priority >= cmd->priority) p = &(*p)->next;
    cmd->next = *p;
    *p = cmd;

    // 尝试立即调度以提升实时性
    if (!arb->is_paused) _arbiter_schedule(arb, bus_id);
    return 0;
}

void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *cmd, int status) {
    if (!arb || bus_id >= MAX_BUSES || !cmd) return;

    if (arb->buses[bus_id].owner == cmd) {
        arb->buses[bus_id].owner = NULL;
        if (cmd->on_complete) cmd->on_complete(cmd, status);
        cmd->next = arb->zombies;
        arb->zombies = cmd;
    }
}

void arbiter_set_exclusive(arbiter_t *arb, int bus_id, bool exclusive) {
    if (arb && bus_id < MAX_BUSES) arb->buses[bus_id].is_exclusive = exclusive;
}

int arbiter_get_statuses(arbiter_t *arb, arbiter_bus_status_t *out_stats, int max_count) {
    if (!arb || !out_stats) return -1;
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
    return count;
}

void arbiter_shutdown(arbiter_t *arb) {
    arbiter_destroy(arb);
}

void arbiter_destroy(arbiter_t *arb) {
    if (!arb || arb->is_shutting_down) return;
    arb->is_shutting_down = true;

    uv_timer_stop(&arb->tick);

    for (int i = 0; i < MAX_BUSES; i++) {
        cmd_msg_t *curr = arb->buses[i].queue;
        while (curr) {
            cmd_msg_t *next = curr->next;
            if (curr->on_cleanup) curr->on_cleanup(curr);
            curr = next;
        }
        if (arb->buses[i].owner) {
            if (arb->buses[i].owner->on_cleanup)
                arb->buses[i].owner->on_cleanup(arb->buses[i].owner);
        }
    }

    cmd_msg_t *z = arb->zombies;
    while (z) {
        cmd_msg_t *next = z->next;
        if (z->on_cleanup) z->on_cleanup(z);
        z = next;
    }

    if (!uv_is_closing((uv_handle_t *)&arb->tick)) {
        uv_close((uv_handle_t *)&arb->tick, _on_arb_handle_close);
    }
}