/**
 * @file iems_tcp_link.h
 * @brief 单路 TCP 南向链路的连接状态机（断线重连、指数退避、关闭）。
 *
 * 设计见 /home/edge/migration/tcp-reconnect-design.md §2。链路按
 * DISCONNECTED / CONNECTING / CONNECTED / BACKOFF 四态迁移，至多持有一个
 * io_handle_t；连接经 io 端口 io_executor_tcp_connect_async 异步完成，
 * 重连 = 新建句柄，先 attach 新句柄、再销毁旧句柄。不透明指针 +
 * caller-allocated storage。
 */

#ifndef IEMS_TCP_LINK_H
#define IEMS_TCP_LINK_H

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "io_executor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bus_orchestrator_s bus_orchestrator_t;
typedef struct iems_tcp_link_s iems_tcp_link_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[512];
} iems_tcp_link_storage_t;

/** @brief 每路 TCP 总线的链路槽位：存储 + 已构造的链路指针。 */
typedef struct {
    iems_tcp_link_storage_t storage;
    iems_tcp_link_t *link;
} iems_tcp_link_slot_t;

typedef enum {
    IEMS_TCP_LINK_DISCONNECTED = 0,
    IEMS_TCP_LINK_CONNECTING,
    IEMS_TCP_LINK_CONNECTED,
    IEMS_TCP_LINK_BACKOFF
} iems_tcp_link_state_t;

typedef void (*iems_tcp_link_state_cb_t)(iems_tcp_link_state_t state, void *user_data);

/**
 * @brief 打开一条新的 TCP 连接句柄（旧的一次性连接入口用）。
 *
 * 签名与 io_executor_create_tcp 一致。
 */
typedef io_handle_t *(*iems_tcp_link_open_fn)(uv_loop_t *loop, const char *host, uint16_t port,
                                              net_buf_pool_t *pool);

/**
 * @brief 接管一条已连通的 fd，构造 io 句柄（组合根/测试提供）。
 *
 * 签名与 io_executor_create_from_fd 一致。返回 NULL 表示接管失败，链路关闭
 * 该 fd 并按退避重试。
 */
typedef io_handle_t *(*iems_tcp_link_adopt_fn)(uv_loop_t *loop, int fd, net_buf_pool_t *pool);

typedef struct {
    uint32_t backoff_base_ms;          /**< 0 用默认 1000ms */
    uint32_t backoff_cap_ms;           /**< 0 用默认 30000ms */
    uint32_t connect_timeout_ms;       /**< 0 用默认 3000ms */
    uint32_t keepalive_idle_s;         /**< 0 用 IO_TCP_KEEPALIVE_IDLE_S */
    uint32_t keepalive_intvl_s;        /**< 0 用 IO_TCP_KEEPALIVE_INTVL_S */
    uint32_t keepalive_cnt;            /**< 0 用 IO_TCP_KEEPALIVE_CNT */
    iems_tcp_link_state_cb_t on_state; /**< 每次状态迁移回调，可为 NULL */
    void *user_data;
    iems_tcp_link_adopt_fn adopt_fd; /**< 必填：接管异步连通的 fd */
    net_buf_pool_t *io_pool;
} iems_tcp_link_config_t;

/**
 * @brief 在调用方存储上构造并启动链路（首次连接异步进行）。
 *
 * 首次尝试失败不会返回 NULL：链路进入 BACKOFF 并按退避继续重试。
 * 构造时状态为 CONNECTING；连接成功后状态回调才报 CONNECTED。
 * cfg->adopt_fd 必填；host 必须是数字 IPv4（异步 connect 不解析主机名）；
 * 参数非法或定时器初始化失败返回 NULL。
 */
iems_tcp_link_t *iems_tcp_link_create_in(iems_tcp_link_storage_t *storage, uv_loop_t *loop,
                                         bus_orchestrator_t *orch, int bus_id, const char *host,
                                         uint16_t port, const iems_tcp_link_config_t *cfg);

/** @brief 主动关闭；幂等，关闭后任何回调都是 no-op。 */
void iems_tcp_link_close(iems_tcp_link_t *link);

/**
 * @brief 关闭一组槽位里的链路；未创建的槽位是 no-op。
 *
 * 必须在 arbiter/orchestrator destroy 之前、紧邻一次 uv_run 调用，
 * 让 close 回调先把句柄释放掉。
 */
void iems_tcp_link_close_all(iems_tcp_link_slot_t *slots, int count);

/** @brief 当前状态（测试与诊断用）。 */
iems_tcp_link_state_t iems_tcp_link_state(const iems_tcp_link_t *link);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_TCP_LINK_H
