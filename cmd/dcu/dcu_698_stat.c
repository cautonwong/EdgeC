#include "dcu_698_stat.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#include "dcu_698_axdr.h"
#include "dcu_698_axdr_internal.h"
#include "dcu_router.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

typedef struct {
    uint8_t count;
    dcu_stat_entry_t entries[DCU_STAT_MAX_ENTRIES];
} dcu_stat_oi_state_t;

static dcu_stat_oi_state_t s_stat[DCU_STAT_OI_COUNT];

static int _oi_index(uint16_t oi) {
    if (oi >= 0x2100 && oi <= 0x2104) return (int)(oi - 0x2100);
    if (oi >= 0x2110 && oi <= 0x2114) return 5 + (int)(oi - 0x2110);
    if (oi >= 0x2120 && oi <= 0x2124) return 10 + (int)(oi - 0x2120);
    return -1;
}

static uint8_t _oi_class(uint16_t oi) {
    if (oi >= 0x2100 && oi <= 0x2104) return IC_INTERVAL_STAT;
    if (oi >= 0x2110 && oi <= 0x2114) return IC_AVERAGE;
    if (oi >= 0x2120 && oi <= 0x2124) return IC_EXTREMUM;
    return 0;
}

bool dcu_698_stat_is_oi(uint16_t oi) {
    return _oi_index(oi) >= 0;
}

void dcu_698_stat_reset(void) {
    memset(s_stat, 0, sizeof(s_stat));
}

static void _reset_entry_metrics(dcu_stat_entry_t *e) {
    e->count = 0;
    e->sum = 0.0;
    e->avg = 0.0;
    e->min = 0.0;
    e->max = 0.0;
    e->min_ts_s = 0;
    e->max_ts_s = 0;
    e->last_ts_s = 0;
    e->first_ts_s = 0;
}

void dcu_698_stat_ingest(uint32_t src_oad, double value, uint64_t ts_s) {
    if (src_oad == 0) return;
    for (int i = 0; i < DCU_STAT_OI_COUNT; i++) {
        dcu_stat_oi_state_t *st = &s_stat[i];
        for (uint8_t j = 0; j < st->count && j < DCU_STAT_MAX_ENTRIES; j++) {
            dcu_stat_entry_t *e = &st->entries[j];
            if (!e->active || e->src_oad != src_oad) continue;
            if (e->count == 0) {
                e->count = 1;
                e->sum = value;
                e->avg = value;
                e->min = value;
                e->max = value;
                e->min_ts_s = ts_s;
                e->max_ts_s = ts_s;
                e->first_ts_s = ts_s;
                e->last_ts_s = ts_s;
            } else {
                e->count++;
                e->sum += value;
                e->avg = e->sum / (double)e->count;
                if (value < e->min) {
                    e->min = value;
                    e->min_ts_s = ts_s;
                }
                if (value > e->max) {
                    e->max = value;
                    e->max_ts_s = ts_s;
                }
                e->last_ts_s = ts_s;
            }
        }
    }
}

int dcu_698_stat_get_entries(uint16_t oi, const dcu_stat_entry_t **out_entries, size_t *out_count) {
    int idx = _oi_index(oi);
    if (idx < 0) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (out_entries) *out_entries = s_stat[idx].entries;
    if (out_count) *out_count = s_stat[idx].count;
    return 0;
}

static int _put_oad(uint8_t *buf, size_t cap, size_t *off, uint32_t oad) {
    if (*off + 5 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_OAD;
    buf[(*off)++] = (uint8_t)(oad >> 24);
    buf[(*off)++] = (uint8_t)(oad >> 16);
    buf[(*off)++] = (uint8_t)(oad >> 8);
    buf[(*off)++] = (uint8_t)oad;
    return 0;
}

static int _put_u32(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint32_t v) {
    if (*off + 5 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = (uint8_t)(v >> 24);
    buf[(*off)++] = (uint8_t)(v >> 16);
    buf[(*off)++] = (uint8_t)(v >> 8);
    buf[(*off)++] = (uint8_t)v;
    return 0;
}

static int32_t _scale_i32_x100(double v) {
    double scaled = v * 100.0;
    if (scaled >= (double)INT32_MAX) return INT32_MAX;
    if (scaled <= (double)INT32_MIN) return INT32_MIN;
    return (int32_t)llround(scaled);
}

static int _put_stat_ts(uint64_t ts_s, bool has_sample, uint8_t *buf, size_t cap, size_t *off) {
    if (has_sample && dcu_698_epoch_is_civil(ts_s)) {
        return dcu_698_put_date_time_s(ts_s, buf, cap, off);
    }
    if (*off + 8 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_DATE_TIME_S;
    for (int i = 0; i < 7; i++) buf[(*off)++] = 0;
    return 0;
}

static int _encode_entry(uint8_t class_id, const dcu_stat_entry_t *e, uint8_t *buf, size_t cap,
                         size_t *off) {
    if (class_id == IC_INTERVAL_STAT) {
        uint32_t elapsed_s =
            (e->count > 0 && e->last_ts_s >= e->first_ts_s) ? (uint32_t)(e->last_ts_s - e->first_ts_s) : 0;
        if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
            _put_oad(buf, cap, off, e->src_oad) != 0 ||
            _put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, 1) != 0 ||
            _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
            _put_u32(buf, cap, off, DLT698_TAG_DOUBLE_LONG_UNSIGNED, elapsed_s) != 0 ||
            _put_u32(buf, cap, off, DLT698_TAG_DOUBLE_LONG_UNSIGNED, e->count) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        return 0;
    }
    if (class_id == IC_AVERAGE) {
        int32_t sum_x100 = (e->count > 0) ? _scale_i32_x100(e->sum) : 0;
        int32_t avg_x100 = (e->count > 0) ? _scale_i32_x100(e->avg) : 0;
        if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 3) != 0 ||
            _put_oad(buf, cap, off, e->src_oad) != 0 ||
            _put_u32(buf, cap, off, DLT698_TAG_DOUBLE_LONG, (uint32_t)sum_x100) != 0 ||
            _put_u32(buf, cap, off, DLT698_TAG_DOUBLE_LONG, (uint32_t)avg_x100) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        return 0;
    }
    if (class_id == IC_EXTREMUM) {
        int32_t max_x100 = (e->count > 0) ? _scale_i32_x100(e->max) : 0;
        int32_t min_x100 = (e->count > 0) ? _scale_i32_x100(e->min) : 0;
        if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 5) != 0 ||
            _put_oad(buf, cap, off, e->src_oad) != 0 ||
            _put_u32(buf, cap, off, DLT698_TAG_DOUBLE_LONG, (uint32_t)max_x100) != 0 ||
            _put_stat_ts(e->max_ts_s, e->count > 0, buf, cap, off) != 0 ||
            _put_u32(buf, cap, off, DLT698_TAG_DOUBLE_LONG, (uint32_t)min_x100) != 0 ||
            _put_stat_ts(e->min_ts_s, e->count > 0, buf, cap, off) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        return 0;
    }
    return EDGE_SLAVE_SRC_UNDEFINED;
}

int dcu_698_stat_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem_idx = (uint8_t)(oad & 0xFF);

    const dcu_698_object_t *obj = dcu_698_find(oi);
    if (!obj) return EDGE_SLAVE_SRC_NOT_FOUND;
    int idx = _oi_index(oi);
    uint8_t class_id = _oi_class(oi);
    if (idx < 0 || class_id == 0) return EDGE_SLAVE_SRC_NOT_FOUND;

    if (attr_id != 2) {
        return dcu_698_class_defines_attr(obj->class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                                  : EDGE_SLAVE_SRC_UNDEFINED;
    }

    const dcu_stat_oi_state_t *st = &s_stat[idx];
    size_t off = 0;

    if (elem_idx > 0) {
        if (elem_idx > st->count) return EDGE_SLAVE_SRC_NOT_FOUND;
        int rc = _encode_entry(class_id, &st->entries[elem_idx - 1], buf, cap, &off);
        if (rc != 0) return rc;
        *len = off;
        return 0;
    }

    if (_put_tagged_u8(buf, cap, &off, DLT698_TAG_ARRAY, st->count) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    for (uint8_t i = 0; i < st->count; i++) {
        int rc = _encode_entry(class_id, &st->entries[i], buf, cap, &off);
        if (rc != 0) return rc;
    }
    *len = off;
    return 0;
}

static uint32_t _ti_to_seconds(uint8_t unit, uint16_t value) {
    uint32_t mult = 1;
    switch (unit) {
        case 0:
            mult = 1;
            break;
        case 1:
            mult = 60;
            break;
        case 2:
            mult = 3600;
            break;
        case 3:
            mult = 86400;
            break;
        default:
            mult = 1;
            break;
    }
    uint64_t prod = (uint64_t)value * (uint64_t)mult;
    return prod > UINT32_MAX ? UINT32_MAX : (uint32_t)prod;
}

static int _skip_data_item(axdr_cursor_t *c, int depth) {
    if (depth > 8) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    const uint8_t *tag_p = NULL;
    if (!_take(c, 1, &tag_p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    uint8_t tag = *tag_p;
    switch (tag) {
        case DLT698_TAG_NULL:
            return 0;
        case DLT698_TAG_BOOLEAN:
        case DLT698_TAG_INTEGER:
        case DLT698_TAG_UNSIGNED:
        case DLT698_TAG_ENUM:
            return _take(c, 1, NULL) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
        case DLT698_TAG_LONG:
        case DLT698_TAG_LONG_UNSIGNED:
            return _take(c, 2, NULL) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
        case DLT698_TAG_DOUBLE_LONG:
        case DLT698_TAG_DOUBLE_LONG_UNSIGNED:
        case DLT698_TAG_FLOAT32:
        case DLT698_TAG_OAD:
            return _take(c, 4, NULL) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
        case DLT698_TAG_TI:
            return _take(c, 3, NULL) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
        case DLT698_TAG_DATE_TIME_S:
            return _take(c, 7, NULL) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
        case DLT698_TAG_LONG64:
        case DLT698_TAG_LONG64_UNSIGNED:
        case DLT698_TAG_FLOAT64:
            return _take(c, 8, NULL) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
        case DLT698_TAG_ARRAY:
        case DLT698_TAG_STRUCTURE: {
            const uint8_t *cnt_p = NULL;
            if (!_take(c, 1, &cnt_p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (uint8_t i = 0; i < *cnt_p; i++) {
                int rc = _skip_data_item(c, depth + 1);
                if (rc != 0) return rc;
            }
            return 0;
        }
        default:
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
}

static int _take_oad(axdr_cursor_t *c, uint32_t *out) {
    const uint8_t *p = NULL;
    if (!_take(c, 5, &p) || p[0] != DLT698_TAG_OAD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *out = ((uint32_t)p[1] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 8) | p[4];
    return 0;
}

int dcu_698_stat_decode_op(uint16_t oi, uint8_t method, const uint8_t *axdr, size_t len,
                           dcu_stat_op_t *out) {
    if (!out) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    memset(out, 0, sizeof(*out));
    out->method = method;

    if (!axdr || len == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    axdr_cursor_t c = {.p = axdr, .len = len, .off = 0};

    switch (method) {
        case 1: {
            const uint8_t *p = NULL;
            if (!_take(&c, 1, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (*p == DLT698_TAG_INTEGER) {
                if (!_take(&c, 1, &p)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            } else if (*p != DLT698_TAG_NULL) {
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            }
            if (!_time_tag_absent(c.p, c.off, c.len)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            return 0;
        }
        case 3: {
            uint8_t class_id = _oi_class(oi);
            uint8_t want_fields = (class_id == IC_INTERVAL_STAT) ? 4 : 3;
            if (_take_head(&c, DLT698_TAG_STRUCTURE, want_fields) != 0)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (_take_oad(&c, &out->src_oad) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (class_id == IC_INTERVAL_STAT) {
                uint8_t thresh_cnt = 0;
                if (_take_array(&c, &thresh_cnt) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
                for (uint8_t i = 0; i < thresh_cnt; i++) {
                    if (_skip_data_item(&c, 0) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
                }
            }
            uint8_t period_cycles = 0;
            if (_take_u8(&c, DLT698_TAG_UNSIGNED, &period_cycles) != 0)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            uint8_t ti_unit = 0;
            uint16_t ti_val = 0;
            if (_take_ti(&c, &ti_unit, &ti_val) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (!_time_tag_absent(c.p, c.off, c.len)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

            uint32_t ti_s = _ti_to_seconds(ti_unit, ti_val);
            out->period_s = (ti_s > 0) ? ti_s : (uint32_t)period_cycles;
            return 0;
        }
        case 4: {
            if (_take_oad(&c, &out->src_oad) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (!_time_tag_absent(c.p, c.off, c.len)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            return 0;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
}

int dcu_698_stat_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                      void *out_val) {
    (void)elem_idx;
    if (!obj) return EDGE_SLAVE_SRC_NOT_FOUND;
    int idx = _oi_index(obj->oi);
    if (idx < 0) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (attr_id != 2) {
        return dcu_698_class_defines_attr(obj->class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                                  : EDGE_SLAVE_SRC_UNDEFINED;
    }
    if (!out_val) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *(uint32_t *)out_val = (uint32_t)s_stat[idx].count;
    return 0;
}

int dcu_698_stat_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                         void *result) {
    (void)result;
    if (!obj) return EDGE_SLAVE_SRC_NOT_FOUND;
    int idx = _oi_index(obj->oi);
    if (idx < 0) return EDGE_SLAVE_SRC_NOT_FOUND;

    dcu_stat_oi_state_t *st = &s_stat[idx];

    if (method_id == 1) {
        for (uint8_t i = 0; i < st->count && i < DCU_STAT_MAX_ENTRIES; i++) {
            _reset_entry_metrics(&st->entries[i]);
        }
        return 0;
    }

    const dcu_stat_op_t *op = (const dcu_stat_op_t *)param;
    if (!op) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    if (method_id == 3) {
        if (op->src_oad == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        for (uint8_t i = 0; i < st->count && i < DCU_STAT_MAX_ENTRIES; i++) {
            if (st->entries[i].active && st->entries[i].src_oad == op->src_oad) {
                st->entries[i].period_s = op->period_s;
                return 0;
            }
        }
        if (st->count >= DCU_STAT_MAX_ENTRIES) return EDGE_SLAVE_SRC_TEMPORARY;
        dcu_stat_entry_t *e = &st->entries[st->count++];
        memset(e, 0, sizeof(*e));
        e->active = true;
        e->src_oad = op->src_oad;
        e->period_s = op->period_s;
        return 0;
    }

    if (method_id == 4) {
        for (uint8_t i = 0; i < st->count && i < DCU_STAT_MAX_ENTRIES; i++) {
            if (st->entries[i].active && st->entries[i].src_oad == op->src_oad) {
                for (uint8_t j = i; j + 1 < st->count; j++) {
                    st->entries[j] = st->entries[j + 1];
                }
                st->count--;
                memset(&st->entries[st->count], 0, sizeof(st->entries[st->count]));
                return 0;
            }
        }
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }

    return dcu_698_class_defines_method(obj->class_id, method_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                                  : EDGE_SLAVE_SRC_UNDEFINED;
}
