#ifndef EDGE_DRIVER_SESSION_PORT_H
#define EDGE_DRIVER_SESSION_PORT_H

#include <stdint.h>

#include "edge_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 会话所需的驱动生命周期端口。
 *
 * edge_session 只创建和销毁驱动上下文，不依赖具体的驱动管理器实现。
 * 适配器负责把 manager 包装成这个端口。
 */
typedef struct {
    void *ctx;
    edge_driver_ctx_t *(*create)(void *ctx, const char *protocol, uint32_t device_id);
    void (*destroy)(edge_driver_ctx_t *session);
} edge_driver_session_port_t;

#ifdef __cplusplus
}
#endif

#endif
