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
    /* ACTION-Request 的 Data 后允许一个时间标签字节。 */
    if ((size_t)off + 1 < len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

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
    if ((size_t)off + 1 < len) {
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
