#include "dcu_698_axdr.h"
#include "dcu_698_appconn.h"

#include <string.h>
#include <uv.h>

#include "dcu_698_object_internal.h"
#include "dcu_698_var.h"
#include "dcu_router.h"
#include "dlt698_codec.h"
#include "edge_log.h"
#include "edge_slave_plane.h"

/* ACTION-Request 的 TimeTag 可选：线格式要么没有这个字节，要么就是一个 0x00。
 * 真实 TimeTag 要做传输时效判定（6.1.1.5），本实现不做，按 DAR 7 拒绝，不静默忽略。 */
static int _time_tag_absent(const uint8_t *buf, size_t off, size_t len) {
    return off == len || (off + 1 == len && buf[off] == DLT698_NO_TIME_TAG);
}

/**
 * @brief 把 `dcu_value_t` 编成完整 A-XDR 值（含类型标签）。
 *
 * 缓冲由调用方给；任何类型放不进 `cap` 都返回 -1，不截断。
 */
static int _encode_axdr_value(const dcu_value_t* v, uint8_t* buf, size_t cap, size_t* len) {
    if (!v || !buf || !len) return -1;

    switch (v->type_tag) {
        case DCU_698_TAG_DOUBLE_LONG_UNSIGNED: {
            if (cap < 5) return -1;
            uint32_t x = (uint32_t)v->raw;
            buf[0] = DCU_698_TAG_DOUBLE_LONG_UNSIGNED;
            buf[1] = (uint8_t)(x >> 24);
            buf[2] = (uint8_t)(x >> 16);
            buf[3] = (uint8_t)(x >> 8);
            buf[4] = (uint8_t)x;
            *len = 5;
            return 0;
        }
        case DCU_698_TAG_LONG64_UNSIGNED: {
            if (cap < 9) return -1;
            uint64_t x = (uint64_t)v->raw;
            buf[0] = DCU_698_TAG_LONG64_UNSIGNED;
            for (int i = 1; i < 9; i++) buf[i] = (uint8_t)(x >> (8 * (8 - i)));
            *len = 9;
            return 0;
        }
        case DCU_698_TAG_LONG: {
            if (cap < 3) return -1;
            uint16_t x = (uint16_t)(int16_t)v->raw;
            buf[0] = DCU_698_TAG_LONG;
            buf[1] = (uint8_t)(x >> 8);
            buf[2] = (uint8_t)x;
            *len = 3;
            return 0;
        }
        case DCU_698_TAG_LONG_UNSIGNED: {
            if (cap < 3) return -1;
            uint16_t x = (uint16_t)v->raw;
            buf[0] = DCU_698_TAG_LONG_UNSIGNED;
            buf[1] = (uint8_t)(x >> 8);
            buf[2] = (uint8_t)x;
            *len = 3;
            return 0;
        }
        case DCU_698_TAG_FLOAT32: {
            if (cap < 5) return -1;
            float f = (float)v->value;
            uint32_t bits = 0;
            memcpy(&bits, &f, sizeof(bits));
            buf[0] = DCU_698_TAG_FLOAT32;
            buf[1] = (uint8_t)(bits >> 24);
            buf[2] = (uint8_t)(bits >> 16);
            buf[3] = (uint8_t)(bits >> 8);
            buf[4] = (uint8_t)bits;
            *len = 5;
            return 0;
        }
        case DCU_698_TAG_BOOLEAN:
        case DCU_698_TAG_ENUM:
        case DCU_698_TAG_UNSIGNED: {
            if (cap < 2) return -1;
            buf[0] = v->type_tag;
            buf[1] = (uint8_t)v->raw;
            *len = 2;
            return 0;
        }
        default:
            return -1;
    }
}

typedef struct {
    const uint8_t *p;
    size_t len;
    size_t off;
} axdr_cursor_t;

static bool _take(axdr_cursor_t *c, size_t n, const uint8_t **out) {
    if (c->off + n < c->off || c->off + n > c->len) return false;
    if (out) *out = c->p + c->off;
    c->off += n;
    return true;
}

static int _take_u8(axdr_cursor_t *c, uint8_t tag, uint8_t *out) {
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != tag) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (out) *out = p[1];
    return 0;
}

static int _take_u16(axdr_cursor_t *c, uint8_t tag, uint16_t *out) {
    const uint8_t *p = NULL;
    if (!_take(c, 3, &p) || p[0] != tag) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (out) *out = (uint16_t)((p[1] << 8) | p[2]);
    return 0;
}

/* 表 24：TI = 0x54 | unit | value_be16，没有内层标签。 */
static int _take_ti(axdr_cursor_t *c, uint8_t *unit, uint16_t *value) {
    const uint8_t *p = NULL;
    if (!_take(c, 4, &p) || p[0] != DLT698_TAG_TI) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (unit) *unit = p[1];
    if (value) *value = (uint16_t)((p[2] << 8) | p[3]);
    return 0;
}

static int _take_date_time(axdr_cursor_t *c, uint8_t out[7]) {
    const uint8_t *p = NULL;
    if (!_take(c, 8, &p) || p[0] != DLT698_TAG_DATE_TIME_S)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    memcpy(out, p + 1, 7);
    return 0;
}

static int _take_head(axdr_cursor_t *c, uint8_t tag, uint8_t count) {
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != tag || p[1] != count) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    return 0;
}

static int _take_array(axdr_cursor_t *c, uint8_t *count) {
    const uint8_t *p = NULL;
    if (!_take(c, 2, &p) || p[0] != DLT698_TAG_ARRAY) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *count = p[1];
    return 0;
}

static int _decode_task_unit(axdr_cursor_t *c, dcu_task_config_t *u) {
    uint8_t delay_unit = 0;
    uint16_t delay_value = 0;
    int rc = _take_head(c, DLT698_TAG_STRUCTURE, 12);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &u->task_id);
    if (rc == 0) rc = _take_ti(c, &u->interval_unit, &u->interval_value);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_ENUM, &u->scheme_type);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &u->scheme_id);
    if (rc == 0) rc = _take_date_time(c, u->start_time);
    if (rc == 0) rc = _take_date_time(c, u->end_time);
    if (rc == 0) rc = _take_ti(c, &delay_unit, &delay_value);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_UNSIGNED, &u->priority);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_ENUM, &u->status);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &u->pre_script);
    if (rc == 0) rc = _take_u16(c, DLT698_TAG_LONG_UNSIGNED, &u->post_script);
    if (rc != 0) return rc;

    /* 时段表：structure{enum type, array<4 字节时段>}。数量解码后用，存储拒绝非空。 */
    uint8_t count = 0;
    rc = _take_head(c, DLT698_TAG_STRUCTURE, 2);
    if (rc == 0) rc = _take_u8(c, DLT698_TAG_ENUM, &u->period_type);
    if (rc == 0) rc = _take_array(c, &count);
    if (rc == 0 && !_take(c, (size_t)count * 4u, NULL)) rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (rc != 0) return rc;

    u->delay_unit = delay_unit;
    u->delay_value = delay_value;
    u->period_count = count;

    /* 枚举越界是形状错误（7）；标准允许但产品不支持的取值留给 store 报 2。 */
    if (u->interval_unit > 5 || u->delay_unit > 5) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (u->scheme_type < 1 || u->scheme_type > 5) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (u->status != DCU_TASK_STATUS_NORMAL && u->status != DCU_TASK_STATUS_DISABLED)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (u->period_type > 3) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    return 0;
}

int dcu_698_decode_task_op(uint8_t method, const uint8_t *buf, size_t len, dcu_task_op_t *out) {
    if (!buf || !out || len == 0 || len > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    axdr_cursor_t c = {.p = buf, .len = len, .off = 0};
    memset(out, 0, sizeof(*out));
    out->method = method;

    uint8_t count = 0;
    int rc = 0;
    switch (method) {
        case 127:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > MAX_TASKS) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _decode_task_unit(&c, &out->units[i]);
                if (rc != 0) return rc;
                for (uint8_t j = 0; j < i; j++) {
                    if (out->units[j].task_id == out->units[i].task_id)
                        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
                }
            }
            out->unit_count = count;
            break;
        case 128:
            rc = _take_array(&c, &count);
            if (rc != 0) return rc;
            if (count == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (count > MAX_TASKS) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t i = 0; i < count; i++) {
                rc = _take_u8(&c, DLT698_TAG_UNSIGNED, &out->ids[i]);
                if (rc != 0) return rc;
            }
            out->id_count = count;
            break;
        case 129: {
            const uint8_t *p = NULL;
            if (!_take(&c, 1, &p) || p[0] != DLT698_TAG_NULL)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            break;
        }
        case 130: {
            rc = _take_head(&c, DLT698_TAG_STRUCTURE, 2);
            if (rc == 0) rc = _take_u8(&c, DLT698_TAG_UNSIGNED, &out->units[0].task_id);
            if (rc == 0) rc = _take_u8(&c, DLT698_TAG_ENUM, &out->units[0].status);
            if (rc != 0) return rc;
            if (out->units[0].status != DCU_TASK_STATUS_NORMAL &&
                out->units[0].status != DCU_TASK_STATUS_DISABLED) {
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            }
            out->unit_count = 1;
            break;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }

    return _time_tag_absent(buf, c.off, c.len) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

int dcu_698_accept_action_axdr(uint32_t omd, const uint8_t *axdr, size_t axdr_len) {
    const dcu_698_object_t *obj = NULL;
    int rc = dcu_698_check_call(omd, &obj);
    if (rc != 0) return rc;

    uint16_t oi = (uint16_t)(omd >> 16);
    uint8_t method = (uint8_t)((omd >> 8) & 0xFF);
    if (oi == OI_TASK_SCHEDULER && method == 1) {
        if (!axdr || axdr[0] != DLT698_TAG_NULL || !_time_tag_absent(axdr, 1, axdr_len))
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        return dcu_698_call(omd, NULL, NULL);
    }
    if (oi != OI_TASK_SCHEDULER) {
        /* 本票只定义 6012 任务的线格式；其它 OI 的处理器不读参数。 */
        return dcu_698_call(omd, NULL, NULL);
    }

    dcu_poller_t *poller = (dcu_poller_t *)obj->instance;
    if (!poller) return EDGE_SLAVE_SRC_TEMPORARY;
    rc = dcu_698_decode_task_op(method, axdr, axdr_len, &poller->task_op);
    if (rc != 0) return rc;
    return dcu_698_call(omd, &poller->task_op, NULL);
}

static int _put_tagged_u8(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint8_t v) {
    if (*off + 2 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = v;
    return 0;
}

static int _put_tagged_u16(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, uint16_t v) {
    if (*off + 3 > cap) return -1;
    buf[(*off)++] = tag;
    buf[(*off)++] = (uint8_t)(v >> 8);
    buf[(*off)++] = (uint8_t)v;
    return 0;
}

static int _put_ti(uint8_t *buf, size_t cap, size_t *off, uint8_t unit, uint16_t value) {
    if (*off + 4 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_TI;
    buf[(*off)++] = unit;
    buf[(*off)++] = (uint8_t)(value >> 8);
    buf[(*off)++] = (uint8_t)value;
    return 0;
}

static int _put_empty_time(uint8_t *buf, size_t cap, size_t *off) {
    if (*off + 8 > cap) return -1;
    buf[(*off)++] = DLT698_TAG_DATE_TIME_S;
    for (int i = 0; i < 7; i++) buf[(*off)++] = 0;
    return 0;
}

/* 6013 A2：12 字段结构，与解码顺序一一对应。 */
static int _encode_task_unit(uint8_t *buf, size_t cap, size_t *off,
                             const dcu_task_unit_t *t) {
    if (_put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 12) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, (uint8_t)t->task_id) != 0 ||
        _put_ti(buf, cap, off, t->interval_unit, t->interval_value) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_ENUM, t->scheme_type) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, (uint8_t)t->scheme_id) != 0 ||
        _put_empty_time(buf, cap, off) != 0 || _put_empty_time(buf, cap, off) != 0 ||
        _put_ti(buf, cap, off, 0, 0) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_UNSIGNED, 0) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_ENUM, t->status) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, 0) != 0 ||
        _put_tagged_u16(buf, cap, off, DLT698_TAG_LONG_UNSIGNED, 0) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_STRUCTURE, 2) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_ENUM, t->period_type) != 0 ||
        _put_tagged_u8(buf, cap, off, DLT698_TAG_ARRAY, 0) != 0) {
        return -1;
    }
    return 0;
}

static uint16_t _live_task_count(const dcu_poller_t *poller) {
    uint16_t n = 0;
    for (int i = 0; i < poller->task_count; i++) {
        if (poller->tasks[i].status != DCU_TASK_STATUS_FREE) n++;
    }
    return n;
}

static const dcu_task_unit_t *_next_task(const dcu_poller_t *poller, uint32_t after,
                                         bool have_after) {
    const dcu_task_unit_t *best = NULL;
    for (int i = 0; i < poller->task_count; i++) {
        const dcu_task_unit_t *t = &poller->tasks[i];
        if (t->status == DCU_TASK_STATUS_FREE) continue;
        if (have_after && t->task_id <= after) continue;
        if (!best || t->task_id < best->task_id) best = t;
    }
    return best;
}

int dcu_698_task_encode_axdr(const dcu_poller_t *poller, uint32_t oad, uint8_t *buf, size_t cap,
                             size_t *len) {
    if (!poller || !buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t elem = (uint8_t)(oad & 0xFF);
    size_t off = 0;

    if (oi == OI_TASK_SCHEDULER) {
        if (attr_id != 2) {
            return dcu_698_class_defines_attr(IC_ACQUISITION, attr_id)
                       ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                       : EDGE_SLAVE_SRC_UNDEFINED;
        }
        uint16_t count = _live_task_count(poller);
        if (count > 255) return EDGE_SLAVE_SRC_TEMPORARY;
        if (_put_tagged_u8(buf, cap, &off, DLT698_TAG_ARRAY, (uint8_t)count) != 0)
            return EDGE_SLAVE_SRC_TEMPORARY;
        uint32_t after = 0;
        bool have_after = false;
        for (uint16_t i = 0; i < count; i++) {
            const dcu_task_unit_t *t = _next_task(poller, after, have_after);
            if (!t) return EDGE_SLAVE_SRC_TEMPORARY;
            if (_encode_task_unit(buf, cap, &off, t) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            after = t->task_id;
            have_after = true;
        }
    } else if (oi == OI_TASK_UNIT) {
        if (attr_id != 2) {
            return dcu_698_class_defines_attr(IC_PARAM, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                                 : EDGE_SLAVE_SRC_UNDEFINED;
        }
        /* 元素索引是 task_id：0 表示最小现有 ID，绝不按数组位置寻址。 */
        const dcu_task_unit_t *t = elem ? dcu_poller_task_find(poller, elem)
                                        : _next_task(poller, 0, false);
        if (!t) return EDGE_SLAVE_SRC_NOT_FOUND;
        if (_encode_task_unit(buf, cap, &off, t) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    } else {
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }

    *len = off;
    return 0;
}

int dcu_north_resolve_axdr(void* ctx, uint32_t oad, uint8_t* buf, size_t cap, size_t* len) {
    (void)ctx;
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    /* Class 20 属性 2：对象列表是 ACL 的唯一来源（ADR-0015）。 */
    if (oad == 0x44000200u) {
        return dcu_698_appconn_object_list(buf, cap, len) == 0 ? 0
                                                               : EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    uint16_t oi = (uint16_t)(oad >> 16);
    if (dcu_698_var_is_oi(oi)) {
        return dcu_698_var_encode_axdr(oad, buf, cap, len);
    }
    if (oi == OI_METER_ARCHIVE || oi == OI_ARCHIVE_UNIT) {
        return dcu_698_archive_encode_axdr(dcu_698_internal_inventory(), oad, buf, cap, len);
    }
    const dcu_698_object_t *event_obj = dcu_698_find(oi);
    if (event_obj && event_obj->class_id == IC_SUB_EVENT) {
        return dcu_698_event_encode_axdr(event_obj, oad, buf, cap, len);
    }
    if (oi == OI_TASK_SCHEDULER || oi == OI_TASK_UNIT) {
        const dcu_698_object_t *task_obj = dcu_698_find(OI_TASK_SCHEDULER);
        dcu_poller_t *poller = task_obj ? (dcu_poller_t *)task_obj->instance : NULL;
        if (!poller) return EDGE_SLAVE_SRC_NOT_FOUND;
        return dcu_698_task_encode_axdr(poller, oad, buf, cap, len);
    }

    dcu_value_t v;
    int rc = dcu_698_value_for_oad(oad, &v);
    if (rc != 0) return rc;
    return (_encode_axdr_value(&v, buf, cap, len) == 0) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
}

/* 表 A.9：8000 的告警状态与命令状态都是 8 路继电器，一次操作最多 8 项。 */
#define ACTION_MAX_ITEMS 8

static int _decode_action_item(uint8_t method, const dlt698_data_t* item,
                               dcu_action_param_t* out) {
    uint8_t want = (method == 129) ? 4 : (method == 130) ? 2 : 3;
    if (item->tag != DLT698_TAG_STRUCTURE || item->val.container.count != want ||
        !item->val.container.items) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    const dlt698_data_t* f = item->val.container.items;
    if (f[0].tag != DLT698_TAG_OAD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    out->relay_oad = f[0].val.u32;

    if (method == 129) {
        if (f[1].tag != DLT698_TAG_UNSIGNED || f[2].tag != DLT698_TAG_LONG_UNSIGNED ||
            f[3].tag != DLT698_TAG_BOOLEAN) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        out->alarm_delay_min = f[1].val.u8;
        out->limit_minutes = f[2].val.u16;
        out->auto_close = f[3].val.u8 != 0;
        return 0;
    }

    if (f[1].tag != DLT698_TAG_ENUM) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    out->command = f[1].val.u8;
    if (method == 131 && f[2].tag != DLT698_TAG_VISIBLE_STRING) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    return 0;
}

int dcu_698_decode_action_param(uint8_t method, const uint8_t* buf, size_t len,
                                dcu_action_param_t* out) {
    if (!buf || !out || len > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint16_t off = 0;
    dlt698_data_t d;
    memset(&d, 0, sizeof(d));
    if (dlt698_decode_data(buf, (uint16_t)len, &off, &d) != 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (!_time_tag_absent(buf, off, len)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    if (method == DCU_ACTION_METHOD_FIRST || method == DCU_ACTION_METHOD_FIRST + 1) {
        return (d.tag == DLT698_TAG_NULL) ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (method < DCU_ACTION_METHOD_FIRST || method > DCU_ACTION_METHOD_LAST) {
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    if (d.tag != DLT698_TAG_ARRAY || !d.val.container.items ||
        d.val.container.count == 0 || d.val.container.count > ACTION_MAX_ITEMS) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    uint8_t count = (uint8_t)d.val.container.count;
    dcu_action_param_t first = {0};
    for (uint8_t i = 0; i < count; i++) {
        dcu_action_param_t item = {0};
        int rc = _decode_action_item(method, &d.val.container.items[i], &item);
        if (rc != 0) return rc;
        if (i == 0) first = item;
    }
    *out = first;
    out->count = count;
    return 0;
}

typedef enum { W_U8, W_BOOL } write_kind_t;

static int _write_kind_for(uint16_t oi, uint8_t attr_id, write_kind_t* kind) {
    if (!kind) return -1;
    if (oi == OI_VOLTAGE_DROP || oi == OI_OVER_CURRENT) {
        if (attr_id == 11) {
            *kind = W_U8;
            return 0;
        }
        if (attr_id == 12) {
            *kind = W_BOOL;
            return 0;
        }
    }
    return -1;
}

/* 处理器回 -2 表示值不合法（如枚举越界），如实报类型不匹配而非暂时失效。 */
static int _write_via_table(uint32_t oad, const void* value) {
    int rc = dcu_698_write(oad, value);
    if (rc == 0) return 0;
    return rc == -2 ? EDGE_SLAVE_SRC_TYPE_MISMATCH : EDGE_SLAVE_SRC_TEMPORARY;
}

/* 用 codec 的 A-XDR 解码器取类型与值：标签宽度只有这一处来源。 */
static int _decode_axdr_scalar(const uint8_t* p, size_t len, int64_t* i, double* f,
                               bool* is_float) {
    if (!p || !i || !f || !is_float || len == 0 || len > UINT16_MAX)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    dlt698_data_t d;
    memset(&d, 0, sizeof(d));
    uint16_t off = 0;
    if (dlt698_decode_data(p, (uint16_t)len, &off, &d) != 0 || off != len) {
        dlt698_free_data(&d);
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    *is_float = false;
    int rc = 0;
    switch (d.tag) {
        case DLT698_TAG_BOOLEAN:
        case DLT698_TAG_UNSIGNED:
        case DLT698_TAG_ENUM:
            *i = d.val.u8;
            break;
        case DLT698_TAG_INTEGER:
            *i = d.val.i8;
            break;
        case DLT698_TAG_LONG:
            *i = d.val.i16;
            break;
        case DLT698_TAG_LONG_UNSIGNED:
            *i = d.val.u16;
            break;
        case DLT698_TAG_DOUBLE_LONG:
            *i = d.val.i32;
            break;
        case DLT698_TAG_DOUBLE_LONG_UNSIGNED:
            *i = d.val.u32;
            break;
        case DLT698_TAG_LONG64:
            *i = d.val.i64;
            break;
        case DLT698_TAG_LONG64_UNSIGNED:
            *i = (int64_t)d.val.u64;
            break;
        case DLT698_TAG_FLOAT32:
            *f = d.val.f32;
            *is_float = true;
            break;
        case DLT698_TAG_FLOAT64:
            *f = d.val.f64;
            *is_float = true;
            break;
        default:
            rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;
            break;
    }
    dlt698_free_data(&d);
    return rc;
}

int dcu_north_accept_axdr(void* ctx, uint32_t oad, const uint8_t* axdr, size_t len) {
    (void)ctx;
    if (!axdr || len == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    const dcu_698_object_t* obj = dcu_698_find(oi);
    if (!obj) return EDGE_SLAVE_SRC_NOT_FOUND;
    if (!dcu_mask_test(obj->attr_mask, attr_id))
        return dcu_698_class_defines_attr(obj->class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                                  : EDGE_SLAVE_SRC_UNDEFINED;
    if (!obj->write) return EDGE_SLAVE_SRC_DENIED;

    write_kind_t kind;
    if (_write_kind_for(oi, attr_id, &kind) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    int64_t i = 0;
    double f = 0.0;
    bool is_float = false;
    int rc = _decode_axdr_scalar(axdr, len, &i, &f, &is_float);
    if (rc != 0) return rc;

    switch (kind) {
        case W_U8: {
            if (is_float || i < 0 || i > UINT8_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            uint8_t value = (uint8_t)i;
            return _write_via_table(oad, &value);
        }
        case W_BOOL: {
            if (is_float || (i != 0 && i != 1)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            bool value = (i != 0);
            return _write_via_table(oad, &value);
        }
        default:
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
}

static int _octets_text(const dlt698_data_t* d, char* out, size_t cap) {
    if (d->tag != DLT698_TAG_OCTET_STRING || !out || cap == 0) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    size_t n = d->val.str.len;
    if (n >= cap) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (n > 0) memcpy(out, d->val.str.ptr, n);
    out[n] = '\0';
    return 0;
}

static int _decode_basic(const dlt698_data_t* d, dcu_archive_unit_t* unit) {
    if (d->tag != DLT698_TAG_STRUCTURE || d->val.container.count != 10 ||
        !d->val.container.items) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    const dlt698_data_t* f = d->val.container.items;
    int rc = _octets_text(&f[0], unit->address, sizeof(unit->address));
    if (rc != 0) return rc;
    if (f[1].tag != DLT698_TAG_ENUM || f[2].tag != DLT698_TAG_ENUM ||
        f[3].tag != DLT698_TAG_OAD || f[5].tag != DLT698_TAG_UNSIGNED ||
        f[6].tag != DLT698_TAG_UNSIGNED || f[7].tag != DLT698_TAG_ENUM ||
        f[8].tag != DLT698_TAG_LONG_UNSIGNED || f[9].tag != DLT698_TAG_LONG_UNSIGNED) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    unit->baud_code = f[1].val.u8;
    unit->protocol_code = f[2].val.u8;
    unit->port = (uint8_t)(f[3].val.u32 & 0xFF);
    rc = _octets_text(&f[4], unit->password, sizeof(unit->password));
    if (rc != 0) return rc;
    unit->tariff_count = f[5].val.u8;
    unit->user_type = f[6].val.u8;
    unit->wiring_mode = f[7].val.u8;
    unit->rated_voltage = f[8].val.u16;
    unit->rated_current = f[9].val.u16;
    return 0;
}

static int _decode_extended(const dlt698_data_t* d, dcu_archive_unit_t* unit) {
    if (d->tag != DLT698_TAG_STRUCTURE || d->val.container.count != 4 ||
        !d->val.container.items) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    const dlt698_data_t* f = d->val.container.items;
    if (f[2].tag != DLT698_TAG_LONG_UNSIGNED || f[3].tag != DLT698_TAG_LONG_UNSIGNED) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    int rc = _octets_text(&f[0], unit->collector_addr, sizeof(unit->collector_addr));
    if (rc != 0) return rc;
    rc = _octets_text(&f[1], unit->asset_no, sizeof(unit->asset_no));
    if (rc != 0) return rc;
    unit->pt_ratio = f[2].val.u16;
    unit->ct_ratio = f[3].val.u16;
    return 0;
}

static int _decode_annex(const dlt698_data_t* d, bool* has_annex) {
    if (d->tag != DLT698_TAG_ARRAY) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    *has_annex = d->val.container.count > 0;
    return 0;
}

/* 6001 A2：structure{配置序号, Basic, Extended, Annex}。 */
static int _decode_unit(const dlt698_data_t* d, dcu_archive_unit_t* unit, bool* has_annex) {
    if (d->tag != DLT698_TAG_STRUCTURE || d->val.container.count != 4 ||
        !d->val.container.items) {
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    const dlt698_data_t* f = d->val.container.items;
    if (f[0].tag != DLT698_TAG_LONG_UNSIGNED) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    unit->config_seq = f[0].val.u16;
    int rc = _decode_basic(&f[1], unit);
    if (rc != 0) return rc;
    rc = _decode_extended(&f[2], unit);
    if (rc != 0) return rc;
    return _decode_annex(&f[3], has_annex);
}

int dcu_698_accept_archive_action(uint32_t omd, const uint8_t* axdr, size_t axdr_len) {
    uint8_t method = (uint8_t)((omd >> 8) & 0xFF);
    dcu_archive_op_t op;
    int rc = dcu_698_decode_archive_op(method, axdr, axdr_len, &op);
    if (rc != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU north ACTION OI=6000 method=%u：参数解码失败 rc=%d", method, rc);
        return rc;
    }
    return dcu_698_call(omd, &op, NULL);
}

int dcu_698_decode_archive_op(uint8_t method, const uint8_t* buf, size_t len,
                              dcu_archive_op_t* out) {
    if (!buf || !out || len == 0 || len > UINT16_MAX) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint16_t off = 0;
    dlt698_data_t d;
    memset(&d, 0, sizeof(d));
    if (dlt698_decode_data(buf, (uint16_t)len, &off, &d) != 0) {
        dlt698_free_data(&d);
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (!_time_tag_absent(buf, off, len)) {
        dlt698_free_data(&d);
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }

    memset(out, 0, sizeof(*out));
    out->method = method;
    int rc = EDGE_SLAVE_SRC_TYPE_MISMATCH;

    switch (method) {
        case 127:
            if (d.tag == DLT698_TAG_STRUCTURE) {
                rc = _decode_unit(&d, &out->units[0], &out->has_annex);
                if (rc == 0) out->unit_count = 1;
            }
            break;
        case 128:
            if (d.tag == DLT698_TAG_ARRAY && d.val.container.count > 0 &&
                d.val.container.count <= DCU_ARCHIVE_BATCH_MAX && d.val.container.items) {
                rc = 0;
                for (uint16_t i = 0; i < d.val.container.count; i++) {
                    bool annex = false;
                    rc = _decode_unit(&d.val.container.items[i], &out->units[i], &annex);
                    if (rc != 0) break;
                    out->has_annex = out->has_annex || annex;
                }
                if (rc == 0) out->unit_count = d.val.container.count;
            }
            break;
        case 129: {
            /* 参数 structure{配置序号, 基本信息}。 */
            if (d.tag == DLT698_TAG_STRUCTURE && d.val.container.count == 2 &&
                d.val.container.items &&
                d.val.container.items[0].tag == DLT698_TAG_LONG_UNSIGNED) {
                out->config_seq = d.val.container.items[0].val.u16;
                rc = _decode_basic(&d.val.container.items[1], &out->units[0]);
                if (rc == 0) out->unit_count = 1;
            }
            break;
        }
        case 130: {
            /* 参数 structure{配置序号, 扩展信息, 附属信息}。 */
            if (d.tag == DLT698_TAG_STRUCTURE && d.val.container.count == 3 &&
                d.val.container.items &&
                d.val.container.items[0].tag == DLT698_TAG_LONG_UNSIGNED) {
                out->config_seq = d.val.container.items[0].val.u16;
                rc = _decode_extended(&d.val.container.items[1], &out->units[0]);
                if (rc == 0) rc = _decode_annex(&d.val.container.items[2], &out->has_annex);
                if (rc == 0) out->unit_count = 1;
            }
            break;
        }
        case 131:
            if (d.tag == DLT698_TAG_LONG_UNSIGNED) {
                out->config_seq = d.val.u16;
                rc = 0;
            }
            break;
        case 132:
            rc = _decode_basic(&d, &out->units[0]);
            if (rc == 0) out->unit_count = 1;
            break;
        case 133: {
            /* 参数 structure{通信地址, 端口号}。 */
            if (d.tag == DLT698_TAG_STRUCTURE && d.val.container.count == 2 &&
                d.val.container.items &&
                d.val.container.items[1].tag == DLT698_TAG_OAD) {
                rc = _octets_text(&d.val.container.items[0], out->units[0].address,
                                  sizeof(out->units[0].address));
                out->units[0].port = (uint8_t)(d.val.container.items[1].val.u32 & 0xFF);
                if (rc == 0) out->unit_count = 1;
            }
            break;
        }
        case 134:
            if (d.tag == DLT698_TAG_NULL) rc = 0;
            break;
        default:
            rc = EDGE_SLAVE_SRC_CLASS_MISMATCH;
            break;
    }

    dlt698_free_data(&d);
    return rc;
}
