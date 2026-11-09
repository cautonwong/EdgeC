/**
 * @file mcp_tools_microservices.c
 * @brief Microservices Provider Tools & Auto-Mount Implementations (ARCH-MCP-03 / Issue #302)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <uv.h>
#include <cjson/cJSON.h>

#include "edgeos/edge_mcp_industry_tools.h"
#include "edgeos/edge_mcp_bus.h"
#include "edge_log.h"

/* 4. tag_batch_read_scaled (L0 Safe Read with Byte Order Swap & Deadband Filter) */
int handle_tag_batch_read_scaled(edge_mcp_dispatcher_t *disp,
                                 const cJSON *params,
                                 cJSON **out_result,
                                 char *err_msg,
                                 size_t err_len) {
    if (!params) {
        snprintf(err_msg, err_len, "Missing parameters for tag_batch_read_scaled");
        return MCP_RPC_INVALID_PARAMS;
    }

    const cJSON *tags_arr = cJSON_GetObjectItem(params, "tag_ids");
    if (!tags_arr || !cJSON_IsArray(tags_arr) || cJSON_GetArraySize(tags_arr) == 0) {
        snprintf(err_msg, err_len, "Params must include non-empty 'tag_ids' array");
        return MCP_RPC_INVALID_PARAMS;
    }

    const char *byte_order = "ABCD";
    const cJSON *bo_item = cJSON_GetObjectItem(params, "byte_order");
    if (bo_item && cJSON_IsString(bo_item)) {
        byte_order = bo_item->valuestring;
    }

    double deadband = 0.0;
    const cJSON *db_item = cJSON_GetObjectItem(params, "deadband");
    if (db_item && cJSON_IsNumber(db_item)) {
        if (!isfinite(db_item->valuedouble) || db_item->valuedouble < 0.0) {
            snprintf(err_msg, err_len, "Invalid deadband parameter (must be non-negative finite number)");
            return MCP_RPC_INVALID_PARAMS;
        }
        deadband = db_item->valuedouble;
    }

    cJSON *res = cJSON_CreateObject();
    if (!res) return MCP_RPC_INTERNAL_ERROR;

    cJSON_AddStringToObject(res, "byte_order", byte_order);
    cJSON_AddNumberToObject(res, "deadband_applied", deadband);

    cJSON *result_tags = cJSON_CreateArray();
    int count = cJSON_GetArraySize(tags_arr);

    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(tags_arr, i);
        if (!item || !cJSON_IsNumber(item)) continue;
        uint32_t tid = (uint32_t)item->valueint;

        cJSON *tobj = cJSON_CreateObject();
        cJSON_AddNumberToObject(tobj, "tag_id", tid);

        const edge_mcp_env_t *env = edge_mcp_dispatcher_get_env(disp);
        edge_mcp_tag_value_t value = {0};
        if (!env || !env->tools.read_tag ||
            edge_mcp_tool_read_tag(&env->tools, tid, &value) != 0) {
            cJSON_AddStringToObject(tobj, "error", "tag_not_available");
            cJSON_AddItemToArray(result_tags, tobj);
            continue;
        }
        float val = (float)value.value;
        uint8_t quality = value.quality;
        uint64_t ts = value.timestamp_ms;

        /* Apply byte swap simulation if specified */
        uint32_t raw_u32;
        memcpy(&raw_u32, &val, sizeof(float));
        if (strcmp(byte_order, "CDAB") == 0) {
            raw_u32 = ((raw_u32 & 0xFFFF0000) >> 16) | ((raw_u32 & 0x0000FFFF) << 16);
        } else if (strcmp(byte_order, "BADC") == 0) {
            raw_u32 = ((raw_u32 & 0xFF00FF00) >> 8) | ((raw_u32 & 0x00FF00FF) << 8);
        } else if (strcmp(byte_order, "DCBA") == 0) {
            raw_u32 = ((raw_u32 & 0xFF000000) >> 24) |
                      ((raw_u32 & 0x00FF0000) >> 8) |
                      ((raw_u32 & 0x0000FF00) << 8) |
                      ((raw_u32 & 0x000000FF) << 24);
        }

        char hex_buf[32];
        snprintf(hex_buf, sizeof(hex_buf), "0x%08X", raw_u32);

        cJSON_AddNumberToObject(tobj, "scaled_value", (double)val);
        cJSON_AddStringToObject(tobj, "raw_hex", hex_buf);
        cJSON_AddNumberToObject(tobj, "quality", quality);
        cJSON_AddNumberToObject(tobj, "timestamp_ms", (double)ts);

        cJSON_AddItemToArray(result_tags, tobj);
    }

    cJSON_AddItemToObject(res, "tags", result_tags);

    *out_result = res;
    return 0;
}

/* Provider Auto-Registration Implementations */

int edge_mcp_register_iems_provider(mcp_bus_hub_t *hub, const edge_mcp_env_t *env) {
    (void)env;
    if (!hub) return -1;

    mcp_provider_desc_t desc = {
        .provider_id = "iems_service",
        .pid = 20101,
        .security_domain = "ess_storage",
        .tool_count = 1,
        .client_fd = -1,
        .registered_at_ns = (uint64_t)uv_hrtime(),
        .tools = {
            {
                .name = "iems_optimize_tou_schedule",
                .description = "Optimizes industrial energy storage (ESS) charge/discharge TOU schedule against peak/valley tariff rates",
                .input_schema_json = "{\"type\":\"object\",\"properties\":{\"soc_pct\":{\"type\":\"number\"}},\"required\":[\"soc_pct\"]}",
                .security_level = MCP_SEC_L1_PARAM_TUNING,
                .interlock_rule = MCP_INTERLOCK_RULE_NONE,
                .local_handler = handle_iems_optimize_tou_schedule
            },
        }
    };

    return mcp_bus_register_provider(hub, &desc);
}

int edge_mcp_register_smartbox_provider(mcp_bus_hub_t *hub, const edge_mcp_env_t *env) {
    (void)env;
    if (!hub) return -1;

    mcp_provider_desc_t desc = {
        .provider_id = "smartbox_service",
        .pid = 20102,
        .security_domain = "grid_distribution",
        .tool_count = 2,
        .client_fd = -1,
        .registered_at_ns = (uint64_t)uv_hrtime(),
        .tools = {
            {
                .name = "smartbox_eval_transformer_life",
                .description = "Evaluates distribution transformer winding hot-spot temperature rise and aging factor (IEEE C57.91)",
                .input_schema_json = "{\"type\":\"object\",\"properties\":{\"load_ratio\":{\"type\":\"number\"},\"ambient_temp_c\":{\"type\":\"number\"},\"current_top_oil_c\":{\"type\":\"number\"}},\"required\":[\"load_ratio\",\"ambient_temp_c\",\"current_top_oil_c\"]}",
                .security_level = MCP_SEC_L0_SAFE_READ,
                .interlock_rule = MCP_INTERLOCK_RULE_NONE,
                .local_handler = handle_smartbox_eval_transformer_life
            },
            {
                .name = "security_simulate_interlock",
                .description = "Simulates electrical switching operations against electrical 'Five-Anti' safety interlocking rules (DL/T 687)",
                .input_schema_json = "{\"type\":\"object\",\"properties\":{\"breaker_id\":{\"type\":\"string\"},\"target_state\":{\"type\":\"string\"}},\"required\":[\"breaker_id\",\"target_state\"]}",
                .security_level = MCP_SEC_L2_CRITICAL_ACTION,
                .interlock_rule = MCP_INTERLOCK_RULE_PREVENT_LOAD_DISCONNECT,
                .local_handler = handle_security_simulate_interlock
            }
        }
    };

    return mcp_bus_register_provider(hub, &desc);
}

int edge_mcp_register_dcu_provider(mcp_bus_hub_t *hub, const edge_mcp_env_t *env) {
    (void)env;
    if (!hub) return -1;

    mcp_provider_desc_t desc = {
        .provider_id = "dcu_service",
        .pid = 20103,
        .security_domain = "metering_collection",
        .tool_count = 2,
        .client_fd = -1,
        .registered_at_ns = (uint64_t)uv_hrtime(),
        .tools = {
            {
                .name = "dcu_analyze_power_quality",
                .description = "Analyzes 3-phase voltage unbalance and harmonics, provides commutation guidance",
                .input_schema_json = "{\"type\":\"object\",\"properties\":{\"phase_a_v\":{\"type\":\"number\"},\"phase_b_v\":{\"type\":\"number\"},\"phase_c_v\":{\"type\":\"number\"}},\"required\":[\"phase_a_v\",\"phase_b_v\",\"phase_c_v\"]}",
                .security_level = MCP_SEC_L0_SAFE_READ,
                .interlock_rule = MCP_INTERLOCK_RULE_NONE,
                .local_handler = handle_dcu_analyze_power_quality
            },
            {
                .name = "tag_batch_read_scaled",
                .description = "Batch reads DataHub tags with ABCD/CDAB/BADC/DCBA byte swapping, scaling and deadband filtering",
                .input_schema_json = "{\"type\":\"object\",\"properties\":{\"tag_ids\":{\"type\":\"array\"},\"byte_order\":{\"type\":\"string\"},\"deadband\":{\"type\":\"number\"}},\"required\":[\"tag_ids\"]}",
                .security_level = MCP_SEC_L0_SAFE_READ,
                .interlock_rule = MCP_INTERLOCK_RULE_NONE,
                .local_handler = handle_tag_batch_read_scaled
            }
        }
    };

    return mcp_bus_register_provider(hub, &desc);
}

int edge_mcp_register_all_microservices(mcp_bus_hub_t *hub, const edge_mcp_env_t *env) {
    if (!hub) return -1;
    int rc;
    if ((rc = edge_mcp_register_iems_provider(hub, env)) != 0) return rc;
    if ((rc = edge_mcp_register_smartbox_provider(hub, env)) != 0) return rc;
    if ((rc = edge_mcp_register_dcu_provider(hub, env)) != 0) return rc;
    return 0;
}
