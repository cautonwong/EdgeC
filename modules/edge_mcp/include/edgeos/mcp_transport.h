/**
 * @file mcp_transport.h
 * @brief Industrial Model Context Protocol (MCP) Transports (stdio & WebSocket/HTTP)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#ifndef MCP_TRANSPORT_H
#define MCP_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <uv.h>
#include "edgeos/edge_mcp_dispatcher.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mcp_transport_s mcp_transport_t;

struct mcp_transport_s {
    edge_mcp_dispatcher_t *disp;
    uv_loop_t *loop;
    void *priv;
    int (*start)(mcp_transport_t *trans);
    void (*stop)(mcp_transport_t *trans);
    void (*destroy)(mcp_transport_t *trans);
};

/**
 * @brief Create stdio transport for local CLI / Claude Desktop / SSH process piping
 * @param loop libuv event loop
 * @param disp MCP dispatcher
 * @return Allocated transport handle or NULL
 */
mcp_transport_t *mcp_transport_stdio_create(uv_loop_t *loop, edge_mcp_dispatcher_t *disp);

/**
 * @brief Create WebSocket / HTTP JSON-RPC transport for Web UI / network clients
 * @param loop libuv event loop
 * @param disp MCP dispatcher
 * @param bind_ip IP address to bind (e.g. "0.0.0.0" or "127.0.0.1")
 * @param port TCP port (e.g. 8089)
 * @return Allocated transport handle or NULL
 */
mcp_transport_t *mcp_transport_ws_create(uv_loop_t *loop,
                                         edge_mcp_dispatcher_t *disp,
                                         const char *bind_ip,
                                         uint16_t port);

/**
 * @brief Create Unix Domain Socket (UDS) IPC transport for local multi-client MCP bus
 * @param loop libuv event loop
 * @param disp MCP dispatcher
 * @param socket_path Path to unix domain socket (e.g. "/tmp/edge_mcp.sock")
 * @return Allocated transport handle or NULL
 */
mcp_transport_t *mcp_transport_uds_create(uv_loop_t *loop,
                                         edge_mcp_dispatcher_t *disp,
                                         const char *socket_path);

#ifdef __cplusplus
}
#endif

#endif /* MCP_TRANSPORT_H */
