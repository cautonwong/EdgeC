#include "dcu_poller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "edge_log.h"

int dcu_poller_init(dcu_poller_t* poller) {
    if (!poller) return -1;
    memset(poller, 0, sizeof(dcu_poller_t));

    // 初始化默认通道调度策略 (AEP-401)
    for (int i = 0; i < DCU_MAX_CHANNELS; i++) {
        poller->channel_policies[i].interval_ms = 1000;
        poller->channel_policies[i].timeout_ms = 500;
        poller->channel_policies[i].max_in_flight = 1;
        poller->channel_policies[i].circuit_break_threshold = 5;
    }
    return 0;
}

int dcu_poller_start(dcu_poller_t* poller, poller_source_ops_t source_ops, poller_bus_ops_t bus_ops,
                     poller_sink_ops_t sink_ops) {
    if (!poller) return -1;
    poller->source_ops = source_ops;
    poller->bus_ops = bus_ops;
    poller->sink_ops = sink_ops;
    return 0;
}

int dcu_poller_step(dcu_poller_t* poller, dcu_inventory_t* inv, uint32_t max_meters_per_step) {
    if (!poller || !inv || inv->count == 0) {
        return 0;
    }

    int meters_processed = 0;

    for (int i = 0; i < poller->task_count; i++) {
        dcu_task_unit_t* task = &poller->tasks[i];
        if (task->interval_s > 0) {
            uint64_t ticks = 0;
            if (edge_periodic_clock_check(&task->clock, &ticks) <= 0) {
                continue;
            }
        }
        task->last_run_time = edge_periodic_clock_now_us(&task->clock) / 1000000ULL;
        task->monitor.total_meters = inv->count;
        dcu_scheme_t* s = dcu_poller_get_scheme(poller, task->scheme_id);
        if (!s) continue;

        uint32_t start_idx = task->cursor_meter_idx % inv->count;
        uint32_t remaining = inv->count - start_idx;
        uint32_t step_limit = max_meters_per_step > 0 ? (max_meters_per_step < remaining ? max_meters_per_step : remaining) : remaining;
        uint32_t count_in_this_step = 0;

        for (uint32_t offset = 0; offset < remaining && count_in_this_step < step_limit; offset++) {
            uint32_t m_idx = start_idx + offset;
            production_meter_t* m = &inv->meters[m_idx];
            uint32_t ch = m->channel_id < DCU_MAX_CHANNELS ? m->channel_id : 0;
            edge_poll_stats_t* cstats = &poller->channel_stats[ch];
            edge_poll_policy_t* cpolicy = &poller->channel_policies[ch];

            // 1. 熔断保护检查 (Circuit Breaker)
            if (cstats->is_circuit_broken) {
                continue;
            }

            // 2. In-flight 限制与背压 (Backpressure)
            uint32_t max_in_flight = cpolicy->max_in_flight > 0 ? cpolicy->max_in_flight : 1;
            if (cstats->in_flight >= max_in_flight) {
                cstats->queue_depth++;
                continue;
            }

            cstats->poll_count++;
            cstats->last_poll_ts = uv_hrtime() / 1000000ULL;
            cstats->in_flight++;
            count_in_this_step++;
            meters_processed++;

            bool meter_success = true;
            for (int j = 0; j < s->data.normal.csd_count; j++) {
                uint32_t tag_id = s->data.normal.csd_list[j];
                double fv = 0.0;
                int read_rc = -1;

                if (poller->source_ops.read_value) {
                    read_rc = poller->source_ops.read_value(poller->source_ops.ctx, (uint32_t)m_idx,
                                                            tag_id, &fv);
                }

                if (read_rc == 0) {
                    if (poller->sink_ops.record_telemetry) {
                        poller->sink_ops.record_telemetry(poller->sink_ops.ctx, m->address,
                                                          (uint32_t)m_idx, tag_id, fv);
                    }

                    if (poller->bus_ops.publish_realtime) {
                        poller->bus_ops.publish_realtime(poller->bus_ops.ctx, tag_id,
                                                         (uint32_t)m_idx, fv, uv_hrtime());
                    }

                    if (poller->bus_ops.push_event) {
                        poller->bus_ops.push_event(poller->bus_ops.ctx, tag_id, (uint32_t)m_idx, fv,
                                                   uv_hrtime());
                    }
                } else {
                    meter_success = false;
                }
            }

            cstats->in_flight--;
            if (meter_success) {
                cstats->success_count++;
                cstats->consecutive_fails = 0;
                cstats->last_success_ts = uv_hrtime() / 1000000ULL;
                task->monitor.success_meters++;
            } else {
                cstats->error_count++;
                cstats->consecutive_fails++;
                if (cpolicy->circuit_break_threshold > 0 &&
                    cstats->consecutive_fails >= cpolicy->circuit_break_threshold) {
                    cstats->is_circuit_broken = true;
                    LOG_ERROR(LOG_CAT_IO, "🚨 [DCU-Poller] Channel %u circuit breaker tripped (fails: %lu >= threshold: %u)",
                              ch, (unsigned long)cstats->consecutive_fails, cpolicy->circuit_break_threshold);
                }
            }
        }
        task->cursor_meter_idx = (uint16_t)((start_idx + count_in_this_step) % inv->count);
    }

    // 批量落盘
    if (poller->sink_ops.commit_batch) {
        poller->sink_ops.commit_batch(poller->sink_ops.ctx);
    }

    return meters_processed;
}

void dcu_poller_pump(dcu_poller_t* poller, dcu_inventory_t* inv) {
    dcu_poller_step(poller, inv, 0);
}

int dcu_poller_set_channel_policy(dcu_poller_t* poller, uint32_t channel_id,
                                  const edge_poll_policy_t* policy) {
    if (!poller || channel_id >= DCU_MAX_CHANNELS || !policy) return -1;
    poller->channel_policies[channel_id] = *policy;
    return 0;
}

int dcu_poller_get_channel_stats(dcu_poller_t* poller, uint32_t channel_id,
                                 edge_poll_stats_t* out_stats) {
    if (!poller || channel_id >= DCU_MAX_CHANNELS || !out_stats) return -1;
    *out_stats = poller->channel_stats[channel_id];
    return 0;
}

int dcu_poller_reset_circuit_breaker(dcu_poller_t* poller, uint32_t channel_id) {
    if (!poller || channel_id >= DCU_MAX_CHANNELS) return -1;
    poller->channel_stats[channel_id].is_circuit_broken = false;
    poller->channel_stats[channel_id].consecutive_fails = 0;
    poller->channel_stats[channel_id].in_flight = 0;
    poller->channel_stats[channel_id].queue_depth = 0;
    return 0;
}

int dcu_poller_add_scheme(dcu_poller_t* poller, uint32_t scheme_id, uint8_t mode,
                          const uint32_t* csds, uint16_t csd_count) {
    dcu_scheme_t* s = &poller->schemes[poller->scheme_count++];
    s->scheme_id = scheme_id;
    s->type = SCHEME_TYPE_NORMAL;
    s->data.normal.capture_mode = mode;
    s->data.normal.csd_count = csd_count;
    for (int i = 0; i < csd_count; i++) s->data.normal.csd_list[i] = csds[i];
    return 0;
}

int dcu_poller_add_task(dcu_poller_t* poller, uint32_t task_id, uint32_t interval_s,
                        uint32_t scheme_id) {
    if (!poller || poller->task_count >= MAX_TASKS) return -1;
    dcu_task_unit_t* t = &poller->tasks[poller->task_count++];
    t->task_id = task_id;
    t->interval_s = interval_s;
    t->scheme_id = scheme_id;
    t->last_run_time = 0;
    t->cursor_meter_idx = 0;

    edge_periodic_clock_config_t clk_cfg = {
        .interval_us = (interval_s > 0) ? ((uint64_t)interval_s * 1000000ULL) : 1ULL,
        .policy = EDGE_OVERRUN_SKIP,
        .wall_clock_align = (interval_s > 0),
        .custom_clock = NULL,
        .clock_ctx = NULL,
    };
    edge_periodic_clock_init(&t->clock, &clk_cfg);
    // 首次允许立即执行
    t->clock.next_tick_us = 0;
    return 0;
}

dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t* poller, uint32_t scheme_id) {
    for (int i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id == scheme_id) return &poller->schemes[i];
    }
    return NULL;
}

void dcu_poller_destroy(dcu_poller_t* poller) {
    (void)poller;
}

void dcu_poller_stop(dcu_poller_t* poller) {
    (void)poller;
}
