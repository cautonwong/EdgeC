/**
 * @file smartbox_ble_adapter.h
 * @brief SmartBox BLE 多外设接入适配器（心率 / 血压 / 血氧 / 康复传感器）
 */

#ifndef SMARTBOX_BLE_ADAPTER_H
#define SMARTBOX_BLE_ADAPTER_H

#include <uv.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 在 loop 上注册 BLE 多设备轮询。
 */
int smartbox_ble_start(uv_loop_t *loop, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif  // SMARTBOX_BLE_ADAPTER_H
