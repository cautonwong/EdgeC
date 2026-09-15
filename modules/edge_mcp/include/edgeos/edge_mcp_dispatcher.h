/**
 * @file edge_mcp_dispatcher.h
 * @brief Industrial Model Context Protocol (MCP) Dispatcher for EdgeOS
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 *
 * Implements JSON-RPC 2.0 Model Context Protocol dispatcher exposing 13 industrial
 * tools across DataHub, Drivers/IO, Stream Engine, DDS Mesh, and Storage planes.
 */

#ifndef EDGE_MCP_DISPATCHER_H
#define EDGE_MCP_DISPATCHER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <cjson/cJSON.h>
#include <uv.h>

#include "datahub.h"
#include "driver_manager.h"
#include "edge_driver.h"
#include "stream_engine.h"
#include "storage_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MCP JSON-RPC 2.0 Error Codes */
#define MCP_RPC_PARSE_ERROR      (-32700)
#define MCP_RPC_INVALID_REQUEST  (-32600)
#define MCP_RPC_METHOD_NOT_FOUND (-32601)
#define MCP_RPC_INVALID_PARAMS   (-32602)
#define MCP_RPC_INTERNAL_ERROR   (-32603)
#define MCP_RPC_TOOL_EXEC_ERROR  (-32000)

typedef struct edge_mcp_dispatcher_s edge_mcp_dispatcher_t;

/**
 * @brief Tool Handler Callback Signature
 * @param disp Dispatcher instance
 * @param params JSON-RPC "params" object (cJSON*)
 * @param out_result Output JSON object allocated by handler, returned to caller
 * @param err_msg Error buffer (min 256 bytes) if handler returns non-zero
 * @param err_msg_len Size of err_msg buffer
 * @return 0 on success, negative MCP_RPC_* error code on failure
 */
typedef int (*edge_mcp_tool_handler_t)(edge_mcp_dispatcher_t *disp,
                                       const cJSON *params,
                                       cJSON **out_result,
                                       char *err_msg,
                                       size_t err_msg_len);

/**
 * @brief Tool Definition Registration Struct
 */
typedef struct {
    const char *name;
    const char *description;
    const char *input_schema_json;
    edge_mcp_tool_handler_t handler;
} edge_mcp_tool_def_t;

/**
 * @brief MCP Dispatcher Environment Context
 */
typedef struct {
    datahub_t *hub;
    driver_manager_t *drv_mgr;
    stream_engine_t *stream_eng;
    storage_driver_t *storage_drv;
    uv_loop_t *loop;
} edge_mcp_env_t;

/**
 * @brief Create a new MCP Dispatcher instance
 * @param env Environment context pointers
 * @return Dispatcher handle or NULL
 */
edge_mcp_dispatcher_t *edge_mcp_dispatcher_create(const edge_mcp_env_t *env);

/**
 * @brief Destroy MCP Dispatcher instance
 */
void edge_mcp_dispatcher_destroy(edge_mcp_dispatcher_t *disp);

/**
 * @brief Get environment context pointers from dispatcher
 */
const edge_mcp_env_t *edge_mcp_dispatcher_get_env(const edge_mcp_dispatcher_t *disp);

/**
 * @brief Register a custom tool handler
 */
int edge_mcp_register_tool(edge_mcp_dispatcher_t *disp, const edge_mcp_tool_def_t *tool_def);

/**
 * @brief Unregister a tool handler by name
 */
int edge_mcp_unregister_tool(edge_mcp_dispatcher_t *disp, const char *tool_name);

/**
 * @brief Dispatch a raw JSON-RPC 2.0 request string and return the response JSON string
 * @param disp Dispatcher instance
 * @param json_req Null-terminated JSON-RPC 2.0 request string
 * @return Allocated response string (caller MUST free with free()), or NULL
 */
char *edge_mcp_dispatch_json(edge_mcp_dispatcher_t *disp, const char *json_req);

/**
 * @brief Direct C API tool invoker
 * @param disp Dispatcher instance
 * @param tool_name Name of the tool
 * @param params_json JSON string of arguments
 * @param out_result_json Allocated result JSON string (caller frees)
 * @return 0 on success, negative error code on failure
 */
int edge_mcp_call_tool(edge_mcp_dispatcher_t *disp,
                       const char *tool_name,
                       const char *params_json,
                       char **out_result_json);

/**
 * @brief Get list of registered tools as a cJSON Array
 */
cJSON *edge_mcp_list_tools_json(edge_mcp_dispatcher_t *disp);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_MCP_DISPATCHER_H */
