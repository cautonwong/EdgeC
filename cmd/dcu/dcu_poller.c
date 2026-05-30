#include "dcu_poller.h"
#include "dcu_router.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "sched_time.h"
#include "edge_log.h"
#include "bus_orchestrator.h"
#include "req_driver_legacy.h"

/* --- 补抄追踪上下文结构 (用于闭环反馈) --- */
typedef struct {
    dcu_poller_t *poller;
    uint8_t retry_count;
    uint8_t channel_id;
    uint32_t task_id;
    char meter_addr[MAX_ADDR_LEN];
    uint32_t oad;
    struct uv__queue node; // 侵入式链表节点
} dcu_trans_ctx_t;

/* --- 内部私有 API --- */
static void _on_poll_cmd_complete(struct cmd_msg *cmd, int status);
static void _on_poll_cmd_cleanup(struct cmd_msg *msg);

/**
 * @brief [Nexus-Pro] 异步泵送引擎实现
 * 根据背压(Backpressure)逻辑，将待抄任务源源不断地泵入 Matrix 总线。
 */
void dcu_poller_pump(dcu_poller_t *poller, uint8_t channel_id) {
    if (!poller || channel_id >= DCU_MAX_CHANNELS) return;
    
    bus_orchestrator_t *orch = (bus_orchestrator_t *)poller->hub_ptr;
    dcu_inventory_t *inv = (dcu_inventory_t *)poller->inventory_ptr;
    driver_manager_t *dm = (driver_manager_t *)poller->drv_mgr_ptr;
    if (!orch || !inv || !dm) return;

    uv_mutex_lock(&poller->lock);

    // 设定通道最大并发 Transaction 数（生产级背压控制）
    // 典型的 RS485 半双工总线通常并发数为 1
    const uint8_t max_concurrency = 1;

    while (poller->in_flight_per_chan[channel_id] < max_concurrency) {
        
        // 【第一优先级】：处理该通道的“即时补抄”队列 (Supplementary Collection)
        if (!uv__queue_empty(&poller->retry_queues[channel_id])) {
            struct uv__queue *q = uv__queue_head(&poller->retry_queues[channel_id]);
            dcu_trans_ctx_t *ctx = uv__queue_data(q, dcu_trans_ctx_t, node);
            uv__queue_remove(q);
            
            cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
            cmd->action = CMD_ACTION_READ;
            cmd->tag_id = ctx->oad;
            cmd->priority = ARBITER_PRIO_HIGH; // 补抄包优先级更高
            cmd->timeout_ms = 1500;
            cmd->params = ctx; 
            cmd->user_data = poller;
            cmd->on_complete = _on_poll_cmd_complete;
            cmd->on_cleanup = _on_poll_cmd_cleanup;
            
            production_meter_t *m = dcu_inventory_get(inv, ctx->meter_addr);
            if (m) {
                strncpy(cmd->protocol, m->protocol, 15);
                cmd->req = req_legacy_driver_create(dm, m->protocol, 1, cmd->action, cmd->tag_id, 0,
                                                   m->inject ? m->inject->lookup_by_tag : NULL,
                                                   m->inject ? m->inject->lookup_by_key : NULL, NULL);
            }

            if (cmd->req && bus_orchestrator_submit_cmd(orch, channel_id, cmd) == 0) {
                poller->in_flight_per_chan[channel_id]++;
                continue; // 成功发送补抄，继续循环直到背压满
            } else {
                if (cmd->req && cmd->req->ops->teardown) cmd->req->ops->teardown(cmd->req);
                free(cmd);
                uv__queue_insert_head(&poller->retry_queues[channel_id], q); // 失败则退回队列首部
                break;
            }
        }

        // 【第二优先级】：处理扫描任务游标 (Standard Collection)
        if (uv__queue_empty(&poller->active_tasks)) break;

        struct uv__queue *q_task = uv__queue_head(&poller->active_tasks);
        dcu_task_unit_t *task = uv__queue_data(q_task, dcu_task_unit_t, node);
        
        // 游标推进，寻找下一个匹配当前通道的表计
        bool found_meter = false;
        uv_mutex_lock(&inv->lock);
        while (task->cursor_meter_idx < inv->count) {
            production_meter_t *m = &inv->meters[task->cursor_meter_idx++];
            
            // 对齐 PRD 规则：通道匹配且必须在线（熔断隔离）
            if (m->channel_id != channel_id || !m->is_online) continue;
            
            // 匹配方案与 MS 集合
            dcu_scheme_t *s = dcu_poller_get_scheme(poller, task->scheme_id);
            if (s && s->ms_type == 2) {
                bool matched = false;
                for(int a=0; a<s->ms_count; a++) if(strcmp(m->address, s->ms_addrs[a])==0) {matched=true; break;}
                if(!matched) continue;
            }

            // 构造全新任务 Transaction
            for (int j = 0; j < s->csd_count; j++) {
                cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
                cmd->action = CMD_ACTION_READ;
                cmd->tag_id = s->csd_list[j];
                cmd->priority = ARBITER_PRIO_NORMAL;
                cmd->timeout_ms = 1500;
                strncpy(cmd->protocol, m->protocol, 15);
                
                dcu_trans_ctx_t *ctx = calloc(1, sizeof(dcu_trans_ctx_t));
                ctx->poller = poller;
                ctx->channel_id = channel_id;
                ctx->task_id = task->task_id;
                ctx->oad = cmd->tag_id;
                ctx->retry_count = 0;
                strncpy(ctx->meter_addr, m->address, MAX_ADDR_LEN-1);
                
                cmd->params = ctx;
                cmd->user_data = poller;
                cmd->on_complete = _on_poll_cmd_complete;
                cmd->on_cleanup = _on_poll_cmd_cleanup;
                
                cmd->req = req_legacy_driver_create(dm, m->protocol, 1, cmd->action, cmd->tag_id, 0,
                                                   m->inject ? m->inject->lookup_by_tag : NULL,
                                                   m->inject ? m->inject->lookup_by_key : NULL, NULL);
                
                if (bus_orchestrator_submit_cmd(orch, channel_id, cmd) == 0) {
                    poller->in_flight_per_chan[channel_id]++;
                    task->active_cmds++;
                } else {
                    if (cmd->req && cmd->req->ops->teardown) cmd->req->ops->teardown(cmd->req);
                    free(ctx); free(cmd);
                }
            }
            found_meter = true;
            break;
        }
        uv_mutex_unlock(&inv->lock);

        if (!found_meter) {
            // 该任务已完成档案全断面扫描，退出泵送队列
            uv__queue_remove(q_task);
            printf("🏁 [SCHEDULER] 任务 %u 全断面扫描完成。\n", task->task_id);
        } else {
            // 移动到任务链表末尾，实现任务间的公平轮转调度
            uv__queue_remove(q_task);
            uv__queue_insert_tail(&poller->active_tasks, q_task);
        }
        break; // 每次泵送一个表计，由后续回调驱动连发
    }

    uv_mutex_unlock(&poller->lock);
}

static void _on_poll_cmd_complete(struct cmd_msg *cmd, int status) {
    if (!cmd || !cmd->params) return;
    dcu_poller_t *poller = (dcu_poller_t *)cmd->user_data;
    dcu_trans_ctx_t *ctx = (dcu_trans_ctx_t *)cmd->params;
    uint8_t chan = ctx->channel_id;

    if (status != 0 && ctx->retry_count < 3) {
        // 【核心闭环】：失败且有次数，不销毁上下文，压入重试队列
        ctx->retry_count++;
        printf("🔄 [RETRY] 物理超时/错误(Status:%d)，已压入补抄队列 -> OAD: %08X, 第 %d 次\n", 
               status, ctx->oad, ctx->retry_count);
        
        uv_mutex_lock(&poller->lock);
        uv__queue_insert_tail(&poller->retry_queues[chan], &ctx->node);
        poller->in_flight_per_chan[chan]--;
        uv_mutex_unlock(&poller->lock);
        
        // 脱钩 params 避免在 cmd_cleanup 中被释放
        cmd->params = NULL; 
    } else {
        // 最终完成 (成功或达到最大重试)
        if (status == 0) {
            // TODO: 原子增加对应任务监控 (6035) 的成功率
        }
        uv_mutex_lock(&poller->lock);
        poller->in_flight_per_chan[chan]--;
        uv_mutex_unlock(&poller->lock);
        
        // 正常的资源释放将由 cmd_cleanup 完成
    }

    // 💡 “抽水”递归：只要 Matrix 引擎空闲一个槽位，立即尝试泵入下一个任务
    dcu_poller_pump(poller, chan);
}

static void _on_poll_cmd_cleanup(struct cmd_msg *msg) {
    if (msg) {
        if (msg->params) free(msg->params); // 释放 dcu_trans_ctx_t
        if (msg->req && msg->req->ops && msg->req->ops->teardown) msg->req->ops->teardown(msg->req);
        free(msg);
    }
}

static void _production_periodic_cb(uv_periodic_t *handle) {
    dcu_poller_t *poller = (dcu_poller_t *)handle->data;
    if (!poller || !poller->sim_running) return;

    uint64_t now_s = (uint64_t)time(NULL);

    uv_mutex_lock(&poller->lock);
    for (uint16_t i = 0; i < poller->task_count; i++) {
        dcu_task_unit_t *t = &poller->tasks[i];
        if (!t->is_active) continue;

        // TI 时间对齐检查
        uint64_t boundary = (now_s / t->interval_s) * t->interval_s;
        uint64_t last_boundary = (t->last_run_time / t->interval_s) * t->interval_s;
        
        if (t->last_run_time == 0 || boundary > last_boundary) {
            t->last_run_time = now_s;
            t->cursor_meter_idx = 0; // 重置扫描游标
            
            // 激活扫描：挂载到活跃队列
            bool in_queue = false;
            struct uv__queue *q;
            uv__queue_foreach(q, &poller->active_tasks) {
                if (uv__queue_data(q, dcu_task_unit_t, node) == t) { in_queue = true; break; }
            }
            if (!in_queue) uv__queue_insert_tail(&poller->active_tasks, &t->node);
        }
    }
    uv_mutex_unlock(&poller->lock);

    // 尝试驱动所有物理通道进行泵送
    for (uint8_t c = 0; c < DCU_MAX_CHANNELS; c++) {
        dcu_poller_pump(poller, c);
    }
}

/* --- 公共 API --- */

int dcu_poller_init(dcu_poller_t *poller) {
    if (!poller) return -1;
    memset(poller, 0, sizeof(dcu_poller_t));
    uv_mutex_init(&poller->lock);
    uv__queue_init(&poller->active_tasks);
    for (int i = 0; i < DCU_MAX_CHANNELS; i++) uv__queue_init(&poller->retry_queues[i]);
    return 0;
}

int dcu_poller_add_task(dcu_poller_t *poller, uint32_t task_id, uint32_t interval_s, uint32_t scheme_id) {
    if (!poller || interval_s == 0 || poller->task_count >= MAX_TASKS) return -1;
    uv_mutex_lock(&poller->lock);
    dcu_task_unit_t *t = &poller->tasks[poller->task_count++];
    t->task_id = task_id; t->interval_s = interval_s; t->scheme_id = scheme_id;
    t->is_active = true; t->last_run_time = 0;
    uv__queue_init(&t->node);
    uv_mutex_unlock(&poller->lock);
    return 0;
}

int dcu_poller_add_scheme(dcu_poller_t *poller, uint32_t scheme_id, uint8_t mode, const uint32_t *csds, uint16_t csd_count) {
    if (!poller || poller->scheme_count >= MAX_SCHEMES) return -1;
    uv_mutex_lock(&poller->lock);
    dcu_scheme_t *s = &poller->schemes[poller->scheme_count++];
    s->scheme_id = scheme_id; s->capture_mode = mode; s->csd_count = csd_count;
    memcpy(s->csd_list, csds, sizeof(uint32_t) * csd_count);
    uv_mutex_unlock(&poller->lock);
    return 0;
}

dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t *poller, uint32_t scheme_id) {
    for (uint16_t i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id == scheme_id) return &poller->schemes[i];
    }
    return NULL;
}

int dcu_poller_start(dcu_poller_t *poller, dcu_inventory_t *inventory, void *orch_ptr, driver_manager_t *drv_mgr, void *loop_ptr) {
    if (!poller || !inventory || !orch_ptr || !loop_ptr) return -1;
    uv_mutex_lock(&poller->lock);
    poller->inventory_ptr = inventory;
    poller->hub_ptr = orch_ptr;
    poller->drv_mgr_ptr = drv_mgr;
    poller->sim_running = true;
    uv_periodic_t *periodic = malloc(sizeof(uv_periodic_t));
    uv_periodic_init((uv_loop_t *)loop_ptr, periodic);
    periodic->data = poller;
    uv_periodic_set(periodic, 0.0, 1000.0, NULL);
    uv_periodic_start(periodic, _production_periodic_cb);
    poller->sim_timer_ptr = periodic;
    uv_mutex_unlock(&poller->lock);
    return 0;
}

void dcu_poller_stop(dcu_poller_t *poller) {
    if (!poller || !poller->sim_running) return;
    uv_mutex_lock(&poller->lock);
    poller->sim_running = false;
    uv_periodic_t *periodic = (uv_periodic_t *)poller->sim_timer_ptr;
    if (periodic) {
        uv_periodic_stop(periodic);
        uv_close((uv_handle_t *)&periodic->timer, (uv_close_cb)free);
        poller->sim_timer_ptr = NULL;
    }
    uv_mutex_unlock(&poller->lock);
}

void dcu_poller_destroy(dcu_poller_t *poller) {
    if (!poller) return;
    uv_mutex_destroy(&poller->lock);
}
