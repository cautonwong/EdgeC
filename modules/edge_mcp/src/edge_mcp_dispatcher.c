/**
 * @file edge_mcp_dispatcher.c
 * @brief Industrial Model Context Protocol (MCP) Dispatcher Implementation
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <uv.h>
#include <cjson/cJSON.h>

#include "edgeos/edge_mcp_dispatcher.h"

#define MAX_MCP_TOOLS 64

struct edge_mcp_dispatcher_s {
    edge_mcp_env_t env;
    edge_mcp_tool_def_t tools[MAX_MCP_TOOLS];
    size_t tool_count;
    uv_mutex_t lock;
};

/* ---------------- 4 Built-in DataHub Tool Handlers ---------------- */

/* 1. datahub_read_tags */
static int handle_datahub_read_tags(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *tags_arr = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "tags", tags_arr);

    const cJSON *tag_ids = cJSON_GetObjectItem(params, "tag_ids");
    if (tag_ids && cJSON_IsArray(tag_ids)) {
        int sz = cJSON_GetArraySize(tag_ids);
        for (int i = 0; i < sz; i++) {
            cJSON *item = cJSON_GetArrayItem(tag_ids, i);
            if (!item || !cJSON_IsNumber(item)) continue;
            uint32_t tid = (uint32_t)item->valueint;

            cJSON *tag_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(tag_obj, "tag_id", tid);

            if (disp->env.tools.read_tag) {
                edge_mcp_tag_value_t value = {0};
                if (edge_mcp_tool_read_tag(&disp->env.tools, tid, &value) == 0) {
                    cJSON_AddNumberToObject(tag_obj, "value", value.value);
                    cJSON_AddNumberToObject(tag_obj, "quality", value.quality);
                    cJSON_AddNumberToObject(tag_obj, "timestamp", (double)value.timestamp_ms);
                    cJSON_AddNumberToObject(tag_obj, "type", value.type);
                } else {
                    cJSON_AddNullToObject(tag_obj, "value");
                    cJSON_AddStringToObject(tag_obj, "status", "not_found");
                }
            } else {
                cJSON_AddNumberToObject(tag_obj, "value", 0.0);
                cJSON_AddNumberToObject(tag_obj, "quality", 0);
            }
            cJSON_AddItemToArray(tags_arr, tag_obj);
        }
    } else {
        snprintf(err_msg, err_len, "Missing or invalid 'tag_ids' array in params");
        cJSON_Delete(res);
        return MCP_RPC_INVALID_PARAMS;
    }

    *out_result = res;
    return 0;
}

/* 2. datahub_write_tag */
static int handle_datahub_write_tag(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    const cJSON *tag_id_item = cJSON_GetObjectItem(params, "tag_id");
    const cJSON *val_item = cJSON_GetObjectItem(params, "value");
    if (!tag_id_item || !cJSON_IsNumber(tag_id_item) || !val_item || !cJSON_IsNumber(val_item)) {
        snprintf(err_msg, err_len, "Params must include numeric 'tag_id' and 'value'");
        return MCP_RPC_INVALID_PARAMS;
    }

    uint32_t tid = (uint32_t)tag_id_item->valueint;
    float val = (float)val_item->valuedouble;
    uint8_t q = 0;
    const cJSON *q_item = cJSON_GetObjectItem(params, "quality");
    if (q_item && cJSON_IsNumber(q_item)) {
        q = (uint8_t)q_item->valueint;
    }

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "tag_id", tid);
    cJSON_AddNumberToObject(res, "written_value", val);

    if (disp->env.tools.write_tag) {
        edge_mcp_tag_value_t value = {
            .tag_id = tid,
            .value = val,
            .quality = q,
            .type = EDGE_MCP_TAG_TYPE_SCALAR,
            .timestamp_ms = (uint64_t)uv_hrtime() / 1000000ULL
        };
        int rc = edge_mcp_tool_write_tag(&disp->env.tools, &value);
        cJSON_AddBoolToObject(res, "success", (rc == 0));
        cJSON_AddNumberToObject(res, "code", rc);
    } else {
        cJSON_AddBoolToObject(res, "success", true);
        cJSON_AddNumberToObject(res, "code", 0);
    }

    *out_result = res;
    return 0;
}

/* 3. datahub_get_tag_meta */
static int handle_datahub_get_tag_meta(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    const cJSON *tag_id_item = cJSON_GetObjectItem(params, "tag_id");
    if (!tag_id_item || !cJSON_IsNumber(tag_id_item)) {
        snprintf(err_msg, err_len, "Missing numeric 'tag_id'");
        return MCP_RPC_INVALID_PARAMS;
    }

    if (!disp->env.tools.get_tag_meta) {
        snprintf(err_msg, err_len, "Tag metadata provider is not available");
        return MCP_RPC_TOOL_EXEC_ERROR;
    }

    uint32_t tid = (uint32_t)tag_id_item->valueint;
    edge_mcp_tag_meta_t meta = {0};
    if (edge_mcp_tool_get_tag_meta(&disp->env.tools, tid, &meta) != 0) {
        snprintf(err_msg, err_len, "Tag metadata not found");
        return MCP_RPC_TOOL_EXEC_ERROR;
    }

    cJSON *res = cJSON_CreateObject();
    if (!res) return MCP_RPC_INTERNAL_ERROR;
    cJSON_AddNumberToObject(res, "tag_id", meta.tag_id);
    cJSON_AddStringToObject(res, "name", meta.name);
    cJSON_AddStringToObject(res, "unit", meta.unit);
    cJSON_AddStringToObject(res, "category", meta.category);
    cJSON_AddNumberToObject(res, "min", meta.min_value);
    cJSON_AddNumberToObject(res, "max", meta.max_value);

    *out_result = res;
    return 0;
}

/* 4. datahub_scan_abnormal */
static int handle_datahub_scan_abnormal(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)params; (void)err_msg; (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *abnormal_list = cJSON_CreateArray();
    cJSON_AddItemToObject(res, "abnormal_tags", abnormal_list);

    int count = 0;
    if (disp->env.tools.read_tag) {
        for (uint32_t tid = 1; tid < 256; tid++) {
            edge_mcp_tag_value_t value = {0};
            if (edge_mcp_tool_read_tag(&disp->env.tools, tid, &value) == 0) {
                if (value.quality != 0) {
                    cJSON *item = cJSON_CreateObject();
                    cJSON_AddNumberToObject(item, "tag_id", tid);
                    cJSON_AddNumberToObject(item, "value", value.value);
                    cJSON_AddNumberToObject(item, "quality", value.quality);
                    cJSON_AddItemToArray(abnormal_list, item);
                    count++;
                }
            }
        }
    }
    cJSON_AddNumberToObject(res, "abnormal_count", count);

    *out_result = res;
    return 0;
}

/* ---------------- Tool Definitions Array ---------------- */

static const edge_mcp_tool_def_t g_default_tools[] = {
    {
        .name = "datahub_read_tags",
        .description = "Read real-time physical telemetry tags from DataHub shared memory matrix by Tag IDs",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"tag_ids\":{\"type\":\"array\",\"items\":{\"type\":\"integer\"},\"description\":\"Array of numerical Tag IDs to query\"}},\"required\":[\"tag_ids\"]}",
        .handler = handle_datahub_read_tags
    },
    {
        .name = "datahub_write_tag",
        .description = "Write a calibrated physical setpoint or telecontrol command to DataHub",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"tag_id\":{\"type\":\"integer\"},\"value\":{\"type\":\"number\"},\"quality\":{\"type\":\"integer\",\"default\":0}},\"required\":[\"tag_id\",\"value\"]}",
        .handler = handle_datahub_write_tag
    },
    {
        .name = "datahub_get_tag_meta",
        .description = "Get static contract metadata (name, engineering unit, category, physical range) for a Tag ID",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"tag_id\":{\"type\":\"integer\"}},\"required\":[\"tag_id\"]}",
        .handler = handle_datahub_get_tag_meta
    },
    {
        .name = "datahub_scan_abnormal",
        .description = "Scan DataHub memory matrix for all tags with non-zero quality or alarm state",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
        .handler = handle_datahub_scan_abnormal
    },
};

/* ---------------- Dispatcher Lifecycle & Registration ---------------- */

edge_mcp_dispatcher_t *edge_mcp_dispatcher_create(const edge_mcp_env_t *env) {
    edge_mcp_dispatcher_t *disp = (edge_mcp_dispatcher_t *)calloc(1, sizeof(edge_mcp_dispatcher_t));
    if (!disp) return NULL;

    if (env) {
        disp->env = *env;
    }
    uv_mutex_init(&disp->lock);

    /* Register the standard DataHub tools. Product-specific tool packs are
     * registered by the composition root after creation. */
    size_t num_defaults = sizeof(g_default_tools) / sizeof(g_default_tools[0]);
    for (size_t i = 0; i < num_defaults; i++) {
        if (disp->env.hide_tag_meta && strcmp(g_default_tools[i].name, "datahub_get_tag_meta") == 0) {
            continue;
        }
        edge_mcp_register_tool(disp, &g_default_tools[i]);
    }

    return disp;
}

void edge_mcp_dispatcher_destroy(edge_mcp_dispatcher_t *disp) {
    if (!disp) return;
    uv_mutex_destroy(&disp->lock);
    free(disp);
}

const edge_mcp_env_t *edge_mcp_dispatcher_get_env(const edge_mcp_dispatcher_t *disp) {
    return disp ? &disp->env : NULL;
}

int edge_mcp_register_tool(edge_mcp_dispatcher_t *disp, const edge_mcp_tool_def_t *tool_def) {
    if (!disp || !tool_def || !tool_def->name || !tool_def->handler) return -1;
    uv_mutex_lock(&disp->lock);
    for (size_t i = 0; i < disp->tool_count; i++) {
        if (strcmp(disp->tools[i].name, tool_def->name) == 0) {
            disp->tools[i] = *tool_def;
            uv_mutex_unlock(&disp->lock);
            return 0;
        }
    }
    if (disp->tool_count >= MAX_MCP_TOOLS) {
        uv_mutex_unlock(&disp->lock);
        return -1;
    }
    disp->tools[disp->tool_count++] = *tool_def;
    uv_mutex_unlock(&disp->lock);
    return 0;
}

int edge_mcp_unregister_tool(edge_mcp_dispatcher_t *disp, const char *tool_name) {
    if (!disp || !tool_name) return -1;
    uv_mutex_lock(&disp->lock);
    for (size_t i = 0; i < disp->tool_count; i++) {
        if (strcmp(disp->tools[i].name, tool_name) == 0) {
            for (size_t j = i; j < disp->tool_count - 1; j++) {
                disp->tools[j] = disp->tools[j + 1];
            }
            disp->tool_count--;
            uv_mutex_unlock(&disp->lock);
            return 0;
        }
    }
    uv_mutex_unlock(&disp->lock);
    return -1;
}

cJSON *edge_mcp_list_tools_json(edge_mcp_dispatcher_t *disp) {
    if (!disp) return NULL;
    uv_mutex_lock(&disp->lock);
    cJSON *tools_arr = cJSON_CreateArray();
    for (size_t i = 0; i < disp->tool_count; i++) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddStringToObject(t, "name", disp->tools[i].name);
        cJSON_AddStringToObject(t, "description", disp->tools[i].description ? disp->tools[i].description : "");
        if (disp->tools[i].input_schema_json) {
            cJSON *schema = cJSON_Parse(disp->tools[i].input_schema_json);
            if (schema) {
                cJSON_AddItemToObject(t, "inputSchema", schema);
            } else {
                cJSON_AddObjectToObject(t, "inputSchema");
            }
        } else {
            cJSON_AddObjectToObject(t, "inputSchema");
        }
        cJSON_AddItemToArray(tools_arr, t);
    }
    uv_mutex_unlock(&disp->lock);
    return tools_arr;
}

int edge_mcp_call_tool(edge_mcp_dispatcher_t *disp,
                       const char *tool_name,
                       const char *params_json,
                       char **out_result_json) {
    if (!disp || !tool_name || !out_result_json) return MCP_RPC_INVALID_REQUEST;

    cJSON *params = NULL;
    if (params_json && strlen(params_json) > 0) {
        params = cJSON_Parse(params_json);
    }

    edge_mcp_tool_handler_t handler = NULL;
    uv_mutex_lock(&disp->lock);
    for (size_t i = 0; i < disp->tool_count; i++) {
        if (strcmp(disp->tools[i].name, tool_name) == 0) {
            handler = disp->tools[i].handler;
            break;
        }
    }
    uv_mutex_unlock(&disp->lock);

    if (!handler) {
        if (params) cJSON_Delete(params);
        return MCP_RPC_METHOD_NOT_FOUND;
    }

    cJSON *result_obj = NULL;
    char err_msg[256] = {0};
    int rc = handler(disp, params, &result_obj, err_msg, sizeof(err_msg));
    if (params) cJSON_Delete(params);

    if (rc == 0 && result_obj) {
        *out_result_json = cJSON_PrintUnformatted(result_obj);
        cJSON_Delete(result_obj);
        return 0;
    }

    if (result_obj) cJSON_Delete(result_obj);
    return (rc != 0) ? rc : MCP_RPC_TOOL_EXEC_ERROR;
}

/* ---------------- JSON-RPC 2.0 Dispatch Engine ---------------- */

static char *build_error_response(const cJSON *id_item, int code, const char *msg) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    if (id_item) {
        cJSON_AddItemToObject(resp, "id", cJSON_Duplicate(id_item, 1));
    } else {
        cJSON_AddNullToObject(resp, "id");
    }
    cJSON *err = cJSON_CreateObject();
    cJSON_AddNumberToObject(err, "code", code);
    cJSON_AddStringToObject(err, "message", msg ? msg : "Internal Error");
    cJSON_AddItemToObject(resp, "error", err);

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    return out;
}

char *edge_mcp_dispatch_json(edge_mcp_dispatcher_t *disp, const char *json_req) {
    if (!disp || !json_req) {
        return build_error_response(NULL, MCP_RPC_INVALID_REQUEST, "Invalid request");
    }

    cJSON *root = cJSON_Parse(json_req);
    if (!root) {
        return build_error_response(NULL, MCP_RPC_PARSE_ERROR, "Parse error");
    }

    const cJSON *id_item = cJSON_GetObjectItem(root, "id");
    const cJSON *method_item = cJSON_GetObjectItem(root, "method");
    if (!method_item || !cJSON_IsString(method_item)) {
        char *resp = build_error_response(id_item, MCP_RPC_INVALID_REQUEST, "Missing method field");
        cJSON_Delete(root);
        return resp;
    }

    const char *method = method_item->valuestring;
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
    if (id_item) {
        cJSON_AddItemToObject(resp, "id", cJSON_Duplicate(id_item, 1));
    } else {
        cJSON_AddNullToObject(resp, "id");
    }

    /* 1. MCP initialize handshake */
    if (strcmp(method, "initialize") == 0) {
        cJSON *res = cJSON_CreateObject();
        cJSON_AddStringToObject(res, "protocolVersion", "2024-11-05");
        cJSON *server_info = cJSON_CreateObject();
        cJSON_AddStringToObject(server_info, "name", "edge-mcp-server");
        cJSON_AddStringToObject(server_info, "version", "2.1.0");
        cJSON_AddItemToObject(res, "serverInfo", server_info);

        cJSON *caps = cJSON_CreateObject();
        cJSON *tools_cap = cJSON_CreateObject();
        cJSON_AddBoolToObject(tools_cap, "listChanged", true);
        cJSON_AddItemToObject(caps, "tools", tools_cap);
        cJSON_AddItemToObject(res, "capabilities", caps);

        cJSON_AddItemToObject(resp, "result", res);
    }
    /* 2. MCP tools/list */
    else if (strcmp(method, "tools/list") == 0) {
        cJSON *res = cJSON_CreateObject();
        cJSON_AddItemToObject(res, "tools", edge_mcp_list_tools_json(disp));
        cJSON_AddItemToObject(resp, "result", res);
    }
    /* 3. MCP tools/call */
    else if (strcmp(method, "tools/call") == 0) {
        const cJSON *params = cJSON_GetObjectItem(root, "params");
        const cJSON *name_item = params ? cJSON_GetObjectItem(params, "name") : NULL;
        const cJSON *args = params ? cJSON_GetObjectItem(params, "arguments") : NULL;

        if (!name_item || !cJSON_IsString(name_item)) {
            cJSON_Delete(resp);
            char *err_resp = build_error_response(id_item, MCP_RPC_INVALID_PARAMS, "tools/call missing params.name");
            cJSON_Delete(root);
            return err_resp;
        }

        const char *tool_name = name_item->valuestring;
        edge_mcp_tool_handler_t handler = NULL;
        uv_mutex_lock(&disp->lock);
        for (size_t i = 0; i < disp->tool_count; i++) {
            if (strcmp(disp->tools[i].name, tool_name) == 0) {
                handler = disp->tools[i].handler;
                break;
            }
        }
        uv_mutex_unlock(&disp->lock);

        if (!handler) {
            cJSON_Delete(resp);
            char *err_resp = build_error_response(id_item, MCP_RPC_METHOD_NOT_FOUND, "Tool not found");
            cJSON_Delete(root);
            return err_resp;
        }

        cJSON *tool_res = NULL;
        char err_msg[256] = {0};
        int rc = handler(disp, args, &tool_res, err_msg, sizeof(err_msg));
        if (rc != 0) {
            cJSON_Delete(resp);
            char *err_resp = build_error_response(id_item, rc, (err_msg[0] != '\0') ? err_msg : "Tool execution failed");
            cJSON_Delete(root);
            return err_resp;
        }

        cJSON *mcp_content = cJSON_CreateObject();
        cJSON *content_arr = cJSON_CreateArray();
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "type", "text");
        char *text_payload = tool_res ? cJSON_PrintUnformatted(tool_res) : strdup("{}");
        cJSON_AddStringToObject(item, "text", text_payload ? text_payload : "{}");
        if (text_payload) free(text_payload);
        if (tool_res) cJSON_Delete(tool_res);

        cJSON_AddItemToArray(content_arr, item);
        cJSON_AddItemToObject(mcp_content, "content", content_arr);
        cJSON_AddBoolToObject(mcp_content, "isError", false);

        cJSON_AddItemToObject(resp, "result", mcp_content);
    }
    /* 4. MCP ping */
    else if (strcmp(method, "ping") == 0) {
        cJSON_AddObjectToObject(resp, "result");
    }
    /* Unknown method */
    else {
        cJSON_Delete(resp);
        char *err_resp = build_error_response(id_item, MCP_RPC_METHOD_NOT_FOUND, "Unknown method");
        cJSON_Delete(root);
        return err_resp;
    }

    char *out = cJSON_PrintUnformatted(resp);
    cJSON_Delete(resp);
    cJSON_Delete(root);
    return out;
}
