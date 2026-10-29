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

    /* 每通道一条独立的链：游标从 0 开始，并发默认 1（串口半双工无法流水）。
     * 想放开某条通道的并发，改对应 chain.depth 即可——但先确认物理层真能流水。 */
    for (int i = 0; i < DCU_MAX_CHANNELS; i++) {
        poller->chains[i].owner = poller;
        poller->chains[i].channel_id = (uint8_t)i;
        poller->chains[i].cursor = 0;
        poller->chains[i].round = 0;
        poller->chains[i].outstanding = 0;
        poller->chains[i].depth = 1;
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
                        if (c->breaker_open && poller->ch_broken[old] > 0) poller->ch_broken[old]--;
                        _recompute_channel(poller, old);
                    }
                    c->channel_id = channel_id;
                    if (channel_id < DCU_MAX_CHANNELS) {
                        poller->ch_tracked[channel_id]++;
                        if (c->breaker_open) poller->ch_broken[channel_id]++;
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

/* 本任务这一轮是否到期。interval_s==0 视为每轮都跑。 */
static bool _task_due(dcu_task_unit_t* task) {
    if (task->interval_s == 0) return true;
    uint64_t ticks = 0;
    return edge_periodic_clock_check(&task->clock, &ticks) > 0;
}

/* 装载"本次访问要抄的项"：所有到期任务的项取并集，按 tag_id 去重。
 * 这就是"避免同一数据重复抄读"的全部实现——同一项被多个任务引用时，
 * 这里只留下一个 tag，后面只会发一帧。 */
static void _chain_load_pending(dcu_poller_t* poller, const dcu_meter_poll_state_t* mp,
                                dcu_chain_t* c) {
    c->pending_n = 0;
    c->pending_i = 0;
    c->visit_ok = 0;
    c->visit_fail = 0;

    for (int t = 0; t < poller->task_count; t++) {
        dcu_task_unit_t* task = &poller->tasks[t];
        if (mp && mp->task_mask_set && !(mp->task_mask & (1u << t))) continue;
        if (!poller->task_due[t]) continue;

        dcu_scheme_t* s = dcu_poller_get_scheme(poller, task->scheme_id);
        if (!s) continue;

        for (int j = 0; j < s->data.normal.csd_count && c->pending_n < MAX_SCHEME_OADS; j++) {
            if (task->item_mask && !(task->item_mask & (1u << j))) continue;
            uint32_t tag = s->data.normal.csd_list[j];
            bool dup = false;
            for (int k = 0; k < c->pending_n; k++) {
                if (c->pending_tags[k] == tag) { dup = true; break; }
            }
            if (!dup) c->pending_tags[c->pending_n++] = tag;
        }
    }
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
            dcu_meter_poll_state_t* mp = _meter_state(poller, m->address, c->channel_id);
            if (!(mp && mp->breaker_open)) {
                _chain_load_pending(poller, mp, c);
                if (c->pending_n > 0) {
                    c->meter_idx = idx;
                    return true;
                }
            }
        }
        if (wrapped) return false;  /* 一圈扫完，本步到此为止（未找到活） */
    }
    c->scanned_in_round = 0;
    return false;
}

/* 一次访问结算：按结果更新表级熔断与通道统计 */
static void _chain_finish_visit(dcu_poller_t* poller, dcu_chain_t* c) {
    dcu_inventory_t* inv = poller->inv;
    if (!inv || c->meter_idx >= inv->count) return;

    production_meter_t* m = &inv->meters[c->meter_idx];
    dcu_meter_poll_state_t* mp = _meter_state(poller, m->address, c->channel_id);
    edge_poll_stats_t* st = &poller->channel_stats[c->channel_id];
    edge_poll_policy_t* pol = &poller->channel_policies[c->channel_id];

    if (c->visit_fail > 0) {
        st->consecutive_fails++;
        if (mp) {
            if (mp->consecutive_fails < UINT32_MAX) mp->consecutive_fails++;
            if (pol->circuit_break_threshold > 0 &&
                mp->consecutive_fails >= pol->circuit_break_threshold) {
                mp->breaker_open = true;
                poller->ch_broken[c->channel_id]++;
                _recompute_channel(poller, c->channel_id);
                LOG_ERROR(LOG_CAT_IO,
                          "🚨 [DCU-Poller] Meter %s (ch %u) circuit breaker tripped (fails: %u "
                          ">= threshold: %u)",
                          m->address, c->channel_id, (unsigned)mp->consecutive_fails,
                          pol->circuit_break_threshold);
            }
        }
    } else if (c->visit_ok > 0) {
        st->success_count++;
        st->consecutive_fails = 0;
        st->last_success_ts = uv_hrtime() / 1000000ULL;
        if (mp) mp->consecutive_fails = 0;
        for (int t = 0; t < poller->task_count; t++) {
            if (mp && mp->task_mask_set && !(mp->task_mask & (1u << t))) continue;
            poller->tasks[t].monitor.success_meters++;
        }
    }
}

/* 读完成：写值、扇出、推进链 */
static void _on_read_done(void* ud, int status, double value) {
    dcu_chain_t* c = (dcu_chain_t*)ud;
    dcu_poller_t* poller = (dcu_poller_t*)c->owner;
    if (!poller) return;

    if (c->outstanding > 0) c->outstanding--;

    dcu_inventory_t* inv = poller->inv;
    if (inv && c->meter_idx < inv->count) {
        production_meter_t* m = &inv->meters[c->meter_idx];
        if (status == 0) {
            c->visit_ok++;
            if (poller->sink_ops.record_telemetry) {
                poller->sink_ops.record_telemetry(poller->sink_ops.ctx, m->address, c->meter_idx,
                                                  c->inflight_tag, value);
            }
            if (poller->bus_ops.publish_realtime) {
                poller->bus_ops.publish_realtime(poller->bus_ops.ctx, c->inflight_tag,
                                                 c->meter_idx, value, uv_hrtime());
            }
        } else {
            c->visit_fail++;
            poller->channel_stats[c->channel_id].error_count++;
        }
    }

    /* 链式：一项回来，立刻推进下一项。submit 保证异步回调，递归深度恒为 1。 */
    extern void _dcu_chain_pump(dcu_poller_t * poller, dcu_chain_t * c);
    _dcu_chain_pump(poller, c);
}

/* 推进一条链：要么提交下一项，要么换下一块表，要么无事可做 */
void _dcu_chain_pump(dcu_poller_t* poller, dcu_chain_t* c) {
    dcu_inventory_t* inv = poller->inv;
    if (!inv || inv->count == 0) return;
    if (c->depth == 0) c->depth = 1;
    if (c->pumping) return;   /* 同步回调重入：交给外层循环继续 */
    c->pumping = true;

    for (;;) {
        if (c->outstanding >= c->depth) break;   /* 在途已满，等回调 */

        if (c->pending_i < c->pending_n) {
            uint32_t tag = c->pending_tags[c->pending_i++];
            c->inflight_tag = tag;

            edge_poll_stats_t* st = &poller->channel_stats[c->channel_id];
            st->poll_count++;
            st->last_poll_ts = uv_hrtime() / 1000000ULL;

            c->outstanding++;
            int rc = poller->source_ops.submit
                         ? poller->source_ops.submit(poller->source_ops.ctx, c->meter_idx, tag,
                                                     _on_read_done, c)
                         : -1;
            if (rc != 0) {
                /* 提交失败不会再有回调：当场结算，避免链卡死 */
                c->outstanding--;
                c->visit_fail++;
                poller->channel_stats[c->channel_id].error_count++;
                continue;
            }
            /* 异步：等回调。同步回调（mock/立即返回）时 outstanding 已归零，
             * 上面那道 pumping 守卫让重入直接返回，于是这里继续循环即可。 */
            if (c->outstanding > 0) break;
            continue;
        }

        if (c->pending_n > 0) {
            _chain_finish_visit(poller, c);
            c->pending_n = 0;
            c->pending_i = 0;
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
                    if (mp->breaker_open && poller->ch_broken[ch] > 0) poller->ch_broken[ch]--;
                    _recompute_channel(poller, ch);
                }
                memset(mp, 0, sizeof(*mp));
                mp->tombstone = true;
            }
        }
        poller->last_inv_rev = inv->revision;
        /* 档案变了，游标可能越界：夹回合法范围 */
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
    return mp ? mp->breaker_open : false;
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
            mp->breaker_open = false;
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
    s->type = SCHEME_TYPE_NORMAL;
    s->data.normal.capture_mode = mode;
    s->data.normal.csd_count = csd_count;
    for (uint16_t i = 0; i < csd_count; i++) s->data.normal.csd_list[i] = csds[i];
    return 0;
}

int dcu_poller_add_task_ex(dcu_poller_t* poller, uint32_t task_id, uint32_t interval_s,
                           uint32_t scheme_id, uint32_t item_mask) {
    if (!poller || poller->task_count >= MAX_TASKS) return -1;
    dcu_task_unit_t* t = &poller->tasks[poller->task_count++];
    memset(t, 0, sizeof(*t));
    t->task_id = task_id;
    t->interval_s = interval_s;
    t->scheme_id = scheme_id;
    t->item_mask = item_mask;
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

int dcu_poller_add_task(dcu_poller_t* poller, uint32_t task_id, uint32_t interval_s,
                        uint32_t scheme_id) {
    /* 旧签名没有项维度：0 在调度里解释为"全部 CSD" */
    return dcu_poller_add_task_ex(poller, task_id, interval_s, scheme_id, 0);
}

int dcu_poller_set_meter_tasks(dcu_poller_t* poller, const char* address, uint8_t channel_id,
                               uint16_t task_mask) {
    if (!poller || !address || !address[0]) return -1;
    if (channel_id >= DCU_MAX_CHANNELS) return -1;
    dcu_meter_poll_state_t* mp = _meter_state(poller, address, channel_id);
    if (!mp) return -1; // 状态表满：fail-open，该表仍按旧行为被全表抄读
    mp->task_mask = task_mask;
    mp->task_mask_set = true;
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
