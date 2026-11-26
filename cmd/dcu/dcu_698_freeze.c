#include "dcu_698_freeze.h"

#include <string.h>
#include <time.h>

#include "dcu_698_freeze_cfg.h"
#include "edge_log.h"
#include "edge_slave_plane.h"

/* 标准属性 3 的冻结周期以 2000-01-01 00:00 为基准，单位随 OI；0 = 非自动。 */
#define FREEZE_BASE_YEAR 2000

static uint64_t _base_epoch(const struct tm *local) {
    struct tm t = *local;
    t.tm_year = FREEZE_BASE_YEAR - 1900;
    t.tm_mon = 0;
    t.tm_mday = 1;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_isdst = -1;
    time_t v = mktime(&t);
    return v < 0 ? 0 : (uint64_t)v;
}

static uint64_t _local_midnight(struct tm t) {
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_isdst = -1;
    time_t v = mktime(&t);
    return v < 0 ? 0 : (uint64_t)v;
}

static uint64_t _oi_boundary(uint16_t oi, uint64_t now, const struct tm *local, uint32_t period) {
    struct tm t = *local;
    uint64_t base = _base_epoch(local);
    if (base == 0) return 0;
    switch (oi) {
        case 0x5001: /* 秒 */
        case 0x5002: /* 分 */
        case 0x5003: { /* 小时 */
            if (period == 0) return 0;
            uint64_t unit = oi == 0x5001 ? 1u : oi == 0x5002 ? 60u : 3600u;
            uint64_t span = unit * period;
            return base + ((now - base) / span) * span;
        }
        case 0x5004: { /* 日：基准日起每 period 天，本地零点 */
            if (period == 0 || (now - base) / 86400u % period != 0) return 0;
            return _local_midnight(t);
        }
        case 0x5005: /* 结算日：每月 1 日 00:00，结算动作驱动 */
            return t.tm_mday == 1 ? _local_midnight(t) : 0;
        case 0x5006: { /* 月：每 period 月 1 日 00:00 */
            if (period == 0) return 0;
            uint32_t months =
                (uint32_t)(t.tm_year - (FREEZE_BASE_YEAR - 1900)) * 12u + (uint32_t)t.tm_mon;
            if (months % period != 0) return 0;
            t.tm_mday = 1;
            return _local_midnight(t);
        }
        case 0x5007: { /* 年：每 period 年 1 月 1 日 00:00 */
            if (period == 0) return 0;
            uint32_t years = (uint32_t)(t.tm_year - (FREEZE_BASE_YEAR - 1900));
            if (years % period != 0) return 0;
            t.tm_mon = 0;
            t.tm_mday = 1;
            return _local_midnight(t);
        }
        default:
            return 0;
    }
}

static uint64_t _now_s(const dcu_freeze_scheduler_t *s) {
    if (!s->clock || !s->clock->now) return 0;
    struct tm tm;
    if (s->clock->now(NULL, &tm) != 0) return 0;
    time_t v = mktime(&tm);
    return v <= 0 ? 0 : (uint64_t)v;
}

static void _trim(const dcu_freeze_scheduler_t *s, const char *address, uint32_t freeze_oad,
                  uint32_t source_oad, uint32_t depth) {
    uint32_t keep = depth == 0 ? 1u : depth;
    (void)dcu_storage_trim_freeze(s->storage, address, freeze_oad, source_oad, keep);
}

static int _fire_oad_locked(dcu_freeze_scheduler_t *s, uint32_t freeze_oad, uint64_t boundary,
                            uint32_t source_oad, uint32_t depth, uint32_t slot_id,
                            const char *address) {
    double value = 0.0;
    if (s->read(s->read_ctx, slot_id, source_oad, &value) != 0) return 0;
    if (dcu_storage_write_freeze(s->storage, address, freeze_oad, boundary, source_oad, value) != 0)
        return 0;
    _trim(s, address, freeze_oad, source_oad, depth);
    return 1;
}

static int _fire_entry_locked(dcu_freeze_scheduler_t *s, const dcu_freeze_cfg_t *cfg, uint8_t idx,
                              uint16_t oi, uint64_t boundary) {
    uint32_t freeze_oad = ((uint32_t)oi << 16) | 0x0200u;
    const dcu_freeze_attr_t *item = &cfg->items[idx];
    int written = 0;
    for (uint32_t m = 0; m < s->inventory->count; m++) {
        const production_meter_t *meter = &s->inventory->meters[m];
        written += _fire_oad_locked(s, freeze_oad, boundary, item->oad, item->depth,
                                    meter->slot_id, meter->address);
    }
    return written;
}

static int _fire_all_locked(dcu_freeze_scheduler_t *s, uint16_t oi, uint64_t boundary) {
    uint32_t freeze_oad = ((uint32_t)oi << 16) | 0x0200u;
    const dcu_freeze_cfg_t *cfg = s->cfgs ? dcu_freeze_cfg_find(s->cfgs, oi) : NULL;
    int written = 0;
    if (cfg) {
        for (uint8_t k = 0; k < cfg->count; k++) {
            written += _fire_entry_locked(s, cfg, k, oi, boundary);
        }
        return written;
    }
    for (uint32_t m = 0; m < s->inventory->count; m++) {
        const production_meter_t *meter = &s->inventory->meters[m];
        for (uint8_t k = 0; k < s->source_count; k++) {
            written += _fire_oad_locked(s, freeze_oad, boundary, s->sources[k], 0, meter->slot_id,
                                        meter->address);
        }
    }
    return written;
}

static int _fire_entry(dcu_freeze_scheduler_t *s, const dcu_freeze_cfg_t *cfg, uint8_t idx,
                       uint16_t oi, uint64_t boundary) {
    if (!s->storage || !s->inventory || !s->read) return -1;
    if (dcu_storage_begin_trans(s->storage) != 0) return -1;
    int written = _fire_entry_locked(s, cfg, idx, oi, boundary);
    if (dcu_storage_commit_trans(s->storage) != 0) return -1;
    return written;
}

static int _fire_all(dcu_freeze_scheduler_t *s, uint16_t oi, uint64_t boundary) {
    if (!s->storage || !s->inventory || !s->read) return -1;
    if (s->cfgs) {
        const dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_find(s->cfgs, oi);
        if (!cfg || cfg->count == 0) return 0;
    } else if (s->source_count == 0) {
        return -1;
    }
    if (dcu_storage_begin_trans(s->storage) != 0) return -1;
    int written = _fire_all_locked(s, oi, boundary);
    if (dcu_storage_commit_trans(s->storage) != 0) return -1;
    return written;
}

void dcu_freeze_scheduler_init(dcu_freeze_scheduler_t *s, dcu_storage_t *storage,
                               dcu_inventory_t *inventory, dcu_freeze_read_fn read, void *read_ctx,
                               const dcu_clock_port_t *clock) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->storage = storage;
    s->inventory = inventory;
    s->read = read;
    s->read_ctx = read_ctx;
    s->clock = clock;
}

void dcu_freeze_scheduler_set_sources(dcu_freeze_scheduler_t *s, const uint32_t *oads, uint8_t n) {
    if (!s) return;
    uint8_t count = n > DCU_FREEZE_MAX_SOURCES ? DCU_FREEZE_MAX_SOURCES : n;
    for (uint8_t i = 0; i < count; i++) s->sources[i] = oads[i];
    s->source_count = count;
}

void dcu_freeze_scheduler_set_configs(dcu_freeze_scheduler_t *s,
                                      const struct dcu_freeze_cfgs *cfgs) {
    if (s) s->cfgs = cfgs;
}

int dcu_freeze_trigger(dcu_freeze_scheduler_t *s, uint16_t oi) {
    uint64_t now = _now_s(s);
    if (now == 0) return -1;
    return _fire_all(s, oi, now);
}

int dcu_freeze_schedule(dcu_freeze_scheduler_t *s, uint16_t oi, uint32_t delay_s) {
    if (!s) return EDGE_SLAVE_SRC_TEMPORARY;
    if (delay_s == 0) return dcu_freeze_trigger(s, oi) >= 0 ? 0 : EDGE_SLAVE_SRC_TEMPORARY;
    uint64_t now = _now_s(s);
    if (now == 0) return EDGE_SLAVE_SRC_TEMPORARY;
    for (uint8_t i = 0; i < DCU_FREEZE_PENDING_MAX; i++) {
        if (s->pending[i].used) continue;
        s->pending[i].used = true;
        s->pending[i].warned = false;
        s->pending[i].oi = oi;
        s->pending[i].due_ms = now * 1000u + (uint64_t)delay_s * 1000u;
        return 0;
    }
    return EDGE_SLAVE_SRC_TEMPORARY;
}

uint64_t dcu_freeze_pending_next_due_ms(const dcu_freeze_scheduler_t *s) {
    if (!s) return 0;
    uint64_t best = 0;
    for (uint8_t i = 0; i < DCU_FREEZE_PENDING_MAX; i++) {
        if (s->pending[i].used && (best == 0 || s->pending[i].due_ms < best))
            best = s->pending[i].due_ms;
    }
    return best;
}

static int _process_pending(dcu_freeze_scheduler_t *s, uint64_t now_epoch_s) {
    uint64_t now_ms = now_epoch_s * 1000u;
    int written = 0;
    for (uint8_t i = 0; i < DCU_FREEZE_PENDING_MAX; i++) {
        if (!s->pending[i].used || s->pending[i].due_ms > now_ms) continue;
        int n = _fire_all(s, s->pending[i].oi, now_epoch_s);
        if (n < 0) {
            if (!s->pending[i].warned) {
                LOG_WARN(LOG_CAT_CORE,
                         "DCU freeze delayed trigger oi=%04X failed; retry next tick",
                         s->pending[i].oi);
                s->pending[i].warned = true;
            }
            continue;
        }
        if (n > 0) written += n;
        s->pending[i].used = false;
    }
    return written;
}

int dcu_freeze_tick(dcu_freeze_scheduler_t *s, uint64_t now_epoch_s) {
    if (!s || now_epoch_s == 0) return 0;
    struct tm local;
    time_t now = (time_t)now_epoch_s;
    if (!localtime_r(&now, &local)) return 0;
    static const uint16_t ois[DCU_FREEZE_OI_COUNT] = {0x5000, 0x5001, 0x5002, 0x5003,
                                                      0x5004, 0x5005, 0x5006, 0x5007};
    int written = _process_pending(s, now_epoch_s);
    for (uint8_t i = 0; i < DCU_FREEZE_OI_COUNT; i++) {
        uint16_t oi = ois[i];
        const dcu_freeze_cfg_t *cfg = s->cfgs ? dcu_freeze_cfg_find(s->cfgs, oi) : NULL;
        uint8_t count = cfg ? cfg->count : 1;
        for (uint8_t k = 0; k < count; k++) {
            uint32_t period = cfg ? cfg->items[k].period : 1;
            uint64_t boundary = _oi_boundary(oi, now_epoch_s, &local, period);
            if (boundary == 0) continue;
            if (!s->primed) {
                s->last_fire[i][k] = boundary;
                continue;
            }
            if (boundary <= s->last_fire[i][k]) continue;
            s->last_fire[i][k] = boundary;
            s->last_boundary_epoch = boundary;
            int n = cfg ? _fire_entry(s, cfg, k, oi, boundary) : _fire_all(s, oi, boundary);
            if (n > 0) written += n;
        }
    }
    if (!s->primed) s->primed = true;
    return written;
}

int dcu_freeze_next_boundary(dcu_freeze_scheduler_t *s, uint64_t now_epoch_s,
                             uint64_t *boundary_s) {
    if (!s || !boundary_s) return 0;
    /* 所有 Class 9 时钟边界都落在整秒上；5001 每秒都要冻结，所以下一个触发点
     * 就是下一整秒。按 ms 排程到这一刻，回调抖动就是对齐误差。 */
    *boundary_s = now_epoch_s + 1;
    return 1;
}
