/**
 * @file edge_rms_scheduler.c
 * @brief Rate Monotonic Scheduling (RMS) Static Deterministic Periodic Scheduler & Budget Watchdog (AEP-713 / Issue #246)
 */

#include "edge_rms_scheduler.h"
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>

/**
 * @brief Internal task entry descriptor
 */
typedef struct {
    edge_rms_task_config_t config;
    bool is_active;
    uint32_t static_priority;        /**< 1 = highest, assigned by period */
    edge_rms_task_state_t state;
    uint64_t next_release_time_us;
    uint64_t current_period_start_us;
    uint64_t current_absolute_deadline_us;
    uint64_t remaining_budget_us;
    uint64_t exec_accumulated_us;
    uint64_t total_job_count;
    uint64_t total_overrun_count;
    uint64_t total_miss_count;
} edge_rms_task_entry_t;

/**
 * @brief Concrete structure for RMS Scheduler
 */
struct edge_rms_scheduler {
    edge_rms_config_t config;
    edge_rms_port_t port;
    edge_rms_stats_t stats;
    edge_rms_task_entry_t tasks[EDGE_RMS_MAX_TASKS];
    uint32_t active_task_count;
    int current_running_index;       /**< -1 if CPU is idle */
    uint64_t current_sim_time_us;
    bool initialized;
};

_Static_assert(sizeof(struct edge_rms_scheduler) <= sizeof(edge_rms_scheduler_storage_t),
               "edge_rms_scheduler_storage_t buffer size too small for struct edge_rms_scheduler");

static void reassign_static_priorities(struct edge_rms_scheduler *s)
{
    if (!s->config.auto_assign_priorities) return;

    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        if (!s->tasks[i].is_active) continue;
        uint32_t prio = 1;
        for (size_t j = 0; j < EDGE_RMS_MAX_TASKS; j++) {
            if (i == j || !s->tasks[j].is_active) continue;
            // Shorter period = higher priority (smaller priority number)
            if (s->tasks[j].config.period_us < s->tasks[i].config.period_us) {
                prio++;
            } else if (s->tasks[j].config.period_us == s->tasks[i].config.period_us &&
                       s->tasks[j].config.task_id < s->tasks[i].config.task_id) {
                prio++; // Tie-breaker by task_id
            }
        }
        s->tasks[i].static_priority = prio;
    }
}

edge_rms_scheduler_t *edge_rms_scheduler_create_in(edge_rms_scheduler_storage_t *store,
                                                  const edge_rms_config_t *cfg,
                                                  const edge_rms_port_t *port)
{
    if (!store || !cfg) {
        return NULL;
    }

    struct edge_rms_scheduler *impl = (struct edge_rms_scheduler *)store->raw;
    memset(impl, 0, sizeof(*impl));
    memcpy(&impl->config, cfg, sizeof(edge_rms_config_t));
    if (port) {
        memcpy(&impl->port, port, sizeof(edge_rms_port_t));
    }

    impl->current_running_index = -1;
    impl->initialized = true;
    return (edge_rms_scheduler_t *)impl;
}

int edge_rms_scheduler_register_task(edge_rms_scheduler_t *sched,
                                    const edge_rms_task_config_t *task_cfg)
{
    if (!sched || !task_cfg || !sched->initialized) {
        return -EINVAL;
    }
    if (task_cfg->period_us == 0 || task_cfg->wcet_budget_us == 0) {
        return -EINVAL;
    }
    if (task_cfg->wcet_budget_us > task_cfg->period_us) {
        return -EINVAL; // Physical WCET budget cannot exceed period
    }

    struct edge_rms_scheduler *s = (struct edge_rms_scheduler *)sched;

    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        if (s->tasks[i].is_active && s->tasks[i].config.task_id == task_cfg->task_id) {
            return -EEXIST;
        }
    }

    int free_slot = -1;
    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        if (!s->tasks[i].is_active) {
            free_slot = (int)i;
            break;
        }
    }
    if (free_slot < 0) {
        return -ENOMEM;
    }

    edge_rms_task_entry_t *entry = &s->tasks[free_slot];
    memset(entry, 0, sizeof(*entry));
    memcpy(&entry->config, task_cfg, sizeof(edge_rms_task_config_t));
    if (entry->config.relative_deadline_us == 0) {
        entry->config.relative_deadline_us = entry->config.period_us;
    }
    entry->is_active = true;
    entry->state = EDGE_RMS_STATE_DORMANT;
    entry->next_release_time_us = task_cfg->initial_phase_us;

    s->active_task_count++;
    reassign_static_priorities(s);

    s->stats.registered_task_count = s->active_task_count;
    s->stats.total_utilization = edge_rms_scheduler_get_utilization(sched);
    s->stats.bound_utilization = edge_rms_scheduler_get_bound(sched);

    if (s->stats.total_utilization > s->stats.bound_utilization) {
        if (s->port.on_schedulability_warning) {
            s->port.on_schedulability_warning(s->port.ctx, s->stats.total_utilization,
                                              s->stats.bound_utilization,
                                              "Utilization exceeds Liu & Layland sufficient bound (requires exact RTA)");
        }
    }

    return 0;
}

int edge_rms_scheduler_unregister_task(edge_rms_scheduler_t *sched, uint32_t task_id)
{
    if (!sched || !sched->initialized) {
        return -EINVAL;
    }

    struct edge_rms_scheduler *s = (struct edge_rms_scheduler *)sched;
    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        if (s->tasks[i].is_active && s->tasks[i].config.task_id == task_id) {
            s->tasks[i].is_active = false;
            s->tasks[i].state = EDGE_RMS_STATE_DORMANT;
            if (s->current_running_index == (int)i) {
                s->current_running_index = -1;
            }
            s->active_task_count--;
            reassign_static_priorities(s);
            s->stats.registered_task_count = s->active_task_count;
            s->stats.total_utilization = edge_rms_scheduler_get_utilization(sched);
            s->stats.bound_utilization = edge_rms_scheduler_get_bound(sched);
            return 0;
        }
    }
    return -ENOENT;
}

double edge_rms_scheduler_get_utilization(const edge_rms_scheduler_t *sched)
{
    if (!sched || !sched->initialized) {
        return -1.0;
    }

    const struct edge_rms_scheduler *s = (const struct edge_rms_scheduler *)sched;
    double u_sum = 0.0;
    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        if (s->tasks[i].is_active && s->tasks[i].config.period_us > 0) {
            u_sum += (double)s->tasks[i].config.wcet_budget_us / (double)s->tasks[i].config.period_us;
        }
    }
    return u_sum;
}

double edge_rms_scheduler_get_bound(const edge_rms_scheduler_t *sched)
{
    if (!sched || !sched->initialized) {
        return -1.0;
    }

    const struct edge_rms_scheduler *s = (const struct edge_rms_scheduler *)sched;
    uint32_t n = s->active_task_count;
    if (n == 0) return 1.0;
    if (n == 1) return 1.0;

    // Liu & Layland bound: U_bound = n * (2^(1/n) - 1)
    return (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);
}

int edge_rms_scheduler_check_schedulability(const edge_rms_scheduler_t *sched,
                                           edge_rms_schedulability_report_t *report)
{
    if (!sched || !report || !sched->initialized) {
        return -EINVAL;
    }

    const struct edge_rms_scheduler *s = (const struct edge_rms_scheduler *)sched;
    memset(report, 0, sizeof(*report));

    report->total_utilization = edge_rms_scheduler_get_utilization(sched);
    report->liu_layland_bound = edge_rms_scheduler_get_bound(sched);
    report->task_count = s->active_task_count;

    if (s->active_task_count == 0) {
        report->is_schedulable = true;
        report->passed_sufficient_bound = true;
        report->passed_exact_rta = true;
        return 0;
    }

    if (report->total_utilization > 1.0) {
        report->is_schedulable = false;
        report->passed_sufficient_bound = false;
        report->passed_exact_rta = false;
        snprintf(report->failure_reason, sizeof(report->failure_reason),
                 "Total utilization U=%.3f > 1.0 (Hardware Capacity Exceeded)", report->total_utilization);
        return 0;
    }

    if (report->total_utilization <= report->liu_layland_bound) {
        report->passed_sufficient_bound = true;
    } else {
        report->passed_sufficient_bound = false;
    }

    // Exact Response Time Analysis (RTA) for each task in priority order
    bool all_tasks_schedulable = true;
    size_t rta_idx = 0;

    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        if (!s->tasks[i].is_active) continue;

        edge_rms_task_rta_info_t *info = &report->task_rta[rta_idx++];
        info->task_id = s->tasks[i].config.task_id;
        info->static_priority = s->tasks[i].static_priority;
        info->task_utilization = (double)s->tasks[i].config.wcet_budget_us / (double)s->tasks[i].config.period_us;

        uint64_t c_i = s->tasks[i].config.wcet_budget_us;
        uint64_t d_i = s->tasks[i].config.relative_deadline_us;
        uint64_t r_cur = c_i;

        // Fixed-point iteration: R^(k+1) = C_i + sum_{j < i} ceil(R^(k) / T_j) * C_j
        bool converged = false;
        for (int iter = 0; iter < 100; iter++) {
            uint64_t r_next = c_i;
            for (size_t j = 0; j < EDGE_RMS_MAX_TASKS; j++) {
                if (!s->tasks[j].is_active || i == j) continue;
                // Higher priority task has smaller static_priority
                if (s->tasks[j].static_priority < s->tasks[i].static_priority) {
                    uint64_t t_j = s->tasks[j].config.period_us;
                    uint64_t c_j = s->tasks[j].config.wcet_budget_us;
                    uint64_t ceil_val = (r_cur + t_j - 1) / t_j;
                    r_next += ceil_val * c_j;
                }
            }

            if (r_next > d_i) {
                r_cur = r_next;
                break; // Deadline missed
            }
            if (r_next == r_cur) {
                converged = true;
                break;
            }
            r_cur = r_next;
        }

        info->worst_case_response_time_us = r_cur;
        if (converged && r_cur <= d_i) {
            info->is_task_schedulable = true;
            info->slack_margin_us = d_i - r_cur;
        } else {
            info->is_task_schedulable = false;
            info->slack_margin_us = 0;
            all_tasks_schedulable = false;
        }
    }

    report->passed_exact_rta = all_tasks_schedulable;
    report->is_schedulable = all_tasks_schedulable;

    if (!all_tasks_schedulable) {
        snprintf(report->failure_reason, sizeof(report->failure_reason),
                 "Exact RTA failed: one or more tasks exceed relative deadline");
    }

    return 0;
}

int edge_rms_scheduler_step(edge_rms_scheduler_t *sched,
                           uint64_t current_time_us,
                           uint64_t step_duration_us,
                           edge_rms_dispatch_result_t *result)
{
    if (!sched || !result || !sched->initialized || step_duration_us == 0) {
        return -EINVAL;
    }

    struct edge_rms_scheduler *s = (struct edge_rms_scheduler *)sched;
    memset(result, 0, sizeof(*result));
    s->current_sim_time_us = current_time_us;

    // 1. Period release management
    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        edge_rms_task_entry_t *t = &s->tasks[i];
        if (!t->is_active) continue;

        if (current_time_us >= t->next_release_time_us) {
            // Check deadline miss from previous period
            if ((t->state == EDGE_RMS_STATE_READY || t->state == EDGE_RMS_STATE_RUNNING) &&
                t->remaining_budget_us > 0) {
                t->total_miss_count++;
                s->stats.total_deadline_misses++;
                result->is_deadline_miss = true;
                result->affected_task_id = t->config.task_id;

                uint64_t overdue = (current_time_us > t->current_absolute_deadline_us) ?
                                   (current_time_us - t->current_absolute_deadline_us) : 0;
                if (s->port.on_deadline_miss) {
                    s->port.on_deadline_miss(s->port.ctx, t->config.task_id, current_time_us, overdue);
                }
            }

            // Release new period instance
            t->current_period_start_us = t->next_release_time_us;
            t->current_absolute_deadline_us = t->next_release_time_us + t->config.relative_deadline_us;
            t->remaining_budget_us = t->config.wcet_budget_us;
            t->exec_accumulated_us = 0;
            if (t->state != EDGE_RMS_STATE_TRIPPED && t->state != EDGE_RMS_STATE_DORMANT) {
                t->state = EDGE_RMS_STATE_READY;
            } else if (t->state == EDGE_RMS_STATE_DORMANT) {
                t->state = EDGE_RMS_STATE_READY;
            }
            t->total_job_count++;
            t->next_release_time_us += t->config.period_us;
        }

        // Check active deadline miss
        if ((t->state == EDGE_RMS_STATE_READY || t->state == EDGE_RMS_STATE_RUNNING) &&
            current_time_us > t->current_absolute_deadline_us && t->remaining_budget_us > 0) {
            t->total_miss_count++;
            s->stats.total_deadline_misses++;
            result->is_deadline_miss = true;
            result->affected_task_id = t->config.task_id;
            uint64_t overdue = current_time_us - t->current_absolute_deadline_us;
            if (s->port.on_deadline_miss) {
                s->port.on_deadline_miss(s->port.ctx, t->config.task_id, current_time_us, overdue);
            }
        }
    }

    // 2. Rate Monotonic Selection: Highest static priority (smallest priority number)
    int best_index = -1;
    uint32_t highest_priority = UINT32_MAX;

    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        edge_rms_task_entry_t *t = &s->tasks[i];
        if (!t->is_active) continue;

        if ((t->state == EDGE_RMS_STATE_READY || t->state == EDGE_RMS_STATE_RUNNING) &&
            t->remaining_budget_us > 0) {
            if (t->static_priority < highest_priority) {
                highest_priority = t->static_priority;
                best_index = (int)i;
            }
        }
    }

    if (best_index < 0) {
        // CPU Idle
        s->current_running_index = -1;
        result->task_dispatched = false;
        return 0;
    }

    edge_rms_task_entry_t *chosen = &s->tasks[best_index];

    // 3. Preemption Check
    if (s->current_running_index >= 0 && s->current_running_index != best_index) {
        edge_rms_task_entry_t *prev = &s->tasks[s->current_running_index];
        if (prev->is_active && prev->state == EDGE_RMS_STATE_RUNNING) {
            prev->state = EDGE_RMS_STATE_READY;
            result->is_preemption = true;
            result->preempted_task_id = prev->config.task_id;
            s->stats.total_preemptions++;

            if (s->port.on_task_preempt) {
                s->port.on_task_preempt(s->port.ctx, prev->config.task_id,
                                        chosen->config.task_id, current_time_us);
            }
        }
    }

    s->current_running_index = best_index;
    chosen->state = EDGE_RMS_STATE_RUNNING;
    s->stats.total_dispatches++;

    // 4. Execution Step
    uint64_t exec_slice = (chosen->remaining_budget_us > step_duration_us) ?
                          step_duration_us : chosen->remaining_budget_us;
    chosen->remaining_budget_us -= exec_slice;
    chosen->exec_accumulated_us += exec_slice;

    if (chosen->config.run) {
        chosen->config.run(chosen->config.task_ctx);
    }

    result->task_dispatched = true;
    result->active_task_id = chosen->config.task_id;
    strncpy(result->active_task_name, chosen->config.name, sizeof(result->active_task_name) - 1);
    result->static_priority = chosen->static_priority;
    result->remaining_budget_us = chosen->remaining_budget_us;

    // 5. Execution Budget Watchdog Check
    if (chosen->remaining_budget_us == 0) {
        // Budget exhausted in this period
        chosen->state = EDGE_RMS_STATE_COMPLETED;
        s->stats.total_completions++;
        s->current_running_index = -1;

        if (s->config.enable_budget_watchdog && chosen->exec_accumulated_us >= chosen->config.wcet_budget_us) {
            // Check if watchdog action needs to be applied
            if (chosen->config.budget_action == EDGE_RMS_ACTION_THROTTLE) {
                chosen->state = EDGE_RMS_STATE_THROTTLED;
            } else if (chosen->config.budget_action == EDGE_RMS_ACTION_SUSPEND) {
                chosen->state = EDGE_RMS_STATE_DORMANT;
            } else if (chosen->config.budget_action == EDGE_RMS_ACTION_CIRCUIT_BREAK) {
                chosen->state = EDGE_RMS_STATE_TRIPPED;
            }

            if (chosen->config.budget_action != EDGE_RMS_ACTION_WARN_ONLY) {
                chosen->total_overrun_count++;
                s->stats.total_budget_overruns++;
                result->is_budget_overrun = true;
                result->affected_task_id = chosen->config.task_id;

                if (s->port.on_budget_exceeded) {
                    s->port.on_budget_exceeded(s->port.ctx, chosen->config.task_id,
                                               chosen->exec_accumulated_us,
                                               chosen->config.wcet_budget_us,
                                               chosen->config.budget_action);
                }
            }
        }

        uint64_t completion_time = current_time_us + exec_slice;
        uint64_t response_time = (completion_time > chosen->current_period_start_us) ?
                                 (completion_time - chosen->current_period_start_us) : 0;
        if (response_time > s->stats.peak_response_time_us) {
            s->stats.peak_response_time_us = response_time;
        }
    }

    return 0;
}

int edge_rms_scheduler_complete_job(edge_rms_scheduler_t *sched,
                                   uint32_t task_id,
                                   uint64_t finish_time_us)
{
    if (!sched || !sched->initialized) {
        return -EINVAL;
    }

    struct edge_rms_scheduler *s = (struct edge_rms_scheduler *)sched;
    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        edge_rms_task_entry_t *t = &s->tasks[i];
        if (t->is_active && t->config.task_id == task_id) {
            t->remaining_budget_us = 0;
            t->state = EDGE_RMS_STATE_COMPLETED;
            s->stats.total_completions++;
            if (s->current_running_index == (int)i) {
                s->current_running_index = -1;
            }
            uint64_t response_time = (finish_time_us > t->current_period_start_us) ?
                                     (finish_time_us - t->current_period_start_us) : 0;
            if (response_time > s->stats.peak_response_time_us) {
                s->stats.peak_response_time_us = response_time;
            }
            return 0;
        }
    }
    return -ENOENT;
}

int edge_rms_scheduler_get_stats(const edge_rms_scheduler_t *sched, edge_rms_stats_t *stats)
{
    if (!sched || !stats || !sched->initialized) {
        return -EINVAL;
    }

    const struct edge_rms_scheduler *s = (const struct edge_rms_scheduler *)sched;
    memcpy(stats, &s->stats, sizeof(edge_rms_stats_t));
    return 0;
}

int edge_rms_scheduler_reset(edge_rms_scheduler_t *sched)
{
    if (!sched || !sched->initialized) {
        return -EINVAL;
    }

    struct edge_rms_scheduler *s = (struct edge_rms_scheduler *)sched;
    s->current_running_index = -1;
    s->current_sim_time_us = 0;
    memset(&s->stats, 0, sizeof(s->stats));
    s->stats.registered_task_count = s->active_task_count;
    s->stats.total_utilization = edge_rms_scheduler_get_utilization(sched);
    s->stats.bound_utilization = edge_rms_scheduler_get_bound(sched);

    for (size_t i = 0; i < EDGE_RMS_MAX_TASKS; i++) {
        if (s->tasks[i].is_active) {
            s->tasks[i].state = EDGE_RMS_STATE_DORMANT;
            s->tasks[i].next_release_time_us = s->tasks[i].config.initial_phase_us;
            s->tasks[i].remaining_budget_us = 0;
            s->tasks[i].exec_accumulated_us = 0;
            s->tasks[i].current_absolute_deadline_us = 0;
            s->tasks[i].current_period_start_us = 0;
        }
    }
    return 0;
}
