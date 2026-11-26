#include "dcu_poller.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbiter.h"
#include "dcu_clock_port.h"
#include "edge_log.h"

static uint64_t _wall_now_s(void) {
    struct tm tm;
    if (!dcu_clock_port_system() || dcu_clock_port_system()->now(NULL, &tm) != 0) return 0;
    time_t t = mktime(&tm);
    return t > 0 ? (uint64_t)t : 0;
}

uint64_t dcu_poller_last_task_start_epoch(const dcu_poller_t* poller) {
    return poller ? poller->last_task_start_epoch_s : 0;
}

uint64_t dcu_poller_last_success_epoch(const dcu_poller_t* poller) {
    return poller ? poller->last_success_epoch_s : 0;
}

uint64_t dcu_poller_last_store_epoch(const dcu_poller_t* poller) {
    return poller ? poller->last_store_epoch_s : 0;
}

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

    /* 每通道一条独立的链：游标从 0 开始，在途读固定最多 1（串口半双工无法流水）。
     * 链只为这一份在途读保留身份快照；要让通道流水，需先把快照扩成按并发度分槽。 */
    for (int i = 0; i < DCU_MAX_CHANNELS; i++) {
        poller->chains[i].owner = poller;
        poller->chains[i].channel_id = (uint8_t)i;
        poller->chains[i].cursor = 0;
        poller->chains[i].round = 0;
        poller->chains[i].outstanding = 0;
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

static uint32_t _addr_hash(const char* address) {
    uint32_t h = 5381;
    for (const char* p = address; *p; ++p) {
        h = h * 33u + (uint8_t)*p;
    }
    return h % METER_TABLE_CAP;
}

/* 通道级熔断标志只在变更点重算：比每次取统计都扫全表便宜 */
static void _recompute_channel(dcu_poller_t* poller, uint32_t ch) {
    if (ch >= DCU_MAX_CHANNELS) return;
    poller->channel_stats[ch].is_circuit_broken =
        (poller->ch_tracked[ch] > 0 && poller->ch_broken[ch] >= poller->ch_tracked[ch]);
}

static const dcu_meter_poll_state_t* _meter_state_find(const dcu_poller_t* poller,
                                                      const char* address) {
    uint32_t h = _addr_hash(address);
    for (uint32_t n = 0; n < METER_TABLE_CAP; n++) {
        const dcu_meter_poll_state_t* c = &poller->meter_poll_state[(h + n) % METER_TABLE_CAP];
        if (!c->tracked && !c->tombstone) return NULL; // 真空槽才终止探测
        if (c->tracked && strncmp(c->address, address, sizeof(c->address)) == 0) return c;
    }
    return NULL;
}

static dcu_meter_poll_state_t* _meter_state(dcu_poller_t* poller, const char* address,
                                            uint8_t channel_id) {
    uint32_t h = _addr_hash(address);
    dcu_meter_poll_state_t* reuse = NULL; // 首个墓碑位，可重复利用
    dcu_meter_poll_state_t* mp = NULL;
    for (uint32_t n = 0; n < METER_TABLE_CAP; n++) {
        dcu_meter_poll_state_t* c = &poller->meter_poll_state[(h + n) % METER_TABLE_CAP];
        if (c->tracked) {
            if (strncmp(c->address, address, sizeof(c->address)) == 0) {
                c->gen = poller->state_gen;
                if (c->channel_id != channel_id) { // 表可能被改配到另一个通道：计数跟着走
                    uint32_t old = c->channel_id;
                    if (old < DCU_MAX_CHANNELS) {
                        if (poller->ch_tracked[old] > 0) poller->ch_tracked[old]--;
                        if (c->breaker_state != DCU_BREAKER_CLOSED && poller->ch_broken[old] > 0)
                            poller->ch_broken[old]--;
                        _recompute_channel(poller, old);
                    }
                    c->channel_id = channel_id;
                    if (channel_id < DCU_MAX_CHANNELS) {
                        poller->ch_tracked[channel_id]++;
                        if (c->breaker_state != DCU_BREAKER_CLOSED) poller->ch_broken[channel_id]++;
                        _recompute_channel(poller, channel_id);
                    }
                }
                return c;
            }
            continue; // 活条目：继续探测
        }
        if (c->tombstone) {
            if (!reuse) reuse = c;
            continue; // 墓碑不终止探测链
        }
        mp = c; // 真正的空槽：探测结束
        break;
    }
    if (reuse) mp = reuse; // 优先回收墓碑：否则正常运行时墓碑永不释放，表终将占满
    if (!mp) return NULL; // 表满：无状态可记，fail-open 继续轮询
    memset(mp, 0, sizeof(*mp));
    mp->tracked = true;
    mp->gen = poller->state_gen;
    strncpy(mp->address, address, sizeof(mp->address) - 1);
    mp->channel_id = channel_id;
    if (channel_id < DCU_MAX_CHANNELS) {
        poller->ch_tracked[channel_id]++;
        _recompute_channel(poller, channel_id); // 新增健康表可能解除通道级熔断
    }
    return mp;
}

/* 本任务这一轮是否到期。TI value 0 = 无周期触发；C API 的 interval_s==0
 * 仍表示"每轮"，但要求 has_interval 为真。 */
static bool _task_due(dcu_task_unit_t* task) {
    if (task->status != DCU_TASK_STATUS_NORMAL) return false;
    if (!task->has_interval) return false;
    if (task->interval_s == 0) return true;
    uint64_t ticks = 0;
    return edge_periodic_clock_check(&task->clock, &ticks) > 0;
}

/* 单一资格谓词：ACTIVE + 任务 NORMAL + 该表在执行 begin 时的 target 快照内。
 * load / note_visit / 结算记账 / 完成判定全部用它（F1/A1/A2 的结构性修复）。 */
static bool _execution_accepts(const dcu_poller_t* poller, int t, uint32_t meter_idx) {
    const dcu_task_unit_t* task = &poller->tasks[t];
    const dcu_task_monitor_t* m = &task->monitor;
    if (m->status != DCU_EXEC_ACTIVE) return false;
    if (task->status != DCU_TASK_STATUS_NORMAL) return false;
    return (m->target[meter_idx / 8] & (uint8_t)(1u << (meter_idx % 8))) != 0;
}

/* begin/重开时快照目标表：档案 + task_mask 一经冻结，执行内不再看实时档案。 */
static void _task_snapshot_targets(dcu_poller_t* poller, int t, const dcu_inventory_t* inv) {
    dcu_task_monitor_t* m = &poller->tasks[t].monitor;
    memset(m->target, 0, sizeof(m->target));
    uint32_t total = 0;
    if (!inv) {
        m->total_meters = 0;
        m->settled_left = 0;
        return;
    }
    for (uint32_t i = 0; i < inv->count; i++) {
        const dcu_meter_poll_state_t* mp =
            _meter_state(poller, inv->meters[i].address, inv->meters[i].channel_id);
        if (mp && mp->task_mask_set && !(mp->task_mask & (1u << t))) continue;
        m->target[i / 8] |= (uint8_t)(1u << (i % 8));
        total++;
    }
    m->total_meters = total;
    m->settled_left = total;
}

/* 结算一张目标表：settled ⊆ target，succeeded ⊆ settled；每表每执行一次。 */
static void _task_settle_target(dcu_poller_t* poller, int t, uint32_t meter_idx, int ok) {
    dcu_task_monitor_t* m = &poller->tasks[t].monitor;
    uint32_t byte = meter_idx / 8, bit = meter_idx % 8;
    if (!(m->target[byte] & (uint8_t)(1u << bit))) return;
    if (m->settled[byte] & (uint8_t)(1u << bit)) return;
    m->settled[byte] |= (uint8_t)(1u << bit);
    if (m->settled_left > 0) m->settled_left--;
    if (ok && !(m->succeeded[byte] & (uint8_t)(1u << bit))) {
        m->succeeded[byte] |= (uint8_t)(1u << bit);
        m->success_meters++;
    }
}

static bool _task_has_pending(const dcu_poller_t* poller, int t) {
    for (int ch = 0; ch < DCU_MAX_CHANNELS; ch++) {
        const dcu_chain_t* c = &poller->chains[ch];
        for (uint8_t k = c->pending_i; k < c->pending_n; k++) {
            if (c->pending_masks[k] & (1u << t)) return true;
        }
    }
    return false;
}

static void _task_maybe_complete(dcu_poller_t* poller, int t) {
    dcu_task_monitor_t* m = &poller->tasks[t].monitor;
    if (m->status != DCU_EXEC_ACTIVE) return;
    if (_task_has_pending(poller, t)) return;
    /* 完成锚定在“目标表全部结算 + 无本任务在途读”，不是 visited/outstanding 的瞬时组合；
     * 同表重叠任务因此不会在共享访问结算前提前完成（A1）。 */
    if (m->settled_left == 0 && m->outstanding == 0) {
        m->status = DCU_EXEC_DONE;
        m->end_epoch_s = _wall_now_s();
    }
}

static void _task_begin(dcu_poller_t* poller, int t, dcu_inventory_t* inv) {
    dcu_task_monitor_t* m = &poller->tasks[t].monitor;
    memset(m, 0, sizeof(*m));
    m->task_id = poller->tasks[t].task_id;
    m->status = DCU_EXEC_ACTIVE;
    m->exec_gen = ++poller->exec_seq;
    m->start_epoch_s = _wall_now_s();
    _task_snapshot_targets(poller, t, inv);
}

/* 重开 ACTIVE 执行：重新快照、作废本轮（visited/settled/succeeded 清零，
 * 在途回调因 exec_gen 落后不结算新执行）。链上 pending 不在这里清：只重开
 * 单个任务时，链的 served_ident 按身份清 served，旧批次的失败/结算由代际挡掉。 */
static void _task_reopen(dcu_poller_t* poller, int t, const dcu_inventory_t* inv) {
    dcu_task_monitor_t* m = &poller->tasks[t].monitor;
    if (m->status != DCU_EXEC_ACTIVE) return;
    memset(m->visited, 0, sizeof(m->visited));
    memset(m->settled, 0, sizeof(m->settled));
    memset(m->succeeded, 0, sizeof(m->succeeded));
    m->success_meters = 0;
    m->sent_packets = 0;
    m->recv_packets = 0;
    m->exec_gen = ++poller->exec_seq;
    m->start_epoch_s = _wall_now_s();
    m->end_epoch_s = 0;
    _task_snapshot_targets(poller, t, inv);
}

static void _poller_reopen_active(dcu_poller_t* poller, const dcu_inventory_t* inv) {
    for (int t = 0; t < poller->task_count; t++) _task_reopen(poller, t, inv);
    for (int ch = 0; ch < DCU_MAX_CHANNELS; ch++) {
        dcu_chain_t* c = &poller->chains[ch];
        /* 作废本轮访问：已装载未发出的项丢弃；在途回调仍按自己的读快照落库，
         * 但 exec_seq 与 0 不符，不再计入任何一次新访问。 */
        c->pending_n = 0;
        c->pending_i = 0;
        c->visit_exec_seq = 0;
        c->visit_ok = 0;
        c->visit_fail = 0;
        c->visit_address[0] = '\0';
        memset(c->served, 0, sizeof(c->served));
        memset(c->served_ident, 0, sizeof(c->served_ident));
        memset(c->fail_ident, 0, sizeof(c->fail_ident));
    }
}

static const dcu_scheme_t* _scheme_find(const dcu_poller_t* poller, uint32_t scheme_id) {
    for (int i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id == scheme_id) return &poller->schemes[i];
    }
    return NULL;
}

static dcu_exec_ident_t _task_exec_ident(const dcu_poller_t* poller,
                                         const dcu_task_unit_t* task) {
    const dcu_scheme_t* s = _scheme_find(poller, task->scheme_id);
    dcu_exec_ident_t id = {task->monitor.exec_gen, task->scheme_id, s ? s->content_gen : 0,
                           task->item_mask};
    return id;
}

static bool _exec_ident_same(dcu_exec_ident_t a, dcu_exec_ident_t b) {
    return a.exec_gen == b.exec_gen && a.scheme_id == b.scheme_id &&
           a.scheme_gen == b.scheme_gen && a.item_mask == b.item_mask;
}

/* 一次失败归到当前访问里、代际不新于装载点的任务。任务已重开（代际更新）时，
 * 这次失败属于旧执行，不得污染同表的新执行。 */
static void _chain_note_visit_fail(dcu_poller_t* poller, dcu_chain_t* c) {
    for (int t = 0; t < poller->task_count; t++) {
        const dcu_task_monitor_t* m = &poller->tasks[t].monitor;
        if (m->status != DCU_EXEC_ACTIVE) continue;
        if (m->exec_gen > c->visit_exec_seq) continue;
        c->fail_ident[t] = _task_exec_ident(poller, &poller->tasks[t]);
    }
}

static bool _task_visited(const dcu_poller_t* poller, int t, uint32_t meter_idx) {
    const dcu_task_monitor_t* m = &poller->tasks[t].monitor;
    return (m->visited[meter_idx / 8] & (uint8_t)(1u << (meter_idx % 8))) != 0;
}

static void _task_mark_visited(dcu_poller_t* poller, int t, uint32_t meter_idx) {
    dcu_task_monitor_t* m = &poller->tasks[t].monitor;
    m->visited[meter_idx / 8] |= (uint8_t)(1u << (meter_idx % 8));
}

/* 熔断跳过一块表：本执行不再访问它。合格但未装载完的任务按失败结算并标 visited，
 * 否则 target 永远等不到这次访问、执行无法收尾。 */
static void _chain_fail_unserved(dcu_poller_t* poller, uint32_t meter_idx) {
    for (int t = 0; t < poller->task_count; t++) {
        if (!_execution_accepts(poller, t, meter_idx)) continue;
        if (_task_visited(poller, t, meter_idx)) continue;
        _task_mark_visited(poller, t, meter_idx);
        _task_settle_target(poller, t, meter_idx, 0);
        _task_maybe_complete(poller, t);
    }
}

static void _mask_task_packet(dcu_poller_t* poller, uint32_t mask, int tx) {
    for (int t = 0; t < poller->task_count; t++) {
        if (!(mask & (1u << t))) continue;
        dcu_task_monitor_t* m = &poller->tasks[t].monitor;
        if (m->status != DCU_EXEC_ACTIVE) continue;
        if (tx) {
            if (m->sent_packets < UINT32_MAX) m->sent_packets++;
        } else if (m->recv_packets < UINT32_MAX) {
            m->recv_packets++;
        }
    }
}

void dcu_poller_note_tx(dcu_poller_t* poller, uint32_t task_mask) {
    if (poller) _mask_task_packet(poller, task_mask, 1);
}

void dcu_poller_note_rx(dcu_poller_t* poller, uint32_t task_mask) {
    if (poller) _mask_task_packet(poller, task_mask, 0);
}

/* 表级熔断状态机 CLOSED/OPEN/HALF_OPEN；迁移集中在本组函数。
 * 生产理由：瞬时总线故障不应让一块表永久失联（旧行为只开不合、无复位入口）。 */
static uint64_t _mono_ms(void) { return uv_hrtime() / 1000000ull; }

static bool _breaker_may_visit(dcu_meter_poll_state_t* mp) {
    if (!mp || mp->breaker_state == DCU_BREAKER_CLOSED) return true;
    if (_mono_ms() - mp->breaker_since_ms < DCU_BREAKER_COOLDOWN_MS) return false;
    /* OPEN 冷却到期 → HALF_OPEN，放行一次探测；HALF_OPEN 卡住同样再放行一次并重新计时。 */
    mp->breaker_state = DCU_BREAKER_HALF_OPEN;
    mp->breaker_since_ms = _mono_ms();
    return true;
}

static void _breaker_note_result(dcu_poller_t* poller, dcu_meter_poll_state_t* mp, int ok,
                                 const char* address, uint8_t channel_id) {
    if (!mp) return;
    if (ok) {
        mp->consecutive_fails = 0;
        if (mp->breaker_state != DCU_BREAKER_CLOSED) {
            mp->breaker_state = DCU_BREAKER_CLOSED;
            mp->breaker_since_ms = 0;
            if (poller->ch_broken[channel_id] > 0) poller->ch_broken[channel_id]--;
            _recompute_channel(poller, channel_id);
        }
        return;
    }
    if (mp->consecutive_fails < UINT32_MAX) mp->consecutive_fails++;
    edge_poll_policy_t* pol = &poller->channel_policies[channel_id];
    bool threshold = pol->circuit_break_threshold > 0 &&
                     mp->consecutive_fails >= pol->circuit_break_threshold;
    if (mp->breaker_state == DCU_BREAKER_HALF_OPEN ||
        (threshold && mp->breaker_state == DCU_BREAKER_CLOSED)) {
        bool was_closed = mp->breaker_state == DCU_BREAKER_CLOSED;
        mp->breaker_state = DCU_BREAKER_OPEN;
        mp->breaker_since_ms = _mono_ms();
        if (was_closed) poller->ch_broken[channel_id]++;
        _recompute_channel(poller, channel_id);
        LOG_ERROR(LOG_CAT_IO,
                  "🚨 [DCU-Poller] Meter %s (ch %u) circuit breaker OPEN (fails: %u >= "
                  "threshold: %u)",
                  address, channel_id, (unsigned)mp->consecutive_fails,
                  pol->circuit_break_threshold);
    } else if (mp->breaker_state == DCU_BREAKER_OPEN) {
        mp->breaker_since_ms = _mono_ms(); /* 探测失败重新计时 */
    }
}

static void _chain_load_pending(dcu_poller_t* poller, const dcu_inventory_t* inv, dcu_chain_t* c) {
    c->pending_n = 0;
    c->pending_i = 0;
    /* visit_ok/visit_fail 跨本表所有批次累计；_chain_next_meter 在新表访问开始时清零。 */
    c->visit_exec_seq = poller->exec_seq;

    /* 访问身份在装载时冻结：档案随后可能删除/重排，回调不得再按下标反查。 */
    c->visit_address[0] = '\0';
    c->read.address[0] = '\0';
    c->read.device_id = 0;
    c->read.exec_seq = c->visit_exec_seq;
    if (inv && c->meter_idx < inv->count) {
        const production_meter_t* m = &inv->meters[c->meter_idx];
        strncpy(c->visit_address, m->address, sizeof(c->visit_address) - 1);
        c->visit_address[sizeof(c->visit_address) - 1] = '\0';
        strncpy(c->read.address, m->address, sizeof(c->read.address) - 1);
        c->read.address[sizeof(c->read.address) - 1] = '\0';
        c->read.device_id = m->slot_id;
    }

    for (int t = 0; t < poller->task_count; t++) {
        dcu_task_unit_t* task = &poller->tasks[t];
        if (!_execution_accepts(poller, t, c->meter_idx)) continue;
        if (_task_visited(poller, t, c->meter_idx)) continue;

        dcu_exec_ident_t id = _task_exec_ident(poller, task);
        if (!_exec_ident_same(c->served_ident[t], id)) {
            c->served[t] = 0;
            c->served_ident[t] = id;
        }

        const dcu_scheme_t* s = _scheme_find(poller, task->scheme_id);
        if (!s) {
            /* 方案缺失按"无可抄项"结算，否则 target 永远等不到这次访问 */
            _task_mark_visited(poller, t, c->meter_idx);
            _task_settle_target(poller, t, c->meter_idx, 0);
            _task_maybe_complete(poller, t);
            continue;
        }
        for (int j = 0; j < s->data.normal.csd_count; j++) {
            if (task->item_mask && !(task->item_mask & (1u << j))) continue;
            if (c->served[t] & (1u << j)) continue;
            uint32_t tag = s->data.normal.csd_list[j];
            bool queued = false;
            for (uint8_t k = 0; k < c->pending_n; k++) {
                if (c->pending_tags[k] == tag) {
                    queued = true;
                    break;
                }
            }
            if (queued || c->pending_n >= MAX_SCHEME_OADS) continue;
            c->pending_tags[c->pending_n] = tag;
            c->pending_masks[c->pending_n] = 0;
            c->pending_n++;
        }

        int selected = 0;
        bool all_served = true;
        for (int j = 0; j < s->data.normal.csd_count; j++) {
            if (task->item_mask && !(task->item_mask & (1u << j))) continue;
            selected++;
            if (c->served[t] & (1u << j)) continue;
            uint32_t tag = s->data.normal.csd_list[j];
            int k = -1;
            for (uint8_t q = 0; q < c->pending_n; q++) {
                if (c->pending_tags[q] == tag) {
                    k = (int)q;
                    break;
                }
            }
            if (k < 0) {
                all_served = false;
                continue;
            }
            c->pending_masks[k] |= (uint32_t)(1u << t);
            c->served[t] |= (uint32_t)(1u << j);
        }

        if (selected == 0) {
            _task_mark_visited(poller, t, c->meter_idx);
            _task_settle_target(poller, t, c->meter_idx, 0);
            _task_maybe_complete(poller, t);
        } else if (all_served) {
            _task_mark_visited(poller, t, c->meter_idx);
        }
    }
}

static bool _meter_has_unserved(const dcu_poller_t* poller, uint32_t meter_idx) {
    for (int t = 0; t < poller->task_count; t++) {
        if (!_execution_accepts(poller, t, meter_idx)) continue;
        if (_task_visited(poller, t, meter_idx)) continue;
        return true;
    }
    return false;
}

static bool _chain_begin_visit(dcu_poller_t* poller, dcu_inventory_t* inv, dcu_chain_t* c) {
    if (!_meter_has_unserved(poller, c->meter_idx)) return false;
    dcu_meter_poll_state_t* mp =
        _meter_state(poller, inv->meters[c->meter_idx].address, c->channel_id);
    if (!_breaker_may_visit(mp)) {
        _chain_fail_unserved(poller, c->meter_idx);
        return false;
    }
    _chain_load_pending(poller, inv, c);
    return c->pending_n > 0;
}

/* 游标推进到下一块"属于本通道且有事可做"的表。
 *
 * 一圈 = 一步：游标绕回 0 就结束本次推进，否则同步回调下会一圈接一圈永不停。
 * 跨回调的续扫靠 scanned_in_round 记着本圈已经看过多少块。 */
static bool _chain_next_meter(dcu_poller_t* poller, dcu_inventory_t* inv, dcu_chain_t* c) {
    if (c->round_ended) return false;   /* 本步的一圈已扫完 */
    while (c->scanned_in_round < inv->count) {
        uint32_t idx = c->cursor;
        c->cursor = (uint16_t)((c->cursor + 1) % inv->count);
        c->scanned_in_round++;

        bool wrapped = (c->cursor == 0);
        if (wrapped) {
            c->round++;            /* 扫完一圈：窗口翻页 */
            c->scanned_in_round = 0;
            c->round_ended = true; /* 这一块表处理完就结束本步 */
        }

        production_meter_t* m = &inv->meters[idx];
        if (m->channel_id == c->channel_id) {
            c->meter_idx = idx;
            c->visit_ok = 0;
            c->visit_fail = 0;
            memset(c->served, 0, sizeof(c->served));
            memset(c->served_ident, 0, sizeof(c->served_ident));
            memset(c->fail_ident, 0, sizeof(c->fail_ident));
            if (_chain_begin_visit(poller, inv, c)) return true;
        }
        if (wrapped) return false;  /* 一圈扫完，本步到此为止（未找到活） */
    }
    c->scanned_in_round = 0;
    return false;
}

/* 整表全部分批结束后结算一次熔断与通道统计（与任务结算同一时点）。
 * 口径：任一批失败即整表失败；整表无失败且至少一次成功才记访问成功。 */
static void _chain_finish_visit(dcu_poller_t* poller, dcu_chain_t* c) {
    if (c->visit_address[0] == '\0') return;

    dcu_meter_poll_state_t* mp = _meter_state(poller, c->visit_address, c->channel_id);
    edge_poll_stats_t* st = &poller->channel_stats[c->channel_id];
    edge_poll_policy_t* pol = &poller->channel_policies[c->channel_id];

    (void)pol;
    if (c->visit_fail > 0) {
        st->consecutive_fails++;
        _breaker_note_result(poller, mp, 0, c->visit_address, c->channel_id);
    } else if (c->visit_ok > 0) {
        st->success_count++;
        st->consecutive_fails = 0;
        st->last_success_ts = uv_hrtime() / 1000000ULL;
        poller->last_success_epoch_s = _wall_now_s();
        _breaker_note_result(poller, mp, 1, c->visit_address, c->channel_id);
    }
}

/* 整表分批结束后再结算，保持旧口径：该表全部批次的 tag 都成功才记成功。
 * 装载点代际早于任务当前执行代际的访问不结算该任务；失败记录按执行身份比对，
 * 旧执行的失败不串入。 */
static void _chain_settle_visited(dcu_poller_t* poller, dcu_chain_t* c) {
    for (int t = 0; t < poller->task_count; t++) {
        if (!_execution_accepts(poller, t, c->meter_idx)) continue;
        dcu_task_monitor_t* mon = &poller->tasks[t].monitor;
        if (mon->exec_gen > c->visit_exec_seq) continue;
        if (!_task_visited(poller, t, c->meter_idx)) continue;
        bool ok = !_exec_ident_same(c->fail_ident[t], _task_exec_ident(poller, &poller->tasks[t]));
        _task_settle_target(poller, t, c->meter_idx, ok);
        _task_maybe_complete(poller, t);
    }
}

/* 读完成：写值、扇出、推进链 */
static void _on_read_done(void* ud, int status, double value) {
    dcu_chain_t* c = (dcu_chain_t*)ud;
    dcu_poller_t* poller = (dcu_poller_t*)c->owner;
    if (!poller) return;

    if (c->outstanding > 0) c->outstanding--;

    /* 回调只认快照：address/slot 是装载时冻结，tag/mask/执行代际是提交时写入，
     * 都可能与当前档案不同。档案已重开（visit_exec_seq==0 或代际翻转）时
     * 仍落库到原表，但不计入新执行。 */
    const dcu_read_ctx_t* r = &c->read;
    bool current_visit = (c->visit_exec_seq != 0 && r->exec_seq == c->visit_exec_seq);
    if (status == 0) {
        if (current_visit) c->visit_ok++;
        if (poller->sink_ops.record_telemetry &&
            poller->sink_ops.record_telemetry(poller->sink_ops.ctx, r->address, r->device_id,
                                              r->tag_id, value) == 0) {
            poller->last_store_epoch_s = _wall_now_s();
        }
        if (poller->bus_ops.publish_realtime) {
            poller->bus_ops.publish_realtime(poller->bus_ops.ctx, r->tag_id, r->device_id, value,
                                             uv_hrtime());
        }
    } else {
        if (current_visit) {
            c->visit_fail++;
            _chain_note_visit_fail(poller, c);
        }
        poller->channel_stats[c->channel_id].error_count++;
    }

    for (int t = 0; t < poller->task_count; t++) {
        dcu_task_monitor_t* m = &poller->tasks[t].monitor;
        if (!(r->task_mask & (1u << t)) || m->status != DCU_EXEC_ACTIVE) continue;
        if (m->outstanding > 0) m->outstanding--;
    }
    /* 完成判据由 pump 的 finish_visit / 步末统一评估：不能在还有 pending 项时结算。 */
    /* 链式：一项回来，立刻推进下一项。submit 保证异步回调，递归深度恒为 1。 */
    extern void _dcu_chain_pump(dcu_poller_t * poller, dcu_chain_t * c);
    _dcu_chain_pump(poller, c);
}

/* 推进一条链：要么提交下一项，要么换下一块表，要么无事可做 */

/* File-local prototypes. */
void _dcu_chain_pump(dcu_poller_t* poller, dcu_chain_t* c);
void _dcu_chain_pump(dcu_poller_t* poller, dcu_chain_t* c) {
    dcu_inventory_t* inv = poller->inv;
    /* 一次访问属于一个档案 revision。revision 已变而 step 尚未重开时，
     * 链不得再提交本轮任何项：下标可能已移位，剩余项会读到顶替表。 */
    if (inv && inv->revision != poller->last_inv_rev) return;
    if (!inv || inv->count == 0) return;
    if (c->pumping) return;   /* 同步回调重入：交给外层循环继续 */
    c->pumping = true;

    for (;;) {
        if (c->outstanding >= 1) break;   /* 在途读固定最多 1，等回调 */

        if (c->pending_i < c->pending_n) {
            uint32_t tag = c->pending_tags[c->pending_i];
            uint32_t mask = c->pending_masks[c->pending_i];
            c->pending_i++;
            c->read.tag_id = tag;
            c->read.task_mask = mask;
            c->read.exec_seq = c->visit_exec_seq;

            edge_poll_stats_t* st = &poller->channel_stats[c->channel_id];
            st->poll_count++;
            st->last_poll_ts = uv_hrtime() / 1000000ULL;

            c->outstanding++;
            for (int t = 0; t < poller->task_count; t++) {
                dcu_task_monitor_t* m = &poller->tasks[t].monitor;
                if ((mask & (1u << t)) && m->status == 1 && m->outstanding < UINT32_MAX) {
                    m->outstanding++;
                }
            }
            int rc = poller->source_ops.submit
                         ? poller->source_ops.submit(poller->source_ops.ctx, c->meter_idx, tag,
                                                     mask, _on_read_done, c)
                         : -1;
            if (rc != 0) {
                /* 提交失败不会再有回调：当场结算，避免链卡死 */
                c->outstanding--;
                for (int t = 0; t < poller->task_count; t++) {
                    dcu_task_monitor_t* m = &poller->tasks[t].monitor;
                    if (!(mask & (1u << t)) || m->status != DCU_EXEC_ACTIVE) continue;
                    if (m->outstanding > 0) m->outstanding--;
                    _task_maybe_complete(poller, t);
                }
                c->visit_fail++;
                _chain_note_visit_fail(poller, c);
                poller->channel_stats[c->channel_id].error_count++;
                continue;
            }
            /* 异步：等回调。同步回调（mock/立即返回）时 outstanding 已归零，
             * 上面那道 pumping 守卫让重入直接返回，于是这里继续循环即可。 */
            if (c->outstanding > 0) break;
            continue;
        }

        if (c->pending_n > 0) {
            c->pending_n = 0;
            c->pending_i = 0;
            if (_chain_begin_visit(poller, inv, c)) continue;
            _chain_finish_visit(poller, c);
            _chain_settle_visited(poller, c);
        }
        if (!_chain_next_meter(poller, inv, c)) break;
    }

    c->pumping = false;
}

int dcu_poller_step(dcu_poller_t* poller, dcu_inventory_t* inv, uint32_t max_meters_per_step) {
    if (!poller || !inv) {
        return 0;
    }
    /* 并发由每通道 depth 控制；max_meters_per_step 不再按表数截断，
     * 否则会在通道之间制造不公平的饥饿。保留形参仅为兼容既有调用。 */
    (void)max_meters_per_step;
    poller->inv = inv;

    // 档案版本变了才做一次退休扫描：已删表的条目必须释放，否则 MAX_METERS 最终被死条目占满。
    // 必须早于 count==0 返回，否则最后一只表被删时不会退休。
    if (poller->last_inv_rev != inv->revision) {
        poller->state_gen++;
        for (uint32_t i = 0; i < inv->count; i++) {
            uint32_t ch = inv->meters[i].channel_id < DCU_MAX_CHANNELS ? inv->meters[i].channel_id : 0;
            (void)_meter_state(poller, inv->meters[i].address, (uint8_t)ch);
        }
        for (uint32_t i = 0; i < METER_TABLE_CAP; i++) {
            dcu_meter_poll_state_t* mp = &poller->meter_poll_state[i];
            if (mp->tracked && mp->gen != poller->state_gen) {
                uint32_t ch = mp->channel_id;
                if (ch < DCU_MAX_CHANNELS) {
                    if (poller->ch_tracked[ch] > 0) poller->ch_tracked[ch]--;
                    if (mp->breaker_state != DCU_BREAKER_CLOSED && poller->ch_broken[ch] > 0)
                        poller->ch_broken[ch]--;
                    _recompute_channel(poller, ch);
                }
                memset(mp, 0, sizeof(*mp));
                mp->tombstone = true;
            }
        }
        poller->last_inv_rev = inv->revision;
        /* 档案变了：ACTIVE 执行重开（重快照+作废本轮），在途回调不结算新执行。 */
        _poller_reopen_active(poller, inv);
        for (int ch = 0; ch < DCU_MAX_CHANNELS; ch++) {
            if (poller->chains[ch].cursor >= inv->count) poller->chains[ch].cursor = 0;
        }
    }

    if (inv->count == 0) {
        return 0;
    }

    /* 每步只问一次"谁到期"：时钟检查会推进 tick，问多次会把后面的任务吃掉 */
    for (int t = 0; t < poller->task_count; t++) {
        poller->task_due[t] = _task_due(&poller->tasks[t]);
        if (poller->task_due[t]) {
            poller->tasks[t].last_run_time = edge_periodic_clock_now_us(&poller->tasks[t].clock) / 1000000ULL;
            poller->last_task_start_epoch_s = _wall_now_s();
            if (poller->tasks[t].monitor.status != DCU_EXEC_ACTIVE) _task_begin(poller, t, inv);
        } else if (poller->tasks[t].status != DCU_TASK_STATUS_NORMAL &&
                   poller->tasks[t].monitor.status == DCU_EXEC_ACTIVE) {
            memset(&poller->tasks[t].monitor, 0, sizeof(poller->tasks[t].monitor));
        }
    }

    /* 通道之间独立推进：一条通道卡住不影响其它通道。
     * 每步只扫一圈：round_ended 在本步开始时清零，扫到游标归零即收工，
     * 否则同步回调下会一圈接一圈永不返回。 */
    for (int ch = 0; ch < DCU_MAX_CHANNELS; ch++) {
        poller->chains[ch].round_ended = false;
        _dcu_chain_pump(poller, &poller->chains[ch]);
    }

    // 批量落盘
    if (poller->sink_ops.commit_batch) {
        poller->sink_ops.commit_batch(poller->sink_ops.ctx);
    }
    /* 没有待抄项的表（熔断/方案为空）也要让任务在本步结束时能结算。 */
    for (int t = 0; t < poller->task_count; t++) _task_maybe_complete(poller, t);
    return 0;
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

bool dcu_poller_meter_breaker_open(const dcu_poller_t* poller, const char* address) {
    if (!poller || !address) return false;
    const dcu_meter_poll_state_t* mp = _meter_state_find(poller, address);
    return mp ? (mp->breaker_state != DCU_BREAKER_CLOSED) : false;
}

int dcu_poller_get_channel_stats(dcu_poller_t* poller, uint32_t channel_id,
                                 edge_poll_stats_t* out_stats) {
    if (!poller || channel_id >= DCU_MAX_CHANNELS || !out_stats) return -1;
    /* is_circuit_broken 由表级状态变更点（熔断/重置/退休）维护，这里只读不写 */
    *out_stats = poller->channel_stats[channel_id];
    return 0;
}

int dcu_poller_reset_circuit_breaker(dcu_poller_t* poller, uint32_t channel_id) {
    if (!poller || channel_id >= DCU_MAX_CHANNELS) return -1;
    poller->ch_broken[channel_id] = 0;
    poller->channel_stats[channel_id].is_circuit_broken = false;
    poller->channel_stats[channel_id].consecutive_fails = 0;
    poller->channel_stats[channel_id].in_flight = 0;
    poller->channel_stats[channel_id].queue_depth = 0;

    // 表级熔断按表复位：只解这条通道上被熔断的表
    for (uint32_t i = 0; i < METER_TABLE_CAP; i++) {
        dcu_meter_poll_state_t* mp = &poller->meter_poll_state[i];
        if (mp->tracked && mp->channel_id == (uint8_t)channel_id) {
            mp->breaker_state = DCU_BREAKER_CLOSED;
            mp->breaker_since_ms = 0;
            mp->consecutive_fails = 0;
        }
    }
    return 0;
}

int dcu_poller_add_scheme(dcu_poller_t* poller, uint32_t scheme_id, uint8_t mode,
                          const uint32_t* csds, uint16_t csd_count) {
    if (!poller) return -1;
    if (csd_count > 0 && !csds) return -1;
    /* schemes[] 与 csd_list[] 都是定长数组，越界会直接写穿相邻的 tasks[]。
     * 方案来自数据库/北向配置，不可信，必须在这里挡住——add_task 查了
     * MAX_TASKS 而这里不查，是同一个聚合里两套规矩。 */
    if (poller->scheme_count >= MAX_SCHEMES) return -1;
    if (csd_count > MAX_SCHEME_OADS) return -1;

    dcu_scheme_t* s = &poller->schemes[poller->scheme_count++];
    memset(s, 0, sizeof(*s));
    s->scheme_id = scheme_id;
    s->content_gen = ++poller->scheme_gen_seq;
    s->type = SCHEME_TYPE_NORMAL;
    s->data.normal.capture_mode = mode;
    s->data.normal.csd_count = csd_count;
    for (uint16_t i = 0; i < csd_count; i++) s->data.normal.csd_list[i] = csds[i];
    return 0;
}

int dcu_task_interval_seconds(uint8_t unit, uint16_t value, uint32_t* out_seconds) {
    if (!out_seconds) return -1;
    uint64_t seconds;
    switch (unit) {
        case 0:
            seconds = value;
            break;
        case 1:
            seconds = (uint64_t)value * 60ULL;
            break;
        case 2:
            seconds = (uint64_t)value * 3600ULL;
            break;
        case 3:
            seconds = (uint64_t)value * 86400ULL;
            break;
        default:
            return -1; /* 月/年没有固定秒数 */
    }
    if (seconds > UINT32_MAX) return -1;
    *out_seconds = (uint32_t)seconds;
    return 0;
}

/* 旧 C API 只有秒：反推一个 A2 能精确读回的 TI（不影响调度）。 */
static void _best_effort_ti(uint32_t interval_s, uint8_t* unit, uint16_t* value) {
    if (interval_s <= UINT16_MAX) {
        *unit = 0;
        *value = (uint16_t)interval_s;
    } else if (interval_s % 60 == 0 && interval_s / 60 <= UINT16_MAX) {
        *unit = 1;
        *value = (uint16_t)(interval_s / 60);
    } else if (interval_s % 3600 == 0 && interval_s / 3600 <= UINT16_MAX) {
        *unit = 2;
        *value = (uint16_t)(interval_s / 3600);
    } else {
        *unit = 3;
        uint64_t days = interval_s / 86400ULL;
        *value = days <= UINT16_MAX ? (uint16_t)days : 0;
    }
}

static void _task_clock_init(dcu_task_unit_t* task) {
    edge_periodic_clock_config_t cfg = {
        .interval_us = (task->interval_s > 0) ? ((uint64_t)task->interval_s * 1000000ULL) : 1ULL,
        .policy = EDGE_OVERRUN_SKIP,
        .wall_clock_align = (task->interval_s > 0),
        .custom_clock = NULL,
        .clock_ctx = NULL,
    };
    edge_periodic_clock_init(&task->clock, &cfg);
    task->clock.next_tick_us = 0; /* 首次允许立即执行 */
}

int dcu_poller_add_task_ex(dcu_poller_t* poller, uint32_t task_id, uint32_t interval_s,
                           uint32_t scheme_id, uint32_t item_mask) {
    if (!poller || poller->task_count >= MAX_TASKS) return -1;
    dcu_task_unit_t* t = &poller->tasks[poller->task_count++];
    memset(t, 0, sizeof(*t));
    t->task_id = task_id;
    t->interval_s = interval_s;
    t->has_interval = true;
    t->status = DCU_TASK_STATUS_NORMAL;
    t->scheme_id = scheme_id;
    t->item_mask = item_mask;
    t->last_run_time = 0;
    t->cursor_meter_idx = 0;
    _best_effort_ti(interval_s, &t->interval_unit, &t->interval_value);

    _task_clock_init(t);
    return 0;
}

int dcu_poller_add_task(dcu_poller_t* poller, uint32_t task_id, uint32_t interval_s,
                        uint32_t scheme_id) {
    /* 旧签名没有项维度：0 在调度里解释为"全部 CSD" */
    return dcu_poller_add_task_ex(poller, task_id, interval_s, scheme_id, 0);
}

const dcu_task_unit_t* dcu_poller_task_find(const dcu_poller_t* poller, uint32_t task_id) {
    if (!poller) return NULL;
    for (int i = 0; i < poller->task_count; i++) {
        if (poller->tasks[i].status != DCU_TASK_STATUS_FREE &&
            poller->tasks[i].task_id == task_id) {
            return &poller->tasks[i];
        }
    }
    return NULL;
}

const dcu_task_unit_t* dcu_poller_task_at(const dcu_poller_t* poller, uint16_t slot) {
    if (!poller || slot >= poller->task_count) return NULL;
    return &poller->tasks[slot];
}

int dcu_poller_upsert_task(dcu_poller_t* poller, const dcu_task_unit_t* task) {
    if (!poller || !task || task->task_id == 0) return -1;

    int slot = -1;
    for (int i = 0; i < poller->task_count; i++) {
        if (poller->tasks[i].status != DCU_TASK_STATUS_FREE &&
            poller->tasks[i].task_id == task->task_id) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        for (int i = 0; i < poller->task_count; i++) {
            if (poller->tasks[i].status == DCU_TASK_STATUS_FREE) {
                slot = i;
                break;
            }
        }
        if (slot < 0) {
            if (poller->task_count >= MAX_TASKS) return -1;
            slot = poller->task_count++;
        }
        memset(&poller->tasks[slot], 0, sizeof(poller->tasks[slot]));
        poller->tasks[slot].task_id = task->task_id;
        poller->tasks[slot].interval_s = task->interval_s;
        poller->tasks[slot].interval_unit = task->interval_unit;
        poller->tasks[slot].interval_value = task->interval_value;
        poller->tasks[slot].has_interval = task->has_interval;
        poller->tasks[slot].status = task->status;
        poller->tasks[slot].period_type = task->period_type;
        poller->tasks[slot].scheme_type = task->scheme_type;
        poller->tasks[slot].scheme_id = task->scheme_id;
        poller->tasks[slot].item_mask = task->item_mask;
        _task_clock_init(&poller->tasks[slot]);
        return 0;
    }

    dcu_task_unit_t* cur = &poller->tasks[slot];
    bool interval_changed = cur->has_interval != task->has_interval ||
                            cur->interval_s != task->interval_s ||
                            cur->interval_unit != task->interval_unit ||
                            cur->interval_value != task->interval_value;
    bool config_changed = cur->scheme_id != task->scheme_id || cur->item_mask != task->item_mask;
    bool resumed = cur->status == DCU_TASK_STATUS_DISABLED &&
                   task->status == DCU_TASK_STATUS_NORMAL;

    cur->interval_s = task->interval_s;
    cur->interval_unit = task->interval_unit;
    cur->interval_value = task->interval_value;
    cur->has_interval = task->has_interval;
    cur->status = task->status;
    cur->period_type = task->period_type;
    cur->scheme_type = task->scheme_type;
    cur->scheme_id = task->scheme_id;
    cur->item_mask = task->item_mask;
    /* cursor/last_run 是运行态，配置变化不重置。upsert 是完整任务单元替换，
     * item_mask 一并同步（6012 当前没有该字段，恒传 0，会清掉既有 mask）。
     * 执行身份变化只重开被改任务的 ACTIVE 执行：其它任务与在途访问不受影响，
     * 旧访问因代际落后不结算新执行。 */
    if (interval_changed || resumed) _task_clock_init(cur);
    if (config_changed) _task_reopen(poller, slot, poller->inv);
    return 0;
}

int dcu_poller_remove_task(dcu_poller_t* poller, uint32_t task_id) {
    if (!poller) return -1;
    for (int i = 0; i < poller->task_count; i++) {
        if (poller->tasks[i].status != DCU_TASK_STATUS_FREE &&
            poller->tasks[i].task_id == task_id) {
            /* 不压缩：task_mask 的位是注册槽，压缩会把表悄悄改指别的任务。 */
            memset(&poller->tasks[i], 0, sizeof(poller->tasks[i]));
            return 0;
        }
    }
    return -1;
}

int dcu_poller_clear_tasks(dcu_poller_t* poller) {
    if (!poller) return -1;
    memset(poller->tasks, 0, sizeof(poller->tasks));
    poller->task_count = 0;
    return 0;
}

int dcu_poller_set_task_status(dcu_poller_t* poller, uint32_t task_id, uint8_t status) {
    if (!poller || (status != DCU_TASK_STATUS_NORMAL && status != DCU_TASK_STATUS_DISABLED)) {
        return -1;
    }
    for (int i = 0; i < poller->task_count; i++) {
        dcu_task_unit_t* t = &poller->tasks[i];
        if (t->status == DCU_TASK_STATUS_FREE || t->task_id != task_id) continue;
        if (t->status == DCU_TASK_STATUS_DISABLED && status == DCU_TASK_STATUS_NORMAL) {
            _task_clock_init(t); /* 恢复时从新周期开始，不补发停用期间的拍 */
        }
        t->status = status;
        if (status == DCU_TASK_STATUS_DISABLED) {
            memset(&t->monitor, 0, sizeof(t->monitor)); /* ACTIVE → IDLE */
        }
        return 0;
    }
    return -1;
}

int dcu_poller_set_meter_tasks(dcu_poller_t* poller, const char* address, uint8_t channel_id,
                               uint16_t task_mask) {
    if (!poller || !address || !address[0]) return -1;
    if (channel_id >= DCU_MAX_CHANNELS) return -1;
    dcu_meter_poll_state_t* mp = _meter_state(poller, address, channel_id);
    if (!mp) return -1; // 状态表满：fail-open，该表仍按旧行为被全表抄读
    mp->task_mask = task_mask;
    mp->task_mask_set = true;
    _poller_reopen_active(poller, poller->inv);
    return 0;
}

dcu_scheme_t* dcu_poller_get_scheme(dcu_poller_t* poller, uint32_t scheme_id) {
    return (dcu_scheme_t*)_scheme_find(poller, scheme_id);
}

/* content_gen 由聚合根维护，不参与内容比较。 */
static bool _scheme_content_same(const dcu_scheme_t* a, const dcu_scheme_t* b) {
    if (a->type != b->type || a->data.normal.capture_mode != b->data.normal.capture_mode ||
        a->data.normal.ms_type != b->data.normal.ms_type ||
        a->data.normal.storage_depth != b->data.normal.storage_depth ||
        a->data.normal.storage_time != b->data.normal.storage_time ||
        a->data.normal.ms_count != b->data.normal.ms_count ||
        a->data.normal.csd_count != b->data.normal.csd_count)
        return false;
    if (memcmp(a->data.normal.ms_addrs, b->data.normal.ms_addrs,
               sizeof(a->data.normal.ms_addrs)) != 0)
        return false;
    return memcmp(a->data.normal.csd_list, b->data.normal.csd_list,
                  sizeof(uint32_t) * a->data.normal.csd_count) == 0;
}

/* 方案内容变化：只重开引用它的 ACTIVE 任务。重开递增 exec_gen，链上
 * served_ident 按新身份比较时清空旧 served 位图，其余任务的进度不受影响。 */
static void _reopen_scheme_tasks(dcu_poller_t* poller, uint32_t scheme_id) {
    for (int t = 0; t < poller->task_count; t++) {
        dcu_task_unit_t* task = &poller->tasks[t];
        if (task->status == DCU_TASK_STATUS_FREE || task->scheme_id != scheme_id) continue;
        _task_reopen(poller, t, poller->inv);
    }
}

int dcu_poller_upsert_scheme(dcu_poller_t* poller, const dcu_scheme_t* scheme) {
    if (!poller || !scheme) return -1;
    if (scheme->data.normal.csd_count > MAX_SCHEME_OADS) return -1;
    dcu_scheme_t* existing = dcu_poller_get_scheme(poller, scheme->scheme_id);
    if (existing) {
        if (_scheme_content_same(existing, scheme)) return 0;
        *existing = *scheme;
        existing->content_gen = ++poller->scheme_gen_seq;
        _reopen_scheme_tasks(poller, scheme->scheme_id);
        return 0;
    }
    if (poller->scheme_count >= MAX_SCHEMES) return -1;
    poller->schemes[poller->scheme_count] = *scheme;
    poller->schemes[poller->scheme_count].content_gen = ++poller->scheme_gen_seq;
    poller->scheme_count++;
    return 0;
}

int dcu_poller_remove_scheme(dcu_poller_t* poller, uint32_t scheme_id) {
    if (!poller) return -1;
    for (int i = 0; i < poller->scheme_count; i++) {
        if (poller->schemes[i].scheme_id != scheme_id) continue;
        memmove(&poller->schemes[i], &poller->schemes[i + 1],
                sizeof(poller->schemes[0]) * (size_t)(poller->scheme_count - i - 1));
        poller->scheme_count--;
        return 0;
    }
    return -1;
}

int dcu_poller_clear_schemes(dcu_poller_t* poller) {
    if (!poller) return -1;
    memset(poller->schemes, 0, sizeof(poller->schemes));
    poller->scheme_count = 0;
    return 0;
}

bool dcu_poller_scheme_referenced(const dcu_poller_t* poller, uint32_t scheme_id) {
    if (!poller) return false;
    for (int i = 0; i < poller->task_count; i++) {
        const dcu_task_unit_t* t = &poller->tasks[i];
        if (t->status != DCU_TASK_STATUS_FREE && t->scheme_id == scheme_id) return true;
    }
    return false;
}

bool dcu_poller_scheme_masked(const dcu_poller_t* poller, uint32_t scheme_id) {
    if (!poller) return false;
    for (int i = 0; i < poller->task_count; i++) {
        const dcu_task_unit_t* t = &poller->tasks[i];
        if (t->status != DCU_TASK_STATUS_FREE && t->scheme_id == scheme_id && t->item_mask != 0)
            return true;
    }
    return false;
}

void dcu_poller_destroy(dcu_poller_t* poller) {
    (void)poller;
}

void dcu_poller_stop(dcu_poller_t* poller) {
    (void)poller;
}

static const dcu_task_unit_t* _task_by_id(const dcu_poller_t* poller, uint32_t task_id) {
    for (int t = 0; t < poller->task_count; t++) {
        if (poller->tasks[t].task_id == task_id) return &poller->tasks[t];
    }
    return NULL;
}

int dcu_poller_task_monitor_count(const dcu_poller_t* poller) {
    if (!poller) return 0;
    int n = 0;
    for (int t = 0; t < poller->task_count; t++) {
        if (poller->tasks[t].task_id != 0 && poller->tasks[t].status != DCU_TASK_STATUS_FREE) n++;
    }
    return n;
}

int dcu_poller_task_monitor_at(const dcu_poller_t* poller, int pos, dcu_task_monitor_t* out) {
    if (!poller || !out || pos < 0 || pos >= dcu_poller_task_monitor_count(poller)) return -1;
    uint32_t last = 0;
    const dcu_task_unit_t* found = NULL;
    for (int rank = 0; rank <= pos; rank++) {
        const dcu_task_unit_t* best = NULL;
        for (int t = 0; t < poller->task_count; t++) {
            const dcu_task_unit_t* task = &poller->tasks[t];
            if (task->task_id <= last) continue;
            if (!best || task->task_id < best->task_id) best = task;
        }
        if (!best) return -1;
        found = best;
        last = best->task_id;
    }
    *out = found->monitor;
    out->task_id = found->task_id;
    return 0;
}

int dcu_poller_task_monitor_get(const dcu_poller_t* poller, uint32_t task_id,
                                dcu_task_monitor_t* out) {
    if (!poller || !out || task_id == 0) return -1;
    const dcu_task_unit_t* task = _task_by_id(poller, task_id);
    if (!task) return -1;
    *out = task->monitor;
    out->task_id = task->task_id;
    return 0;
}
