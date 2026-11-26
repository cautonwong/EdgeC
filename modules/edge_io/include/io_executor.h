#ifndef IO_EXECUTOR_H
#define IO_EXECUTOR_H

#include <stdbool.h>
#include <stdint.h>
typedef struct uv_loop_s uv_loop_t;

#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- IO 抽象接口定义 --- */

/* IO_EVENT_RX 的 payload 是 net_buf_t*，**借用**：所有权属于 io 句柄，
 * 句柄在本轮回调返回之后自行释放。回调只能读；要跨回调使用（如异步转发）
 * 必须自己 net_buf_ref，并在用完时 net_buf_unref。在回调里释放载荷会让回调
 * 栈里新申请的缓冲被句柄的收尾 unref 误释放（TX 帧内容被覆盖）。
 * IO_EVENT_TX_DONE / IO_EVENT_ERROR 的 payload 为 NULL。 */
typedef enum {
    IO_EVENT_RX,      /**< 收到原始数据 / MQTT 消息 */
    IO_EVENT_TX_DONE, /**< 异步发送完成 / MQTT 发布成功 */
    IO_EVENT_ERROR,   /**< 物理层链路错误 */
} io_event_t;

typedef enum {
    IO_TYPE_UART,
    IO_TYPE_UDS,
    IO_TYPE_TCP,
    IO_TYPE_UDP,
    IO_TYPE_HTTP,
    IO_TYPE_MQTT, /**< [新增] MQTT IO 类型 */
    IO_TYPE_CUSTOM
} io_type_t;

// 控制指令定义
#define IO_CTL_SET_BAUD 1
#define IO_CTL_FLUSH 2
#define IO_CTL_DISCARD_RX 3 /**< 丢弃已到达内核但还没读出的字节(事务边界隔离) */
#define IO_CTL_HTTP_SET_URL 10
#define IO_CTL_HTTP_SET_METHOD 11
#define IO_CTL_HTTP_ADD_HEADER 12

// MQTT 专用控制指令
#define IO_CTL_MQTT_SET_TOPIC 20
#define IO_CTL_MQTT_SUBSCRIBE 21

// TCP keepalive 默认值：idle 秒无数据后每 intvl 秒探测 cnt 次
#define IO_TCP_KEEPALIVE_IDLE_S 30
#define IO_TCP_KEEPALIVE_INTVL_S 10
#define IO_TCP_KEEPALIVE_CNT 3

typedef struct io_handle_s io_handle_t;

typedef struct {
    int (*enable_read)(io_handle_t *h, bool enable);
    int (*write_async)(io_handle_t *h, net_buf_t *buf);
    int (*ioctl)(io_handle_t *h, int cmd, void *arg);
    void (*destroy)(io_handle_t *h);
} io_ops_t;

typedef struct {
    io_type_t type;
    union {
        struct {
            char dev[64];
            uint32_t baud;
        } uart;
        struct {
            char path[128];
        } uds;
        struct {
            char host[64];
            uint16_t port;
        } tcp;
        struct {
            char local_ip[32];
            uint16_t local_port;
            char remote_ip[32];
            uint16_t remote_port;
        } udp;
        struct {
            char url[256];
            char method[16];
        } http;
        struct {
            char broker[128];
            char client_id[64];
            char topic[128];
        } mqtt;
    } params;
} io_config_t;

io_handle_t *io_executor_create(uv_loop_t *loop, const io_config_t *cfg, net_buf_pool_t *pool);
void io_executor_cleanup(void);
io_handle_t *io_executor_create_from_fd(uv_loop_t *loop, int fd, net_buf_pool_t *pool);
void io_executor_http_request(uv_loop_t *loop, const char *url, const char *method);

/* --- 异步 TCP 连接端口 --- */

/** 不透明连接票据：适配器分配，调用方只持有指针。 */
typedef struct io_tcp_connect_s io_tcp_connect_t;

typedef struct {
    const char *host; /**< 数字 IPv4；适配器同步读取，调用后无需保留 */
    uint16_t port;
    uint32_t keepalive_idle_s;  /**< 0 用 IO_TCP_KEEPALIVE_IDLE_S */
    uint32_t keepalive_intvl_s; /**< 0 用 IO_TCP_KEEPALIVE_INTVL_S */
    uint32_t keepalive_cnt;     /**< 0 用 IO_TCP_KEEPALIVE_CNT */
} io_tcp_connect_opts_t;

/**
 * @brief 连接完成回调。
 *
 * err 为 0 表示成功，fd 已连通、非阻塞，所有权归调用方；keepalive 已尝试设置
 * （setsockopt 失败仅告警）。失败时 fd == -1、err 为负 errno，适配器已关闭 fd。
 * 回调在 loop 线程只触发一次，回调期间票据已失效。始终异步：connect() 立即
 * 成功也走下一次 loop。
 */
typedef void (*io_tcp_connect_cb_t)(void *user_data, int fd, int err);

/**
 * @brief 启动一次非阻塞连接，成功返回票据。
 *
 * 启动前失败（参数、地址、socket/fcntl/uv_poll 初始化）返回 NULL 并置 errno；
 * 已启动后的结果只经 cb 上报。所有调用必须在 loop 线程。
 */
io_tcp_connect_t *io_executor_tcp_connect_async(uv_loop_t *loop, const io_tcp_connect_opts_t *opts,
                                                io_tcp_connect_cb_t cb, void *user_data);

/**
 * @brief 取消进行中的连接。
 *
 * 幂等、NULL 安全；返回后不再回调，fd 由适配器关闭。必须在 loop 线程调用，
 * 回调内不得调用（票据已失效）。
 */
void io_tcp_connect_cancel(io_tcp_connect_t *op);

/**
 * @brief 开/关该句柄的读事件。
 *
 * 产品层只需要这一个动作；驱动的 ops 表是 edge_io 的内部约定。
 * 驱动没实现 enable_read 时返回 -1，不崩。
 */
int io_handle_enable_read(io_handle_t *h, bool enable);

/** @brief 释放句柄（驱动内部的 ops->destroy；产品层不直接碰 ops 表）。 */
void io_handle_destroy(io_handle_t *h);

/**
 * @brief 注册链路存亡回调（产品层用）。
 *
 * 只上报 IO_EVENT_ERROR，且在句柄数据回调 cb 之后调用；正常 RX/TX_DONE 不触发。
 * 每个新建句柄都必须注册，否则断线只有引擎知道、产品看不到。
 */
void io_handle_set_link_cb(io_handle_t *h,
                           void (*cb)(io_handle_t *h, io_event_t evt, void *user_data),
                           void *user_data);

#ifdef __cplusplus
}
#endif

#endif  // IO_EXECUTOR_H