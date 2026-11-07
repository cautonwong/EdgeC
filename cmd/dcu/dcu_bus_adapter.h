/**
 * @file dcu_bus_adapter.h
 * @brief DCU 南向 RS485 通道接线适配器
 */

#ifndef DCU_BUS_ADAPTER_H
#define DCU_BUS_ADAPTER_H

#include <uv.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "dcu_inventory.h"
#include "driver_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 按通道档案建立主站并尝试连接 /tmp/rs485_<n>.sock。
 *
 * 空通道不建主站；协议未注册的通道只报错跳过。
 */
void dcu_bus_attach_channels(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch,
                             driver_manager_t *drv_mgr, dcu_inventory_t *inv);

#ifdef __cplusplus
}
#endif

#endif  // DCU_BUS_ADAPTER_H
