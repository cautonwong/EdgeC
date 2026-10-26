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
    uint64_t owner_start_ts_ns;
} bus_slot_t;

struct arbiter {
    uv_mutex_t lock;
    uv_loop_t *loop;
    bus_slot_t buses[ARBITER_MAX_BUSES];
    cmd_msg_t *zombies;
    bool paused;
    bool is_allocated;
    bool timer_initialized;
    uv_timer_t timer;
    arb_executor_ops_t executor;  // 💡 [Consumer-Driven] 注入的执行器
};

#define ARBITER_AGING_INTERVAL_MS 50
#define ARBITER_AGING_BOOST_PER_STEP 10

static inline uint16_t _calc_effective_priority(const cmd_msg_t *cmd, uint64_t now_ns) {
    if (!cmd) return 0;
    uint16_t prio = cmd->priority;
    if (now_ns > cmd->submit_ts) {
        uint64_t wait_ms = (now_ns - cmd->submit_ts) / 1000000ULL;
        uint16_t boost = (uint16_t)((wait_ms / ARBITER_AGING_INTERVAL_MS) * ARBITER_AGING_BOOST_PER_STEP);
        prio += boost;
        if (prio > ARBITER_PRIO_EMERGENCY) {
            prio = ARBITER_PRIO_EMERGENCY;
        }
    }
    return prio;
}

static void _arbiter_timer_cb(uv_timer_t *handle) {
    arbiter_t *arb = (arbiter_t *)handle->data;
    if (arb) {
        arbiter_check_timeouts(arb);
    }
}

static void _arbiter_setup(arbiter_t *arb, uv_loop_t *loop) {
    arb->loop = loop;
    uv_mutex_init(&arb->lock);
    arb->paused = true;
    for (int i = 0; i < ARBITER_MAX_BUSES; i++) {
        arb->buses[i].max_queue_len = ARBITER_MAX_QUEUE_PER_BUS;
    }
    if (loop) {
        uv_timer_init(loop, &arb->timer);
        arb->timer.data = arb;
        arb->timer_initialized = true;
        uv_timer_start(&arb->timer, _arbiter_timer_cb, 20, 20);
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
    if (!arb) return;
    // 按字段打补丁,不整份覆盖:另一个层可能已经装了 execute,整份覆盖会把它抹掉
    if (ops.execute) arb->executor.execute = ops.execute;
    if (ops.on_io_done) arb->executor.on_io_done = ops.on_io_done;
    if (ops.ctx) arb->executor.ctx = ops.ctx;
}

bool arbiter_owns_cmd(arbiter_t *arb, int bus_id, cmd_msg_t *cmd) {
    if (!arb || !cmd || bus_id < 0 || bus_id >= ARBITER_MAX_BUSES) return false;
    uv_mutex_lock(&arb->lock);
    bool owned = (arb->buses[bus_id].owner == cmd);
    uv_mutex_unlock(&arb->lock);
    return owned;
}

void arbiter_pause(arbiter_t *arb) {
    uv_mutex_lock(&arb->lock); arb->paused = true; uv_mutex_unlock(&arb->lock);
}

static void _dispatch_next_locked(arbiter_t *arb, int bus_id) {
    if (!arb || arb->paused) return;
    bus_slot_t *slot = &arb->buses[bus_id];
    if (slot->owner != NULL || slot->queue == NULL) return;

    if (slot->is_exclusive && slot->queue->priority < ARBITER_PRIO_EMERGENCY) {
        return; // Exclusive bus blocks non-emergency commands
    }

    slot->owner = slot->queue;
    slot->queue = slot->queue->next;
    slot->queue_len--;
    slot->owner_start_ts_ns = uv_hrtime();
    cmd_msg_t *job = slot->owner;

    uv_mutex_unlock(&arb->lock);
    if (arb->executor.execute) {
        arb->executor.execute(arb->executor.ctx, bus_id, job);
    } else if (job->on_start) {
        job->on_start(job);
    }
    uv_mutex_lock(&arb->lock);
}

void arbiter_resume(arbiter_t *arb) {
    if (!arb) return;
    uv_mutex_lock(&arb->lock);
    arb->paused = false;
    
    // 💡 [MTT 补强] 恢复时主动扫描所有总线，触发积压任务，防止“只开闸不放水”
    for (int i = 0; i < ARBITER_MAX_BUSES; i++) {
        _dispatch_next_locked(arb, i);
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

    // 1. 初始化时间维度与单调截止期 (Issue #215)
    cmd->submit_ts = (cmd->submit_ts > 0) ? cmd->submit_ts : uv_hrtime();
    cmd->timeout_ms = (cmd->timeout_ms > 0) ? cmd->timeout_ms : ARBITER_DEFAULT_TIMEOUT_MS;
    cmd->deadline_ns = (cmd->deadline_ns > 0) ? cmd->deadline_ns :
                       (cmd->submit_ts + (uint64_t)cmd->timeout_ms * 1000000ULL);

    uint64_t now = uv_hrtime();
    uint16_t new_prio = _calc_effective_priority(cmd, now);

    // 2. 优先级 (含 Aging 防饿死) + 同级最早截止期优先 (EDF) 稳定排序入队
    cmd_msg_t **p = &slot->queue;
    while (*p) {
        uint16_t curr_prio = _calc_effective_priority(*p, now);
        if (new_prio > curr_prio) {
            break;
        }
        if (new_prio == curr_prio) {
            if (cmd->deadline_ns < (*p)->deadline_ns) {
                break;
            }
        }
        p = &(*p)->next;
    }
    cmd->next = *p;
    *p = cmd;
    slot->queue_len++;
    
    // 3. 尝试调度
    _dispatch_next_locked(arb, bus_id);

    uv_mutex_unlock(&arb->lock);
    return 0;
}

void arbiter_on_io_done(arbiter_t *arb, int bus_id, cmd_msg_t *msg, int status) {
    if (!arb || bus_id < 0 || bus_id >= ARBITER_MAX_BUSES || !msg) return;

    // 先摘 owner 再结算:msg 马上就要被 free,而引擎侧 arbiter_owns_cmd() 是靠
    // 这个指针判断"我手里的命令还是我的吗";顺序反过来就会出现"已是 owner 的
    // 已释放命令"(断链/超时 use-after-free 的根因)
    uv_mutex_lock(&arb->lock);
    if (arb->buses[bus_id].owner == msg) {
        arb->buses[bus_id].owner = NULL;
    }
    uv_mutex_unlock(&arb->lock);

    // 💡 [Consumer-Driven] 优先通过 executor 回调处理
    int handled = 0;
    if (arb->executor.on_io_done) {
        handled = arb->executor.on_io_done(arb->executor.ctx, bus_id, msg, status);
    }

    if (!handled) {
        if (msg->on_complete) msg->on_complete(msg, status);
        if (msg->on_cleanup) msg->on_cleanup(msg);
        free(msg);
    }

    uv_mutex_lock(&arb->lock);
    _dispatch_next_locked(arb, bus_id);
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
        if (!exclusive) {
            _dispatch_next_locked(arb, bus_id);
        }
        uv_mutex_unlock(&arb->lock);
    }
}

int arbiter_check_timeouts(arbiter_t *arb) {
    if (!arb) return 0;
    uint64_t now_ns = uv_hrtime();
    int timed_out_count = 0;

    cmd_msg_t *expired_cmds[ARBITER_MAX_BUSES];
    int expired_buses[ARBITER_MAX_BUSES];
    int expired_n = 0;

    uv_mutex_lock(&arb->lock);

    for (int i = 0; i < ARBITER_MAX_BUSES; i++) {
        bus_slot_t *slot = &arb->buses[i];
        if (slot->owner != NULL) {
            cmd_msg_t *owner = slot->owner;
            uint64_t deadline = owner->deadline_ns;
            if (deadline == 0 && slot->owner_start_ts_ns > 0) {
                deadline = slot->owner_start_ts_ns + (uint64_t)owner->timeout_ms * 1000000ULL;
            }
            if (deadline > 0 && now_ns >= deadline) {
                expired_cmds[expired_n] = owner;
                expired_buses[expired_n] = i;
                expired_n++;

                slot->owner = NULL;
                slot->owner_start_ts_ns = 0;

                _dispatch_next_locked(arb, i);
            }
        }
    }

    uv_mutex_unlock(&arb->lock);

    for (int j = 0; j < expired_n; j++) {
        cmd_msg_t *cmd = expired_cmds[j];
        int bus_id = expired_buses[j];
        timed_out_count++;

        int handled = 0;
        if (arb->executor.on_io_done) {
            handled = arb->executor.on_io_done(arb->executor.ctx, bus_id, cmd, CMD_STATUS_TIMEOUT);
        }
        if (!handled) {
            if (cmd->on_complete) {
                cmd->on_complete(cmd, CMD_STATUS_TIMEOUT);
            }
            if (cmd->on_cleanup) {
                cmd->on_cleanup(cmd);
            }
            free(cmd);
        }
    }

    return timed_out_count;
}

void arbiter_destroy(arbiter_t *arb) {
    if (!arb) return;

    if (arb->timer_initialized) {
        uv_timer_stop(&arb->timer);
        uv_close((uv_handle_t *)&arb->timer, NULL);
        arb->timer_initialized = false;
        if (arb->loop) {
            uv_run(arb->loop, UV_RUN_NOWAIT);
        }
    }

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
