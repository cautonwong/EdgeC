#include "bus_orchestrator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "shmring.h"

typedef struct poll_task_s {
    uv_timer_t timer;
    struct bus_orchestrator_s *parent;
    edge_poll_group_t group;
    struct poll_task_s *next;
} poll_task_t;

struct bus_orchestrator_s {
    edge_driver_t *driver;
    datahub_t *hub;
    arbiter_t *arb;
    poll_task_t *tasks;
    uv_loop_t *loop;
    int is_running;
};

static void _on_cmd_complete(cmd_msg_t *cmd, int status) {
    (void)status;
    if (cmd) free(cmd);
}

static void _on_cmd_cleanup(cmd_msg_t *cmd) {
    // 内存已由 on_complete 接管
    (void)cmd;
}

static void _on_driver_msg(pipe_msg_t *msg, void *user_data) {
    bus_orchestrator_t *ctx = (bus_orchestrator_t *)user_data;
    if (ctx && ctx->hub) {
        datahub_publish(ctx->hub, msg);
    }
}

typedef struct {
    cmd_msg_t base;
    poll_task_t *task;
} poll_cmd_t;

static void _real_on_cmd_start(void *user_data) {
    poll_cmd_t *pcmd = (poll_cmd_t *)user_data;
    poll_task_t *task = pcmd->task;
    bus_orchestrator_t *ctx = task->parent;

    if (!ctx || !ctx->is_running) {
        // 如果已经停止，仍然需要通过 Arbiter 流程完成，由 Arbiter 触发 cleanup
        // 但这里我们不执行业务
        arbiter_on_io_done(ctx->arb, 0, (cmd_msg_t *)pcmd);
        return;
    }

    if (ctx->driver && ctx->driver->build_batch_req) {
        uint8_t tx[256];
        size_t tx_len = 0;
        if (ctx->driver->build_batch_req(&task->group, tx, &tx_len) == 0) {
            // [模拟响应] 6.35 kWh
            // CS 计算: 68+78+56+34+12+00+00+68+91+08+33+33+33+33+68+39+33+33 = 0x04F4 -> F4
            uint8_t mock_rx[] = {0x68, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x68, 0x91, 0x08,
                                 0x33, 0x33, 0x33, 0x33, 0x68, 0x39, 0x33, 0x33, 0x50, 0x16};
            if (ctx->driver->parse_batch_res)
                ctx->driver->parse_batch_res(&task->group, mock_rx, sizeof(mock_rx), _on_driver_msg,
                                             ctx);
        }
    }
    arbiter_on_io_done(ctx->arb, 0, (cmd_msg_t *)pcmd);
}

static void _real_on_poll_timer(uv_timer_t *handle) {
    poll_task_t *task = (poll_task_t *)handle->data;
    bus_orchestrator_t *ctx = task->parent;
    if (!ctx || !ctx->is_running) return;

    poll_cmd_t *pcmd = calloc(1, sizeof(poll_cmd_t));
    pcmd->task = task;
    pcmd->base.priority = ARBITER_PRIO_HIGH;
    pcmd->base.action = CMD_ACTION_POLL;
    pcmd->base.on_start = _real_on_cmd_start;
    pcmd->base.on_complete = _on_cmd_complete;
    pcmd->base.on_cleanup = _on_cmd_cleanup;
    pcmd->base.user_data = pcmd;  // 让回调拿到自己

    snprintf(pcmd->base.req_id, sizeof(pcmd->base.req_id), "P-%d", task->group.slave_id);

    if (arbiter_submit_cmd(ctx->arb, 0, &pcmd->base) != 0) {
        free(pcmd);
    }
}

bus_orchestrator_t *bus_orchestrator_create(edge_driver_t *driver, datahub_t *hub, arbiter_t *arb) {
    bus_orchestrator_t *ctx = calloc(1, sizeof(struct bus_orchestrator_s));
    ctx->driver = driver;
    ctx->hub = hub;
    ctx->arb = arb;
    ctx->loop = hub->loop;
    ctx->is_running = 1;
    return ctx;
}

int bus_orchestrator_add_poll(bus_orchestrator_t *ctx, int interval_ms,
                              const edge_poll_group_t *group) {
    if (!ctx || !group) return -1;
    poll_task_t *task = calloc(1, sizeof(poll_task_t));
    task->parent = ctx;
    task->group = *group;
    task->next = ctx->tasks;
    ctx->tasks = task;
    uv_timer_init(ctx->loop, &task->timer);
    task->timer.data = task;
    uv_timer_start(&task->timer, _real_on_poll_timer, interval_ms, interval_ms);
    return 0;
}

void bus_orchestrator_run(bus_orchestrator_t *ctx) {
    if (ctx) ctx->is_running = 1;
}

static void _on_task_timer_close(uv_handle_t *h) {
    poll_task_t *task = (poll_task_t *)h->data;
    free(task);
}

void bus_orchestrator_destroy(bus_orchestrator_t *ctx) {
    if (!ctx) return;
    ctx->is_running = 0;
    poll_task_t *t = ctx->tasks;
    while (t) {
        poll_task_t *next = t->next;
        uv_timer_stop(&t->timer);
        if (!uv_is_closing((uv_handle_t *)&t->timer)) {
            t->timer.data = t;
            uv_close((uv_handle_t *)&t->timer, _on_task_timer_close);
        } else {
            // Should not happen normally if managed correctly
            free(t);
        }
        t = next;
    }
    ctx->tasks = NULL;
    free(ctx);
}