#include "edge_sched_deadline.h"

#include <string.h>
#include <time.h>

/**
 * @brief 单个任务插槽
 */
struct edge_sched_deadline_slot {
    edge_sched_task_config_t config;
    edge_sched_task_stats_t  stats;
    bool                     occupied;
    uint64_t                 expected_next_start_us;
};

/**
 * @brief 确定性死线监控器内部聚合根结构 (AEP-720)
 */
struct edge_sched_deadline_monitor {
    edge_sched_deadline_ports_t ports;
    struct edge_sched_deadline_slot tasks[EDGE_SCHED_MAX_TASKS];
    uint32_t task_count;
};

/* -------------------------------------------------------------------------- */
/* 编译期大小与对齐静态双重断言 (AEP-752)                                        */
/* -------------------------------------------------------------------------- */
_Static_assert(sizeof(struct edge_sched_deadline_monitor) <= sizeof(edge_sched_deadline_storage_t),
               "edge_sched_deadline_storage_t size buffer too small for struct edge_sched_deadline_monitor");
_Static_assert(_Alignof(struct edge_sched_deadline_monitor) <= _Alignof(edge_sched_deadline_storage_t),
               "edge_sched_deadline_storage_t alignment mismatch for struct edge_sched_deadline_monitor");

static inline uint64_t edge_sched_get_time_us(const struct edge_sched_deadline_monitor *mon) {
    if (mon && mon->ports.get_now_us) {
        return mon->ports.get_now_us(mon->ports.ctx);
    }
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
    }
    return 0;
}

edge_sched_deadline_monitor_t *edge_sched_deadline_create_in(edge_sched_deadline_storage_t *store,
                                                             const edge_sched_deadline_ports_t *ports) {
    if (!store) {
        return NULL;
    }
    memset(store, 0, sizeof(*store));
    struct edge_sched_deadline_monitor *mon = (struct edge_sched_deadline_monitor *)store;
    if (ports) {
        mon->ports = *ports;
    }
    return mon;
}

int edge_sched_deadline_init(edge_sched_deadline_monitor_t *mon,
                             const edge_sched_deadline_ports_t *ports) {
    if (!mon) {
        return -1;
    }
    memset(mon, 0, sizeof(*mon));
    if (ports) {
        mon->ports = *ports;
    }
    return 0;
}

int edge_sched_deadline_register_task(edge_sched_deadline_monitor_t *mon,
                                      const edge_sched_task_config_t *cfg) {
    if (!mon || !cfg || cfg->deadline_us == 0) {
        return -1;
    }
    for (uint32_t i = 0; i < EDGE_SCHED_MAX_TASKS; ++i) {
        if (mon->tasks[i].occupied && mon->tasks[i].config.task_id == cfg->task_id) {
            return -2;
        }
    }
    for (uint32_t i = 0; i < EDGE_SCHED_MAX_TASKS; ++i) {
        if (!mon->tasks[i].occupied) {
            struct edge_sched_deadline_slot *slot = &mon->tasks[i];
            slot->occupied = true;
            slot->config = *cfg;
            if (slot->config.max_consecutive_overruns == 0) {
                slot->config.max_consecutive_overruns = EDGE_SCHED_DEFAULT_TRIP_OVERRUNS;
            }
            memset(&slot->stats, 0, sizeof(slot->stats));
            slot->stats.task_id = cfg->task_id;
            strncpy(slot->stats.name, cfg->name, sizeof(slot->stats.name) - 1);
            slot->stats.state = EDGE_SCHED_STATE_NORMAL;
            slot->stats.min_duration_us = UINT64_MAX;
            slot->expected_next_start_us = 0;
            mon->task_count++;
            return 0;
        }
    }
    return -3;
}

int edge_sched_deadline_task_begin(edge_sched_deadline_monitor_t *mon,
                                   uint32_t task_id,
                                   edge_sched_token_t *token) {
    if (!mon || !token) {
        return -1;
    }
    struct edge_sched_deadline_slot *slot = NULL;
    for (uint32_t i = 0; i < EDGE_SCHED_MAX_TASKS; ++i) {
        if (mon->tasks[i].occupied && mon->tasks[i].config.task_id == task_id) {
            slot = &mon->tasks[i];
            break;
        }
    }
    if (!slot) {
        return -2;
    }
    uint64_t now_us = edge_sched_get_time_us(mon);
    if (slot->stats.total_runs > 0 && slot->config.period_us > 0 && slot->expected_next_start_us > 0) {
        int64_t diff = (int64_t)now_us - (int64_t)slot->expected_next_start_us;
        uint64_t jitter = diff >= 0 ? (uint64_t)diff : (uint64_t)(-diff);
        slot->stats.last_jitter_us = jitter;
        if (jitter > slot->stats.max_jitter_us) {
            slot->stats.max_jitter_us = jitter;
        }
        if (slot->config.max_allowed_jitter_us > 0 && jitter > slot->config.max_allowed_jitter_us) {
            if (slot->stats.state == EDGE_SCHED_STATE_NORMAL) {
                slot->stats.state = EDGE_SCHED_STATE_WARNING;
            }
        }
        slot->expected_next_start_us = now_us + slot->config.period_us;
    } else if (slot->config.period_us > 0) {
        slot->expected_next_start_us = now_us + slot->config.period_us;
    }

    slot->stats.last_start_time_us = now_us;
    token->task_id = task_id;
    token->start_time_us = now_us;
    token->active = true;

    if (slot->stats.state == EDGE_SCHED_STATE_TRIPPED) {
        return 1;
    }
    return 0;
}

int edge_sched_deadline_task_end(edge_sched_deadline_monitor_t *mon,
                                 edge_sched_token_t *token,
                                 edge_sched_action_t *out_action) {
    if (!mon || !token || !token->active) {
        return -1;
    }
    struct edge_sched_deadline_slot *slot = NULL;
    for (uint32_t i = 0; i < EDGE_SCHED_MAX_TASKS; ++i) {
        if (mon->tasks[i].occupied && mon->tasks[i].config.task_id == token->task_id) {
            slot = &mon->tasks[i];
            break;
        }
    }
    if (!slot) {
        return -2;
    }
    uint64_t now_us = edge_sched_get_time_us(mon);
    uint64_t duration_us = (now_us >= token->start_time_us) ? (now_us - token->start_time_us) : 0;
    token->active = false;

    slot->stats.last_end_time_us = now_us;
    slot->stats.last_duration_us = duration_us;
    slot->stats.total_runs++;

    if (duration_us < slot->stats.min_duration_us) {
        slot->stats.min_duration_us = duration_us;
    }
    if (duration_us > slot->stats.max_duration_us) {
        slot->stats.max_duration_us = duration_us;
    }
    if (slot->stats.total_runs == 1) {
        slot->stats.avg_duration_us = (double)duration_us;
    } else {
        slot->stats.avg_duration_us = 0.1 * (double)duration_us + 0.9 * slot->stats.avg_duration_us;
    }

    bool violation = (slot->config.deadline_us > 0 && duration_us > slot->config.deadline_us);
    edge_sched_action_t action = EDGE_SCHED_ACTION_LOG_ONLY;
    int ret_val = 0;

    if (violation) {
        slot->stats.deadline_misses++;
        slot->stats.consecutive_misses++;
        if (slot->stats.consecutive_misses >= slot->config.max_consecutive_overruns) {
            slot->stats.state = EDGE_SCHED_STATE_TRIPPED;
            action = slot->config.fallback_action;
            ret_val = 2;
        } else {
            slot->stats.state = EDGE_SCHED_STATE_WARNING;
            action = EDGE_SCHED_ACTION_WARN;
            ret_val = 1;
        }
        if (mon->ports.on_violation) {
            mon->ports.on_violation(mon->ports.ctx, token->task_id, &slot->stats, action);
        }
    } else {
        slot->stats.consecutive_misses = 0;
        bool jitter_violation = (slot->config.max_allowed_jitter_us > 0 &&
                                 slot->stats.last_jitter_us > slot->config.max_allowed_jitter_us);
        if (slot->stats.state == EDGE_SCHED_STATE_WARNING && !jitter_violation) {
            slot->stats.state = EDGE_SCHED_STATE_NORMAL;
        }
    }

    if (out_action) {
        *out_action = action;
    }
    return ret_val;
}

int edge_sched_deadline_get_task_stats(const edge_sched_deadline_monitor_t *mon,
                                       uint32_t task_id,
                                       edge_sched_task_stats_t *out_stats) {
    if (!mon || !out_stats) {
        return -1;
    }
    for (uint32_t i = 0; i < EDGE_SCHED_MAX_TASKS; ++i) {
        if (mon->tasks[i].occupied && mon->tasks[i].config.task_id == task_id) {
            *out_stats = mon->tasks[i].stats;
            if (out_stats->min_duration_us == UINT64_MAX) {
                out_stats->min_duration_us = 0;
            }
            return 0;
        }
    }
    return -2;
}

int edge_sched_deadline_reset_circuit(edge_sched_deadline_monitor_t *mon,
                                      uint32_t task_id) {
    if (!mon) {
        return -1;
    }
    for (uint32_t i = 0; i < EDGE_SCHED_MAX_TASKS; ++i) {
        if (mon->tasks[i].occupied && mon->tasks[i].config.task_id == task_id) {
            mon->tasks[i].stats.state = EDGE_SCHED_STATE_NORMAL;
            mon->tasks[i].stats.consecutive_misses = 0;
            return 0;
        }
    }
    return -2;
}

uint32_t edge_sched_deadline_get_task_count(const edge_sched_deadline_monitor_t *mon) {
    return mon ? mon->task_count : 0;
}
