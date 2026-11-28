#include "dcu_event_monitor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "datahub.h"
#include "dcu_698_var.h"
#include "dcu_inventory.h"
#include "edge_log.h"

// ==============================================================================
// 1. 失压与过流流式监控判定算子 (stage_event_monitor) 实现
// ==============================================================================
typedef struct {
    dcu_inventory_t* inv;
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
    /* pipe 消息的 device_id 是 slot_id（稳定设备身份）；档案下标删除即变，不能混用。 */
    uint32_t slot = ev->device_id;

    // 💡 1. 拦截电压实时刷新事件 (OAD_VOLTAGE) -> 失压判定
    if (ev->type == PIPEMSG_TYPE_SCALAR &&
        (ev->tag_id == OAD_VOLTAGE || ev->tag_id == TAG_VOLTAGE_B)) {
        dcu_event_obj_t* evt = ctx->evt_3000;
        if (evt && evt->is_enabled) {
            float voltage = (float)ev->payload.val.f;
            if (voltage < evt->threshold) {
                if (ctx->v_overlimit_start_ts[slot] == 0) {
                    ctx->v_overlimit_start_ts[slot] = now_ns;
                    if (slot % 100 == 0)
                        printf(
                            "⚠️ [MONITOR] Dev %u: Voltage %.1fV < %.1fV. Starting %us "
                            "countdown...\n",
                            slot, voltage, evt->threshold, evt->delay_s);
                } else if (!ctx->v_event_triggered[slot] &&
                           (now_ns - ctx->v_overlimit_start_ts[slot] >=
                            (uint64_t)evt->delay_s * 1000000000ULL)) {
                    ctx->v_event_triggered[slot] = true;

                    const production_meter_t* meter = dcu_inventory_by_slot(ctx->inv, slot);
                    const char* addr = meter ? meter->address : "Unknown";

                    pipe_msg_t event_msg;
                    memset(&event_msg, 0, sizeof(pipe_msg_t));
                    event_msg.tag_id = OI_VOLTAGE_DROP;
                    event_msg.type = PIPEMSG_TYPE_EVENT;
                    event_msg.ts = now_ns;
                    event_msg.device_id = slot;
                    event_msg.payload.val.f = voltage;

                    printf(
                        "🚨🚨🚨 [ALARM TRIGGER] Dev %u (TSA: %s) Voltage %.1fV below threshold! "
                        "Writing to DB...\n",
                        slot, addr, voltage);
                    stream_engine_push(pipeline_stage_get_engine(stage), &event_msg);
                }
            } else {
                if (ctx->v_event_triggered[slot]) {
                    ctx->v_event_triggered[slot] = false;
                    pipe_msg_t event_msg;
                    memset(&event_msg, 0, sizeof(pipe_msg_t));
                    event_msg.tag_id = OI_VOLTAGE_DROP;
                    event_msg.type = PIPEMSG_TYPE_EVENT;
                    event_msg.ts = now_ns;
                    event_msg.device_id = slot;
                    event_msg.payload.val.f = -voltage;
                    printf("💚 [RECOVER] Dev %u: Voltage Restored to %.1fV.\n", slot, voltage);
                    stream_engine_push(pipeline_stage_get_engine(stage), &event_msg);
                }
                ctx->v_overlimit_start_ts[slot] = 0;
            }
        }
        return STAGE_OK;
    }

    // 💡 2. 拦截电流实时刷新事件 (OAD_CURRENT) -> 过流判定
    if (ev->type == PIPEMSG_TYPE_SCALAR &&
        (ev->tag_id == OAD_CURRENT || ev->tag_id == TAG_CURRENT_B)) {
        dcu_event_obj_t* evt = ctx->evt_3005;
        if (evt && evt->is_enabled) {
            float current = (float)ev->payload.val.f;

            if (current > evt->threshold) {
                if (ctx->c_overlimit_start_ts[slot] == 0) {
                    ctx->c_overlimit_start_ts[slot] = now_ns;
                } else if (!ctx->c_event_triggered[slot] &&
                           (now_ns - ctx->c_overlimit_start_ts[slot] >=
                            (uint64_t)evt->delay_s * 1000000000ULL)) {
                    ctx->c_event_triggered[slot] = true;

                    pipe_msg_t event_msg;
                    memset(&event_msg, 0, sizeof(pipe_msg_t));
                    event_msg.tag_id = OI_OVER_CURRENT;
                    event_msg.type = PIPEMSG_TYPE_EVENT;
                    event_msg.ts = now_ns;
                    event_msg.device_id = slot;
                    event_msg.payload.val.f = current;

                    stream_engine_push(pipeline_stage_get_engine(stage), &event_msg);
                }
            } else {
                if (ctx->c_event_triggered[slot]) {
                    ctx->c_event_triggered[slot] = false;
                    pipe_msg_t event_msg;
                    memset(&event_msg, 0, sizeof(pipe_msg_t));
                    event_msg.tag_id = OI_OVER_CURRENT;
                    event_msg.type = PIPEMSG_TYPE_EVENT;
                    event_msg.ts = now_ns;
                    event_msg.device_id = slot;
                    event_msg.payload.val.f = -current;
                    stream_engine_push(pipeline_stage_get_engine(stage), &event_msg);
                }
                ctx->c_overlimit_start_ts[slot] = 0;
            }
        }
        return STAGE_OK;
    }

    return STAGE_OK;
}

static void _monitor_cleanup(void* priv) {
    if (priv) free(priv);
}

pipeline_stage_t* stage_event_monitor_create(dcu_inventory_t* inv, dcu_event_obj_t* evt_3000,
                                             dcu_event_obj_t* evt_3005) {
    pipeline_stage_t* s = calloc(1, sizeof(pipeline_stage_t));
    if (!s) return NULL;
    monitor_stage_ctx_t* ctx = calloc(1, sizeof(monitor_stage_ctx_t));
    if (!ctx) {
        free(s);
        return NULL;
    }

    ctx->inv = inv;
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
    dcu_inventory_t* inv;
    dcu_event_obj_t* evt_3000;
    dcu_event_obj_t* evt_3005;
    datahub_t* hub;
    event_sink_ops_t sink_ops;
} logger_stage_ctx_t;

/* uv_hrtime 是单调时钟，不能当公历时间；事件记录的墙钟只从 CLOCK_REALTIME 取。 */
static uint64_t _wall_epoch_s(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0 || ts.tv_sec < 0) return 0;
    return (uint64_t)ts.tv_sec;
}

static void _capture_record_snapshots(const dcu_event_obj_t* evt, uint32_t device_id,
                                      dcu_event_record_t* rec) {
    for (uint8_t i = 0; i < evt->snapshot_count && i < MAX_EVENT_SNAPSHOTS; i++) {
        size_t len = 0;
        if (dcu_698_var_encode_axdr_device(evt->snapshot_oads[i], device_id, rec->snapshot[i],
                                           sizeof(rec->snapshot[i]), &len) == 0 &&
            len <= sizeof(rec->snapshot[i])) {
            rec->snapshot_len[i] = (uint8_t)len;
        }
    }
}

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
            uint64_t wall_s = _wall_epoch_s();

            /* 发生与恢复都按 slot_id 取地址：档案删除重排后下标 N 可能已是另一只表。 */
            const production_meter_t* meter = dcu_inventory_by_slot(ctx->inv, ev->device_id);
            const char* addr = meter ? meter->address : "Unknown";

            dcu_event_stamp_t slot_start = {0};
            if (is_recover) {
                bool found = false;
                for (uint16_t i = 0; i < evt->record_count && i < MAX_EVENT_RECORDS; i++) {
                    if (evt->records[i].is_active && evt->records[i].slot_id == ev->device_id) {
                        slot_start = evt->records[i].start_time;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    /* 发生记录已被挤出窗口：A6 发生时间是必填非空字段，不伪造起点，跳过追加。 */
                    evt->last_end_time = (dcu_event_stamp_t){.mono_ns = ev->ts, .epoch_s = wall_s};
                    printf(
                        "📝 [SNAPSHOT] 事件 %04X 恢复 (表: %s)！发生记录已不在 10 条窗口内，不追加恢复记录。\n",
                        evt->oi, addr);
                    return STAGE_DROP;
                }
            }

            // 💡 倒序排列追加事件记录表 (新产生的排在最前)
            uint16_t limit = (evt->record_count < MAX_EVENT_RECORDS) ? evt->record_count
                                                                     : (MAX_EVENT_RECORDS - 1);
            for (int i = limit; i > 0; i--) {
                evt->records[i] = evt->records[i - 1];
            }

            dcu_event_record_t* new_rec = &evt->records[0];
            memset(new_rec, 0, sizeof(*new_rec));
            new_rec->index =
                (evt->record_count > 0) ? (uint16_t)(evt->records[1].index + 1u) : 1u;
            new_rec->slot_id = ev->device_id;
            new_rec->snapshot_value = actual_val;
            new_rec->is_active = !is_recover;

            if (!is_recover) {
                new_rec->start_time.mono_ns = ev->ts;
                new_rec->start_time.epoch_s = wall_s;
                evt->last_start_time = new_rec->start_time;

                // 【Generic Snapshot Engine】根据 Attr 3 抓拍现场数据
                pipe_msg_t snapshots[8];
                int captured = dcu_obj_snapshot(evt->oi, ev->device_id, snapshots, 8);
                _capture_record_snapshots(evt, ev->device_id, new_rec);

                // 💡 物理落盘：记录到事件历史表 (Class 7 Attr 2 落地)
                if (ctx->sink_ops.archive_event) {
                    ctx->sink_ops.archive_event(ctx->sink_ops.ctx, addr, evt->oi,
                                                new_rec->start_time.epoch_s, 0, actual_val);
                }

                printf("📝 [SNAPSHOT] 事件 %04X 触发 (表: %s)！抓取 %d 项关联快照并入库。\n",
                       evt->oi, addr, captured);
            } else {
                new_rec->start_time = slot_start;
                new_rec->end_time.mono_ns = ev->ts;
                new_rec->end_time.epoch_s = wall_s;
                evt->last_end_time = new_rec->end_time;
                _capture_record_snapshots(evt, ev->device_id, new_rec);

                if (ctx->sink_ops.archive_event) {
                    ctx->sink_ops.archive_event(ctx->sink_ops.ctx, addr, evt->oi,
                                                slot_start.epoch_s, new_rec->end_time.epoch_s,
                                                actual_val);
                }

                printf("📝 [SNAPSHOT] 事件 %04X 恢复 (表: %s)！记录更新。\n", evt->oi, addr);
            }

            if (evt->record_count < MAX_EVENT_RECORDS) {
                evt->record_count++;
            }

            /* 主动上报等 CHOICE[2] 记录型上报（D1-07）落地后再从这里入队。
             * 不能报 A4：D1-18b 后 A4 是最大记录数（常量），拿它当当前数上报就是假语义；
             * A3 的四表结构还没由对象模型暴露，在别处拼会变成第二份真相。 */
        }
        return STAGE_DROP;  // 拦截事件，不继续流往下游
    }

    return STAGE_OK;
}

static void _logger_cleanup(void* priv) {
    if (priv) free(priv);
}

pipeline_stage_t* stage_event_logger_create(dcu_inventory_t* inv, dcu_event_obj_t* evt_3000,
                                            dcu_event_obj_t* evt_3005, datahub_t* hub,
                                            event_sink_ops_t sink_ops) {
    pipeline_stage_t* s = calloc(1, sizeof(pipeline_stage_t));
    if (!s) return NULL;
    logger_stage_ctx_t* ctx = calloc(1, sizeof(logger_stage_ctx_t));
    if (!ctx) {
        free(s);
        return NULL;
    }

    ctx->inv = inv;
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
    dcu_inventory_t* inv;
    datahub_t* hub;
    event_sink_ops_t sink_ops;
} freeze_stage_ctx_t;

static stage_ret_t _freeze_proc(pipeline_stage_t* stage, pipe_msg_t* ev) {
    freeze_stage_ctx_t* ctx = (freeze_stage_ctx_t*)stage->priv_state;
    if (!ctx || ev->device_id >= MAX_METERS) return STAGE_DROP;

    // 定时触发标签：匹配标准日冻结定时触发 OAD (OAD_DAILY_FREEZE)
    if (ev->tag_id == OAD_DAILY_FREEZE) {
        /* slot_id 是稳定身份：地址与影子抓拍必须来自同一只表，不再用 count 当下标界。 */
        uint32_t slot = ev->device_id;
        const production_meter_t* meter = dcu_inventory_by_slot(ctx->inv, slot);
        if (!meter) return STAGE_DROP;
        const char* addr = meter->address;

        // 💡 [2D Matrix Snapshot] 瞬间抓取 DataHub Shadow 缓存全断面电参量快照
        double active_energy = 0.0, voltage = 0.0, current = 0.0;
        pipe_msg_t shadow_msg;

        // 抓拍电能 (Offset 1)
        if (datahub_read_shadow_device(ctx->hub, OAD_ACTIVE_ENERGY, slot, &shadow_msg) == 0) {
            active_energy = shadow_msg.payload.val.f;
        }
        // 抓拍电压 (Offset 0)
        if (datahub_read_shadow_device(ctx->hub, OAD_VOLTAGE, slot, &shadow_msg) == 0) {
            voltage = shadow_msg.payload.val.f;
        }
        // 抓拍电流 (Offset 2)
        if (datahub_read_shadow_device(ctx->hub, OAD_CURRENT, slot, &shadow_msg) == 0) {
            current = shadow_msg.payload.val.f;
        }

        uint64_t freeze_ts = ev->ts;
        if (ctx->sink_ops.archive_freeze) {
            ctx->sink_ops.archive_freeze(ctx->sink_ops.ctx, addr, OAD_DAILY_FREEZE, freeze_ts,
                                         OAD_ACTIVE_ENERGY, active_energy);
            ctx->sink_ops.archive_freeze(ctx->sink_ops.ctx, addr, OAD_DAILY_FREEZE, freeze_ts,
                                         OAD_VOLTAGE, voltage);
            ctx->sink_ops.archive_freeze(ctx->sink_ops.ctx, addr, OAD_DAILY_FREEZE, freeze_ts,
                                         OAD_CURRENT, current);
        }

        return STAGE_DROP;
    }

    return STAGE_OK;
}

static void _freeze_cleanup(void* priv) {
    if (priv) free(priv);
}

pipeline_stage_t* stage_periodic_freeze_create_ex(dcu_inventory_t* inv, datahub_t* hub,
                                                  event_sink_ops_t sink_ops) {
    pipeline_stage_t* s = calloc(1, sizeof(pipeline_stage_t));
    if (!s) return NULL;
    freeze_stage_ctx_t* ctx = calloc(1, sizeof(freeze_stage_ctx_t));
    if (!ctx) {
        free(s);
        return NULL;
    }

    ctx->inv = inv;
    ctx->hub = hub;
    ctx->sink_ops = sink_ops;

    strncpy(s->name, "PER_FREEZE", 31);
    s->process = _freeze_proc;
    s->priv_state = ctx;
    s->cleanup_state = _freeze_cleanup;
    return s;
}
