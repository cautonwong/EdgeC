/**
 * @file edge_coop_guard.c
 * @brief Cooperative Timeslice Overrun Watchdog and Soft Circuit Breaker (AEP-719 / Issue #250)
 */

#include "edge_coop_guard.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t task_id;
    char name[32];
    edge_coop_state_t state;
    uint64_t timeslice_budget_us;
    uint32_t max_degrade_overruns;
    uint32_t max_trip_overruns;
    uint64_t cooldown_us;

    uint64_t current_slice_start_us;
    uint64_t tripped_timestamp_us;
    uint64_t last_execution_us;
    uint64_t max_execution_us;
    uint32_t consecutive_overruns;
    uint64_t total_overruns;
    uint64_t total_executions;
} edge_coop_task_slot_t;

struct edge_coop_guard {
    edge_coop_guard_config_t config;
    edge_coop_guard_port_t port;
    edge_coop_task_slot_t tasks[EDGE_COOP_MAX_TASKS];
    uint32_t task_count;

    uint64_t total_overrun_events;
    uint64_t total_tripped_events;
    uint64_t total_recovery_events;

    pthread_mutex_t lock;
    bool initialized;
};

_Static_assert(sizeof(struct edge_coop_guard) <= sizeof(edge_coop_guard_storage_t),
               "edge_coop_guard_storage_t size is insufficient for struct edge_coop_guard");

edge_coop_guard_t *edge_coop_guard_create_in(edge_coop_guard_storage_t *storage,
                                             const edge_coop_guard_config_t *cfg,
                                             const edge_coop_guard_port_t *port) {
    if (!storage || !cfg) {
        return NULL;
    }

    struct edge_coop_guard *guard = (struct edge_coop_guard *)storage->raw;
    memset(guard, 0x00, sizeof(*guard));
    guard->config = *cfg;
    if (guard->config.default_budget_us == 0) guard->config.default_budget_us = 10000;
    if (guard->config.default_degrade_threshold == 0) guard->config.default_degrade_threshold = 3;
    if (guard->config.default_trip_threshold == 0) guard->config.default_trip_threshold = 5;
    if (guard->config.default_cooldown_us == 0) guard->config.default_cooldown_us = 1000000; // 1s

    if (port) {
        guard->port = *port;
    }

    pthread_mutex_init(&guard->lock, NULL);
    guard->initialized = true;
    return guard;
}

int edge_coop_guard_destroy(edge_coop_guard_t *guard) {
    if (!guard || !guard->initialized) {
        return -EINVAL;
    }
    guard->initialized = false;
    pthread_mutex_destroy(&guard->lock);
    return 0;
}

int edge_coop_guard_register_task(edge_coop_guard_t *guard, const edge_coop_task_config_t *tcfg) {
    if (!guard || !guard->initialized || !tcfg) {
        return -EINVAL;
    }

    pthread_mutex_lock(&guard->lock);
    if (guard->task_count >= EDGE_COOP_MAX_TASKS) {
        pthread_mutex_unlock(&guard->lock);
        return -ENOMEM;
    }

    for (uint32_t i = 0; i < guard->task_count; i++) {
        if (guard->tasks[i].task_id == tcfg->task_id) {
            pthread_mutex_unlock(&guard->lock);
            return -EEXIST;
        }
    }

    edge_coop_task_slot_t *slot = &guard->tasks[guard->task_count++];
    slot->task_id = tcfg->task_id;
    strncpy(slot->name, tcfg->name, sizeof(slot->name) - 1);
    slot->state = EDGE_COOP_STATE_HEALTHY;
    slot->timeslice_budget_us = (tcfg->timeslice_budget_us > 0) ? tcfg->timeslice_budget_us : guard->config.default_budget_us;
    slot->max_degrade_overruns = (tcfg->max_consecutive_overruns_to_degrade > 0) ? tcfg->max_consecutive_overruns_to_degrade : guard->config.default_degrade_threshold;
    slot->max_trip_overruns = (tcfg->max_consecutive_overruns_to_trip > 0) ? tcfg->max_consecutive_overruns_to_trip : guard->config.default_trip_threshold;
    slot->cooldown_us = (tcfg->recovery_cooldown_us > 0) ? tcfg->recovery_cooldown_us : guard->config.default_cooldown_us;

    pthread_mutex_unlock(&guard->lock);
    return 0;
}

bool edge_coop_guard_can_execute(edge_coop_guard_t *guard, uint32_t task_id, uint64_t now_us) {
    if (!guard || !guard->initialized) {
        return false;
    }

    pthread_mutex_lock(&guard->lock);
    edge_coop_task_slot_t *slot = NULL;
    for (uint32_t i = 0; i < guard->task_count; i++) {
        if (guard->tasks[i].task_id == task_id) {
            slot = &guard->tasks[i];
            break;
        }
    }

    if (!slot) {
        pthread_mutex_unlock(&guard->lock);
        return false;
    }

    if (slot->state == EDGE_COOP_STATE_HEALTHY || slot->state == EDGE_COOP_STATE_DEGRADED) {
        pthread_mutex_unlock(&guard->lock);
        return true;
    }

    if (slot->state == EDGE_COOP_STATE_TRIPPED) {
        if (now_us >= slot->tripped_timestamp_us + slot->cooldown_us) {
            edge_coop_state_t prev = slot->state;
            slot->state = EDGE_COOP_STATE_HALF_OPEN;
            pthread_mutex_unlock(&guard->lock);

            if (guard->port.on_state_transition) {
                guard->port.on_state_transition(guard->port.ctx, task_id, prev, EDGE_COOP_STATE_HALF_OPEN, "cooldown_probe_half_open");
            }
            return true;
        }
        pthread_mutex_unlock(&guard->lock);
        return false;
    }

    pthread_mutex_unlock(&guard->lock);
    return true; // HALF_OPEN
}

int edge_coop_guard_enter_slice(edge_coop_guard_t *guard, uint32_t task_id, uint64_t start_time_us) {
    if (!guard || !guard->initialized) {
        return -EINVAL;
    }

    pthread_mutex_lock(&guard->lock);
    for (uint32_t i = 0; i < guard->task_count; i++) {
        if (guard->tasks[i].task_id == task_id) {
            guard->tasks[i].current_slice_start_us = start_time_us;
            pthread_mutex_unlock(&guard->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&guard->lock);
    return -ENOENT;
}

int edge_coop_guard_exit_slice(edge_coop_guard_t *guard, uint32_t task_id, uint64_t end_time_us) {
    if (!guard || !guard->initialized) {
        return -EINVAL;
    }

    pthread_mutex_lock(&guard->lock);
    edge_coop_task_slot_t *slot = NULL;
    for (uint32_t i = 0; i < guard->task_count; i++) {
        if (guard->tasks[i].task_id == task_id) {
            slot = &guard->tasks[i];
            break;
        }
    }

    if (!slot) {
        pthread_mutex_unlock(&guard->lock);
        return -ENOENT;
    }

    uint64_t duration_us = (end_time_us >= slot->current_slice_start_us) ? (end_time_us - slot->current_slice_start_us) : 0;
    slot->last_execution_us = duration_us;
    if (duration_us > slot->max_execution_us) {
        slot->max_execution_us = duration_us;
    }
    slot->total_executions++;

    bool is_overrun = (duration_us > slot->timeslice_budget_us);
    edge_coop_state_t prev_state = slot->state;
    edge_coop_state_t new_state = prev_state;
    const char *reason = NULL;

    if (is_overrun) {
        slot->consecutive_overruns++;
        slot->total_overruns++;
        guard->total_overrun_events++;

        if (slot->state == EDGE_COOP_STATE_HALF_OPEN) {
            new_state = EDGE_COOP_STATE_TRIPPED;
            slot->tripped_timestamp_us = end_time_us;
            guard->total_tripped_events++;
            reason = "probe_failed_re_tripped";
        } else if (slot->consecutive_overruns >= slot->max_trip_overruns) {
            new_state = EDGE_COOP_STATE_TRIPPED;
            slot->tripped_timestamp_us = end_time_us;
            guard->total_tripped_events++;
            reason = "consecutive_overruns_trip";
        } else if (slot->consecutive_overruns >= slot->max_degrade_overruns) {
            new_state = EDGE_COOP_STATE_DEGRADED;
            reason = "consecutive_overruns_degrade";
        }
    } else {
        slot->consecutive_overruns = 0;
        if (slot->state == EDGE_COOP_STATE_HALF_OPEN || slot->state == EDGE_COOP_STATE_DEGRADED) {
            new_state = EDGE_COOP_STATE_HEALTHY;
            guard->total_recovery_events++;
            reason = "execution_within_budget_recovered";
        }
    }

    slot->state = new_state;
    pthread_mutex_unlock(&guard->lock);

    if (is_overrun && guard->port.on_timeslice_overrun) {
        guard->port.on_timeslice_overrun(guard->port.ctx, task_id, duration_us, slot->timeslice_budget_us);
    }

    if (new_state != prev_state && guard->port.on_state_transition) {
        guard->port.on_state_transition(guard->port.ctx, task_id, prev_state, new_state, reason);
    }

    return 0;
}

int edge_coop_guard_get_task_status(const edge_coop_guard_t *guard, uint32_t task_id, edge_coop_task_status_t *status) {
    if (!guard || !guard->initialized || !status) {
        return -EINVAL;
    }

    for (uint32_t i = 0; i < guard->task_count; i++) {
        if (guard->tasks[i].task_id == task_id) {
            status->task_id = task_id;
            status->state = guard->tasks[i].state;
            status->timeslice_budget_us = guard->tasks[i].timeslice_budget_us;
            status->last_execution_us = guard->tasks[i].last_execution_us;
            status->max_execution_us = guard->tasks[i].max_execution_us;
            status->consecutive_overruns = guard->tasks[i].consecutive_overruns;
            status->total_overruns = guard->tasks[i].total_overruns;
            status->total_executions = guard->tasks[i].total_executions;
            return 0;
        }
    }
    return -ENOENT;
}

int edge_coop_guard_reset_task(edge_coop_guard_t *guard, uint32_t task_id) {
    if (!guard || !guard->initialized) {
        return -EINVAL;
    }

    pthread_mutex_lock(&guard->lock);
    for (uint32_t i = 0; i < guard->task_count; i++) {
        if (guard->tasks[i].task_id == task_id) {
            guard->tasks[i].state = EDGE_COOP_STATE_HEALTHY;
            guard->tasks[i].consecutive_overruns = 0;
            pthread_mutex_unlock(&guard->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&guard->lock);
    return -ENOENT;
}

int edge_coop_guard_get_stats(const edge_coop_guard_t *guard, edge_coop_guard_stats_t *stats) {
    if (!guard || !guard->initialized || !stats) {
        return -EINVAL;
    }

    memset(stats, 0x00, sizeof(*stats));
    stats->registered_task_count = guard->task_count;
    stats->total_overrun_events = guard->total_overrun_events;
    stats->total_tripped_events = guard->total_tripped_events;
    stats->total_recovery_events = guard->total_recovery_events;

    for (uint32_t i = 0; i < guard->task_count; i++) {
        if (guard->tasks[i].state == EDGE_COOP_STATE_HEALTHY) {
            stats->healthy_task_count++;
        } else if (guard->tasks[i].state == EDGE_COOP_STATE_DEGRADED) {
            stats->degraded_task_count++;
        } else {
            stats->tripped_task_count++;
        }
    }

    return 0;
}
