#include <stdio.h>
#include <string.h>

#include "dcu_698_axdr.h"
#include "dcu_698_object_internal.h"
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

static int _put_null(uint8_t *buf, size_t cap, size_t *off) {
    if (*off + 1 > cap) return -1;
    buf[(*off)++] = 0x00;
    return 0;
}

/* 没有墙钟就拒绝，不把单调时标或 0 编成公历日期。 */
static int _put_required_stamp(const dcu_event_stamp_t *stamp, uint8_t *buf, size_t cap,
                               size_t *off) {
    if (stamp->mono_ns == 0 || !dcu_698_epoch_is_civil(stamp->epoch_s)) return -1;
    return dcu_698_put_date_time_s(stamp->epoch_s, buf, cap, off);
}

/* 表49 的非法 date_time_s：年 0xFFFF，其余字段 0xFF。不由 _put_date_time_s 产生。 */
static int _put_invalid_stamp(uint8_t *buf, size_t cap, size_t *off) {
    if (*off + 8 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_DATE_TIME_S;
    for (int i = 0; i < 7; i++) buf[(*off)++] = 0xFF;
    return 0;
}

/* 表194 A6 结束时间没有 NULL 槽位：事件未结束编表49 非法时标；有墙钟但不可编码则拒绝。 */
static int _put_a6_end_stamp(const dcu_event_stamp_t *stamp, uint8_t *buf, size_t cap,
                             size_t *off) {
    if (stamp->mono_ns == 0) return _put_invalid_stamp(buf, cap, off);
    if (!dcu_698_epoch_is_civil(stamp->epoch_s)) return -1;
    return dcu_698_put_date_time_s(stamp->epoch_s, buf, cap, off);
}

/* 表194 A14 明确允许 NULL：未发生/未结束编 0x00。 */
static int _put_optional_stamp(const dcu_event_stamp_t *stamp, uint8_t *buf, size_t cap,
                               size_t *off) {
    if (stamp->mono_ns == 0) return _put_null(buf, cap, off);
    if (!dcu_698_epoch_is_civil(stamp->epoch_s)) return -1;
    return dcu_698_put_date_time_s(stamp->epoch_s, buf, cap, off);
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
    } else if (attr_id == 6) {
        /* 表194：事件记录表1 = array<structure{序号, 发生时间, 结束时间, 上报状态 array,
         * n×关联数据}>。表2-4 未实例化；上报状态没有按通道存储，编空数组。关联数据在
         * 生成记录时抓拍，取不到就编 NULL，不拿读取时的当前值冒充。 */
        uint16_t count = evt->record_count;
        if (count > MAX_EVENT_RECORDS) count = MAX_EVENT_RECORDS;
        uint8_t n = evt->snapshot_count;
        if (n > MAX_EVENT_SNAPSHOTS) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        rc = _put_head(buf, cap, &off, DCU_698_TAG_ARRAY, (uint8_t)count);
        for (uint16_t i = 0; rc == 0 && i < count; i++) {
            const dcu_event_record_t *rec = &evt->records[i];
            rc = _put_head(buf, cap, &off, DCU_698_TAG_STRUCTURE, (uint8_t)(4 + n));
            if (rc == 0) {
                rc = _put_u32(buf, cap, &off, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, rec->index);
            }
            if (rc == 0) rc = _put_required_stamp(&rec->start_time, buf, cap, &off);
            if (rc == 0) rc = _put_a6_end_stamp(&rec->end_time, buf, cap, &off);
            if (rc == 0) rc = _put_head(buf, cap, &off, DCU_698_TAG_ARRAY, 0);
            for (uint8_t k = 0; rc == 0 && k < n; k++) {
                uint8_t blob_len = rec->snapshot_len[k];
                if (blob_len == 0) {
                    rc = _put_null(buf, cap, &off);
                } else if (off + blob_len > cap) {
                    rc = -1;
                } else {
                    memcpy(buf + off, rec->snapshot[k], blob_len);
                    off += blob_len;
                }
            }
        }
    } else if (attr_id == 14) {
        /* 表194：时间状态记录表 = array<structure{最近一次发生时间, 最近一次结束时间}>。 */
        rc = _put_head(buf, cap, &off, DCU_698_TAG_ARRAY, 1);
        if (rc == 0) rc = _put_head(buf, cap, &off, DCU_698_TAG_STRUCTURE, 2);
        if (rc == 0) rc = _put_optional_stamp(&evt->last_start_time, buf, cap, &off);
        if (rc == 0) rc = _put_optional_stamp(&evt->last_end_time, buf, cap, &off);
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
        evt->last_start_time = (dcu_event_stamp_t){0};
        evt->last_end_time = (dcu_event_stamp_t){0};
        if (result) *(int *)result = 0;
        printf("🧹 [EVENT] ACTION: 成功复位事件 %04X 的历史事件记录表。\n",
               evt->oi);
        return 0;
    }
    return -2;
}
