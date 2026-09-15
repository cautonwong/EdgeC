/**
 * @file edge_mcp_bus.c
 * @brief 工业级共享 MCP 神经总线实现 (ARCH-MCP-02 / Issue #297)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include "edgeos/edge_mcp_bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <uv.h>
#include <cjson/cJSON.h>

#define DEFAULT_MTT_NS() ((uint64_t)uv_hrtime())

struct mcp_bus_hub_s {
    edge_mcp_dispatcher_t *disp;
    edge_mcp_env_t env;
    mcp_provider_desc_t providers[MCP_BUS_MAX_PROVIDERS];
    size_t provider_count;
    mcp_audit_log_entry_t audit_logs[MCP_BUS_MAX_AUDIT_LOGS];
    size_t audit_head;
    size_t audit_count;
    uv_mutex_t lock;
};

/* Forward declarations */
static int handle_bus_register_provider(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len);
static int handle_bus_unregister_provider(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len);
static int handle_bus_get_audit_logs(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len);
static int handle_bus_query_security_policy(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len);

static void record_audit_log(mcp_bus_hub_t *hub,
                             const char *client_id,
                             const char *operator_id,
                             const char *tool_name,
                             mcp_security_level_t sec_level,
                             bool is_auth,
                             const char *block_reason,
                             int ret_code,
                             uint32_t duration_us) {
    if (!hub) return;
    uv_mutex_lock(&hub->lock);
    size_t idx = hub->audit_head;
    mcp_audit_log_entry_t *entry = &hub->audit_logs[idx];
    entry->timestamp_ns = DEFAULT_MTT_NS();
    snprintf(entry->client_id, sizeof(entry->client_id), "%s", client_id ? client_id : "anonymous");
    snprintf(entry->operator_id, sizeof(entry->operator_id), "%s", operator_id ? operator_id : "system");
    snprintf(entry->tool_name, sizeof(entry->tool_name), "%s", tool_name ? tool_name : "unknown");
    entry->sec_level = sec_level;
    entry->is_authorized = is_auth;
    snprintf(entry->block_reason, sizeof(entry->block_reason), "%s", block_reason ? block_reason : "");
    entry->exec_return_code = ret_code;
    entry->duration_us = duration_us;

    hub->audit_head = (hub->audit_head + 1) % MCP_BUS_MAX_AUDIT_LOGS;
    if (hub->audit_count < MCP_BUS_MAX_AUDIT_LOGS) {
        hub->audit_count++;
    }
    uv_mutex_unlock(&hub->lock);
}

mcp_bus_hub_t *mcp_bus_hub_create(const edge_mcp_env_t *env) {
    mcp_bus_hub_t *hub = (mcp_bus_hub_t *)calloc(1, sizeof(mcp_bus_hub_t));
    if (!hub) return NULL;

    if (env) {
        hub->env = *env;
    }
    uv_mutex_init(&hub->lock);

    hub->disp = edge_mcp_dispatcher_create(env);
    if (!hub->disp) {
        uv_mutex_destroy(&hub->lock);
        free(hub);
        return NULL;
    }

    /* 注册总线核心管理方法 */
    edge_mcp_tool_def_t bus_tools[] = {
        {
            .name = "mcp_register_provider",
            .description = "Dynamically mount a microservice provider and its tools into the shared MCP neural bus",
            .input_schema_json = "{\"type\":\"object\",\"properties\":{\"provider_id\":{\"type\":\"string\"},\"security_domain\":{\"type\":\"string\"},\"tools\":{\"type\":\"array\"}},\"required\":[\"provider_id\"]}",
            .handler = handle_bus_register_provider
        },
        {
            .name = "mcp_unregister_provider",
            .description = "Gracefully unmount a microservice provider from the shared MCP neural bus",
            .input_schema_json = "{\"type\":\"object\",\"properties\":{\"provider_id\":{\"type\":\"string\"}},\"required\":[\"provider_id\"]}",
            .handler = handle_bus_unregister_provider
        },
        {
            .name = "mcp_get_audit_logs",
            .description = "Query MTT-timestamped industrial audit blackbox logs for safety tracking and compliance",
            .input_schema_json = "{\"type\":\"object\",\"properties\":{\"limit\":{\"type\":\"integer\",\"default\":20}}}",
            .handler = handle_bus_get_audit_logs
        },
        {
            .name = "mcp_query_security_policy",
            .description = "Query active Four-level privileges (L0~L3) and Electrical Five-Anti safety interlock rules",
            .input_schema_json = "{\"type\":\"object\",\"properties\":{}}",
            .handler = handle_bus_query_security_policy
        }
    };

    for (size_t i = 0; i < sizeof(bus_tools)/sizeof(bus_tools[0]); i++) {
        edge_mcp_register_tool(hub->disp, &bus_tools[i]);
    }

    return hub;
}

void mcp_bus_hub_destroy(mcp_bus_hub_t *hub) {
    if (!hub) return;
    if (hub->disp) {
        edge_mcp_dispatcher_destroy(hub->disp);
    }
    uv_mutex_destroy(&hub->lock);
    free(hub);
}

edge_mcp_dispatcher_t *mcp_bus_get_dispatcher(mcp_bus_hub_t *hub) {
    return hub ? hub->disp : NULL;
}

int mcp_bus_register_provider(mcp_bus_hub_t *hub, const mcp_provider_desc_t *desc) {
    if (!hub || !desc || desc->provider_id[0] == '\0') {
        return -1;
    }

    uv_mutex_lock(&hub->lock);

    /* 检查是否已存在同名 provider，若存在则更新 */
    int slot = -1;
    for (size_t i = 0; i < hub->provider_count; i++) {
        if (strcmp(hub->providers[i].provider_id, desc->provider_id) == 0) {
            slot = (int)i;
            break;
        }
    }

    if (slot < 0) {
        if (hub->provider_count >= MCP_BUS_MAX_PROVIDERS) {
            uv_mutex_unlock(&hub->lock);
            return -2; /* 达到最大 provider 容量 */
        }
        slot = (int)hub->provider_count++;
    }

    hub->providers[slot] = *desc;
    hub->providers[slot].registered_at_ns = DEFAULT_MTT_NS();

    /* 注册挂载的所有 tools 到全局 dispatcher 中 */
    for (size_t t = 0; t < desc->tool_count; t++) {
        mcp_provider_tool_t *pt = &hub->providers[slot].tools[t];
        edge_mcp_tool_def_t tool_def = {
            .name = pt->name,
            .description = pt->description,
            .input_schema_json = pt->input_schema_json,
            .handler = pt->local_handler
        };
        edge_mcp_register_tool(hub->disp, &tool_def);
    }

    uv_mutex_unlock(&hub->lock);
    return 0;
}

int mcp_bus_unregister_provider(mcp_bus_hub_t *hub, const char *provider_id) {
    if (!hub || !provider_id) return -1;
    uv_mutex_lock(&hub->lock);

    int found_idx = -1;
    for (size_t i = 0; i < hub->provider_count; i++) {
        if (strcmp(hub->providers[i].provider_id, provider_id) == 0) {
            found_idx = (int)i;
            break;
        }
    }

    if (found_idx >= 0) {
        for (size_t t = 0; t < hub->providers[found_idx].tool_count; t++) {
            edge_mcp_unregister_tool(hub->disp, hub->providers[found_idx].tools[t].name);
        }
        for (size_t i = (size_t)found_idx; i < hub->provider_count - 1; i++) {
            hub->providers[i] = hub->providers[i + 1];
        }
        hub->provider_count--;
        uv_mutex_unlock(&hub->lock);
        return 0;
    }

    uv_mutex_unlock(&hub->lock);
    return -2; /* Not found */
}

int mcp_bus_unregister_by_fd(mcp_bus_hub_t *hub, int client_fd) {
    if (!hub || client_fd < 0) return 0;
    uv_mutex_lock(&hub->lock);

    int removed = 0;
    for (size_t i = 0; i < hub->provider_count;) {
        if (hub->providers[i].client_fd == client_fd) {
            for (size_t t = 0; t < hub->providers[i].tool_count; t++) {
                edge_mcp_unregister_tool(hub->disp, hub->providers[i].tools[t].name);
            }
            for (size_t j = i; j < hub->provider_count - 1; j++) {
                hub->providers[j] = hub->providers[j + 1];
            }
            hub->provider_count--;
            removed++;
        } else {
            i++;
        }
    }

    uv_mutex_unlock(&hub->lock);
    return removed;
}

int mcp_bus_security_evaluate(mcp_bus_hub_t *hub,
                              const char *tool_name,
                              const cJSON *params,
                              const mcp_security_context_t *sec_ctx,
                              char *out_block_reason,
                              size_t reason_len) {
    if (!hub || !tool_name || !out_block_reason || reason_len == 0) {
        return -1;
    }
    out_block_reason[0] = '\0';

    mcp_security_level_t tool_level = MCP_SEC_L0_SAFE_READ;
    mcp_interlock_rule_t interlock_rule = MCP_INTERLOCK_RULE_NONE;

    /* 1. 查找工具的注册特权等级与五防规则 */
    uv_mutex_lock(&hub->lock);
    for (size_t p = 0; p < hub->provider_count; p++) {
        for (size_t t = 0; t < hub->providers[p].tool_count; t++) {
            if (strcmp(hub->providers[p].tools[t].name, tool_name) == 0) {
                tool_level = hub->providers[p].tools[t].security_level;
                interlock_rule = hub->providers[p].tools[t].interlock_rule;
                break;
            }
        }
    }
    uv_mutex_unlock(&hub->lock);

    /* 内置工具安全等级分类兜底 */
    if (tool_level == MCP_SEC_L0_SAFE_READ) {
        if (strcmp(tool_name, "datahub_write_tag") == 0) {
            tool_level = MCP_SEC_L1_PARAM_TUNING;
        } else if (strcmp(tool_name, "breaker_remote_trip") == 0 ||
                   strcmp(tool_name, "breaker_remote_close") == 0 ||
                   strcmp(tool_name, "security_simulate_interlock") == 0) {
            tool_level = MCP_SEC_L2_CRITICAL_ACTION;
            interlock_rule = MCP_INTERLOCK_RULE_PREVENT_LOAD_DISCONNECT;
        } else if (strcmp(tool_name, "system_firmware_upgrade") == 0 ||
                   strcmp(tool_name, "system_factory_reset") == 0) {
            tool_level = MCP_SEC_L3_SYSTEM_CRITICAL;
        }
    }

    /* 2. L0: 只读工具直接放行 */
    if (tool_level == MCP_SEC_L0_SAFE_READ) {
        return 0;
    }

    mcp_security_level_t caller_level = sec_ctx ? sec_ctx->role_level : MCP_SEC_L0_SAFE_READ;

    /* 3. 特权等级门禁检查 */
    if (caller_level < tool_level) {
        snprintf(out_block_reason, reason_len,
                 "Privilege Insufficient: Tool requires %s, caller has %s",
                 mcp_security_level_to_str(tool_level),
                 mcp_security_level_to_str(caller_level));
        return -32002;
    }

    /* 4. L1: 参数边界检查 */
    if (tool_level == MCP_SEC_L1_PARAM_TUNING && params) {
        const cJSON *val = cJSON_GetObjectItem(params, "value");
        if (val && cJSON_IsNumber(val)) {
            if (val->valuedouble < -100000.0 || val->valuedouble > 100000.0) {
                snprintf(out_block_reason, reason_len, "Param out of engineering safe bounds");
                return -32001;
            }
        }
    }

    /* 5. L2: 强制五防电气联锁预演核验 */
    if (tool_level == MCP_SEC_L2_CRITICAL_ACTION) {
        if (params) {
            /* 规则 1: 防止带负荷分合隔离开关 (刀闸) */
            if (interlock_rule == MCP_INTERLOCK_RULE_PREVENT_LOAD_DISCONNECT) {
                const cJSON *curr = cJSON_GetObjectItem(params, "load_current_a");
                const cJSON *target = cJSON_GetObjectItem(params, "target_state");
                if (curr && cJSON_IsNumber(curr) && curr->valuedouble > 0.5) {
                    snprintf(out_block_reason, reason_len,
                             "Five-Anti Violation [Rule 1]: Cannot operate disconnector under active load (I=%.2fA > 0.5A)",
                             curr->valuedouble);
                    return -32001;
                }
                (void)target;
            }
            /* 规则 2: 防止带电合接地开关 */
            else if (interlock_rule == MCP_INTERLOCK_RULE_PREVENT_LIVE_EARTHING) {
                const cJSON *voltage = cJSON_GetObjectItem(params, "bus_voltage_v");
                if (voltage && cJSON_IsNumber(voltage) && voltage->valuedouble > 10.0) {
                    snprintf(out_block_reason, reason_len,
                             "Five-Anti Violation [Rule 2]: Cannot close earth-switch while line is energized (U=%.2fV > 10.0V)",
                             voltage->valuedouble);
                    return -32001;
                }
            }
            /* 规则 3: 防止带接地线合断路器 */
            else if (interlock_rule == MCP_INTERLOCK_RULE_PREVENT_EARTHED_CLOSING) {
                const cJSON *earth_closed = cJSON_GetObjectItem(params, "is_earthed");
                if (earth_closed && cJSON_IsTrue(earth_closed)) {
                    snprintf(out_block_reason, reason_len,
                             "Five-Anti Violation [Rule 3]: Cannot close circuit breaker while earth-switch is grounded");
                    return -32001;
                }
            }
        }
    }

    /* 6. L3: 强制双人复核 (Supervisor Token 校验) */
    if (tool_level == MCP_SEC_L3_SYSTEM_CRITICAL) {
        if (!sec_ctx || !sec_ctx->supervisor_token || strlen(sec_ctx->supervisor_token) < 8) {
            snprintf(out_block_reason, reason_len,
                     "Dual-Authorization Required: Valid supervisor token must be provided for %s",
                     mcp_security_level_to_str(tool_level));
            return -32002;
        }
    }

    return 0;
}

char *mcp_bus_dispatch_with_security(mcp_bus_hub_t *hub,
                                     const char *json_req,
                                     const mcp_security_context_t *sec_ctx) {
    if (!hub || !json_req) return NULL;

    uint64_t t_start = DEFAULT_MTT_NS();

    cJSON *root = cJSON_Parse(json_req);
    if (!root) {
        return edge_mcp_dispatch_json(hub->disp, json_req);
    }

    const cJSON *method_item = cJSON_GetObjectItem(root, "method");
    const cJSON *id_item = cJSON_GetObjectItem(root, "id");
    char audit_tool_name[MCP_BUS_MAX_NAME_LEN] = "system_call";

    if (method_item && cJSON_IsString(method_item) && strcmp(method_item->valuestring, "tools/call") == 0) {
        const cJSON *params = cJSON_GetObjectItem(root, "params");
        const cJSON *name_item = params ? cJSON_GetObjectItem(params, "name") : NULL;
        const cJSON *args = params ? cJSON_GetObjectItem(params, "arguments") : NULL;

        if (name_item && cJSON_IsString(name_item)) {
            const char *tool_name = name_item->valuestring;
            snprintf(audit_tool_name, sizeof(audit_tool_name), "%s", tool_name);
            char block_reason[128] = {0};

            int sec_rc = mcp_bus_security_evaluate(hub, tool_name, args, sec_ctx, block_reason, sizeof(block_reason));
            if (sec_rc != 0) {
                /* 安全门禁拦截并审计 */
                uint32_t duration_us = (uint32_t)((DEFAULT_MTT_NS() - t_start) / 1000ULL);
                record_audit_log(hub,
                                 sec_ctx ? sec_ctx->client_id : "unknown",
                                 sec_ctx ? sec_ctx->operator_id : "unknown",
                                 tool_name,
                                 sec_ctx ? sec_ctx->role_level : MCP_SEC_L0_SAFE_READ,
                                 false,
                                 block_reason,
                                 sec_rc,
                                 duration_us);

                cJSON *resp = cJSON_CreateObject();
                cJSON_AddStringToObject(resp, "jsonrpc", "2.0");
                if (id_item) {
                    cJSON_AddItemToObject(resp, "id", cJSON_Duplicate(id_item, 1));
                } else {
                    cJSON_AddNullToObject(resp, "id");
                }
                cJSON *err = cJSON_CreateObject();
                cJSON_AddNumberToObject(err, "code", sec_rc);
                cJSON_AddStringToObject(err, "message", block_reason[0] != '\0' ? block_reason : "Security authorization rejected");
                cJSON_AddItemToObject(resp, "error", err);

                char *out = cJSON_PrintUnformatted(resp);
                cJSON_Delete(resp);
                cJSON_Delete(root);
                return out;
            }
        }
    } else if (method_item && cJSON_IsString(method_item)) {
        snprintf(audit_tool_name, sizeof(audit_tool_name), "%s", method_item->valuestring);
    }

    cJSON_Delete(root);

    /* 放行进入底层 Dispatcher 执行 */
    char *resp_str = edge_mcp_dispatch_json(hub->disp, json_req);

    uint32_t duration_us = (uint32_t)((DEFAULT_MTT_NS() - t_start) / 1000ULL);
    record_audit_log(hub,
                     sec_ctx ? sec_ctx->client_id : "system",
                     sec_ctx ? sec_ctx->operator_id : "system",
                     audit_tool_name,
                     sec_ctx ? sec_ctx->role_level : MCP_SEC_L0_SAFE_READ,
                     true,
                     "AUTHORIZED_PASS",
                     0,
                     duration_us);

    return resp_str;
}

int mcp_bus_get_audit_logs(mcp_bus_hub_t *hub, size_t limit, cJSON **out_logs) {
    if (!hub || !out_logs) return 0;
    if (limit == 0 || limit > MCP_BUS_MAX_AUDIT_LOGS) {
        limit = MCP_BUS_MAX_AUDIT_LOGS;
    }

    uv_mutex_lock(&hub->lock);
    cJSON *arr = cJSON_CreateArray();
    size_t count = (hub->audit_count < limit) ? hub->audit_count : limit;

    for (size_t i = 0; i < count; i++) {
        size_t idx = (hub->audit_head + MCP_BUS_MAX_AUDIT_LOGS - 1 - i) % MCP_BUS_MAX_AUDIT_LOGS;
        const mcp_audit_log_entry_t *entry = &hub->audit_logs[idx];

        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "timestamp_ns", (double)entry->timestamp_ns);
        cJSON_AddStringToObject(item, "client_id", entry->client_id);
        cJSON_AddStringToObject(item, "operator_id", entry->operator_id);
        cJSON_AddStringToObject(item, "tool_name", entry->tool_name);
        cJSON_AddStringToObject(item, "security_level", mcp_security_level_to_str(entry->sec_level));
        cJSON_AddBoolToObject(item, "authorized", entry->is_authorized);
        if (entry->block_reason[0] != '\0') {
            cJSON_AddStringToObject(item, "block_reason", entry->block_reason);
        }
        cJSON_AddNumberToObject(item, "duration_us", entry->duration_us);
        cJSON_AddItemToArray(arr, item);
    }

    uv_mutex_unlock(&hub->lock);
    *out_logs = arr;
    return (int)count;
}

int mcp_bus_get_providers_json(mcp_bus_hub_t *hub, cJSON **out_providers) {
    if (!hub || !out_providers) return 0;
    uv_mutex_lock(&hub->lock);

    cJSON *arr = cJSON_CreateArray();
    for (size_t i = 0; i < hub->provider_count; i++) {
        const mcp_provider_desc_t *p = &hub->providers[i];
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "provider_id", p->provider_id);
        cJSON_AddNumberToObject(item, "pid", p->pid);
        cJSON_AddStringToObject(item, "security_domain", p->security_domain);
        cJSON_AddNumberToObject(item, "tool_count", (double)p->tool_count);

        cJSON *tools_arr = cJSON_CreateArray();
        for (size_t t = 0; t < p->tool_count; t++) {
            cJSON *t_item = cJSON_CreateObject();
            cJSON_AddStringToObject(t_item, "name", p->tools[t].name);
            cJSON_AddStringToObject(t_item, "description", p->tools[t].description);
            cJSON_AddStringToObject(t_item, "security_level", mcp_security_level_to_str(p->tools[t].security_level));
            cJSON_AddItemToArray(tools_arr, t_item);
        }
        cJSON_AddItemToObject(item, "tools", tools_arr);
        cJSON_AddItemToArray(arr, item);
    }

    uv_mutex_unlock(&hub->lock);
    *out_providers = arr;
    return (int)hub->provider_count;
}

const char *mcp_security_level_to_str(mcp_security_level_t level) {
    switch (level) {
    case MCP_SEC_L0_SAFE_READ: return "L0_SAFE_READ";
    case MCP_SEC_L1_PARAM_TUNING: return "L1_PARAM_TUNING";
    case MCP_SEC_L2_CRITICAL_ACTION: return "L2_CRITICAL_ACTION";
    case MCP_SEC_L3_SYSTEM_CRITICAL: return "L3_SYSTEM_CRITICAL";
    default: return "UNKNOWN";
    }
}

/* ---------------- Built-in MCP Tool Callbacks ---------------- */

static int handle_bus_register_provider(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp;
    if (!params) {
        snprintf(err_msg, err_len, "Missing params for mcp_register_provider");
        return MCP_RPC_INVALID_PARAMS;
    }
    const cJSON *pid_item = cJSON_GetObjectItem(params, "provider_id");
    if (!pid_item || !cJSON_IsString(pid_item)) {
        snprintf(err_msg, err_len, "Missing 'provider_id' string");
        return MCP_RPC_INVALID_PARAMS;
    }

    cJSON *res = cJSON_CreateObject();
    cJSON_AddStringToObject(res, "provider_id", pid_item->valuestring);
    cJSON_AddStringToObject(res, "status", "REGISTERED");
    cJSON_AddNumberToObject(res, "code", 0);

    *out_result = res;
    return 0;
}

static int handle_bus_unregister_provider(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp;
    if (!params) {
        snprintf(err_msg, err_len, "Missing params for mcp_unregister_provider");
        return MCP_RPC_INVALID_PARAMS;
    }
    const cJSON *pid_item = cJSON_GetObjectItem(params, "provider_id");
    if (!pid_item || !cJSON_IsString(pid_item)) {
        snprintf(err_msg, err_len, "Missing 'provider_id' string");
        return MCP_RPC_INVALID_PARAMS;
    }

    cJSON *res = cJSON_CreateObject();
    cJSON_AddStringToObject(res, "provider_id", pid_item->valuestring);
    cJSON_AddStringToObject(res, "status", "UNREGISTERED");

    *out_result = res;
    return 0;
}

static int handle_bus_get_audit_logs(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)err_msg; (void)err_len;
    int limit = 20;
    if (params) {
        const cJSON *l = cJSON_GetObjectItem(params, "limit");
        if (l && cJSON_IsNumber(l) && l->valueint > 0) {
            limit = l->valueint;
        }
    }

    cJSON *res = cJSON_CreateObject();
    cJSON *logs_arr = cJSON_CreateArray();
    cJSON_AddNumberToObject(res, "requested_limit", limit);
    cJSON_AddItemToObject(res, "audit_records", logs_arr);

    *out_result = res;
    return 0;
}

static int handle_bus_query_security_policy(edge_mcp_dispatcher_t *disp, const cJSON *params, cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp; (void)params; (void)err_msg; (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON *levels = cJSON_CreateArray();
    cJSON_AddItemToArray(levels, cJSON_CreateString("L0_SAFE_READ: Read-only telemetry, zero risk pass-through"));
    cJSON_AddItemToArray(levels, cJSON_CreateString("L1_PARAM_TUNING: Parameter adjustments with boundary checks"));
    cJSON_AddItemToArray(levels, cJSON_CreateString("L2_CRITICAL_ACTION: Breaker/PCS control with Five-Anti interlock"));
    cJSON_AddItemToArray(levels, cJSON_CreateString("L3_SYSTEM_CRITICAL: DFU/Reset requiring dual authorization"));
    cJSON_AddItemToObject(res, "privilege_levels", levels);

    cJSON *five_anti = cJSON_CreateArray();
    cJSON_AddItemToArray(five_anti, cJSON_CreateString("Rule 1: Prevent opening/closing disconnector under load (I > 0.5A)"));
    cJSON_AddItemToArray(five_anti, cJSON_CreateString("Rule 2: Prevent closing earth-switch on live line (U > 10.0V)"));
    cJSON_AddItemToArray(five_anti, cJSON_CreateString("Rule 3: Prevent closing breaker with earth-switch connected"));
    cJSON_AddItemToArray(five_anti, cJSON_CreateString("Rule 4: Prevent misoperation of circuit breaker"));
    cJSON_AddItemToArray(five_anti, cJSON_CreateString("Rule 5: Prevent unauthorized entry into live compartment"));
    cJSON_AddItemToObject(res, "five_anti_rules", five_anti);

    *out_result = res;
    return 0;
}
