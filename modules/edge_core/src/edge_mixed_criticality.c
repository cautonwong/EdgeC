/**
 * @file edge_mixed_criticality.c
 * @brief Mixed-Criticality Real-Time Dispatcher and MTT Deterministic Timing (AEP-602 / Issue #221)
 */

#include "edge_mixed_criticality.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    edge_mixed_crit_task_config_t config;
    uint64_t next_release_time_us;
    uint32_t skipped_cycles;
    uint64_t last_dispatch_time_us;
    int64_t last_drift_us;
    bool is_active;
} edge_mixed_task_slot_t;

struct edge_mixed_crit_dispatcher {
    edge_mixed_crit_config_t config;
    edge_mixed_crit_port_t port;
    edge_mixed_task_slot_t tasks[EDGE_MIXED_CRIT_MAX_TASKS];
    uint32_t task_count;

    float current_temp_c;
    float current_cpu_load;
    bool thermal_warning;
    bool thermal_critical;

    uint64_t l1_count;
    uint64_t l2_count;
    uint64_t l3_count;
    uint64_t l4_count;
    uint64_t l3_skipped_count;
    uint64_t l4_dropped_count;
    uint64_t inversions_prevented;
    int64_t max_mtt_drift_us;

    pthread_mutex_t lock;
    bool initialized;
};

_Static_assert(sizeof(struct edge_mixed_crit_dispatcher) <= sizeof(edge_mixed_crit_storage_t),
               "edge_mixed_crit_storage_t size is insufficient for struct edge_mixed_crit_dispatcher");

static inline uint64_t get_time_now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000);
}

edge_mixed_crit_dispatcher_t *edge_mixed_crit_create_in(edge_mixed_crit_storage_t *storage,
                                                         const edge_mixed_crit_config_t *cfg,
                                                         const edge_mixed_crit_port_t *port) {
    if (!storage || !cfg) {
        return NULL;
    }

    struct edge_mixed_crit_dispatcher *disp = (struct edge_mixed_crit_dispatcher *)storage->raw;
    memset(disp, 0x00, sizeof(*disp));
    disp->config = *cfg;
    if (disp->config.thermal_warning_temp_c <= 0.0f) disp->config.thermal_warning_temp_c = 70.0f;
    if (disp->config.thermal_critical_temp_c <= 0.0f) disp->config.thermal_critical_temp_c = 85.0f;
    if (disp->config.l3_decimation_factor == 0) disp->config.l3_decimation_factor = 4;

    if (port) {
        disp->port = *port;
    }

    pthread_mutex_init(&disp->lock, NULL);
    disp->initialized = true;
    return disp;
}

int edge_mixed_crit_destroy(edge_mixed_crit_dispatcher_t *disp) {
    if (!disp || !disp->initialized) {
        return -EINVAL;
    }
    disp->initialized = false;
    pthread_mutex_destroy(&disp->lock);
    return 0;
}

int edge_mixed_crit_register_task(edge_mixed_crit_dispatcher_t *disp, const edge_mixed_crit_task_config_t *tcfg) {
    if (!disp || !disp->initialized || !tcfg) {
        return -EINVAL;
    }

    pthread_mutex_lock(&disp->lock);
    if (disp->task_count >= EDGE_MIXED_CRIT_MAX_TASKS) {
        pthread_mutex_unlock(&disp->lock);
        return -ENOMEM;
    }

    for (uint32_t i = 0; i < disp->task_count; i++) {
        if (disp->tasks[i].config.task_id == tcfg->task_id) {
            pthread_mutex_unlock(&disp->lock);
            return -EEXIST;
        }
    }

    edge_mixed_task_slot_t *slot = &disp->tasks[disp->task_count++];
    slot->config = *tcfg;
    slot->next_release_time_us = 0;
    slot->skipped_cycles = 0;
    slot->last_dispatch_time_us = 0;
    slot->last_drift_us = 0;
    slot->is_active = true;

    pthread_mutex_unlock(&disp->lock);
    return 0;
}

int edge_mixed_crit_update_thermal(edge_mixed_crit_dispatcher_t *disp, float junction_temp_c, float cpu_load_pct) {
    if (!disp || !disp->initialized) {
        return -EINVAL;
    }

    pthread_mutex_lock(&disp->lock);
    disp->current_temp_c = junction_temp_c;
    disp->current_cpu_load = cpu_load_pct;

    bool was_throttling = disp->thermal_warning || disp->thermal_critical;
    disp->thermal_warning = (junction_temp_c >= disp->config.thermal_warning_temp_c);
    disp->thermal_critical = (junction_temp_c >= disp->config.thermal_critical_temp_c);
    bool now_throttling = disp->thermal_warning || disp->thermal_critical;

    pthread_mutex_unlock(&disp->lock);

    if (now_throttling && !was_throttling && disp->port.on_thermal_throttling_engaged) {
        disp->port.on_thermal_throttling_engaged(disp->port.ctx, junction_temp_c, disp->thermal_critical);
    }

    return 0;
}

int edge_mixed_crit_step(edge_mixed_crit_dispatcher_t *disp, uint64_t now_us, edge_mixed_crit_dispatch_result_t *res) {
    if (!disp || !disp->initialized || !res) {
        return -EINVAL;
    }

    memset(res, 0x00, sizeof(*res));
    pthread_mutex_lock(&disp->lock);

    // 1. Scan ready tasks and select candidate with highest criticality (L1 > L2 > L3 > L4)
    int best_idx = -1;
    edge_criticality_level_t highest_crit = CRITICALITY_L4_CLOUD + 1;

    for (uint32_t i = 0; i < disp->task_count; i++) {
        edge_mixed_task_slot_t *slot = &disp->tasks[i];
        if (!slot->is_active) continue;

        if (slot->next_release_time_us <= now_us) {
            if (slot->config.level < highest_crit) {
                highest_crit = slot->config.level;
                best_idx = (int)i;
            }
        }
    }

    if (best_idx < 0) {
        pthread_mutex_unlock(&disp->lock);
        return 0; // No task ready
    }

    edge_mixed_task_slot_t *chosen = &disp->tasks[best_idx];

    // Check if L1 safety preempted lower level ready tasks
    for (uint32_t i = 0; i < disp->task_count; i++) {
        if ((int)i != best_idx && disp->tasks[i].next_release_time_us <= now_us) {
            if (disp->tasks[i].config.level > chosen->config.level && chosen->config.level == CRITICALITY_L1_SAFETY) {
                disp->inversions_prevented++;
                res->preempted_lower_task = true;
                if (disp->port.on_criticality_inversion_prevented) {
                    disp->port.on_criticality_inversion_prevented(disp->port.ctx, chosen->config.task_id, disp->tasks[i].config.task_id);
                }
                break;
            }
        }
    }

    // Thermal throttling checks for L3 & L4
    if (chosen->config.level == CRITICALITY_L3_COMPUTE && disp->config.enable_thermal_throttling) {
        if (disp->thermal_critical) {
            disp->l3_skipped_count++;
            chosen->next_release_time_us = now_us + chosen->config.period_us;
            pthread_mutex_unlock(&disp->lock);
            return 0; // Skipped L3 under critical thermal
        } else if (disp->thermal_warning) {
            chosen->skipped_cycles++;
            if (chosen->skipped_cycles < disp->config.l3_decimation_factor) {
                disp->l3_skipped_count++;
                chosen->next_release_time_us = now_us + chosen->config.period_us;
                pthread_mutex_unlock(&disp->lock);
                return 0; // Throttled cycle
            }
            chosen->skipped_cycles = 0;
        }
    } else if (chosen->config.level == CRITICALITY_L4_CLOUD && disp->config.enable_thermal_throttling) {
        if (disp->thermal_warning || disp->thermal_critical) {
            disp->l4_dropped_count++;
            chosen->next_release_time_us = now_us + chosen->config.period_us;
            pthread_mutex_unlock(&disp->lock);
            return 0; // Dropped L4 under thermal stress
        }
    }

    // MTT Wall-clock drift calculation for L2
    int64_t drift_us = 0;
    if (chosen->config.level == CRITICALITY_L2_MTT && chosen->config.mtt_wall_clock_interval_us > 0) {
        uint64_t interval = chosen->config.mtt_wall_clock_interval_us;
        uint64_t aligned_base = (now_us / interval) * interval;
        drift_us = (int64_t)now_us - (int64_t)aligned_base;
        chosen->last_drift_us = drift_us;
        if (labs(drift_us) > labs(disp->max_mtt_drift_us)) {
            disp->max_mtt_drift_us = drift_us;
        }
        if (disp->port.on_mtt_aligned_dispatch) {
            disp->port.on_mtt_aligned_dispatch(disp->port.ctx, chosen->config.task_id, drift_us);
        }
    }

    // Execute task
    uint64_t t_start = get_time_now_us();
    int ret = 0;
    if (chosen->config.execute_fn) {
        ret = chosen->config.execute_fn(chosen->config.ctx);
    }
    uint64_t t_end = get_time_now_us();
    uint64_t latency = (t_end >= t_start) ? (t_end - t_start) : 0;

    // Update stats and advance release time
    if (chosen->config.level == CRITICALITY_L1_SAFETY) disp->l1_count++;
    else if (chosen->config.level == CRITICALITY_L2_MTT) disp->l2_count++;
    else if (chosen->config.level == CRITICALITY_L3_COMPUTE) disp->l3_count++;
    else if (chosen->config.level == CRITICALITY_L4_CLOUD) disp->l4_count++;

    chosen->last_dispatch_time_us = now_us;
    if (chosen->config.period_us > 0) {
        chosen->next_release_time_us = now_us + chosen->config.period_us;
    } else {
        chosen->next_release_time_us = now_us + 1000;
    }

    res->task_dispatched = true;
    res->active_task_id = chosen->config.task_id;
    res->active_level = chosen->config.level;
    res->execution_latency_us = latency;
    res->wall_clock_drift_us = drift_us;
    res->task_return_code = ret;

    pthread_mutex_unlock(&disp->lock);
    return 0;
}

int edge_mixed_crit_get_stats(const edge_mixed_crit_dispatcher_t *disp, edge_mixed_crit_stats_t *stats) {
    if (!disp || !disp->initialized || !stats) {
        return -EINVAL;
    }

    memset(stats, 0x00, sizeof(*stats));
    stats->registered_task_count = disp->task_count;
    stats->l1_dispatch_count = disp->l1_count;
    stats->l2_dispatch_count = disp->l2_count;
    stats->l3_dispatch_count = disp->l3_count;
    stats->l4_dispatch_count = disp->l4_count;
    stats->l3_skipped_throttled_count = disp->l3_skipped_count;
    stats->l4_dropped_count = disp->l4_dropped_count;
    stats->criticality_inversions_prevented = disp->inversions_prevented;
    stats->max_mtt_drift_us = disp->max_mtt_drift_us;
    stats->current_temperature_c = disp->current_temp_c;
    stats->thermal_throttle_active = disp->thermal_warning || disp->thermal_critical;

    return 0;
}
