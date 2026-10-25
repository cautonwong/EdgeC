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

int dcu_poller_step(dcu_poller_t* poller, dcu_inventory_t* inv, uint32_t max_meters_per_step) {
    if (!poller || !inv) {
        return 0;
    }

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
                // 表已从档案删除：释放槽位并同步通道计数
                uint32_t ch = mp->channel_id;
                if (ch < DCU_MAX_CHANNELS) {
                    if (poller->ch_tracked[ch] > 0) poller->ch_tracked[ch]--;
                    if (mp->breaker_open && poller->ch_broken[ch] > 0) poller->ch_broken[ch]--;
                    _recompute_channel(poller, ch);
                }
                // 留墓碑而不是清空：否则探测链上后面的活条目会变成不可达
                memset(mp, 0, sizeof(*mp));
                mp->tombstone = true;
            }
        }
        poller->last_inv_rev = inv->revision;
    }

    if (inv->count == 0) {
        return 0;
    }

    int meters_processed = 0;

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
        dcu_scheme_t* s = dcu_poller_get_scheme(poller, task->scheme_id);
        if (!s) continue;

        uint32_t start_idx = task->cursor_meter_idx % inv->count;
        // 一轮扫描从游标开始绕整圈：熔断跳过的表不能浪费一轮
        uint32_t step_limit = max_meters_per_step > 0
                                  ? (max_meters_per_step < inv->count ? max_meters_per_step
                                                                     : inv->count)
                                  : inv->count;
        uint32_t count_in_this_step = 0;
        uint32_t offsets_used = 0;

        for (uint32_t offset = 0; offset < inv->count && count_in_this_step < step_limit; offset++) {
            uint32_t m_idx = (start_idx + offset) % inv->count;
            offsets_used = offset + 1;
            production_meter_t* m = &inv->meters[m_idx];
            uint32_t ch = m->channel_id < DCU_MAX_CHANNELS ? m->channel_id : 0;
            edge_poll_stats_t* cstats = &poller->channel_stats[ch];
            edge_poll_policy_t* cpolicy = &poller->channel_policies[ch];
            dcu_meter_poll_state_t* mp = _meter_state(poller, m->address, (uint8_t)ch);

            // 1. 熔断保护检查：按表熔断，单表静默不拖累通道其余表
            if (mp && mp->breaker_open) {
                continue;
            }

            // 2. In-flight 限制与背压 (Backpressure)
            uint32_t max_in_flight = cpolicy->max_in_flight > 0 ? cpolicy->max_in_flight : 1;
            if (cstats->in_flight >= max_in_flight) {
                cstats->queue_depth++;
                continue;
            }

            cstats->poll_count++;
            cstats->last_poll_ts = uv_hrtime() / 1000000ULL;
            cstats->in_flight++;
            count_in_this_step++;
            meters_processed++;

            bool meter_failed = false;
            bool meter_pending = false;
            int meter_ok = 0;
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
                    meter_ok++;
                } else if (read_rc == POLL_READ_PENDING) {
                    meter_pending = true; // 中性：请求在途，不改判连败/成功
                } else {
                    meter_failed = true;
                }
            }

            cstats->in_flight--;
            if (meter_failed) {
                cstats->error_count++;
                cstats->consecutive_fails++;  // 仅汇总可观测，不参与熔断判定
                if (mp) {
                    if (mp->consecutive_fails < UINT32_MAX) {
                        mp->consecutive_fails++;
                    }
                    if (cpolicy->circuit_break_threshold > 0 &&
                        mp->consecutive_fails >= cpolicy->circuit_break_threshold) {
                        mp->breaker_open = true;
                        if (ch < DCU_MAX_CHANNELS) {
                            poller->ch_broken[ch]++;
                            _recompute_channel(poller, ch);
                        }
                        LOG_ERROR(LOG_CAT_IO,
                                  "🚨 [DCU-Poller] Meter %s (ch %u) circuit breaker tripped (fails: %u "
                                  ">= threshold: %u)",
                                  m->address, ch, (unsigned)mp->consecutive_fails,
                                  cpolicy->circuit_break_threshold);
                    }
                }
            } else if (meter_ok == 0 && (meter_pending || s->data.normal.csd_count == 0)) {
                // 本轮结果未知（全部 CSD 在途，或该方案根本没有 CSD）：不记成功也不记失败
            } else {
                cstats->success_count++;
                cstats->consecutive_fails = 0;  // 仅汇总可观测，不参与熔断判定
                cstats->last_success_ts = uv_hrtime() / 1000000ULL;
                task->monitor.success_meters++;
                if (mp) {
                    mp->consecutive_fails = 0;
                }
            }
        }
        // 游标至少推进一格：全部被熔断跳过时也必须继续扫描，否则任务卡死在单表
        task->cursor_meter_idx =
            (uint16_t)((start_idx + (offsets_used > 0 ? offsets_used : 1)) % inv->count);
    }

    // 批量落盘
    if (poller->sink_ops.commit_batch) {
        poller->sink_ops.commit_batch(poller->sink_ops.ctx);
    }

    return meters_processed;
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
