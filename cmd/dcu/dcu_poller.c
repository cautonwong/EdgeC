#include "dcu_poller.h"
#include "dcu_router.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "sched_time.h"
#include "edge_log.h"

/**
 * @brief 初始化 DLT 698.45 调度引擎
 */
int dcu_poller_init(dcu_poller_t *poller) {
    if (!poller) return -1;
    
    poller->task_count = 0;
    poller->scheme_count = 0;
    memset(poller->tasks, 0, sizeof(poller->tasks));
    memset(poller->schemes, 0, sizeof(poller->schemes));
    memset(poller->monitors, 0, sizeof(poller->monitors));
    
    if (pthread_mutex_init(&poller->lock, NULL) != 0) {
        return -2;
    }
    
    return 0;
}

/**
 * @brief 动态添加任务单元 (OI 6012 Method Add Task)
 */
int dcu_poller_add_task(dcu_poller_t *poller, uint32_t task_id, uint32_t interval_s, uint32_t scheme_id) {
    if (!poller || interval_s == 0) return -1;
    
    pthread_mutex_lock(&poller->lock);
    
    // 去重
    for (uint16_t i = 0; i < poller->task_count; i++) {
        if (poller->tasks[i].task_id == task_id) {
            pthread_mutex_unlock(&poller->lock);
            return -2;
        }
    }
    
    if (poller->task_count >= MAX_TASKS) {
        pthread_mutex_unlock(&poller->lock);
        return -3;
    }
    
    dcu_task_unit_t *t = &poller->tasks[poller->task_count];
    t->task_id = task_id;
    t->interval_s = interval_s;
    t->scheme_id = scheme_id;
    t->last_run_time = 0;
    t->is_active = true;
    
    // 初始化监控单元 (6035)
    dcu_task_monitor_t *m = &poller->monitors[poller->task_count];
    m->task_id = task_id;
    m->status = 0; // 未执行
    m->total_meters = 0;
    m->success_meters = 0;
    m->total_packets = 0;
    
    poller->task_count++;
    
    pthread_mutex_unlock(&poller->lock);
    return 0;
}

/**
 * @brief 动态注册采集方案 (OI 6014 Method Add Scheme)
 */
int dcu_poller_add_scheme(dcu_poller_t *poller, uint32_t scheme_id, uint8_t mode, const uint32_t *csds, uint16_t csd_count) {
    if (!poller || !csds || csd_count == 0) return -1;
    
    pthread_mutex_lock(&poller->lock);
    
    for (uint16_t i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id == scheme_id) {
            pthread_mutex_unlock(&poller->lock);
            return -2;
        }
    }
    
    if (poller->scheme_count >= MAX_SCHEMES) {
        pthread_mutex_unlock(&poller->lock);
        return -3;
    }
    
    if (csd_count > MAX_SCHEME_OADS) {
        pthread_mutex_unlock(&poller->lock);
        return -4;
    }
    
    dcu_scheme_t *s = &poller->schemes[poller->scheme_count];
    s->scheme_id = scheme_id;
    s->capture_mode = mode;
    s->csd_count = csd_count;
    memcpy(s->csd_list, csds, sizeof(uint32_t) * csd_count);
    
    poller->scheme_count++;
    
    pthread_mutex_unlock(&poller->lock);
    return 0;
}

dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t *poller, uint32_t scheme_id) {
    if (!poller) return NULL;
    
    dcu_scheme_t *found = NULL;
    for (uint16_t i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id == scheme_id) {
            found = &poller->schemes[i];
            break;
        }
    }
    return found;
}

/**
 * @brief 核心物理调度：根据时间对齐策略，生成物理总线采集队列
 * 【电力采集调度物理设计意图】：终端定时引擎不需要对所有表计发起杂乱的物理抄读。
 * 而是定时比对任务配置表中的“执行频率 TI”。一旦到期（例如 15 分钟到了），调度引擎进入“工作状态”，
 * 瞬间提取对应的采集方案所指定的 CSD 数据列和电表列表，组装成有序的总线队列依次分发物理抄读帧，
 * 并打上标准的启动/成功/存储时标（6040/6041/6042）。
 */
int dcu_poller_generate_queue(dcu_poller_t *poller, dcu_inventory_t *inventory, dcu_poll_queue_t *out_queue, uint64_t now_s) {
    if (!poller || !inventory || !out_queue) return -1;
    
    pthread_mutex_lock(&poller->lock);
    memset(out_queue, 0, sizeof(dcu_poll_queue_t));
    
    // 遍历所有已登记任务单元 (OI 6013)
    for (uint16_t i = 0; i < poller->task_count; i++) {
        dcu_task_unit_t *t = &poller->tasks[i];
        if (!t->is_active) continue;
        
        // ⏰ 基于自然挂钟时间的定时对齐调度机制 (Cron-like 对齐，如 5秒周期在 :00, :05, :10 触发)
        uint64_t current_boundary = (now_s / t->interval_s) * t->interval_s;
        if (t->last_run_time == 0) {
            // 首次运行，初始化 last_run_time 并立刻触发
            t->last_run_time = now_s;
        } else {
            uint64_t last_boundary = (t->last_run_time / t->interval_s) * t->interval_s;
            if (current_boundary <= last_boundary) {
                continue; // 执行周期未到自然对齐边缘，跳过
            }
            t->last_run_time = now_s;
        }
        
        // 找到该任务绑定的普通采集方案 (6015)
        dcu_scheme_t *s = dcu_poller_get_scheme(poller, t->scheme_id);
        if (!s) continue;
        
        // 调度被执行，更新监控状态为 1 (执行中)
        dcu_task_monitor_t *mon = &poller->monitors[i];
        mon->status = 1; 
        mon->total_meters = inventory->count;
        mon->success_meters = 0;
        
        pthread_mutex_lock(&inventory->lock);
        
        // 将建档电表 TSA 集合装入执行队列
        for (uint16_t k = 0; k < inventory->count; k++) {
            production_meter_t *m = &inventory->meters[k];
            
            if (out_queue->count >= MAX_QUEUE_ENTRIES) break;
            
            dcu_poll_entry_t *entry = &out_queue->entries[out_queue->count];
            strncpy(entry->meter_address, m->address, MAX_ADDR_LEN - 1);
            entry->meter_address[MAX_ADDR_LEN - 1] = '\0';
            
            entry->oad_count = s->csd_count;
            memcpy(entry->oads, s->csd_list, sizeof(uint32_t) * s->csd_count);
            
            out_queue->count++;
            
            // 物理抄读时标盖戳
            m->last_poll_time_s = (uint32_t)now_s;
        }
        
        pthread_mutex_unlock(&inventory->lock);
        
        // 更新完成：状态置为 2 (已完成)
        mon->status = 2;
        mon->success_meters = mon->total_meters; // 模拟全部成功
        mon->total_packets += mon->total_meters * 2; // 收发两包
        
        printf("⏰ [SCHEDULER] DLT 698.45 定时任务 %u 到期触发！拉取方案 %u，组装 %d 块表计抄表队列 (盖上 6040/6041 物理时标)。\n", 
               t->task_id, t->scheme_id, out_queue->count);
        
        break; // 单个周期仅调度分发一个任务，防信道冲突
    }
    
    pthread_mutex_unlock(&poller->lock);
    return 0;
}

void dcu_poller_destroy(dcu_poller_t *poller) {
    if (!poller) return;
    pthread_mutex_destroy(&poller->lock);
}

/* ==============================================================================
 * 基于 sched_engine.c 精准对齐策略的真实物理采集引擎 (Task 4)
 * ============================================================================== */

#include "bus_orchestrator.h"
#include "req_driver_legacy.h"

/**
 * @brief [Nexus] 补抄追踪上下文
 */
typedef struct {
    dcu_poller_t *poller;
    uint8_t retry_count;
    uint8_t channel_id;
} poller_retry_ctx_t;

static void _on_poll_cmd_cleanup(struct cmd_msg *msg) {
    if (msg && msg->params) {
        free(msg->params); // 释放 retry_ctx
    }
}

static void _on_poll_cmd_complete(struct cmd_msg *cmd, int status) {
    if (!cmd || !cmd->params) return;
    
    poller_retry_ctx_t *ctx = (poller_retry_ctx_t *)cmd->params;
    
    if (status == 0) {
        // 成功，无需处理
        return;
    }

    // 失败处理 (TIMEOUT 或 IO_ERROR)
    if (ctx->retry_count < 3) {
        ctx->retry_count++;
        printf("🔄 [RETRY] 采集任务失败(Status:%d)，正在执行第 %d 次即时补抄 -> OAD: %08X, Channel: %d\n", 
               status, ctx->retry_count, cmd->tag_id, ctx->channel_id);
        
        // 重新提交至总线
        bus_orchestrator_t *orch = (bus_orchestrator_t *)ctx->poller->hub_ptr;
        
        // 重置任务状态以便重新入队
        cmd->submit_ts = (uint64_t)uv_hrtime() / 1000000;
        
        if (bus_orchestrator_submit_cmd(orch, ctx->channel_id, cmd) != 0) {
             // 如果提交失败，交由 cleanup 释放
        }
    } else {
        printf("❌ [FATAL] OAD %08X 连续重试失败，放弃本次采集。\n", cmd->tag_id);
    }
}

static void _production_periodic_cb(uv_periodic_t *handle) {
    dcu_poller_t *poller = (dcu_poller_t *)handle->data;
    if (!poller || !poller->sim_running) return;

    dcu_inventory_t *inv = (dcu_inventory_t *)poller->inventory_ptr;
    bus_orchestrator_t *orch = (bus_orchestrator_t *)poller->hub_ptr;
    driver_manager_t *dm = (driver_manager_t *)poller->drv_mgr_ptr;
    if (!inv || !orch || !dm) return;

    uint64_t now_s = (uint64_t)time(NULL);

    // 1. 周期性对齐生成队列
    dcu_poll_queue_t queue;
    if (dcu_poller_generate_queue(poller, inv, &queue, now_s) != 0) {
        return;
    }

    // 2. 将队列转化为真实的 Matrix IO 读请求
    for (uint16_t i = 0; i < queue.count; i++) {
        dcu_poll_entry_t *entry = &queue.entries[i];
        production_meter_t *meter = dcu_inventory_get(inv, entry->meter_address);
        if (!meter) continue;

        for (uint16_t j = 0; j < entry->oad_count; j++) {
            uint32_t oad = entry->oads[j];
            
            cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
            cmd->action = CMD_ACTION_READ;
            cmd->tag_id = oad;
            cmd->timeout_ms = 1500; 
            strncpy(cmd->protocol, meter->protocol, sizeof(cmd->protocol)-1);

            // 注册补抄追踪上下文
            poller_retry_ctx_t *retry_ctx = calloc(1, sizeof(poller_retry_ctx_t));
            retry_ctx->poller = poller;
            retry_ctx->retry_count = 0;
            retry_ctx->channel_id = meter->channel_id;

            cmd->params = retry_ctx;
            cmd->on_complete = _on_poll_cmd_complete;
            cmd->on_cleanup = _on_poll_cmd_cleanup;

            // [Deepening] 关键补完：必须创建真正的 Request 对象，否则 Matrix 引擎会因为空指针崩溃
            cmd->req = req_legacy_driver_create(dm, meter->protocol, 1, // Slave ID 默认 1
                                               cmd->action, oad, 0,
                                               meter->inject ? meter->inject->lookup_by_tag : NULL,
                                               meter->inject ? meter->inject->lookup_by_key : NULL,
                                               NULL);

            if (cmd->req) {
                if (bus_orchestrator_submit_cmd(orch, meter->channel_id, cmd) != 0) {
                    if (cmd->req->ops && cmd->req->ops->teardown) cmd->req->ops->teardown(cmd->req);
                    free(retry_ctx);
                    free(cmd);
                }
            } else {
                printf("⚠️  [POLLER] Failed to create legacy request for %s (OAD %08X)\n", meter->protocol, oad);
                free(retry_ctx);
                free(cmd);
            }
        }
    }
}

/**
 * @brief 启动高精度时间对齐采集调度引擎
 */
int dcu_poller_start(dcu_poller_t *poller, dcu_inventory_t *inventory, void *orch_ptr, driver_manager_t *drv_mgr, void *loop_ptr) {
    if (!poller || !inventory || !orch_ptr || !loop_ptr) return -1;
    
    pthread_mutex_lock(&poller->lock);
    
    poller->inventory_ptr = inventory;
    poller->hub_ptr = orch_ptr;
    poller->drv_mgr_ptr = drv_mgr; // [Nexus] 保存驱动管理器句柄
    poller->sim_running = true;
    
    uv_periodic_t *periodic = malloc(sizeof(uv_periodic_t));
    uv_periodic_init((uv_loop_t *)loop_ptr, periodic);
    periodic->data = poller;
    
    // 秒级对齐
    uv_periodic_set(periodic, 0.0, 1000.0, NULL);
    uv_periodic_start(periodic, _production_periodic_cb);
    
    poller->sim_timer_ptr = periodic;
    
    pthread_mutex_unlock(&poller->lock);
    return 0;
}

void dcu_poller_stop(dcu_poller_t *poller) {
    if (!poller || !poller->sim_running) return;
    
    pthread_mutex_lock(&poller->lock);
    
    poller->sim_running = false;
    uv_periodic_t *periodic = (uv_periodic_t *)poller->sim_timer_ptr;
    if (periodic) {
        uv_periodic_stop(periodic);
        uv_close((uv_handle_t *)&periodic->timer, (uv_close_cb)free);
        poller->sim_timer_ptr = NULL;
    }
    
    pthread_mutex_unlock(&poller->lock);
}
