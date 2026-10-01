/**
 * @file edge_prio_booster.c
 * @brief Dynamic Priority Boosting and Annealing Decay Engine (AEP-718 / Issue #249)
 */

#define _GNU_SOURCE
#include "edge_prio_booster.h"

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t task_id;
    char name[32];
    int base_priority;
    int current_priority;
    int peak_boost_priority;
    uint64_t hold_remaining_us;
    bool is_boosted;
    pthread_t thread_handle;
} edge_boost_task_slot_t;

struct edge_prio_booster {
    edge_prio_booster_config_t config;
    edge_prio_booster_port_t port;
    edge_boost_task_slot_t tasks[EDGE_BOOSTER_MAX_TASKS];
    uint32_t task_count;
    uint64_t total_boost_events;
    uint64_t total_anneal_events;
    pthread_mutex_t lock;
    bool initialized;
};

_Static_assert(sizeof(struct edge_prio_booster) <= sizeof(edge_prio_booster_storage_t),
               "edge_prio_booster_storage_t size is insufficient for struct edge_prio_booster");

static void apply_os_thread_priority(pthread_t thread, int prio) {
    if (thread == (pthread_t)0) return;
    struct sched_param sp;
    sp.sched_priority = prio;
    pthread_setschedparam(thread, SCHED_FIFO, &sp);
}

edge_prio_booster_t *edge_prio_booster_create_in(edge_prio_booster_storage_t *storage,
                                                 const edge_prio_booster_config_t *cfg,
                                                 const edge_prio_booster_port_t *port) {
    if (!storage || !cfg) {
        return NULL;
    }

    struct edge_prio_booster *booster = (struct edge_prio_booster *)storage->raw;
    memset(booster, 0x00, sizeof(*booster));
    booster->config = *cfg;
    if (booster->config.max_boosted_prio <= 0 || booster->config.max_boosted_prio > 99) {
        booster->config.max_boosted_prio = 99;
    }
    if (booster->config.default_boost_hold_us == 0) {
        booster->config.default_boost_hold_us = 50000; // 50ms default
    }
    if (port) {
        booster->port = *port;
    }

    pthread_mutex_init(&booster->lock, NULL);
    booster->initialized = true;
    return booster;
}

int edge_prio_booster_destroy(edge_prio_booster_t *booster) {
    if (!booster || !booster->initialized) {
        return -EINVAL;
    }

    pthread_mutex_lock(&booster->lock);
    for (uint32_t i = 0; i < booster->task_count; i++) {
        if (booster->tasks[i].is_boosted) {
            booster->tasks[i].current_priority = booster->tasks[i].base_priority;
            booster->tasks[i].is_boosted = false;
            apply_os_thread_priority(booster->tasks[i].thread_handle, booster->tasks[i].base_priority);
        }
    }
    booster->initialized = false;
    pthread_mutex_unlock(&booster->lock);
    pthread_mutex_destroy(&booster->lock);

    return 0;
}

int edge_prio_booster_register_task(edge_prio_booster_t *booster, const edge_boost_task_config_t *tcfg) {
    if (!booster || !booster->initialized || !tcfg) {
        return -EINVAL;
    }

    pthread_mutex_lock(&booster->lock);
    if (booster->task_count >= EDGE_BOOSTER_MAX_TASKS) {
        pthread_mutex_unlock(&booster->lock);
        return -ENOMEM;
    }

    // Check duplicate ID
    for (uint32_t i = 0; i < booster->task_count; i++) {
        if (booster->tasks[i].task_id == tcfg->task_id) {
            pthread_mutex_unlock(&booster->lock);
            return -EEXIST;
        }
    }

    edge_boost_task_slot_t *slot = &booster->tasks[booster->task_count++];
    slot->task_id = tcfg->task_id;
    strncpy(slot->name, tcfg->name, sizeof(slot->name) - 1);
    slot->base_priority = (tcfg->base_priority > 0) ? tcfg->base_priority : 1;
    slot->current_priority = slot->base_priority;
    slot->peak_boost_priority = slot->base_priority;
    slot->hold_remaining_us = 0;
    slot->is_boosted = false;
    slot->thread_handle = tcfg->thread_handle;

    pthread_mutex_unlock(&booster->lock);
    return 0;
}

int edge_prio_booster_trigger_boost(edge_prio_booster_t *booster,
                                    uint32_t task_id,
                                    int target_boost_prio,
                                    uint64_t hold_duration_us,
                                    const char *reason) {
    if (!booster || !booster->initialized) {
        return -EINVAL;
    }

    pthread_mutex_lock(&booster->lock);
    edge_boost_task_slot_t *target = NULL;
    for (uint32_t i = 0; i < booster->task_count; i++) {
        if (booster->tasks[i].task_id == task_id) {
            target = &booster->tasks[i];
            break;
        }
    }

    if (!target) {
        pthread_mutex_unlock(&booster->lock);
        return -ENOENT;
    }

    int old_prio = target->current_priority;
    int boosted_prio = target_boost_prio;
    if (boosted_prio <= 0 || boosted_prio > booster->config.max_boosted_prio) {
        boosted_prio = booster->config.max_boosted_prio;
    }

    target->current_priority = boosted_prio;
    target->peak_boost_priority = boosted_prio;
    target->hold_remaining_us = (hold_duration_us > 0) ? hold_duration_us : booster->config.default_boost_hold_us;
    target->is_boosted = true;
    booster->total_boost_events++;

    apply_os_thread_priority(target->thread_handle, boosted_prio);
    pthread_mutex_unlock(&booster->lock);

    if (booster->port.on_priority_boosted) {
        booster->port.on_priority_boosted(booster->port.ctx, task_id, old_prio, boosted_prio,
                                          reason ? reason : "emergency_boost");
    }

    return 0;
}

int edge_prio_booster_tick(edge_prio_booster_t *booster, uint64_t elapsed_us) {
    if (!booster || !booster->initialized) {
        return -EINVAL;
    }

    pthread_mutex_lock(&booster->lock);
    for (uint32_t i = 0; i < booster->task_count; i++) {
        edge_boost_task_slot_t *slot = &booster->tasks[i];
        if (!slot->is_boosted) {
            continue;
        }

        if (slot->hold_remaining_us > 0) {
            if (elapsed_us >= slot->hold_remaining_us) {
                slot->hold_remaining_us = 0;
            } else {
                slot->hold_remaining_us -= elapsed_us;
            }
        }

        if (slot->hold_remaining_us == 0) {
            // Anneal priority
            if (booster->config.anneal_curve == EDGE_ANNEAL_STEP) {
                slot->current_priority = slot->base_priority;
                slot->is_boosted = false;
                booster->total_anneal_events++;
                apply_os_thread_priority(slot->thread_handle, slot->base_priority);

                if (booster->port.on_priority_annealed) {
                    pthread_mutex_unlock(&booster->lock);
                    booster->port.on_priority_annealed(booster->port.ctx, slot->task_id, slot->current_priority, slot->base_priority);
                    pthread_mutex_lock(&booster->lock);
                }
            } else if (booster->config.anneal_curve == EDGE_ANNEAL_LINEAR) {
                // Decay by step
                int decay_step = 5;
                if (slot->current_priority > slot->base_priority + decay_step) {
                    slot->current_priority -= decay_step;
                    apply_os_thread_priority(slot->thread_handle, slot->current_priority);
                } else {
                    slot->current_priority = slot->base_priority;
                    slot->is_boosted = false;
                    booster->total_anneal_events++;
                    apply_os_thread_priority(slot->thread_handle, slot->base_priority);

                    if (booster->port.on_priority_annealed) {
                        pthread_mutex_unlock(&booster->lock);
                        booster->port.on_priority_annealed(booster->port.ctx, slot->task_id, slot->current_priority, slot->base_priority);
                        pthread_mutex_lock(&booster->lock);
                    }
                }
            }
        }
    }
    pthread_mutex_unlock(&booster->lock);

    return 0;
}

int edge_prio_booster_get_task_priority(const edge_prio_booster_t *booster, uint32_t task_id, int *current_prio) {
    if (!booster || !booster->initialized || !current_prio) {
        return -EINVAL;
    }

    int rc = -ENOENT;
    for (uint32_t i = 0; i < booster->task_count; i++) {
        if (booster->tasks[i].task_id == task_id) {
            *current_prio = booster->tasks[i].current_priority;
            rc = 0;
            break;
        }
    }
    return rc;
}

int edge_prio_booster_get_stats(const edge_prio_booster_t *booster, edge_prio_booster_stats_t *stats) {
    if (!booster || !booster->initialized || !stats) {
        return -EINVAL;
    }

    memset(stats, 0x00, sizeof(*stats));
    stats->registered_task_count = booster->task_count;
    stats->total_boost_events = booster->total_boost_events;
    stats->total_anneal_events = booster->total_anneal_events;

    uint32_t active = 0;
    for (uint32_t i = 0; i < booster->task_count; i++) {
        if (booster->tasks[i].is_boosted) {
            active++;
        }
    }
    stats->active_boosted_count = active;

    return 0;
}
