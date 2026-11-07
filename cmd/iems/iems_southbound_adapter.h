/**
 * @file iems_southbound_adapter.h
 * @brief IEMS 南向物理总线连接适配器（UN*X socket → bus_orchestrator）
 */

#ifndef IEMS_SOUTHBOUND_ADAPTER_H
#define IEMS_SOUTHBOUND_ADAPTER_H

#include <uv.h>

#include "bus_orchestrator.h"
#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IEMS_MAX_VBUS 32

typedef enum {
    BUS_STATE_DISCONNECTED = 0,
    BUS_STATE_CONNECTING,
    BUS_STATE_CONNECTED,
    BUS_STATE_LISTENING
} bus_conn_state_t;

/**
 * @brief 启动时连一次南向物理总线（只连一次、不重试）。
 *
 * @param loop 事件循环
 * @param hub DataHub（提供 IO 内存池）
 * @param orch 总线编排器
 * @param bus_state 每路总线的连接状态，按 bus id 索引
 * @param configured 出参：契约表里配置的南向总线数（可为 NULL）
 * @return 连上的南向总线数；<0 参数非法或装配契约表缺失
 */
int iems_southbound_connect(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch,
                            bus_conn_state_t *bus_state, int *configured);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_SOUTHBOUND_ADAPTER_H
