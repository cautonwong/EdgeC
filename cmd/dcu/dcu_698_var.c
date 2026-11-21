/**
 * @file dcu_698_var.c
 * @brief 698 变量类（Class 1/3/4/6）处理器（D1-04 从 dcu_698_object_model.c 迁出）
 */

#include "dcu_698_var.h"

#include <string.h>

#include "datahub.h"

typedef struct {
    uint16_t oi;
    uint32_t shadow_oad;
    uint8_t type_tag;
    bool supports_attr4; /* Class 1 的属性 2 与 4 都读同一个电能量 */
} var_desc_t;

static const var_desc_t k_vars[] = {
    {0x0010, OAD_ACTIVE_ENERGY, DCU_698_TAG_LONG64_UNSIGNED, true},
    {0x0020, OAD_REACTIVE_ENERGY, DCU_698_TAG_LONG64_UNSIGNED, true},
    {0x2000, OAD_VOLTAGE, DCU_698_TAG_FLOAT32, false},
    {0x2001, OAD_CURRENT, DCU_698_TAG_FLOAT32, false},
    {0x2004, OAD_ACTIVE_POWER_TOTAL, DCU_698_TAG_FLOAT32, false},
    /* 目录里 200F 是电网频率（Class 6），200A 是功率因数。现有
     * `OAD_GRID_FREQUENCY` 宏仍指向 200A——影子数据也写在那里；宏与目录的
     * 不一致属 D1-01 的核对范围，本单不改数据源。 */
    {0x200F, OAD_GRID_FREQUENCY, DCU_698_TAG_LONG_UNSIGNED, false},
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
    if (attr_id != 2 && !(attr_id == 4 && desc->supports_attr4)) return -1;

    const router_bus_ops_t *bus = dcu_698_object_model_bus_ops();
    if (!bus || !bus->read_shadow) return -1;

    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    if (bus->read_shadow(bus->ctx, desc->shadow_oad, 0, &msg) != 0) return -1;

    memset(out, 0, sizeof(*out));
    out->type_tag = desc->type_tag;
    out->value = msg.payload.val.f;
    /* 类型标签决定编码宽度；原始整数由阴影工程值截断得到，
     * 量纲换算仍归 dcu_driver_adapter，本文件不复制 scale 表。 */
    out->raw = (int64_t)msg.payload.val.f;
    out->ts_ms = msg.ts / 1000000ULL;
    out->quality = msg.quality;
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
        case DCU_698_TAG_LONG64_UNSIGNED:
            *(uint64_t *)out_val = (uint64_t)v.raw;
            return 0;
        case DCU_698_TAG_LONG_UNSIGNED:
            *(uint16_t *)out_val = (uint16_t)v.raw;
            return 0;
        default:
            return -1;
    }
}
