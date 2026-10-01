/**
 * @file edge_edf_scheduler.c
 * @brief Earliest Deadline First (EDF) Dynamic Hard Real-Time Preemptive Scheduler (AEP-712 / Issue #245)
 */

#include "edge_edf_scheduler.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

/**
 * @brief Internal task entry descriptor
 */
typedef struct {
    edge_edf_task_config_t config;
    bool is_active;
    edge_edf_task_state_t state;
    uint64_t next_release_time_us;
    uint64_t current_job_release_us;
    uint64_t current_absolute_deadline_us;
    uint64_t remaining_exec_us;
    uint64_t last_start_time_us;
    uint64_t total_job_count;
    uint64_t total_miss_count;
} edge_edf_task_entry_t;

/**
 * @brief Concrete structure for EDF Scheduler
 */
struct edge_edf_scheduler {
    edge_edf_config_t config;
    edge_edf_port_t port;
    edge_edf_stats_t stats;
    edge_edf_task_entry_t tasks[EDGE_EDF_MAX_TASKS];
    uint32_t active_task_count;
    int current_running_index;   /**< -1 if CPU is idle */
    uint64_t current_sim_time_us;
    bool initialized;
};

_Static_assert(sizeof(struct edge_edf_scheduler) <= sizeof(edge_edf_scheduler_storage_t),
               "edge_edf_scheduler_storage_t buffer size too small for struct edge_edf_scheduler");

static uint64_t gcd_u64(uint64_t a, uint64_t b)
{
    while (b != 0) {
        uint64_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

static uint64_t lcm_u64(uint64_t a, uint64_t b)
{
    if (a == 0 || b == 0) return 0;
    return (a / gcd_u64(a, b)) * b;
}

edge_edf_scheduler_t *edge_edf_scheduler_create_in(edge_edf_scheduler_storage_t *store,
                                                  const edge_edf_config_t *cfg,
                                                  const edge_edf_port_t *port)
{
    if (!store || !cfg) {
        return NULL;
    }

    struct edge_edf_scheduler *impl = (struct edge_edf_scheduler *)store->raw;
    memset(impl, 0, sizeof(*impl));
    memcpy(&impl->config, cfg, sizeof(edge_edf_config_t));
    if (port) {
        memcpy(&impl->port, port, sizeof(edge_edf_port_t));
    }

    impl->current_running_index = -1;
    impl->initialized = true;
    return (edge_edf_scheduler_t *)impl;
}

int edge_edf_scheduler_register_task(edge_edf_scheduler_t *sched,
                                    const edge_edf_task_config_t *task_cfg)
{
    if (!sched || !task_cfg || !sched->initialized) {
        return -EINVAL;
    }
    if (task_cfg->period_us == 0 || task_cfg->wcet_us == 0) {
        return -EINVAL;
    }
    if (task_cfg->wcet_us > task_cfg->period_us) {
        return -EINVAL; // Physical WCET cannot exceed task period
    }

    struct edge_edf_scheduler *s = (struct edge_edf_scheduler *)sched;

    // Check for duplicate ID
    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        if (s->tasks[i].is_active && s->tasks[i].config.task_id == task_cfg->task_id) {
            return -EEXIST;
        }
    }

    // Find free slot
    int free_slot = -1;
    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        if (!s->tasks[i].is_active) {
            free_slot = (int)i;
            break;
        }
    }
    if (free_slot < 0) {
        return -ENOMEM; // Capacity exhausted
    }

    edge_edf_task_entry_t *entry = &s->tasks[free_slot];
    memset(entry, 0, sizeof(*entry));
    memcpy(&entry->config, task_cfg, sizeof(edge_edf_task_config_t));
    if (entry->config.relative_deadline_us == 0) {
        entry->config.relative_deadline_us = entry->config.period_us;
    }
    entry->is_active = true;
    entry->state = EDGE_EDF_STATE_DORMANT;
    entry->next_release_time_us = task_cfg->initial_phase_us;

    s->active_task_count++;
    s->stats.registered_task_count = s->active_task_count;
    s->stats.total_utilization = edge_edf_scheduler_get_utilization(sched);

    if (s->stats.total_utilization > 1.0) {
        if (s->port.on_schedulability_warning) {
            s->port.on_schedulability_warning(s->port.ctx, s->stats.total_utilization,
                                              "Task set utilization exceeds 100% (Liu & Layland overload)");
        }
    }

    return 0;
}

int edge_edf_scheduler_unregister_task(edge_edf_scheduler_t *sched, uint32_t task_id)
{
    if (!sched || !sched->initialized) {
        return -EINVAL;
    }

    struct edge_edf_scheduler *s = (struct edge_edf_scheduler *)sched;
    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        if (s->tasks[i].is_active && s->tasks[i].config.task_id == task_id) {
            s->tasks[i].is_active = false;
            s->tasks[i].state = EDGE_EDF_STATE_DORMANT;
            if (s->current_running_index == (int)i) {
                s->current_running_index = -1;
            }
            s->active_task_count--;
            s->stats.registered_task_count = s->active_task_count;
            s->stats.total_utilization = edge_edf_scheduler_get_utilization(sched);
            return 0;
        }
    }
    return -ENOENT;
}

double edge_edf_scheduler_get_utilization(const edge_edf_scheduler_t *sched)
{
    if (!sched || !sched->initialized) {
        return -1.0;
    }

    const struct edge_edf_scheduler *s = (const struct edge_edf_scheduler *)sched;
    double u_sum = 0.0;
    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        if (s->tasks[i].is_active && s->tasks[i].config.period_us > 0) {
            u_sum += (double)s->tasks[i].config.wcet_us / (double)s->tasks[i].config.period_us;
        }
    }
    return u_sum;
}

int edge_edf_scheduler_check_schedulability(const edge_edf_scheduler_t *sched,
                                           edge_edf_schedulability_report_t *report)
{
    if (!sched || !report || !sched->initialized) {
        return -EINVAL;
    }

    const struct edge_edf_scheduler *s = (const struct edge_edf_scheduler *)sched;
    memset(report, 0, sizeof(*report));

    report->total_utilization = edge_edf_scheduler_get_utilization(sched);
    if (s->active_task_count == 0) {
        report->is_schedulable = true;
        report->hyperperiod_us = 0;
        return 0;
    }

    // 1. Check total utilization bound U <= 1.0
    if (report->total_utilization > 1.0) {
        report->is_schedulable = false;
        snprintf(report->failure_reason, sizeof(report->failure_reason),
                 "Total utilization U=%.3f > 1.0 (Liu & Layland Bound Violated)", report->total_utilization);
        return 0;
    }

    // 2. Compute Hyperperiod H = LCM(T_1, ..., T_n)
    uint64_t h_period = 0;
    bool has_constrained_deadline = false;
    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        if (s->tasks[i].is_active) {
            if (h_period == 0) {
                h_period = s->tasks[i].config.period_us;
            } else {
                h_period = lcm_u64(h_period, s->tasks[i].config.period_us);
            }
            if (s->tasks[i].config.relative_deadline_us < s->tasks[i].config.period_us) {
                has_constrained_deadline = true;
            }
        }
    }
    report->hyperperiod_us = h_period;

    // 3. For implicit deadline systems (D_i == T_i), U <= 1.0 is exact and sufficient
    if (!has_constrained_deadline) {
        report->is_schedulable = true;
        return 0;
    }

    // 4. Processor Demand Criteria (Baruah DBF Test) for D_i <= T_i
    // Check h(t) = sum(max(0, floor((t - D_i)/T_i) + 1) * C_i) <= t for all check points t <= H
    uint64_t max_test_t = h_period;
    if (max_test_t > 500000ULL) {
        max_test_t = 500000ULL; // Bound simulation time for fast evaluation
    }

    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        if (!s->tasks[i].is_active) continue;
        uint64_t d_i = s->tasks[i].config.relative_deadline_us;
        uint64_t t_i = s->tasks[i].config.period_us;

        for (uint64_t t = d_i; t <= max_test_t; t += t_i) {
            uint64_t demand = 0;
            for (size_t j = 0; j < EDGE_EDF_MAX_TASKS; j++) {
                if (!s->tasks[j].is_active) continue;
                uint64_t dj = s->tasks[j].config.relative_deadline_us;
                uint64_t tj = s->tasks[j].config.period_us;
                uint64_t cj = s->tasks[j].config.wcet_us;
                if (t >= dj) {
                    uint64_t count = ((t - dj) / tj) + 1;
                    demand += count * cj;
                }
            }
            if (demand > t) {
                report->is_schedulable = false;
                snprintf(report->failure_reason, sizeof(report->failure_reason),
                         "Processor Demand h(%lu)=%lu > %lu (Baruah DBF Criterion Violated)",
                         (unsigned long)t, (unsigned long)demand, (unsigned long)t);
                return 0;
            }
        }
    }

    report->is_schedulable = true;
    return 0;
}

int edge_edf_scheduler_step(edge_edf_scheduler_t *sched,
                           uint64_t current_time_us,
                           uint64_t step_duration_us,
                           edge_edf_dispatch_result_t *result)
{
    if (!sched || !result || !sched->initialized || step_duration_us == 0) {
        return -EINVAL;
    }

    struct edge_edf_scheduler *s = (struct edge_edf_scheduler *)sched;
    memset(result, 0, sizeof(*result));
    s->current_sim_time_us = current_time_us;

    // 1. Release periodic jobs
    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        edge_edf_task_entry_t *t = &s->tasks[i];
        if (!t->is_active) continue;

        // Check if task period release is triggered
        if (current_time_us >= t->next_release_time_us) {
            // Check if previous job missed its deadline
            if (t->state == EDGE_EDF_STATE_READY || t->state == EDGE_EDF_STATE_RUNNING) {
                if (t->remaining_exec_us > 0) {
                    t->total_miss_count++;
                    s->stats.total_deadline_misses++;
                    result->is_deadline_miss = true;
                    result->missed_task_id = t->config.task_id;

                    uint64_t overdue = (current_time_us > t->current_absolute_deadline_us) ?
                                       (current_time_us - t->current_absolute_deadline_us) : 0;
                    if (s->port.on_deadline_miss) {
                        s->port.on_deadline_miss(s->port.ctx, t->config.task_id, current_time_us, overdue);
                    }

                    if (s->config.drop_on_deadline_miss) {
                        t->state = EDGE_EDF_STATE_MISSED;
                        t->remaining_exec_us = 0;
                    }
                }
            }

            // Release new job instance
            t->current_job_release_us = t->next_release_time_us;
            t->current_absolute_deadline_us = t->next_release_time_us + t->config.relative_deadline_us;
            t->remaining_exec_us = t->config.wcet_us;
            t->state = EDGE_EDF_STATE_READY;
            t->total_job_count++;
            t->next_release_time_us += t->config.period_us;
        }

        // Check active deadline overrun
        if ((t->state == EDGE_EDF_STATE_READY || t->state == EDGE_EDF_STATE_RUNNING) &&
            current_time_us > t->current_absolute_deadline_us && t->remaining_exec_us > 0) {
            t->total_miss_count++;
            s->stats.total_deadline_misses++;
            result->is_deadline_miss = true;
            result->missed_task_id = t->config.task_id;
            uint64_t overdue = current_time_us - t->current_absolute_deadline_us;
            if (s->port.on_deadline_miss) {
                s->port.on_deadline_miss(s->port.ctx, t->config.task_id, current_time_us, overdue);
            }
            if (s->config.drop_on_deadline_miss) {
                t->state = EDGE_EDF_STATE_MISSED;
                t->remaining_exec_us = 0;
            }
        }
    }

    // 2. Earliest Deadline First Selection
    int best_index = -1;
    uint64_t earliest_deadline = UINT64_MAX;

    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        edge_edf_task_entry_t *t = &s->tasks[i];
        if (!t->is_active) continue;

        if ((t->state == EDGE_EDF_STATE_READY || t->state == EDGE_EDF_STATE_RUNNING) &&
            t->remaining_exec_us > 0) {
            if (t->current_absolute_deadline_us < earliest_deadline) {
                earliest_deadline = t->current_absolute_deadline_us;
                best_index = (int)i;
            } else if (t->current_absolute_deadline_us == earliest_deadline && best_index >= 0) {
                // Tie breaker: smaller task ID
                if (t->config.task_id < s->tasks[best_index].config.task_id) {
                    best_index = (int)i;
                }
            }
        }
    }

    if (best_index < 0) {
        // CPU Idle
        s->current_running_index = -1;
        result->task_dispatched = false;
        return 0;
    }

    edge_edf_task_entry_t *chosen = &s->tasks[best_index];

    // 3. Preemption Check
    if (s->current_running_index >= 0 && s->current_running_index != best_index) {
        edge_edf_task_entry_t *prev = &s->tasks[s->current_running_index];
        if (prev->is_active && prev->state == EDGE_EDF_STATE_RUNNING) {
            prev->state = EDGE_EDF_STATE_READY;
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
    chosen->state = EDGE_EDF_STATE_RUNNING;
    s->stats.total_dispatches++;

    // 4. Execution Step
    uint64_t exec_slice = (chosen->remaining_exec_us > step_duration_us) ?
                          step_duration_us : chosen->remaining_exec_us;
    chosen->remaining_exec_us -= exec_slice;

    if (chosen->config.run) {
        chosen->config.run(chosen->config.task_ctx);
    }

    result->task_dispatched = true;
    result->active_task_id = chosen->config.task_id;
    strncpy(result->active_task_name, chosen->config.name, sizeof(result->active_task_name) - 1);
    result->absolute_deadline_us = chosen->current_absolute_deadline_us;
    result->remaining_budget_us = chosen->remaining_exec_us;

    // 5. Job Completion Check
    if (chosen->remaining_exec_us == 0) {
        chosen->state = EDGE_EDF_STATE_COMPLETED;
        s->stats.total_completions++;
        s->current_running_index = -1;

        uint64_t completion_time = current_time_us + exec_slice;
        uint64_t response_time = (completion_time > chosen->current_job_release_us) ?
                                 (completion_time - chosen->current_job_release_us) : 0;
        if (response_time > s->stats.peak_response_time_us) {
            s->stats.peak_response_time_us = response_time;
        }
    }

    return 0;
}

int edge_edf_scheduler_complete_job(edge_edf_scheduler_t *sched,
                                   uint32_t task_id,
                                   uint64_t finish_time_us)
{
    if (!sched || !sched->initialized) {
        return -EINVAL;
    }

    struct edge_edf_scheduler *s = (struct edge_edf_scheduler *)sched;
    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        edge_edf_task_entry_t *t = &s->tasks[i];
        if (t->is_active && t->config.task_id == task_id) {
            t->remaining_exec_us = 0;
            t->state = EDGE_EDF_STATE_COMPLETED;
            s->stats.total_completions++;
            if (s->current_running_index == (int)i) {
                s->current_running_index = -1;
            }
            uint64_t response_time = (finish_time_us > t->current_job_release_us) ?
                                     (finish_time_us - t->current_job_release_us) : 0;
            if (response_time > s->stats.peak_response_time_us) {
                s->stats.peak_response_time_us = response_time;
            }
            return 0;
        }
    }
    return -ENOENT;
}

int edge_edf_scheduler_get_stats(const edge_edf_scheduler_t *sched, edge_edf_stats_t *stats)
{
    if (!sched || !stats || !sched->initialized) {
        return -EINVAL;
    }

    const struct edge_edf_scheduler *s = (const struct edge_edf_scheduler *)sched;
    memcpy(stats, &s->stats, sizeof(edge_edf_stats_t));
    return 0;
}

int edge_edf_scheduler_reset(edge_edf_scheduler_t *sched)
{
    if (!sched || !sched->initialized) {
        return -EINVAL;
    }

    struct edge_edf_scheduler *s = (struct edge_edf_scheduler *)sched;
    s->current_running_index = -1;
    s->current_sim_time_us = 0;
    memset(&s->stats, 0, sizeof(s->stats));
    s->stats.registered_task_count = s->active_task_count;
    s->stats.total_utilization = edge_edf_scheduler_get_utilization(sched);

    for (size_t i = 0; i < EDGE_EDF_MAX_TASKS; i++) {
        if (s->tasks[i].is_active) {
            s->tasks[i].state = EDGE_EDF_STATE_DORMANT;
            s->tasks[i].next_release_time_us = s->tasks[i].config.initial_phase_us;
            s->tasks[i].remaining_exec_us = 0;
            s->tasks[i].current_absolute_deadline_us = 0;
            s->tasks[i].current_job_release_us = 0;
        }
    }
    return 0;
}
