#ifndef THREAD_PORT_UV_H
#define THREAD_PORT_UV_H

#include "edge_thread_port.h"

/** libuv 线程实现；由组合根在启动前安装。 */
extern const edge_thread_port_t g_uv_thread_port;

void edge_thread_port_install_uv(void);

#endif /* THREAD_PORT_UV_H */
