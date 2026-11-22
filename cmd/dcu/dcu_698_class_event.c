#include "dcu_698_object_internal.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "dcu_698_var.h"
#include "dlt698_codec.h"

int dcu_698_event_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                       void *out_val) {
    (void)elem_idx;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt || !out_val) return -1;

    // 7.3.24 表193：A4 最大记录数、A11 上报标识、A12 有效标识。
    if (attr_id == 4) {
        *(uint16_t *)out_val = MAX_EVENT_RECORDS;
        return 0;
    }
    if (attr_id == 11) {
        *(uint8_t *)out_val = evt->report_flag;
        return 0;
    }
    if (attr_id == 12) {
        *(bool *)out_val = evt->is_enabled;
        return 0;
    }
    return -2;
}

int dcu_698_event_write(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                        const void *in_val) {
    (void)elem_idx;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt || !in_val) return -1;

    if (attr_id == 11) {
        uint8_t v = *(const uint8_t *)in_val;
        if (v > 3) return -2;
        evt->report_flag = v;
        return 0;
    }
    if (attr_id == 12) {
        evt->is_enabled = *(const bool *)in_val;
        return 0;
    }
    return -2;
}

static int _put_head(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint8_t count) {
    if (*off + 2 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = count;
    return 0;
}

static int _put_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint8_t v) {
    if (*off + 2 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = v;
    return 0;
}

static int _put_u16(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint16_t v) {
    if (*off + 3 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = (uint8_t)(v >> 8);
    buf[(*off)++] = (uint8_t)v;
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

static int _put_oad(uint8_t *buf, size_t cap, size_t *off, uint32_t oad) {
    return _put_u32(buf, cap, off, 0x51, oad);
}

static int _put_f32(uint8_t *buf, size_t cap, size_t *off, float f) {
    uint32_t bits = 0;
    memcpy(&bits, &f, sizeof(bits));
    return _put_u32(buf, cap, off, DCU_698_TAG_FLOAT32, bits);
}

/* 记录时标是纳秒；0 表示未知，发 NULL 而不是 1970 年。 */
static int _put_time(uint8_t *buf, size_t cap, size_t *off, uint64_t ts_ns) {
    if (ts_ns == 0) {
        if (*off + 1 > cap) return -1;
        buf[(*off)++] = 0x00;
        return 0;
    }
    time_t secs = (time_t)(ts_ns / 1000000000ULL);
    struct tm tm;
    if (!gmtime_r(&secs, &tm)) return -1;
    if (*off > UINT16_MAX || cap > UINT16_MAX) return -1;
    uint16_t at = (uint16_t)*off;
    if (dlt698_encode_date_time_s(buf, (uint16_t)cap, &at, (uint16_t)(tm.tm_year + 1900),
                                  (uint8_t)(tm.tm_mon + 1), (uint8_t)tm.tm_mday,
                                  (uint8_t)tm.tm_hour, (uint8_t)tm.tm_min,
                                  (uint8_t)tm.tm_sec) != 0) {
        return -1;
    }
    *off = at;
    return 0;
}

int dcu_698_event_encode_axdr(const dcu_698_object_t *obj, uint32_t oad, uint8_t *buf, size_t cap,
                              size_t *len) {
    if (!obj || !buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt) return EDGE_SLAVE_SRC_NOT_FOUND;

    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    if (!dcu_mask_test(obj->attr_mask, attr_id)) {
        return dcu_698_class_defines_attr(obj->class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                                  : EDGE_SLAVE_SRC_UNDEFINED;
    }

    size_t off = 0;
    int rc = 0;

    if (attr_id == 4) {
        rc = _put_u16(buf, cap, &off, DCU_698_TAG_LONG_UNSIGNED, MAX_EVENT_RECORDS);
    } else if (attr_id == 11) {
        rc = _put_u8(buf, cap, &off, DCU_698_TAG_ENUM, evt->report_flag);
    } else if (attr_id == 12) {
        rc = _put_u8(buf, cap, &off, DCU_698_TAG_BOOLEAN, evt->is_enabled ? 1 : 0);
    } else if (attr_id == 2) {
        /* 表194：array，每个元素是一个关联对象属性 OAD。 */
        if (evt->snapshot_count > 8) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        rc = _put_head(buf, cap, &off, DCU_698_TAG_ARRAY, evt->snapshot_count);
        for (uint8_t i = 0; rc == 0 && i < evt->snapshot_count; i++) {
            rc = _put_oad(buf, cap, &off, evt->snapshot_oads[i]);
        }
    } else if (attr_id == 3) {
        /* 表194：structure{表1..表4 当前记录数}；表2-4 没有实例化，条数为 0。 */
        rc = _put_head(buf, cap, &off, DCU_698_TAG_STRUCTURE, 4);
        if (rc == 0) rc = _put_u16(buf, cap, &off, DCU_698_TAG_LONG_UNSIGNED, evt->record_count);
        for (int i = 0; rc == 0 && i < 3; i++) {
            rc = _put_u16(buf, cap, &off, DCU_698_TAG_LONG_UNSIGNED, 0);
        }
    } else if (attr_id == 5) {
        /* 表194：structure{门限, 延时(秒)}；延时在标准里是 long-unsigned。 */
        if (evt->delay_s > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        rc = _put_head(buf, cap, &off, DCU_698_TAG_STRUCTURE, 2);
        if (rc == 0) rc = _put_f32(buf, cap, &off, evt->threshold);
        if (rc == 0) {
            rc = _put_u16(buf, cap, &off, DCU_698_TAG_LONG_UNSIGNED, (uint16_t)evt->delay_s);
        }
    } else if (attr_id == 6) {
        /* 记录单元还要求 n 个关联 Data；dcu_event_record_t 不存这些值，不能拿当前值冒充。 */
        if (evt->snapshot_count != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        uint16_t count = evt->record_count;
        if (count > MAX_EVENT_RECORDS) count = MAX_EVENT_RECORDS;
        rc = _put_head(buf, cap, &off, DCU_698_TAG_ARRAY, (uint8_t)count);
        for (uint16_t i = 0; rc == 0 && i < count; i++) {
            const dcu_event_record_t *rec = &evt->records[i];
            rc = _put_head(buf, cap, &off, DCU_698_TAG_STRUCTURE, 3);
            if (rc == 0) {
                rc = _put_u32(buf, cap, &off, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, rec->index);
            }
            if (rc == 0) rc = _put_time(buf, cap, &off, rec->start_time);
            if (rc == 0) rc = _put_time(buf, cap, &off, rec->end_time);
        }
    } else if (attr_id == 14) {
        /* 表194：array{structure{最近发生时间, 最近结束时间}}，未知时间是 NULL。 */
        rc = _put_head(buf, cap, &off, DCU_698_TAG_ARRAY, 1);
        if (rc == 0) rc = _put_head(buf, cap, &off, DCU_698_TAG_STRUCTURE, 2);
        if (rc == 0) rc = _put_time(buf, cap, &off, evt->last_start_time);
        if (rc == 0) rc = _put_time(buf, cap, &off, evt->last_end_time);
    } else {
        return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }

    if (rc != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *len = off;
    return 0;
}

int dcu_698_event_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                          void *result) {
    (void)param;
    dcu_event_obj_t *evt = (dcu_event_obj_t *)obj->instance;
    if (!evt) return -1;

    if (method_id == 1) {
        evt->record_count = 0;
        memset(evt->records, 0, sizeof(evt->records));
        evt->last_start_time = 0;
        evt->last_end_time = 0;
        if (result) *(int *)result = 0;
        printf("🧹 [EVENT] ACTION: 成功复位事件 %04X 的历史事件记录表。\n", evt->oi);
        return 0;
    }
    return -2;
}
