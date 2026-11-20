/**
 * @file iems_north_listener.h
 * @brief IEMS 北向 Modbus 从站监听（UDS → bus_orchestrator 从站）
 *
 * 与 DCU 的 698 从站同一套机制：uv_pipe 监听 + accept 后 attach_io +
 * 从站取数由产品实现。区别只在协议与点位标识。
 */

#ifndef IEMS_NORTH_LISTENER_H
#define IEMS_NORTH_LISTENER_H

#include <stdbool.h>
#include <uv.h>

#include "bus_orchestrator.h"
#include "datahub.h"
#include "iems_southbound_adapter.h"
#include "io_executor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bus_orchestrator_t *orch;
    datahub_t *hub;
    int bus_id;
    /* 从站是单客户端语义:同一时刻只服务一路。断链后句柄不立即销毁(引擎仍
     * 指向它),留到下一次连接或 listener 关闭时释放。 */
    io_handle_t *active_io;
    int active_fd; /* active_io 占用的 fd 号,用于在新连接时探测旧客户端是否已关 */
    bool active_stale;
} iems_north_accept_ctx_t;

typedef struct {
    uv_pipe_t *srv[IEMS_MAX_VBUS];
    iems_north_accept_ctx_t ctx[IEMS_MAX_VBUS];
} iems_north_listener_t;

/**
 * @brief 为每条 NORTHBOUND 通道建立从站监听。
 *
 * @param first_sock 覆盖第一条 NORTHBOUND 的路径（可为 NULL）；其余条目用契约表自带的
 *                   simulation_path，都没有则退回 /tmp/iems_north_slave.sock（仅第一条）。
 * @param bus_state 每条总线的连接状态，监听成功记为 BUS_STATE_LISTENING（可为 NULL）
 */
void iems_north_listener_start(iems_north_listener_t *l, uv_loop_t *loop, datahub_t *hub,
                               bus_orchestrator_t *orch, const char *first_sock,
                               bus_conn_state_t *bus_state);

/**
 * @brief 关闭全部监听句柄。
 */
void iems_north_listener_close(iems_north_listener_t *l);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_NORTH_LISTENER_H
