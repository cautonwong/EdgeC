/**
 * @file smartbox_driver_adapter.h
 * @brief SmartBox 智能盒子驱动适配层与本地点表 (No-Contract-Compiler)
 */

#ifndef SMARTBOX_DRIVER_ADAPTER_H
#define SMARTBOX_DRIVER_ADAPTER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "driver_manager.h"
#include "edge_driver.h"
#include "contract_data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EDGE_SHM_BUS_NAME
#define EDGE_SHM_BUS_NAME "smart_box"
#endif

/* SmartBox Standard Tag IDs */
#define TAG_BOX_SYSTEM_STATUS          12001
#define TAG_BOX_INDICATOR_LED          12002
#define TAG_BOX_FALL_ALERT             12003
#define TAG_BOX_HEART_RATE_ALERT       12004
#define TAG_RAW_HEART_RATE             12005
#define TAG_RAW_ACCELERATION           12006
#define TAG_REHAB_COMPLETION_RATE      12007
#define TAG_PRIVACY_MODE_ACTIVE        12008
#define TAG_SYS_POWER_SOURCE           12009
#define TAG_BOX_VOICE_COMMAND          12010
#define TAG_BOX_USER_COINS             12011
#define TAG_BOX_CURRENT_SCENE          12012
#define TAG_BATTERY_SOC                12013
#define TAG_SMARTBOX_BATTERY_SOC       12013
#define TAG_ABNORMAL_BEHAVIOR_ALERT    12014
#define TAG_REHAB_PRESCRIPTION_ID      12015
#define TAG_NET_ACTIVE_INTERFACE       12016
#define TAG_BLE_SLOT_BINDING_ID        12017
#define TAG_REMOTE_MIRROR_STATUS       12018
#define TAG_LED_RGB_STATUS             12019
#define TAG_ALSA_AUDIO_ROUTE           12020

#define TAG_HDMI_CEC_POWER_STATE       12051
#define TAG_GAME_SCORE_REALTIME        12052
#define TAG_CARDIAC_SAFETY_ALERT       12053
#define TAG_UNSUPPORTED_DEVICE_BLOCKED 12054
#define TAG_BOX_USER_MODE              12055
#define TAG_DEV_UNBIND_TRIGGER         12056
#define TAG_DEV_OTA_UPDATE_STATUS      12057
#define TAG_HAP_GAME_UPDATE_STATE      12058
#define TAG_ADMIN_AUTH_STATE           12059
#define TAG_REMOTE_LOG_UPLOAD          12060
#define TAG_GPIO_PHOENIX_DI_DO         12061

/**
 * @brief 初始化并注册 SmartBox 驱动模板
 */
int smartbox_driver_registry_init(driver_manager_t *mgr);

/**
 * @brief SmartBox 本地测点到 DataHub 影子槽位 O(1) 映射函数
 */
int smartbox_tag_map_fn(uint32_t tag_id, uint32_t device_id);

/**
 * @brief 获取测点静态元数据表
 */
const edge_tag_meta_t *smartbox_get_tag_metadata(uint32_t *count);
const edge_tag_meta_t *smartbox_get_tag_meta(uint32_t tag_id);

/* 兼容旧式全局符号与单测链接 */
extern const edge_tag_meta_t *g_tag_metadata;
extern const size_t g_tag_metadata_len;
int edge_tag_get_compact_index(uint32_t tag_id);
const edge_tag_meta_t *contract_tag_get_meta(edge_tag_id_t tag_id);
int edge_driver_registry_init(driver_manager_t *mgr);
extern const edge_driver_reg_entry_t *g_edge_driver_registry;
extern const size_t g_edge_driver_registry_count;

#ifdef __cplusplus
}
#endif

#endif  // SMARTBOX_DRIVER_ADAPTER_H
