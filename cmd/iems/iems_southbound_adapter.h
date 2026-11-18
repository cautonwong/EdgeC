/**
 * @file iems_southbound_adapter.h
 * @brief IEMS 南向物理总线连接适配器（UART / TCP / UN*X socket → bus_orchestrator）
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

typedef enum {
    IEMS_BUS_UDS = 0,
    IEMS_BUS_UART,
    IEMS_BUS_TCP,
    IEMS_BUS_UNSUPPORTED
} iems_bus_transport_t;

/**
 * @brief 按契约的传输类型与解析出的设备路径判定南向连接方式。
 *
 * CAN 只有模拟路径能走 UDS；真实 CAN 设备尚未实现时返回 UNSUPPORTED，
 * 让启动摘要如实报缺，而不是拿 AF_UNIX 去连一个字符设备。
 * TCP 以路径为准：tcp://host:port 压过声明类型；没有前缀的路径仍走 UDS 模拟。
 */
iems_bus_transport_t iems_southbound_transport_for(const char *type, const char *path);

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
