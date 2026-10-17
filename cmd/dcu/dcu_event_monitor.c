#include "dcu_event_monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "datahub.h"
#include "dcu_inventory.h"
#include "edge_log.h"
#include "stream_engine_priv.h"

// ==============================================================================
// 1. 失压与过流流式监控判定算子 (stage_event_monitor) 实现
// ==============================================================================
typedef struct {
    dcu_event_obj_t* evt_3000;
    dcu_event_obj_t* evt_3005;

    // [Industrial Scaling] 为 4096 只表提供独立的状态机
    uint64_t v_overlimit_start_ts[MAX_METERS];
    bool v_event_triggered[MAX_METERS];

    uint64_t c_overlimit_start_ts[MAX_METERS];
    bool c_event_triggered[MAX_METERS];
} monitor_stage_ctx_t;

static stage_ret_t _monitor_proc(pipeline_stage_t* stage, pipe_msg_t* ev) {
    monitor_stage_ctx_t* ctx = (monitor_stage_ctx_t*)stage->priv_state;
    if (!ctx || ev->device_id >= MAX_METERS) return STAGE_DROP;

    uint64_t now_ns = ev->ts;  // 核心：这是纳秒 (uv_hrtime)
    uint32_t dev = ev->device_id;

    // 💡 1. 拦截电压实时刷新事件 (OAD_VOLTAGE) -> 失压判定
    if (ev->type == PIPEMSG_TYPE_SCALAR &&
        (ev->tag_id == OAD_VOLTAGE || ev->tag_id == 0x20000300)) {
        dcu_event_obj_t* evt = ctx->evt_3000;
        if (evt && evt->is_enabled) {
            float voltage = (float)ev->payload.val.f;

            if (voltage < evt->threshold) {
                if (ctx->v_overlimit_start_ts[dev] == 0) {
                    ctx->v_overlimit_start_ts[dev] = now_ns;
                    if (dev % 100 == 0)
                        printf(
                            "⚠️ [MONITOR] Dev %u: Voltage %.1fV < %.1fV. Starting %us "
                            "countdown...\n",
                            dev, voltage, evt->threshold, evt->delay_s);
                } else if (!ctx->v_event_triggered[dev] &&
                           (now_ns - ctx->v_overlimit_start_ts[dev] >=
                            (uint64_t)evt->delay_s * 1000000000ULL)) {
                    ctx->v_event_triggered[dev] = true;

                    extern dcu_inventory_t g_dcu_inventory;
                    const char* addr = (dev < g_dcu_inventory.count)
                                           ? g_dcu_inventory.meters[dev].address
                                           : "Unknown";

                    pipe_msg_t event_msg;
                    memset(&event_msg, 0, sizeof(pipe_msg_t));
                    event_msg.tag_id = OI_VOLTAGE_DROP;
                    event_msg.type = PIPEMSG_TYPE_EVENT;
                    event_msg.ts = now_ns;
                    event_msg.device_id = dev;
                    event_msg.payload.val.f = voltage;

                    printf(
                        "🚨🚨🚨 [ALARM TRIGGER] Dev %u (TSA: %s) Voltage %.1fV below threshold! "
                        "Writing to DB...\n",
                        dev, addr, voltage);
                    stream_engine_push(stage->pipeline->engine, &event_msg);
                }
            } else {
                if (ctx->v_event_triggered[dev]) {
                    ctx->v_event_triggered[dev] = false;
                    pipe_msg_t event_msg;
                    memset(&event_msg, 0, sizeof(pipe_msg_t));
                    event_msg.tag_id = OI_VOLTAGE_DROP;
                    event_msg.type = PIPEMSG_TYPE_EVENT;
                    event_msg.ts = now_ns;
                    event_msg.device_id = dev;
                    event_msg.payload.val.f = -voltage;
                    printf("💚 [RECOVER] Dev %u: Voltage Restored to %.1fV.\n", dev, voltage);
                    stream_engine_push(stage->pipeline->engine, &event_msg);
                }
                ctx->v_overlimit_start_ts[dev] = 0;
            }
        }
        return STAGE_OK;
    }

    // 💡 2. 拦截电流实时刷新事件 (OAD_CURRENT) -> 过流判定
    if (ev->type == PIPEMSG_TYPE_SCALAR &&
        (ev->tag_id == OAD_CURRENT || ev->tag_id == 0x20010300)) {
        dcu_event_obj_t* evt = ctx->evt_3005;
        if (evt && evt->is_enabled) {
            float current = (float)ev->payload.val.f;

            if (current > evt->threshold) {
                if (ctx->c_overlimit_start_ts[dev] == 0) {
                    ctx->c_overlimit_start_ts[dev] = now_ns;
                } else if (!ctx->c_event_triggered[dev] &&
                           (now_ns - ctx->c_overlimit_start_ts[dev] >=
                            (uint64_t)evt->delay_s * 1000000000ULL)) {
                    ctx->c_event_triggered[dev] = true;

                    pipe_msg_t event_msg;
                    memset(&event_msg, 0, sizeof(pipe_msg_t));
                    event_msg.tag_id = OI_OVER_CURRENT;
                    event_msg.type = PIPEMSG_TYPE_EVENT;
                    event_msg.ts = now_ns;
                    event_msg.device_id = dev;
                    event_msg.payload.val.f = current;

                    stream_engine_push(stage->pipeline->engine, &event_msg);
                }
            } else {
                if (ctx->c_event_triggered[dev]) {
                    ctx->c_event_triggered[dev] = false;
                    pipe_msg_t event_msg;
                    memset(&event_msg, 0, sizeof(pipe_msg_t));
                    event_msg.tag_id = OI_OVER_CURRENT;
                    event_msg.type = PIPEMSG_TYPE_EVENT;
                    event_msg.ts = now_ns;
                    event_msg.device_id = dev;
                    event_msg.payload.val.f = -current;
                    stream_engine_push(stage->pipeline->engine, &event_msg);
                }
                ctx->c_overlimit_start_ts[dev] = 0;
            }
        }
        return STAGE_OK;
    }

    return STAGE_OK;
}

static void _monitor_cleanup(void* priv) {
    if (priv) free(priv);
}

pipeline_stage_t* stage_event_monitor_create(dcu_event_obj_t* evt_3000, dcu_event_obj_t* evt_3005) {
    pipeline_stage_t* s = calloc(1, sizeof(pipeline_stage_t));
    monitor_stage_ctx_t* ctx = calloc(1, sizeof(monitor_stage_ctx_t));

    ctx->evt_3000 = evt_3000;
    ctx->evt_3005 = evt_3005;

    strncpy(s->name, "EVT_MONITOR", 31);
    s->process = _monitor_proc;
    s->priv_state = ctx;
    s->cleanup_state = _monitor_cleanup;
    return s;
}

// ==============================================================================
// 2. 事件快照追加与主动上报算子 (stage_event_logger) 实现
// ==============================================================================
typedef struct {
    dcu_event_obj_t* evt_3000;
    dcu_event_obj_t* evt_3005;
    datahub_t* hub;
    event_sink_ops_t sink_ops;
} logger_stage_ctx_t;

static stage_ret_t _logger_proc(pipeline_stage_t* stage, pipe_msg_t* ev) {
    logger_stage_ctx_t* ctx = (logger_stage_ctx_t*)stage->priv_state;
    if (!ctx) return STAGE_DROP;

    // 我们只处理 PIPEMSG_TYPE_EVENT 告警事件消息
    if (ev->type == PIPEMSG_TYPE_EVENT) {
        dcu_event_obj_t* evt = NULL;
        if (ev->tag_id == OI_VOLTAGE_DROP)
            evt = ctx->evt_3000;
        else if (ev->tag_id == OI_OVER_CURRENT)
            evt = ctx->evt_3005;

        if (evt && evt->is_enabled) {
            float val = (float)ev->payload.val.f;
            bool is_recover = (val < 0.0f);
            float actual_val = is_recover ? -val : val;

            // 💡 倒序排列追加事件记录表 (新产生的排在最前)
            uint16_t limit = (evt->record_count < MAX_EVENT_RECORDS) ? evt->record_count
                                                                     : (MAX_EVENT_RECORDS - 1);
            for (int i = limit; i > 0; i--) {
                evt->records[i] = evt->records[i - 1];
            }

            dcu_event_record_t* new_rec = &evt->records[0];
            new_rec->index = (uint16_t)(evt->record_count + 1);
            new_rec->snapshot_value = actual_val;
            new_rec->is_active = !is_recover;

            if (!is_recover) {
                new_rec->start_time = ev->ts;
                new_rec->end_time = 0;
                evt->last_start_time = ev->ts;  // 更新最近发生时标

                // 【Generic Snapshot Engine】根据 Attr 3 抓拍现场数据
                pipe_msg_t snapshots[8];
                int captured = dcu_obj_snapshot(evt->oi, ev->device_id, snapshots, 8);

                // 💡 物理落盘：记录到事件历史表 (Class 7 Attr 2 落地)
                extern dcu_inventory_t g_dcu_inventory;
                const char* addr = (ev->device_id < g_dcu_inventory.count)
                                       ? g_dcu_inventory.meters[ev->device_id].address
                                       : "Unknown";

                if (ctx->sink_ops.archive_event) {
                    ctx->sink_ops.archive_event(ctx->sink_ops.ctx, addr, evt->oi, ev->ts, 0,
                                                actual_val);
                }

                printf("📝 [SNAPSHOT] 事件 %04X 触发 (表: %s)！抓取 %d 项关联快照并入库。\n",
                       evt->oi, addr, captured);
            } else {
                new_rec->start_time = evt->last_start_time;
                new_rec->end_time = ev->ts;
                evt->last_end_time = ev->ts;  // 更新最近恢复时标

                extern dcu_inventory_t g_dcu_inventory;
                const char* addr = (ev->device_id < g_dcu_inventory.count)
                                       ? g_dcu_inventory.meters[ev->device_id].address
                                       : "Unknown";

                if (ctx->sink_ops.archive_event) {
                    ctx->sink_ops.archive_event(ctx->sink_ops.ctx, addr, evt->oi,
                                                evt->last_start_time, ev->ts, actual_val);
                }

                printf("📝 [SNAPSHOT] 事件 %04X 恢复 (表: %s)！记录更新。\n", evt->oi, addr);
            }

            if (evt->record_count < MAX_EVENT_RECORDS) {
                evt->record_count++;
            }

            // 💡 3. 【主动上报推送】如果配置了上报标识，则触发推送告警报文
            if (evt->report_flag > 0) {
                printf(
                    "📣 [REPORT] 主动上报！终端向主站主动推送 3xxx 事件告警帧，OID: %04X, "
                    "记录序号: %d\n",
                    evt->oi, new_rec->index);
            }
        }
        return STAGE_DROP;  // 拦截事件，不继续流往下游
    }

    return STAGE_OK;
}

static void _logger_cleanup(void* priv) {
    if (priv) free(priv);
}

pipeline_stage_t* stage_event_logger_create(dcu_event_obj_t* evt_3000, dcu_event_obj_t* evt_3005,
                                            datahub_t* hub, event_sink_ops_t sink_ops) {
    pipeline_stage_t* s = calloc(1, sizeof(pipeline_stage_t));
    logger_stage_ctx_t* ctx = calloc(1, sizeof(logger_stage_ctx_t));

    ctx->evt_3000 = evt_3000;
    ctx->evt_3005 = evt_3005;
    ctx->hub = hub;
    ctx->sink_ops = sink_ops;

    strncpy(s->name, "EVT_LOGGER", 31);
    s->process = _logger_proc;
    s->priv_state = ctx;
    s->cleanup_state = _logger_cleanup;
    return s;
}

// ==============================================================================
// 3. 日冻结定期快照算子 (stage_periodic_freeze) 实现
// ==============================================================================
typedef struct {
    datahub_t* hub;
    event_sink_ops_t sink_ops;
} freeze_stage_ctx_t;

static stage_ret_t _freeze_proc(pipeline_stage_t* stage, pipe_msg_t* ev) {
    freeze_stage_ctx_t* ctx = (freeze_stage_ctx_t*)stage->priv_state;
    if (!ctx || ev->device_id >= MAX_METERS) return STAGE_DROP;

    // 定时触发标签：匹配标准日冻结定时触发 OAD (OAD_DAILY_FREEZE)
    if (ev->tag_id == OAD_DAILY_FREEZE) {
        uint32_t dev = ev->device_id;
        extern dcu_inventory_t g_dcu_inventory;
        if (dev >= g_dcu_inventory.count) return STAGE_DROP;
        const char* addr = g_dcu_inventory.meters[dev].address;

        // 💡 [2D Matrix Snapshot] 瞬间抓取 DataHub Shadow 缓存全断面电参量快照
        double active_energy = 0.0, voltage = 0.0, current = 0.0;
        pipe_msg_t shadow_msg;

        // 抓拍电能 (Offset 1)
        if (datahub_read_shadow_device(ctx->hub, OAD_ACTIVE_ENERGY, dev, &shadow_msg) == 0) {
            active_energy = shadow_msg.payload.val.f;
        }
        // 抓拍电压 (Offset 0)
        if (datahub_read_shadow_device(ctx->hub, OAD_VOLTAGE, dev, &shadow_msg) == 0) {
            voltage = shadow_msg.payload.val.f;
        }
        // 抓拍电流 (Offset 2)
        if (datahub_read_shadow_device(ctx->hub, 0x20010200, dev, &shadow_msg) == 0) {
            current = shadow_msg.payload.val.f;
        }

        uint64_t freeze_ts = ev->ts;
        if (ctx->sink_ops.archive_freeze) {
            ctx->sink_ops.archive_freeze(ctx->sink_ops.ctx, addr, OAD_DAILY_FREEZE, freeze_ts,
                                         OAD_ACTIVE_ENERGY, active_energy);
            ctx->sink_ops.archive_freeze(ctx->sink_ops.ctx, addr, OAD_DAILY_FREEZE, freeze_ts,
                                         OAD_VOLTAGE, voltage);
            ctx->sink_ops.archive_freeze(ctx->sink_ops.ctx, addr, OAD_DAILY_FREEZE, freeze_ts,
                                         0x20010200, current);
        }

        return STAGE_DROP;
    }

    return STAGE_OK;
}

static void _freeze_cleanup(void* priv) {
    if (priv) free(priv);
}

pipeline_stage_t* stage_periodic_freeze_create_ex(datahub_t* hub, event_sink_ops_t sink_ops) {
    pipeline_stage_t* s = calloc(1, sizeof(pipeline_stage_t));
    freeze_stage_ctx_t* ctx = calloc(1, sizeof(freeze_stage_ctx_t));

    ctx->hub = hub;
    ctx->sink_ops = sink_ops;

    strncpy(s->name, "PER_FREEZE", 31);
    s->process = _freeze_proc;
    s->priv_state = ctx;
    s->cleanup_state = _freeze_cleanup;
    return s;
}
