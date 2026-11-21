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
    uint16_t oi;
    uint32_t shadow_oad;
    uint8_t type_tag;       /* 属性 2 的 A-XDR 标签 */
    uint8_t type_tag_attr4; /* 属性 4 的标签；0 = 不支持属性 4 */
} var_desc_t;

static const var_desc_t k_vars[] = {
    {0x0010, OAD_ACTIVE_ENERGY, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, DCU_698_TAG_LONG64_UNSIGNED},
    {0x0020, OAD_REACTIVE_ENERGY, DCU_698_TAG_DOUBLE_LONG_UNSIGNED, DCU_698_TAG_LONG64_UNSIGNED},
    {0x2000, OAD_VOLTAGE, DCU_698_TAG_FLOAT32, 0},
    {0x2001, OAD_CURRENT, DCU_698_TAG_FLOAT32, 0},
    {0x2004, OAD_ACTIVE_POWER_TOTAL, DCU_698_TAG_FLOAT32, 0},
    {0x200A, OAD_POWER_FACTOR_TOTAL, DCU_698_TAG_LONG, 0},
    {0x200F, OAD_GRID_FREQUENCY, DCU_698_TAG_LONG_UNSIGNED, 0},
};

static const var_desc_t *_find_var(uint16_t oi) {
    for (size_t i = 0; i < sizeof(k_vars) / sizeof(k_vars[0]); i++) {
        if (k_vars[i].oi == oi) return &k_vars[i];
    }
    return NULL;
}

int dcu_698_var_read_value(uint32_t oad, dcu_value_t *out) {
    if (!out) return -1;

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    const var_desc_t *desc = _find_var(oi);
    if (!desc) return -1;
    if (attr_id != 2 && !(attr_id == 4 && desc->type_tag_attr4 != 0)) return -1;

    const router_bus_ops_t *bus = dcu_698_object_model_bus_ops();
    if (!bus || !bus->read_shadow) return -1;

    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    if (bus->read_shadow(bus->ctx, desc->shadow_oad, 0, &msg) != 0) return -1;

    memset(out, 0, sizeof(*out));
    out->type_tag = (attr_id == 4) ? desc->type_tag_attr4 : desc->type_tag;
    out->value = msg.payload.val.f;
    /* 类型标签决定编码宽度；原始整数由阴影工程值截断得到，
     * 量纲换算仍归 dcu_driver_adapter，本文件不复制 scale 表。 */
    out->raw = (int64_t)msg.payload.val.f;
    out->ts_ms = msg.ts / 1000000ULL;
    out->quality = msg.quality;
    return 0;
}

typedef enum { VK_U16, VK_U32, VK_U8, VK_BOOL, VK_F32 } value_kind_t;

typedef struct {
    uint16_t oi;
    uint8_t attr_id;
    value_kind_t kind;
} scalar_desc_t;

static const scalar_desc_t k_scalars[] = {
    {OI_METER_ARCHIVE, 2, VK_U16},
    {OI_TASK_SCHEDULER, 2, VK_U16},
    {(uint16_t)((OAD_DAILY_FREEZE >> 16) & 0xFFFF), 2, VK_U16},
    {OI_CHANNEL_CONFIG, 2, VK_U32},
    {OI_CHANNEL_CONFIG, 3, VK_U8},
    {OI_VOLTAGE_DROP, 4, VK_U16},
    {OI_VOLTAGE_DROP, 5, VK_U16},
    {OI_VOLTAGE_DROP, 6, VK_F32},
    {OI_VOLTAGE_DROP, 8, VK_U8},
    {OI_VOLTAGE_DROP, 9, VK_BOOL},
    {OI_OVER_CURRENT, 4, VK_U16},
    {OI_OVER_CURRENT, 5, VK_U16},
    {OI_OVER_CURRENT, 6, VK_F32},
    {OI_OVER_CURRENT, 8, VK_U8},
    {OI_OVER_CURRENT, 9, VK_BOOL},
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
            return 1;
        case DCU_698_TAG_LONG:
        case DCU_698_TAG_LONG_UNSIGNED:
            return 2;
        case DCU_698_TAG_DOUBLE_LONG_UNSIGNED:
        case DCU_698_TAG_FLOAT32:
            return 4;
        case DCU_698_TAG_LONG64_UNSIGNED:
            return 8;
        default:
            return 0;
    }
}

int dcu_698_value_for_oad(uint32_t oad, dcu_value_t *out) {
    if (!out) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (dcu_698_var_read_value(oad, out) == 0) {
        out->width = dcu_698_tag_width(out->type_tag);
        return 0;
    }

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    const scalar_desc_t *desc = _find_scalar(oi, attr_id);
    if (!desc) {
        /* 属性存在但没有标量表示（数组/结构）时明确拒绝，不截断。 */
        const dcu_698_object_t *obj = dcu_698_find(oi);
        if (obj && dcu_mask_test(obj->attr_mask, attr_id)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }

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
    (void)elem_idx;
    if (!obj || !out_val) return -1;

    uint32_t oad = ((uint32_t)obj->oi << 16) | ((uint32_t)attr_id << 8);
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
