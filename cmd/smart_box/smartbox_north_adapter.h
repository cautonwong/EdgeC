/**
 * @file smartbox_north_adapter.h
 * @brief SmartBox 北向装配适配器（组合根之外，只做接线）
 */

#ifndef SMARTBOX_NORTH_ADAPTER_H
#define SMARTBOX_NORTH_ADAPTER_H

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 装配并启动 SmartBox 北向 MQTT 通道。
 *
 * 启动失败只告警，不影响本地采集。
 */
void smartbox_north_attach(datahub_t *hub);

/**
 * @brief 停止并销毁北向通道（北向线程在自己的 loop 上跑）。
 */
void smartbox_north_detach(void);

#ifdef __cplusplus
}
#endif

#endif  // SMARTBOX_NORTH_ADAPTER_H
