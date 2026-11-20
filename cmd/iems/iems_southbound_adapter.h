/**
 * @file iems_southbound_adapter.h
 * @brief IEMS 南向物理总线连接适配器（UART / TCP / UN*X socket → bus_orchestrator）
 */

#ifndef IEMS_SOUTHBOUND_ADAPTER_H
#define IEMS_SOUTHBOUND_ADAPTER_H

#include <uv.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "iems_tcp_link.h"

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
 * @brief 每路总线的启动状态投影 + TCP 链路槽位，由组合根按 bus id 持有。
 *
 * state 给北向监听与启动摘要读；link 为空表示该路不是 TCP 或尚未创建。
 */
typedef struct {
    bus_conn_state_t state[IEMS_MAX_VBUS];
    iems_tcp_link_slot_t link[IEMS_MAX_VBUS];
} iems_bus_wiring_t;

/**
 * @brief 按契约的传输类型与解析出的设备路径判定南向连接方式。
 *
 * CAN 只有模拟路径能走 UDS；真实 CAN 设备尚未实现时返回 UNSUPPORTED，
 * 让启动摘要如实报缺，而不是拿 AF_UNIX 去连一个字符设备。
 * TCP 以路径为准：tcp://host:port 压过声明类型；没有前缀的路径仍走 UDS 模拟。
 */
iems_bus_transport_t iems_southbound_transport_for(const char *type, const char *path);

/**
 * @brief 严格解析 OMNIS_BUS_<n>_BAUD 的值。
 *
 * 只接受十进制整数且无尾随字符, 且必须是 IO 层支持的线速; `9600abc`、
 * 0、负数与未支持的速率返回 -1, 由调用方拒绝该总线而不是静默改线速。
 */
int iems_southbound_parse_baud(const char *value, uint32_t *baud_out);

/**
 * @brief 启动时连一次南向物理总线（旧的兼容入口，TCP 只连一次、不重试）。
 *
 * 等价于 iems_southbound_connect_in(..., link_slots = NULL, ...)。产品装配路径
 * 必须用 iems_southbound_connect_in 并传自己的链路存储。
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

/**
 * @brief 启动南向总线；TCP 走 iems_tcp_link（断线后退避重连）。
 *
 * 启动返回值是「已启动的数量」：TCP 首次连接失败也返回非 NULL 的 link，
 * 计入已启动（BACKOFF），因此设备启动时离线不再让组合根判启动失败。
 * bus_state 按 link 状态迁移更新（CONNECTED / CONNECTING / DISCONNECTED）。
 *
 * @param link_slots 每路总线的链路存储；NULL 退回旧的一次性 TCP 连接
 * @param link_cfg TCP 链路退避配置；NULL 用默认值
 * @return 已启动的南向总线数；<0 参数非法或装配契约表缺失
 */
int iems_southbound_connect_in(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch,
                               bus_conn_state_t *bus_state, iems_tcp_link_slot_t *link_slots,
                               const iems_tcp_link_config_t *link_cfg, int *configured);

/**
 * @brief 启动一路 tcp://host:port。
 *
 * slot 非 NULL 时建 link（失败也返回 true，进入 BACKOFF）；slot 为 NULL 时
 * 退回旧的一次性连接。open_io 由组合根传入（通常是 io_executor_create_tcp），
 * 链路模块不自己装配 edge_io。
 *
 * @return 该总线是否已启动；地址非法或句柄创建失败返回 false
 */
bool iems_tcp_link_start_bus(uv_loop_t *loop, bus_orchestrator_t *orch, int bus_id,
                             const char *label, const char *path, bus_conn_state_t *bus_state,
                             iems_tcp_link_slot_t *slot, const iems_tcp_link_config_t *cfg,
                             iems_tcp_link_open_fn open_io, net_buf_pool_t *io_pool);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_SOUTHBOUND_ADAPTER_H
