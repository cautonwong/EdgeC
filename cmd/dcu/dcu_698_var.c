/**
 * @file dcu_698_var.c
 * @brief 698 变量类（Class 1/3/4/6）处理器
 */

#include "dcu_698_var.h"

#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "dcu_router.h"
#include "edge_slave_plane.h"

typedef struct {
    uint32_t tag;
    const char *name; /* 仅 attr2 元素有北向名；attr4 元素不参与命名 */
} var_element_t;

typedef struct {
    uint8_t count;
    uint8_t tag;
    int8_t scaler;
    uint8_t unit;
    var_element_t elements[4];
} var_shape_t;

typedef struct {
    uint16_t oi;
    uint8_t class_id;
    var_shape_t attr2;
    var_shape_t attr4; /* tag == 0 表示没有高精度属性 */
} var_desc_t;

/* 名字与 IEMS 的 edge_tag_meta_t.name 同风格（大写蛇形），云端字段名不随设备变形。 */
static const var_desc_t k_vars[] = {
    {0x0010, 1,
     {1, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, -2, 33, {{OAD_ACTIVE_ENERGY, "ACTIVE_ENERGY"}}},
     {1, DCU_698_TAG_LONG64_UNSIGNED, -4, 33, {{OAD_ACTIVE_ENERGY, NULL}}}},
    {0x0020, 1,
     {1, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, -2, 33, {{OAD_REACTIVE_ENERGY, "REACTIVE_ENERGY"}}},
     {1, DCU_698_TAG_LONG64_UNSIGNED, -4, 33, {{OAD_REACTIVE_ENERGY, NULL}}}},
    {0x2000, 3,
     {3, DCU_698_TAG_LONG_UNSIGNED, -1, 38,
      {{OAD_VOLTAGE, "VOLTAGE_A"}, {TAG_VOLTAGE_B, "VOLTAGE_B"}, {TAG_VOLTAGE_C, "VOLTAGE_C"}}},
     {0}},
    {0x2001, 3,
     {3, DCU_698_TAG_DOUBLE_LONG, -3, 36,
      {{OAD_CURRENT, "CURRENT_A"}, {TAG_CURRENT_B, "CURRENT_B"}, {TAG_CURRENT_C, "CURRENT_C"}}},
     {0}},
    {0x2004, 4,
     {4, DCU_698_TAG_DOUBLE_LONG, -1, 27,
      {{OAD_ACTIVE_POWER_TOTAL, "ACTIVE_POWER_TOTAL"},
       {TAG_ACTIVE_POWER_A, "ACTIVE_POWER_A"},
       {TAG_ACTIVE_POWER_B, "ACTIVE_POWER_B"},
       {TAG_ACTIVE_POWER_C, "ACTIVE_POWER_C"}}},
     {0}},
    {0x200A, 4, {1, DCU_698_TAG_LONG, -3, 0, {{OAD_POWER_FACTOR_TOTAL, "POWER_FACTOR_TOTAL"}}}, {0}},
    {0x200F, 6,
     {1, DCU_698_TAG_LONG_UNSIGNED, -2, 47, {{OAD_GRID_FREQUENCY, "GRID_FREQUENCY"}}},
     {0}},
};

static const var_desc_t *_find_var(uint16_t oi) {
    for (size_t i = 0; i < sizeof(k_vars) / sizeof(k_vars[0]); i++) {
        if (k_vars[i].oi == oi) return &k_vars[i];
    }
    return NULL;
}

bool dcu_698_var_is_oi(uint16_t oi) { return _find_var(oi) != NULL; }

size_t dcu_698_var_poll_tags(uint32_t *out, size_t cap) {
    if (!out) return 0;
    size_t n = 0;
    for (size_t i = 0; i < sizeof(k_vars) / sizeof(k_vars[0]); i++) {
        for (uint8_t e = 0; e < k_vars[i].attr2.count && n < cap; e++) {
            out[n++] = k_vars[i].attr2.elements[e].tag;
        }
    }
    return n;
}

const char *dcu_698_var_metric_name(uint32_t tag) {
    for (size_t i = 0; i < sizeof(k_vars) / sizeof(k_vars[0]); i++) {
        const var_shape_t *shape = &k_vars[i].attr2;
        for (uint8_t e = 0; e < shape->count; e++) {
            if (shape->elements[e].tag == tag) return shape->elements[e].name;
        }
    }
    return NULL;
}

int dcu_698_var_metric_resolve(const char *name, uint32_t *tag) {
    if (!name || !tag) return -1;
    for (size_t i = 0; i < sizeof(k_vars) / sizeof(k_vars[0]); i++) {
        const var_shape_t *shape = &k_vars[i].attr2;
        for (uint8_t e = 0; e < shape->count; e++) {
            if (shape->elements[e].name && strcmp(shape->elements[e].name, name) == 0) {
                *tag = shape->elements[e].tag;
                return 0;
            }
        }
    }
    return -1;
}

/* 工程值 → 标准整数（raw = value * 10^-scaler），四舍五入，不截断。 */
static int64_t _scale_to_raw(double value, int8_t scaler) {
    double factor = 1.0;
    for (int i = 0; i < -scaler; i++) factor *= 10.0;
    for (int i = 0; i < scaler; i++) factor /= 10.0;
    double x = value * factor;
    return (int64_t)(x >= 0 ? x + 0.5 : x - 0.5);
}

static int _read_element(const var_shape_t *shape, uint32_t device_id, uint8_t elem, int64_t *raw,
                         double *value, uint64_t *ts_ms, uint8_t *quality) {
    if (!shape || !raw || !value || elem >= shape->count) return -1;
    const router_bus_ops_t *bus = dcu_698_object_model_bus_ops();
    if (!bus || !bus->read_shadow) return -1;

    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    if (bus->read_shadow(bus->ctx, shape->elements[elem].tag, device_id, &msg) != 0) return -1;
    *value = msg.payload.val.f;
    *raw = _scale_to_raw(*value, shape->scaler);
    *ts_ms = msg.ts / 1000000ULL;
    if (quality) *quality = msg.quality;
    return 0;
}

int dcu_698_var_read_value(uint32_t oad, dcu_value_t *out) {
    if (!out) return -1;

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    uint8_t slot = (uint8_t)(oad & 0xFF);
    const var_desc_t *desc = _find_var(oi);
    /* 只有 Class 6 的属性 2 是单值；Class 1/3/4 的属性 2 是数组，不走标量端口。 */
    if (!desc || desc->class_id != 6 || attr_id != 2) return -1;

    int64_t raw = 0;
    double value = 0.0;
    uint64_t ts_ms = 0;
    uint8_t quality = 0;
    if (_read_element(&desc->attr2, slot, 0, &raw, &value, &ts_ms, &quality) != 0) return -1;

    memset(out, 0, sizeof(*out));
    out->type_tag = desc->attr2.tag;
    out->value = value;
    out->raw = raw;
    out->ts_ms = ts_ms;
    out->quality = quality;
    out->width = dcu_698_tag_width(out->type_tag);
    return 0;
}

typedef enum { VK_U16, VK_U32, VK_U8, VK_BOOL, VK_ENUM, VK_F32 } value_kind_t;

typedef struct {
    uint16_t oi;
    uint8_t attr_id;
    value_kind_t kind;
} scalar_desc_t;

static const scalar_desc_t k_scalars[] = {
    {OI_METER_DISCOVERY, 3, VK_U16}, {OI_METER_DISCOVERY, 6, VK_U16},
    {OI_METER_DISCOVERY, 10, VK_ENUM},
    {OI_VOLTAGE_DROP, 4, VK_U16},    {OI_VOLTAGE_DROP, 11, VK_ENUM},
    {OI_VOLTAGE_DROP, 12, VK_BOOL},  {OI_OVER_CURRENT, 4, VK_U16},
    {OI_OVER_CURRENT, 11, VK_ENUM},  {OI_OVER_CURRENT, 12, VK_BOOL},
};

static const scalar_desc_t *_find_scalar(uint16_t oi, uint8_t attr_id) {
    for (size_t i = 0; i < sizeof(k_scalars) / sizeof(k_scalars[0]); i++) {
        if (k_scalars[i].oi == oi && k_scalars[i].attr_id == attr_id) return &k_scalars[i];
    }
    return NULL;
}

uint8_t dcu_698_tag_width(uint8_t tag) {
    switch (tag) {
        case DCU_698_TAG_BOOLEAN:
        case DCU_698_TAG_UNSIGNED:
        case DCU_698_TAG_ENUM:
            return 1;
        case DCU_698_TAG_LONG:
        case DCU_698_TAG_LONG_UNSIGNED:
            return 2;
        case DCU_698_TAG_DOUBLE_LONG:
        case DCU_698_TAG_DOUBLE_LONG_UNSIGNED:
        case DCU_698_TAG_FLOAT32:
            return 4;
        case DCU_698_TAG_LONG64_UNSIGNED:
            return 8;
        default:
            return 0;
    }
}

static int _put_element(uint8_t *buf, size_t cap, size_t *off, uint8_t tag, int64_t raw) {
    size_t need = 1 + dcu_698_tag_width(tag);
    if (!buf || !off || need <= 1 || *off + need > cap) return -1;
    uint8_t *p = buf + *off;
    p[0] = tag;
    switch (tag) {
        case DCU_698_TAG_LONG_UNSIGNED: {
            uint16_t x = (uint16_t)raw;
            p[1] = (uint8_t)(x >> 8);
            p[2] = (uint8_t)x;
            break;
        }
        case DCU_698_TAG_LONG: {
            uint16_t x = (uint16_t)(int16_t)raw;
            p[1] = (uint8_t)(x >> 8);
            p[2] = (uint8_t)x;
            break;
        }
        case DCU_698_TAG_DOUBLE_LONG:
        case DCU_698_TAG_DOUBLE_LONG_UNSIGNED: {
            uint32_t x = (uint32_t)raw;
            p[1] = (uint8_t)(x >> 24);
            p[2] = (uint8_t)(x >> 16);
            p[3] = (uint8_t)(x >> 8);
            p[4] = (uint8_t)x;
            break;
        }
        case DCU_698_TAG_LONG64_UNSIGNED: {
            uint64_t x = (uint64_t)raw;
            for (int i = 1; i < 9; i++) p[i] = (uint8_t)(x >> (8 * (8 - i)));
            break;
        }
        case DCU_698_TAG_UNSIGNED:
        case DCU_698_TAG_ENUM:
            p[1] = (uint8_t)raw;
            break;
        default:
            return -1;
    }
    *off += need;
    return 0;
}

static int _put_scaler_unit(uint8_t *buf, size_t cap, size_t *off, int8_t scaler, uint8_t unit) {
    if (*off + 6 > cap) return -1;
    buf[(*off)++] = DCU_698_TAG_STRUCTURE;
    buf[(*off)++] = 2;
    buf[(*off)++] = DCU_698_TAG_INTEGER;
    buf[(*off)++] = (uint8_t)scaler;
    buf[(*off)++] = DCU_698_TAG_ENUM;
    buf[(*off)++] = unit;
    return 0;
}

static int _var_encode_axdr(uint32_t oad, uint32_t device_id, uint8_t *buf, size_t cap,
                            size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    const var_desc_t *desc = _find_var(oi);
    if (!desc) return EDGE_SLAVE_SRC_NOT_FOUND;

    const var_shape_t *shape = NULL;
    if (attr_id == 2) {
        shape = &desc->attr2;
    } else if (attr_id == 4 && desc->attr4.tag != 0) {
        shape = &desc->attr4;
    } else if (attr_id == 3 || attr_id == 5) {
        const var_shape_t *s = (attr_id == 3) ? &desc->attr2 : &desc->attr4;
        if (!s->tag) return EDGE_SLAVE_SRC_NOT_FOUND;
        size_t off = 0;
        if (_put_scaler_unit(buf, cap, &off, s->scaler, s->unit) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        *len = off;
        return 0;
    } else {
        /* 变量类按类表分类：已定义未实现 -> DAR 5，类表外 -> DAR 4。 */
        return dcu_698_oi_defines_attr(oi, desc->class_id, attr_id)
                   ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                   : EDGE_SLAVE_SRC_UNDEFINED;
    }

    size_t off = 0;
    if (desc->class_id == 6) {
        int64_t raw = 0;
        double value = 0.0;
        uint64_t ts_ms = 0;
        if (_read_element(shape, device_id, 0, &raw, &value, &ts_ms, NULL) != 0) {
            return EDGE_SLAVE_SRC_NOT_FOUND;
        }
        if (_put_element(buf, cap, &off, shape->tag, raw) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        *len = off;
        return 0;
    }

    if (off + 2 > cap) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    buf[off++] = DCU_698_TAG_ARRAY;
    buf[off++] = shape->count;
    for (uint8_t i = 0; i < shape->count; i++) {
        int64_t raw = 0;
        double value = 0.0;
        uint64_t ts_ms = 0;
        if (_read_element(shape, device_id, i, &raw, &value, &ts_ms, NULL) != 0) {
            return EDGE_SLAVE_SRC_NOT_FOUND;
        }
        if (_put_element(buf, cap, &off, shape->tag, raw) != 0) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
    }
    *len = off;
    return 0;
}

int dcu_698_var_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    return _var_encode_axdr(oad, (uint8_t)(oad & 0xFF), buf, cap, len);
}

int dcu_698_var_encode_axdr_device(uint32_t oad, uint32_t device_id, uint8_t *buf, size_t cap,
                                   size_t *len) {
    return _var_encode_axdr(oad, device_id, buf, cap, len);
}

static int _unadvertised_attr_rc(const dcu_698_object_t *obj, uint8_t attr_id) {
    return dcu_698_oi_defines_attr(obj->oi, obj->class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                                    : EDGE_SLAVE_SRC_UNDEFINED;
}

int dcu_698_value_for_oad(uint32_t oad, dcu_value_t *out) {
    if (!out) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    if (dcu_698_var_is_oi(oi)) {
        if (dcu_698_var_read_value(oad, out) == 0) return 0;
        /* A2..A5 是数组/结构，标量通道回 DAR 7；其余按类定义回 DAR 5/4。 */
        if (attr_id >= 2 && attr_id <= 5) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        const dcu_698_object_t *var_obj = dcu_698_find(oi);
        return var_obj ? _unadvertised_attr_rc(var_obj, attr_id) : EDGE_SLAVE_SRC_NOT_FOUND;
    }

    const dcu_698_object_t *obj = dcu_698_find(oi);
    const scalar_desc_t *desc = _find_scalar(oi, attr_id);
    if (!desc) {
        /* 属性存在但没有标量表示（数组/结构）时明确拒绝，不截断。 */
        if (!obj) return EDGE_SLAVE_SRC_NOT_FOUND;
        if (dcu_mask_test(obj->attr_mask, attr_id)) {
            if (oi == OI_ETHERNET_COMM_FIRST && attr_id == 5) return EDGE_SLAVE_SRC_CLASS_MISMATCH;
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        return _unadvertised_attr_rc(obj, attr_id);
    }
    if (obj && !dcu_mask_test(obj->attr_mask, attr_id)) return _unadvertised_attr_rc(obj, attr_id);

    memset(out, 0, sizeof(*out));
    switch (desc->kind) {
        case VK_U16: {
            uint16_t x = 0;
            if (dcu_698_read(oad, &x) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
            out->type_tag = DCU_698_TAG_LONG_UNSIGNED;
            out->raw = x;
            out->value = (double)x;
            break;
        }
        case VK_U32: {
            uint32_t x = 0;
            if (dcu_698_read(oad, &x) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
            out->type_tag = DCU_698_TAG_DOUBLE_LONG_UNSIGNED;
            out->raw = x;
            out->value = (double)x;
            break;
        }
        case VK_U8: {
            uint8_t x = 0;
            if (dcu_698_read(oad, &x) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
            out->type_tag = DCU_698_TAG_UNSIGNED;
            out->raw = x;
            out->value = (double)x;
            break;
        }
        case VK_ENUM: {
            uint8_t x = 0;
            if (dcu_698_read(oad, &x) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
            out->type_tag = DCU_698_TAG_ENUM;
            out->raw = x;
            out->value = (double)x;
            break;
        }
        case VK_BOOL: {
            bool x = false;
            if (dcu_698_read(oad, &x) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
            out->type_tag = DCU_698_TAG_BOOLEAN;
            out->raw = x ? 1 : 0;
            out->value = x ? 1.0 : 0.0;
            break;
        }
        case VK_F32: {
            float x = 0.0f;
            if (dcu_698_read(oad, &x) != 0) return EDGE_SLAVE_SRC_NOT_FOUND;
            out->type_tag = DCU_698_TAG_FLOAT32;
            out->raw = (int64_t)x;
            out->value = (double)x;
            break;
        }
        default:
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    out->width = dcu_698_tag_width(out->type_tag);
    out->ts_ms = (uint64_t)(uv_hrtime() / 1000000ULL);
    return 0;
}

int dcu_698_var_read(const dcu_698_object_t *obj, uint8_t attr_id, uint8_t elem_idx,
                     void *out_val) {
    if (!obj || !out_val) return -1;

    uint32_t oad = ((uint32_t)obj->oi << 16) | ((uint32_t)attr_id << 8) | elem_idx;
    dcu_value_t v;
    if (dcu_698_var_read_value(oad, &v) != 0) return -1;

    switch (v.type_tag) {
        case DCU_698_TAG_FLOAT32:
            *(float *)out_val = (float)v.value;
            return 0;
        case DCU_698_TAG_DOUBLE_LONG_UNSIGNED:
            *(uint32_t *)out_val = (uint32_t)v.raw;
            return 0;
        case DCU_698_TAG_LONG64_UNSIGNED:
            *(uint64_t *)out_val = (uint64_t)v.raw;
            return 0;
        case DCU_698_TAG_LONG:
            *(int16_t *)out_val = (int16_t)v.raw;
            return 0;
        case DCU_698_TAG_LONG_UNSIGNED:
            *(uint16_t *)out_val = (uint16_t)v.raw;
            return 0;
        default:
            return -1;
    }
}
