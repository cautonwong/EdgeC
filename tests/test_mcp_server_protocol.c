/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_mcp_server_protocol.c
 * @brief Unit tests for Industrial MCP Transports & Protocol Framing (AEP-506)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <assert.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "datahub.h"
#include "edgeos/edge_mcp_dispatcher.h"
#include "edgeos/mcp_transport.h"

static void test_ws_transport_lifecycle(void) {
    printf("[TEST] Testing WebSocket / HTTP MCP server transport lifecycle...\n");
    uv_loop_t *loop = uv_default_loop();

    edge_mcp_env_t env = {.loop = loop};
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(&env);
    assert(disp != NULL);

    /* Create WS server on ephemeral port 18089 */
    mcp_transport_t *ws_trans = mcp_transport_ws_create(loop, disp, "127.0.0.1", 18089);
    assert(ws_trans != NULL);

    int rc = ws_trans->start(ws_trans);
    assert(rc == 0);
    printf("  --> WebSocket / HTTP transport bound & listening on port 18089.\n");

    /* Stop & Destroy */
    ws_trans->stop(ws_trans);
    ws_trans->destroy(ws_trans);
    edge_mcp_dispatcher_destroy(disp);
    printf("  --> WebSocket transport lifecycle verified successfully!\n");
}

static void test_stdio_transport_lifecycle(void) {
    printf("[TEST] Testing stdio MCP transport lifecycle...\n");
    uv_loop_t *loop = uv_default_loop();

    edge_mcp_env_t env = {.loop = loop};
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(&env);
    assert(disp != NULL);

    mcp_transport_t *stdio_trans = mcp_transport_stdio_create(loop, disp);
    assert(stdio_trans != NULL);

    stdio_trans->destroy(stdio_trans);
    edge_mcp_dispatcher_destroy(disp);
    printf("  --> stdio transport lifecycle verified successfully!\n");
}

int main(void) {
    printf("=== EdgeOS Industrial MCP Server Protocol Tests (AEP-506) ===\n");
    test_ws_transport_lifecycle();
    test_stdio_transport_lifecycle();
    printf("=== All MCP Server Protocol Tests PASSED ===\n");
    return 0;
}
