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

#ifdef __cplusplus
}
#endif

#endif
