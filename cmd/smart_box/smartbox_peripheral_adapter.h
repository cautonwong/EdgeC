/**
 * @file smartbox_peripheral_adapter.h
 * @brief SmartBox 外设适配器（NFC SPI / Modbus RTU 康复外骨骼）
 */

#ifndef SMARTBOX_PERIPHERAL_ADAPTER_H
#define SMARTBOX_PERIPHERAL_ADAPTER_H

#include <uv.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 在 loop 上注册 NFC 扫卡与 Modbus RTU 轮询。
 */
int smartbox_peripheral_start(uv_loop_t *loop, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif  // SMARTBOX_PERIPHERAL_ADAPTER_H
