#include "dcu_poller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "edge_log.h"

int dcu_poller_init(dcu_poller_t* poller) {
    if (!poller) return -1;
    memset(poller, 0, sizeof(dcu_poller_t));
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

void dcu_poller_pump(dcu_poller_t* poller, dcu_inventory_t* inv) {
    if (!poller || !inv || inv->count == 0) {
        return;
    }

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
        task->monitor.success_meters = 0;
        task->cursor_meter_idx = 0;

        for (int m_idx = 0; m_idx < inv->count; m_idx++) {
            production_meter_t* m = &inv->meters[m_idx];
            dcu_scheme_t* s = dcu_poller_get_scheme(poller, task->scheme_id);
            if (!s) continue;

            task->cursor_meter_idx++;

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
                    task->monitor.success_meters++;
                }
            }
        }
    }

    // 强制落盘
    if (poller->sink_ops.commit_batch) {
        poller->sink_ops.commit_batch(poller->sink_ops.ctx);
    }
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
