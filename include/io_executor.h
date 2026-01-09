#ifndef IO_EXECUTOR_H
#define IO_EXECUTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "net_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Phase 0: 抽象 IO 接口 (The Un-collapse) --- */

// 句柄定义：对上层完全不透明，禁止上层访问 fd
typedef struct io_handle_s io_handle_t;

// 事件定义：上层只关心业务相关的状态转换
typedef enum {
    IO_EVENT_RX,       // 物理层收到数据 (预读或就绪通知)
    IO_EVENT_TX_DONE,  // 异步发送完成
    IO_EVENT_ERROR     // 物理链路错误 (断线、校验错误等)
} io_event_t;

/**
 * @brief IO 回调函数
 * @param h 触发事件的句柄
 * @param evt 事件类型
 * @param evt_data 事件关联数据 (例如 RX 事件对应的 net_buf_t*)
 * @param user_data 绑定的用户数据 (通常是 Engine 实例)
 */
typedef void (*io_callback_t)(io_handle_t *h, io_event_t evt, void *evt_data, void *user_data);

// 核心操作集 (虚函数表)
typedef struct {
    // 开启/关闭监听。对于串口是开启接收中断/Poll；对于 TCP 是 socket read。
    int (*enable_read)(io_handle_t *h, bool enable);

    // 异步发送。要求底层处理 buffering，直到发完触发 TX_DONE。
    // net_buf_t 的引用计数由底层接管。
    int (*write_async)(io_handle_t *h, net_buf_t *buf);

    // 物理配置。
    int (*ioctl)(io_handle_t *h, int cmd, void *arg);

    // 销毁资源
    void (*destroy)(io_handle_t *h);
} io_ops_t;

/* --- IOCTL Commands --- */
#define IO_CTL_SET_BAUD 0x01
#define IO_CTL_FLUSH 0x02

typedef struct {
    uint32_t baud;
} edge_phy_params_t;

// 公共基类：由平台适配层实例化
struct io_handle_s {
    const io_ops_t *ops;
    io_callback_t cb;
    void *user_data;
};

/* --- 平台适配层工厂 (具体实现在 src/platform) --- */

/**
 * @brief 从现有文件描述符创建 IO 句柄 (用于环回测试或高级集成)
 */
io_handle_t *io_executor_create_from_fd(uv_loop_t *loop, int fd, net_buf_pool_t *pool);

#ifdef __linux__
#include <uv.h>
/**
 * @brief 创建基于 Linux Libuv Poll 的 UART 句柄
 */
io_handle_t *io_executor_create_uart(uv_loop_t *loop, const char *dev_path, uint32_t baud,
                                     net_buf_pool_t *pool);

/**
 * @brief 创建基于 Linux Unix Domain Socket 的仿真句柄 (用于重构后的仿真测试)
 */
io_handle_t *io_executor_create_uds(uv_loop_t *loop, const char *sock_path);
#endif

/* --- 遗留兼容层 (逐步弃用) --- */
// 为保证现有代码能编译，暂时保留部分旧结构，待 Phase 1 完成后清理
typedef enum { IO_BUS_RS485_1 = 0, IO_BUS_RS485_2, IO_BUS_CAN_1, IO_BUS_MAX } io_bus_id_t;

#ifdef __cplusplus
}
#endif

#endif  // IO_EXECUTOR_H