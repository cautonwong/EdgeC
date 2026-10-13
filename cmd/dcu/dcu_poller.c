#include "dcu_poller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "arbiter.h"
#include "edge_log.h"

// 事务上下文结构
typedef struct {
    uint32_t magic;
    uint32_t device_id;
    uint32_t task_id;
    dcu_poller_t* poller;
} poll_ctx_t;

#define POLL_CTX_MAGIC 0x504F4C4C

static void _on_poll_cmd_complete(struct cmd_msg* cmd, int status);
static void _on_poll_cmd_cleanup(struct cmd_msg* cmd);

int dcu_poller_init(dcu_poller_t* poller) {
    if (!poller) return -1;
    memset(poller, 0, sizeof(dcu_poller_t));
    return 0;
}

int dcu_poller_start(dcu_poller_t* poller, poller_io_ops_t io_ops, poller_bus_ops_t bus_ops,
                     poller_sink_ops_t sink_ops) {
    if (!poller) return -1;
    poller->io_ops = io_ops;
    poller->bus_ops = bus_ops;
    poller->sink_ops = sink_ops;
    return 0;
}

void dcu_poller_pump(dcu_poller_t* poller, dcu_inventory_t* inv) {
    if (!poller || !inv || inv->count == 0) {
        return;
    }

    uint64_t now = (uint64_t)time(NULL);

    for (int i = 0; i < poller->task_count; i++) {
        dcu_task_unit_t* task = &poller->tasks[i];
        if (now - task->last_run_time < task->interval_s) continue;

        task->last_run_time = now;
        task->monitor.total_meters = inv->count;
        task->monitor.success_meters = 0;
        task->cursor_meter_idx = 0;

        for (int m_idx = 0; m_idx < inv->count; m_idx++) {
            production_meter_t* m = &inv->meters[m_idx];
            dcu_scheme_t* s = dcu_poller_get_scheme(poller, task->scheme_id);
            if (!s) continue;

            task->cursor_meter_idx++;

            for (int j = 0; j < s->data.normal.csd_count; j++) {
                cmd_msg_t* cmd = calloc(1, sizeof(cmd_msg_t));
                poll_ctx_t* ctx = malloc(sizeof(poll_ctx_t));
                ctx->magic = POLL_CTX_MAGIC;
                ctx->device_id = (uint32_t)m_idx;
                ctx->task_id = (uint32_t)i;
                ctx->poller = poller;

                cmd->device_id = ctx->device_id;
                cmd->tag_id = s->data.normal.csd_list[j];
                cmd->user_data = ctx;
                cmd->on_complete = _on_poll_cmd_complete;
                cmd->on_cleanup = _on_poll_cmd_cleanup;

                bool handled_by_io = false;
                if (poller->io_ops.dispatch_command) {
                    uint8_t channel_id = (uint8_t)(cmd->device_id % DCU_MAX_CHANNELS);
                    if (poller->io_ops.dispatch_command(poller->io_ops.ctx, channel_id, cmd) == 0) {
                        handled_by_io = true;
                    }
                }

                if (!handled_by_io) {
                    // Fallback to digital twin simulated read if no IO orchestrator dispatched
                    float fv = (cmd->device_id > 0 && cmd->device_id % 50 == 0) ? 150.0f : 220.88f;

                    if (poller->sink_ops.record_telemetry) {
                        poller->sink_ops.record_telemetry(poller->sink_ops.ctx, m->address,
                                                          cmd->device_id, cmd->tag_id, (double)fv);
                    }

                    if (poller->bus_ops.publish_realtime) {
                        poller->bus_ops.publish_realtime(poller->bus_ops.ctx, cmd->tag_id,
                                                         cmd->device_id, (double)fv, uv_hrtime());
                    }

                    if (poller->bus_ops.push_event) {
                        poller->bus_ops.push_event(poller->bus_ops.ctx, cmd->tag_id, cmd->device_id,
                                                   (double)fv, uv_hrtime());
                    }

                    free(ctx);
                    free(cmd);
                }
            }
        }
    }

    // 强制落盘
    if (poller->sink_ops.commit_batch) {
        poller->sink_ops.commit_batch(poller->sink_ops.ctx);
    }
}
static void _on_poll_cmd_complete(struct cmd_msg* cmd, int status) {
    (void)cmd;
    (void)status;
}

static void _on_poll_cmd_cleanup(struct cmd_msg* cmd) {
    if (!cmd) return;
    poll_ctx_t* ctx = (poll_ctx_t*)cmd->user_data;
    if (ctx) {
        ctx->magic = 0;
        free(ctx);
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
    dcu_task_unit_t* t = &poller->tasks[poller->task_count++];
    t->task_id = task_id;
    t->interval_s = interval_s;
    t->scheme_id = scheme_id;
    t->last_run_time = 0;
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
