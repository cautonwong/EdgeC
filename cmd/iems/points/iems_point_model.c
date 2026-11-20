#include "iems_point_model.h"

#include <string.h>

/* 槽位公式与 DataHub 1,024 槽实测一致，改公式必须同时改容量证据。 */
_Static_assert(IEMS_POINT_MAX_SLOTS == 1024, "slot math must stay at 1024");

const iems_point_name_entry_t g_iems_point_names[] = {
    /* EdgeC 14 条元数据短名；dense_idx 与 iems_tag_map_fn 的静态表一致。 */
    {.name = "VOLTAGE_PHASE_A", .tag_id = 1001, .dense_idx = 0},
    {.name = "CURRENT_PHASE_A", .tag_id = 1002, .dense_idx = 1},
    {.name = "GRID_POWER", .tag_id = 1003, .dense_idx = 2},
    {.name = "GRID_FREQUENCY", .tag_id = 1004, .dense_idx = 3},
    {.name = "ADC_RAW_DATA", .tag_id = 2001, .dense_idx = 4},
    {.name = "ADC_VOLTAGE_RMS", .tag_id = 2002, .dense_idx = 5},
    {.name = "ENERGY_ACTIVE_IMP", .tag_id = 3001, .dense_idx = 6},
    {.name = "BATTERY_SOC", .tag_id = 4001, .dense_idx = 7},
    {.name = "BATTERY_CURRENT", .tag_id = 4002, .dense_idx = 8},
    {.name = "BATTERY_POWER", .tag_id = 4003, .dense_idx = 9},
    {.name = "SOLAR_POWER", .tag_id = 8001, .dense_idx = 10},
    {.name = "SOLAR_VOLTS", .tag_id = 8003, .dense_idx = 11},
    {.name = "SOLAR_RELAY", .tag_id = 8004, .dense_idx = 12},
    {.name = "PCS_CONTROL", .tag_id = 8005, .dense_idx = 13},
    /* iems_v3 旧名样例：名字已确认，tag_id 待 B1-B4 模板/点表输入。
     * LC6..LC10.PCS.* 与 BMS2.* 是前缀类，不是可列表的完整名。 */
    {.name = "LC1.PCS.SmokeDetectStatus",
     .tag_id = IEMS_POINT_TAG_NONE,
     .dense_idx = IEMS_POINT_DENSE_NONE},
    {.name = "EMS.EMS.DcTotalVolUnusual",
     .tag_id = IEMS_POINT_TAG_NONE,
     .dense_idx = IEMS_POINT_DENSE_NONE},
    {.name = "EMS.ActivePower", .tag_id = IEMS_POINT_TAG_NONE, .dense_idx = IEMS_POINT_DENSE_NONE},
    {.name = "EMS.SetConnect", .tag_id = IEMS_POINT_TAG_NONE, .dense_idx = IEMS_POINT_DENSE_NONE},
    {.name = "PCS.AphaseApparentPower",
     .tag_id = IEMS_POINT_TAG_NONE,
     .dense_idx = IEMS_POINT_DENSE_NONE},
    {.name = "PCS_METER.AphaseActivePower",
     .tag_id = IEMS_POINT_TAG_NONE,
     .dense_idx = IEMS_POINT_DENSE_NONE},
    {.name = "EMS.EnIoProtect", .tag_id = IEMS_POINT_TAG_NONE, .dense_idx = IEMS_POINT_DENSE_NONE},
    {.name = "pcs456.constant vol_chg_vol_setting",
     .tag_id = IEMS_POINT_TAG_NONE,
     .dense_idx = IEMS_POINT_DENSE_NONE},
};
const size_t g_iems_point_names_len = sizeof(g_iems_point_names) / sizeof(g_iems_point_names[0]);

int iems_point_key_slot(const iems_point_key_t *key, uint32_t *slot) {
    if (!key || !slot) return -1;
    if (key->device_id >= IEMS_POINT_MAX_DEVICES) return -1;
    if (key->dense_idx >= IEMS_POINT_DENSE_PER_DEVICE) return -1;
    *slot = (uint32_t)key->device_id * IEMS_POINT_DENSE_PER_DEVICE + key->dense_idx;
    return 0;
}

int iems_point_lookup_name(const char *name, uint32_t *tag_id) {
    if (!name || !tag_id) return -1;
    for (size_t i = 0; i < g_iems_point_names_len; i++) {
        if (g_iems_point_names[i].name && strcmp(g_iems_point_names[i].name, name) == 0) {
            if (g_iems_point_names[i].tag_id == IEMS_POINT_TAG_NONE) return -1;
            *tag_id = g_iems_point_names[i].tag_id;
            return 0;
        }
    }
    return -1;
}

int iems_point_lookup_tag(uint32_t tag_id, const char **name) {
    if (!name || tag_id == IEMS_POINT_TAG_NONE) return -1;
    for (size_t i = 0; i < g_iems_point_names_len; i++) {
        if (g_iems_point_names[i].tag_id == tag_id) {
            *name = g_iems_point_names[i].name;
            return 0;
        }
    }
    return -1;
}

int iems_point_slot(uint32_t tag_id, uint32_t *slot) {
    if (!slot || tag_id == IEMS_POINT_TAG_NONE) return -1;
    for (size_t i = 0; i < g_iems_point_names_len; i++) {
        if (g_iems_point_names[i].tag_id != tag_id) continue;
        iems_point_key_t key = {.device_id = 0, .dense_idx = g_iems_point_names[i].dense_idx};
        return iems_point_key_slot(&key, slot);
    }
    return -1;
}
