/**
 * @file smartbox_driver_adapter.c
 * @brief SmartBox 智能盒子驱动适配层与本地点表实现 (No-Contract)
 */

#include "smartbox_driver_adapter.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "contract_data_types.h"

/* 外部协议驱动模板声明 */
extern const edge_driver_t modbus_rtu_driver;
extern const edge_driver_t modbus_tcp_driver;
extern const edge_driver_t can_driver;

/* ==============================================================================
 * 1. SmartBox 专属静态驱动注册表
 * ============================================================================== */
static const edge_driver_reg_entry_t s_smartbox_driver_registry[] = {
    EDGE_DRIVER_ENTRY("ModbusRTU", modbus_rtu_driver),
    EDGE_DRIVER_ENTRY("ModbusTCP", modbus_tcp_driver),
    EDGE_DRIVER_ENTRY("CAN", can_driver),
};

static const size_t s_smartbox_driver_registry_count =
    sizeof(s_smartbox_driver_registry) / sizeof(s_smartbox_driver_registry[0]);

int smartbox_driver_registry_init(driver_manager_t *mgr) {
    if (!mgr) return -1;
    return driver_manager_register_table(mgr, s_smartbox_driver_registry, s_smartbox_driver_registry_count);
}

/* ==============================================================================
 * 2. SmartBox 测点到 DataHub 影子槽位 OCP 表驱动映射
 * ============================================================================== */
typedef struct {
    uint32_t tag_id;
    uint32_t offset;
} smartbox_tag_slot_t;

static const smartbox_tag_slot_t s_smartbox_tag_slots[] = {
    {TAG_BOX_SYSTEM_STATUS,          0},
    {TAG_BOX_INDICATOR_LED,          1},
    {TAG_BOX_FALL_ALERT,             2},
    {TAG_BOX_HEART_RATE_ALERT,       3},
    {TAG_RAW_HEART_RATE,             4},
    {TAG_RAW_ACCELERATION,           5},
    {TAG_REHAB_COMPLETION_RATE,      6},
    {TAG_PRIVACY_MODE_ACTIVE,        7},
    {TAG_SYS_POWER_SOURCE,           8},
    {TAG_BOX_VOICE_COMMAND,          9},
    {TAG_BOX_USER_COINS,             10},
    {TAG_BOX_CURRENT_SCENE,          11},
    {TAG_BATTERY_SOC,                12},
    {TAG_ABNORMAL_BEHAVIOR_ALERT,    13},
    {TAG_REHAB_PRESCRIPTION_ID,      14},
    {TAG_NET_ACTIVE_INTERFACE,       15},
    {TAG_BLE_SLOT_BINDING_ID,        16},
    {TAG_REMOTE_MIRROR_STATUS,       17},
    {TAG_LED_RGB_STATUS,             18},
    {TAG_ALSA_AUDIO_ROUTE,           19},
    {TAG_HDMI_CEC_POWER_STATE,       20},
    {TAG_GAME_SCORE_REALTIME,        21},
    {TAG_CARDIAC_SAFETY_ALERT,       22},
    {TAG_UNSUPPORTED_DEVICE_BLOCKED, 23},
    {TAG_BOX_USER_MODE,              24},
    {TAG_DEV_UNBIND_TRIGGER,         25},
    {TAG_DEV_OTA_UPDATE_STATUS,      26},
    {TAG_HAP_GAME_UPDATE_STATE,      27},
    {TAG_ADMIN_AUTH_STATE,           28},
    {TAG_REMOTE_LOG_UPLOAD,          29},
    {TAG_GPIO_PHOENIX_DI_DO,         30},
};
static const size_t s_smartbox_tag_slots_count =
    sizeof(s_smartbox_tag_slots) / sizeof(s_smartbox_tag_slots[0]);

int smartbox_tag_map_fn(uint32_t tag_id, uint32_t device_id) {
    for (size_t i = 0; i < s_smartbox_tag_slots_count; i++) {
        if (s_smartbox_tag_slots[i].tag_id == tag_id) {
            return (int)(device_id * 32 + s_smartbox_tag_slots[i].offset);
        }
    }
    if (tag_id < 64) return (int)tag_id;
    return -1;
}

/* ==============================================================================
 * 3. SmartBox 静态测点元数据定义
 * ============================================================================== */
static const edge_tag_meta_t s_smartbox_tag_metadata[] = {
    { .id = TAG_BOX_SYSTEM_STATUS,          .name = "BOX_SYSTEM_STATUS",          .type = "INT32",   .unit = "",    .category = "DIAGNOSTIC", .min = 0, .max = 10,  .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_BOX_INDICATOR_LED,          .name = "BOX_INDICATOR_LED",          .type = "INT32",   .unit = "",    .category = "CONTROL",    .min = 0, .max = 3,   .persist = false, .upload = false, .poll_interval_ms = 1000 },
    { .id = TAG_BOX_FALL_ALERT,             .name = "BOX_FALL_ALERT",             .type = "INT32",   .unit = "",    .category = "ALERT",      .min = 0, .max = 1,   .persist = true,  .upload = true,  .poll_interval_ms = 100 },
    { .id = TAG_BOX_HEART_RATE_ALERT,       .name = "BOX_HEART_RATE_ALERT",       .type = "INT32",   .unit = "",    .category = "ALERT",      .min = 0, .max = 1,   .persist = true,  .upload = true,  .poll_interval_ms = 100 },
    { .id = TAG_RAW_HEART_RATE,             .name = "RAW_HEART_RATE",             .type = "FLOAT",   .unit = "bpm", .category = "SENSOR",     .min = 0, .max = 250, .persist = false, .upload = true,  .poll_interval_ms = 200 },
    { .id = TAG_RAW_ACCELERATION,           .name = "RAW_ACCELERATION",           .type = "FLOAT",   .unit = "m/s2",.category = "SENSOR",     .min = 0, .max = 8,   .persist = false, .upload = false, .poll_interval_ms = 50 },
    { .id = TAG_REHAB_COMPLETION_RATE,      .name = "REHAB_COMPLETION_RATE",      .type = "FLOAT",   .unit = "%",   .category = "REHAB",      .min = 0, .max = 100, .persist = true,  .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_PRIVACY_MODE_ACTIVE,        .name = "PRIVACY_MODE_ACTIVE",        .type = "INT32",   .unit = "",    .category = "SECURITY",   .min = 0, .max = 1,   .persist = true,  .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_SYS_POWER_SOURCE,           .name = "SYS_POWER_SOURCE",           .type = "INT32",   .unit = "",    .category = "DIAGNOSTIC", .min = 0, .max = 2,   .persist = false, .upload = true,  .poll_interval_ms = 5000 },
    { .id = TAG_BOX_VOICE_COMMAND,          .name = "BOX_VOICE_COMMAND",          .type = "INT32",   .unit = "",    .category = "CONTROL",    .min = 0, .max = 10,  .persist = false, .upload = true,  .poll_interval_ms = 50 },
    { .id = TAG_BOX_USER_COINS,             .name = "BOX_USER_COINS",             .type = "INT32",   .unit = "",    .category = "REHAB",      .min = 0, .max = 100000,.persist = true, .upload = true,  .poll_interval_ms = 10000 },
    { .id = TAG_BOX_CURRENT_SCENE,          .name = "BOX_CURRENT_SCENE",          .type = "INT32",   .unit = "",    .category = "POLICY",     .min = 0, .max = 5,   .persist = true,  .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_BATTERY_SOC,                .name = "SMARTBOX_BATTERY_SOC",       .type = "FLOAT",   .unit = "%",   .category = "DIAGNOSTIC", .min = 0, .max = 100, .persist = true,  .upload = true,  .poll_interval_ms = 5000 },
    { .id = TAG_ABNORMAL_BEHAVIOR_ALERT,    .name = "ABNORMAL_BEHAVIOR_ALERT",    .type = "INT32",   .unit = "",    .category = "ALERT",      .min = 0, .max = 3,   .persist = true,  .upload = true,  .poll_interval_ms = 500 },
    { .id = TAG_REHAB_PRESCRIPTION_ID,      .name = "REHAB_PRESCRIPTION_ID",      .type = "INT32",   .unit = "",    .category = "REHAB",      .min = 0, .max = 100000,.persist = true, .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_NET_ACTIVE_INTERFACE,       .name = "NET_ACTIVE_INTERFACE",       .type = "INT32",   .unit = "",    .category = "DIAGNOSTIC", .min = 0, .max = 3,   .persist = false, .upload = true,  .poll_interval_ms = 2000 },
    { .id = TAG_BLE_SLOT_BINDING_ID,        .name = "BLE_SLOT_BINDING_ID",        .type = "INT32",   .unit = "",    .category = "CONTROL",    .min = 0, .max = 1000, .persist = true,  .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_REMOTE_MIRROR_STATUS,       .name = "REMOTE_MIRROR_STATUS",       .type = "INT32",   .unit = "",    .category = "CONTROL",    .min = 0, .max = 2,   .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_LED_RGB_STATUS,             .name = "LED_RGB_STATUS",             .type = "INT32",   .unit = "",    .category = "CONTROL",    .min = 0, .max = 5,   .persist = false, .upload = false, .poll_interval_ms = 1000 },
    { .id = TAG_ALSA_AUDIO_ROUTE,           .name = "ALSA_AUDIO_ROUTE",           .type = "INT32",   .unit = "",    .category = "CONTROL",    .min = 0, .max = 2,   .persist = false, .upload = false, .poll_interval_ms = 1000 },
    { .id = TAG_HDMI_CEC_POWER_STATE,       .name = "HDMI_CEC_POWER_STATE",       .type = "INT32",   .unit = "",    .category = "CONTROL",    .min = 0, .max = 1,   .persist = false, .upload = false, .poll_interval_ms = 1000 },
    { .id = TAG_GAME_SCORE_REALTIME,        .name = "GAME_SCORE_REALTIME",        .type = "FLOAT",   .unit = "pts", .category = "REHAB",      .min = 0, .max = 10000,.persist = false, .upload = true,  .poll_interval_ms = 200 },
    { .id = TAG_CARDIAC_SAFETY_ALERT,       .name = "CARDIAC_SAFETY_ALERT",       .type = "INT32",   .unit = "",    .category = "ALERT",      .min = 0, .max = 1,   .persist = true,  .upload = true,  .poll_interval_ms = 100 },
    { .id = TAG_UNSUPPORTED_DEVICE_BLOCKED, .name = "UNSUPPORTED_DEVICE_BLOCKED", .type = "INT32",   .unit = "",    .category = "SECURITY",   .min = 0, .max = 1,   .persist = true,  .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_BOX_USER_MODE,              .name = "BOX_USER_MODE",              .type = "INT32",   .unit = "",    .category = "UI",         .min = 0, .max = 2,   .persist = true,  .upload = false, .poll_interval_ms = 5000 },
    { .id = TAG_DEV_UNBIND_TRIGGER,         .name = "DEV_UNBIND_TRIGGER",         .type = "INT32",   .unit = "",    .category = "CONTROL",    .min = 0, .max = 3,   .persist = false, .upload = false, .poll_interval_ms = 1000 },
    { .id = TAG_DEV_OTA_UPDATE_STATUS,      .name = "DEV_OTA_UPDATE_STATUS",      .type = "INT32",   .unit = "",    .category = "OTA",        .min = 0, .max = 5,   .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_HAP_GAME_UPDATE_STATE,      .name = "HAP_GAME_UPDATE_STATE",      .type = "INT32",   .unit = "",    .category = "OTA",        .min = 0, .max = 5,   .persist = false, .upload = true,  .poll_interval_ms = 1000 },
    { .id = TAG_ADMIN_AUTH_STATE,           .name = "ADMIN_AUTH_STATE",           .type = "INT32",   .unit = "",    .category = "SECURITY",   .min = 0, .max = 2,   .persist = true,  .upload = false, .poll_interval_ms = 1000 },
    { .id = TAG_REMOTE_LOG_UPLOAD,          .name = "REMOTE_LOG_UPLOAD",          .type = "INT32",   .unit = "",    .category = "DIAGNOSTIC", .min = 0, .max = 2,   .persist = false, .upload = false, .poll_interval_ms = 5000 },
    { .id = TAG_GPIO_PHOENIX_DI_DO,         .name = "GPIO_PHOENIX_DI_DO",         .type = "INT32",   .unit = "",    .category = "HARDWARE",   .min = 0, .max = 255, .persist = false, .upload = true,  .poll_interval_ms = 100 },
};

const edge_tag_meta_t *g_tag_metadata = s_smartbox_tag_metadata;
const size_t g_tag_metadata_len = sizeof(s_smartbox_tag_metadata) / sizeof(s_smartbox_tag_metadata[0]);

const edge_tag_meta_t *smartbox_get_tag_metadata(uint32_t *count) {
    if (count) *count = (uint32_t)g_tag_metadata_len;
    return s_smartbox_tag_metadata;
}

const edge_tag_meta_t *smartbox_get_tag_meta(uint32_t tag_id) {
    for (size_t i = 0; i < g_tag_metadata_len; i++) {
        if (s_smartbox_tag_metadata[i].id == tag_id) {
            return &s_smartbox_tag_metadata[i];
        }
    }
    return NULL;
}

const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t tag_id) {
    return smartbox_get_tag_meta((uint32_t)tag_id);
}

int edge_tag_get_compact_index(uint32_t tag_id) {
    return smartbox_tag_map_fn(tag_id, 0);
}

const edge_driver_reg_entry_t *g_edge_driver_registry = s_smartbox_driver_registry;
const size_t g_edge_driver_registry_count = sizeof(s_smartbox_driver_registry) / sizeof(s_smartbox_driver_registry[0]);

int edge_driver_registry_init(driver_manager_t *mgr) {
    return smartbox_driver_registry_init(mgr);
}
