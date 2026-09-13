/**
 * @file main.c
 * @brief Industrial edge-mcp-server Daemon Entry Point
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <uv.h>

#include "edgeos/edge_mcp_dispatcher.h"
#include "edgeos/mcp_transport.h"
#include "datahub.h"
#include "edge_log.h"

static uv_loop_t *g_loop = NULL;
static uv_signal_t g_sigint;
static uv_signal_t g_sigterm;

static void on_signal(uv_signal_t *handle, int signum) {
    (void)handle;
    (void)signum;
    LOG_INFO(LOG_CAT_CORE, "Stopping edge-mcp-server gracefully...");
    if (g_loop) {
        uv_stop(g_loop);
    }
}

static void print_usage(const char *prog) {
    printf("EdgeOS Industrial Model Context Protocol (MCP) Server\n");
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  --stdio             Run in stdio line streaming mode (Claude Desktop / SSH pipe)\n");
    printf("  --sock, --uds <path> Unix Domain Socket path (default: /tmp/edge_mcp.sock)\n");
    printf("  --port <port>       WebSocket / HTTP JSON-RPC port (default: 8089, 0 to disable)\n");
    printf("  --bind <ip>         Bind IP address (default: 0.0.0.0)\n");
    printf("  --no-ws             Disable WebSocket / HTTP transport\n");
    printf("  --product <name>    Physical product domain (default: IEMS)\n");
    printf("  --help              Display this help message\n");
}

int main(int argc, char **argv) {
    bool use_stdio = false;
    bool use_uds = false;
    bool disable_ws = false;
    const char *sock_path = "/tmp/edge_mcp.sock";
    uint16_t port = 8089;
    const char *bind_ip = "0.0.0.0";
    const char *product = "IEMS";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--stdio") == 0) {
            use_stdio = true;
        } else if (strcmp(argv[i], "--sock") == 0 || strcmp(argv[i], "--uds") == 0) {
            use_uds = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                sock_path = argv[++i];
            }
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = (uint16_t)atoi(argv[++i]);
            if (port == 0) disable_ws = true;
        } else if (strcmp(argv[i], "--bind") == 0 && i + 1 < argc) {
            bind_ip = argv[++i];
        } else if (strcmp(argv[i], "--no-ws") == 0) {
            disable_ws = true;
        } else if (strcmp(argv[i], "--product") == 0 && i + 1 < argc) {
            product = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    g_loop = uv_default_loop();

    /* Initialize DataHub */
    datahub_t *hub = (datahub_t *)calloc(1, sizeof(datahub_t));
    if (!hub || datahub_init(hub, g_loop, "/edgeos_mcp", 1024, 65536, NULL, 0, NULL, NULL, NULL) != 0) {
        fprintf(stderr, "Failed to initialize DataHub for product %s\n", product);
        free(hub);
        return 1;
    }

    edge_mcp_env_t env = {
        .hub = hub,
        .drv_mgr = NULL,
        .stream_eng = NULL,
        .storage_drv = NULL,
        .loop = g_loop
    };

    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(&env);
    if (!disp) {
        fprintf(stderr, "Failed to create MCP Dispatcher\n");
        datahub_destroy(hub);
        free(hub);
        return 1;
    }

    mcp_transport_t *stdio_trans = NULL;
    mcp_transport_t *ws_trans = NULL;
    mcp_transport_t *uds_trans = NULL;

    if (use_stdio) {
        stdio_trans = mcp_transport_stdio_create(g_loop, disp);
        if (!stdio_trans || stdio_trans->start(stdio_trans) != 0) {
            fprintf(stderr, "Failed to start MCP stdio transport\n");
            if (stdio_trans) stdio_trans->destroy(stdio_trans);
            edge_mcp_dispatcher_destroy(disp);
            datahub_destroy(hub);
            free(hub);
            return 1;
        }
    } else {
        if (use_uds) {
            uds_trans = mcp_transport_uds_create(g_loop, disp, sock_path);
            if (!uds_trans || uds_trans->start(uds_trans) != 0) {
                fprintf(stderr, "Failed to start MCP UDS transport at %s\n", sock_path);
                if (uds_trans) uds_trans->destroy(uds_trans);
                edge_mcp_dispatcher_destroy(disp);
                datahub_destroy(hub);
                free(hub);
                return 1;
            }
            printf("[MCP Server] Listening on unix domain socket: %s\n", sock_path);
        }

        if (!disable_ws) {
            ws_trans = mcp_transport_ws_create(g_loop, disp, bind_ip, port);
            if (!ws_trans || ws_trans->start(ws_trans) != 0) {
                fprintf(stderr, "Failed to start MCP WS transport\n");
                if (ws_trans) ws_trans->destroy(ws_trans);
                if (uds_trans) { uds_trans->stop(uds_trans); uds_trans->destroy(uds_trans); }
                edge_mcp_dispatcher_destroy(disp);
                datahub_destroy(hub);
                free(hub);
                return 1;
            }
            printf("[MCP Server] Listening on ws://%s:%u/mcp and http://%s:%u/api/v1/mcp/call\n",
                   bind_ip, port, bind_ip, port);
        }
    }

    uv_signal_init(g_loop, &g_sigint);
    uv_signal_start(&g_sigint, on_signal, SIGINT);
    uv_signal_init(g_loop, &g_sigterm);
    uv_signal_start(&g_sigterm, on_signal, SIGTERM);

    uv_run(g_loop, UV_RUN_DEFAULT);

    if (stdio_trans) {
        stdio_trans->stop(stdio_trans);
        stdio_trans->destroy(stdio_trans);
    }
    if (ws_trans) {
        ws_trans->stop(ws_trans);
        ws_trans->destroy(ws_trans);
    }
    if (uds_trans) {
        uds_trans->stop(uds_trans);
        uds_trans->destroy(uds_trans);
    }
    edge_mcp_dispatcher_destroy(disp);
    datahub_destroy(hub);
    free(hub);

    return 0;
}
