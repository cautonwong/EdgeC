/**
 * @file dcu_poller.c
 * @brief 集中器业务轮询引擎 (最终对齐 Linker 版)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dcu_poller.h"
#include "edge_log.h"
#include "arbiter.h"
#include "bus_orchestrator.h"
#include "datahub.h"

typedef struct {
    dcu_poller_t *poller;
    uint32_t task_id;
    uint32_t oad_internal; 
    char meter_addr[MAX_ADDR_LEN];
} dcu_trans_ctx_t;

static void _on_poll_cmd_complete(struct cmd_msg *cmd, int status);

int dcu_poller_init(dcu_poller_t *poller) {
    if (!poller) return -1;
    memset(poller, 0, sizeof(dcu_poller_t));
    uv_mutex_init(&poller->lock);
    return 0;
}

static void _poller_timer_cb(uv_timer_t *handle) {
    dcu_poller_t *poller = (dcu_poller_t *)handle->data;
    dcu_poller_pump(poller);
}

int dcu_poller_start(dcu_poller_t *poller, dcu_inventory_t *inv, void *orch, void *hub, driver_manager_t *drv_mgr, void *loop) {
    if (!poller) return -1;
    poller->inventory = inv;
    poller->orch = orch;
    poller->hub = hub;
    poller->drv_mgr = drv_mgr;
    
    uv_timer_init((uv_loop_t *)loop, &poller->timer);
    poller->timer.data = poller;
    uv_timer_start(&poller->timer, _poller_timer_cb, 1000, 1000);
    return 0;
}

void dcu_poller_pump(dcu_poller_t *poller) {
    uv_mutex_lock(&poller->lock);
    for (int i = 0; i < poller->task_count; i++) {
        dcu_task_unit_t *task = &poller->tasks[i];
        dcu_inventory_t *inv = (dcu_inventory_t *)poller->inventory;
        if (!inv) continue;

        while (task->cursor_meter_idx < inv->count) {
            production_meter_t *m = &inv->meters[task->cursor_meter_idx++];
            dcu_scheme_t *s = dcu_poller_get_scheme(poller, task->scheme_id);
            if (!s || s->type != SCHEME_TYPE_NORMAL) continue;

            for (int j = 0; j < s->data.normal.csd_count; j++) {
                cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
                cmd->action = CMD_ACTION_READ;
                cmd->tag_id = (uint32_t)((task->cursor_meter_idx - 1) * 10 + j);
                cmd->priority = ARBITER_PRIO_NORMAL;
                cmd->timeout_ms = 1500;
                cmd->device_id = (uint32_t)task->cursor_meter_idx - 1;
                strncpy(cmd->protocol, m->protocol, 15);
                
                dcu_trans_ctx_t *ctx = calloc(1, sizeof(dcu_trans_ctx_t));
                ctx->poller = poller; ctx->task_id = (uint32_t)i;
                ctx->oad_internal = cmd->tag_id;
                strncpy(ctx->meter_addr, m->address, MAX_ADDR_LEN-1);
                
                cmd->params = ctx;
                cmd->on_complete = _on_poll_cmd_complete;
                bus_orchestrator_submit_cmd((bus_orchestrator_t *)poller->orch, m->channel_id, cmd);
            }
            break; 
        }
        if (task->cursor_meter_idx >= inv->count) task->cursor_meter_idx = 0;
    }
    uv_mutex_unlock(&poller->lock);
}

static void _on_poll_cmd_complete(struct cmd_msg *cmd, int status) {
    dcu_trans_ctx_t *ctx = (dcu_trans_ctx_t *)cmd->params;
    if (status == 0) {
        pipe_msg_t msg;
        pipe_msg_init_scalar(&msg, ctx->oad_internal, cmd->value, uv_hrtime());
        datahub_publish((datahub_t *)ctx->poller->hub, &msg);
    }
    free(ctx); free(cmd);
}

int dcu_poller_add_scheme(dcu_poller_t *poller, uint32_t scheme_id, uint8_t mode, const uint32_t *csds, uint16_t csd_count) {
    uv_mutex_lock(&poller->lock);
    dcu_scheme_t *s = &poller->schemes[poller->scheme_count++];
    s->scheme_id = scheme_id; s->type = SCHEME_TYPE_NORMAL;
    s->data.normal.capture_mode = mode; s->data.normal.csd_count = csd_count;
    memcpy(s->data.normal.csd_list, csds, sizeof(uint32_t) * csd_count);
    uv_mutex_unlock(&poller->lock);
    return 0;
}

int dcu_poller_add_task(dcu_poller_t *poller, uint32_t task_id, uint32_t interval_s, uint32_t scheme_id) {
    uv_mutex_lock(&poller->lock);
    dcu_task_unit_t *t = &poller->tasks[poller->task_count++];
    t->task_id = task_id; t->interval_s = interval_s; t->scheme_id = scheme_id;
    uv_mutex_unlock(&poller->lock);
    return 0;
}

dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t *poller, uint32_t scheme_id) {
    for (int i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id == scheme_id) return &poller->schemes[i];
    }
    return NULL;
}

void dcu_poller_stop(dcu_poller_t *poller) {
    uv_timer_stop(&poller->timer);
}
