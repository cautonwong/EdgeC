#ifndef EDGE_IO_EXECUTOR_LINUX_H
#define EDGE_IO_EXECUTOR_LINUX_H

#include "io_executor.h"

#ifdef __cplusplus
extern "C" {
#endif

io_handle_t *io_executor_create_uart(uv_loop_t *loop, const char *dev_path, uint32_t baud,
                                     net_buf_pool_t *pool);
io_handle_t *io_executor_create_tcp(uv_loop_t *loop, const char *host, uint16_t port,
                                    net_buf_pool_t *pool);

/* 创建前就能判定线速是否可配置; 不支持的速率必须让调用方拒绝, 不能静默退回。 */
bool io_executor_uart_baud_supported(uint32_t baud);

/* 白盒诊断/测试用：返回 poll 句柄底层 fd；非 fd 类型返回 -1。 */
int io_executor_linux_fd(const io_handle_t *h);

#ifdef __cplusplus
}
#endif

#endif
