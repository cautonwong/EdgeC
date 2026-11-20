/**
 * @file iems_north_iec104.h
 * @brief IEMS 北向 IEC104 服务端：TCP 监听 → bus_orchestrator 从站会话
 *
 * 与 UDS 的 Modbus 北向同一套机制：accept 后 dup fd、attach_io，从站取数由
 * 产品实现。IEC104 的点位标识是 (type_id << 24) | ioa，静态点表在
 * iems_driver_adapter.c。
 *
 * 当前产品语义是单客户端：活动链路正常时第二路连接被拒绝并告警，不覆盖引擎
 * 已挂的句柄；活动链路报错后允许新客户端接管。多主站并发留待产品范围裁决。
 */

#ifndef IEMS_NORTH_IEC104_H
#define IEMS_NORTH_IEC104_H

#include <stdbool.h>
#include <uv.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "iems_node.h"
#include "iems_north_adapter.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uv_tcp_t *srv;
    datahub_t *hub;
    bus_orchestrator_t *orch;
    iems_iec104_source_ctx_t src;
    int bus_id;             /**< 契约表里的 IEC104 北向总线；-1 = 未接线 */
    int port;               /**< 监听端口；传 0 时为内核分配的临时端口 */
    io_handle_t *active_io; /**< 当前唯一主站的 IO 句柄；NULL = 无客户端 */
    int active_fd;          /**< 活动链路 dup 出的 fd（句柄持有）；-1 = 无客户端 */
    bool active_stale; /**< 活动链路已报错；新客户端可接管，旧句柄在接管或 close 时销毁 */
} iems_north_iec104_t;

/**
 * @brief 按契约表接线 IEC104 北向从站并开始监听 TCP。
 *
 * 契约表里 category=NORTHBOUND、type=IEC104 的总线会 setup_slave 并注入
 * source ops；只有第一条绑定端口（当前产品只有一个 2404 端点）。
 *
 * @param host 绑定地址；NULL/空 = 0.0.0.0
 * @param port 端口；0 = 临时端口（测试用）
 * @return 0 成功（含契约表无 IEC104 条目）；<0 参数非法或绑定失败
 */
int iems_north_iec104_init(iems_north_iec104_t *l, uv_loop_t *loop, datahub_t *hub,
                           bus_orchestrator_t *orch, iems_node_t *node, sqlite_svc_t *db,
                           const char *host, int port);

void iems_north_iec104_close(iems_north_iec104_t *l);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_NORTH_IEC104_H
