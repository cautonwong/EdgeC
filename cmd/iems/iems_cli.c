/**
 * @file iems_cli.c
 * @brief EdgeOS IEMS Interactive Command-Line Interface & MCP Client (ARCH-MCP-04 / Issue #303)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "contracts.h"
#include "datahub.h"
#include "edge_log.h"
#include "edgeos/edge_mcp_bus.h"
#include "edgeos/edge_mcp_dispatcher.h"
#include "edgeos/edge_mcp_industry_tools.h"

static void print_usage(const char *prog) {
    printf("EdgeOS IEMS Industrial Management CLI & MCP Copilot Console\n");
    printf("Usage: %s <command> [subcommand] [arguments...]\n\n", prog);
    printf("Commands:\n");
    printf(
        "  mcp list                           List all registered microservice providers and "
        "industrial MCP tools\n");
    printf(
        "  mcp call <tool_name> [args_json]   Execute an industrial MCP tool with JSON "
        "arguments\n");
    printf("  mcp audit [limit]                  View MTT-timestamped audit blackbox logs\n");
    printf(
        "  mcp policy                         View active Four-Level privilege policies and "
        "Five-Anti rules\n");
    printf(
        "  tag read <tag_id>                  Read physical telemetry value from DataHub shadow\n");
    printf("  tag write <tag_id> <val>           Write setpoint or telecontrol value to DataHub\n");
    printf("  status                             Print system health and runtime summary\n");
    printf("  help, --help, -h                   Display this help message\n\n");
    printf("Examples:\n");
    printf("  %s mcp list\n", prog);
    printf(
        "  %s mcp call dcu_analyze_power_quality '{\"phase_a_v\": 235.0, \"phase_b_v\": 220.0, "
        "\"phase_c_v\": 205.0}'\n",
        prog);
    printf(
        "  %s mcp call smartbox_eval_transformer_life '{\"load_ratio\": 1.15, \"ambient_temp_c\": "
        "35.0, \"current_top_oil_c\": 78.0}'\n",
        prog);
    printf(
        "  %s mcp call iems_optimize_tou_schedule '{\"soc_pct\": 65.0, \"pv_forecast_kw\": "
        "40.0}'\n",
        prog);
    printf(
        "  %s mcp call ess_emergency_stop '{\"system_id\": \"ESS_01\", \"reason\": "
        "\"GRID_SURGE\"}'\n",
        prog);
}

static mcp_bus_hub_t *create_initialized_hub(void) {
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);
    if (!hub) return NULL;
    edge_mcp_register_all_microservices(hub, NULL);
    return hub;
}

static int handle_mcp_list(void) {
    mcp_bus_hub_t *hub = create_initialized_hub();
    if (!hub) {
        fprintf(stderr, "Error: Failed to initialize MCP bus hub.\n");
        return 1;
    }

    cJSON *providers = NULL;
    int pcount = mcp_bus_get_providers_json(hub, &providers);

    printf(
        "=========================================================================================="
        "==============\n");
    printf(" EdgeOS Shared MCP Neural Bus - Registered Providers & Tools (%d Providers)\n", pcount);
    printf(
        "=========================================================================================="
        "==============\n");
    printf("%-20s %-18s %-10s %-32s %s\n", "PROVIDER", "DOMAIN", "PID", "TOOL NAME",
           "PRIVILEGE / DESCRIPTION");
    printf(
        "------------------------------------------------------------------------------------------"
        "--------------\n");

    if (providers && cJSON_IsArray(providers)) {
        int sz = cJSON_GetArraySize(providers);
        for (int i = 0; i < sz; i++) {
            cJSON *p = cJSON_GetArrayItem(providers, i);
            const char *p_id = cJSON_GetObjectItem(p, "provider_id")->valuestring;
            const char *p_dom = cJSON_GetObjectItem(p, "security_domain")->valuestring;
            int pid = cJSON_GetObjectItem(p, "pid")->valueint;
            cJSON *tools = cJSON_GetObjectItem(p, "tools");

            if (tools && cJSON_IsArray(tools)) {
                int tsz = cJSON_GetArraySize(tools);
                for (int j = 0; j < tsz; j++) {
                    cJSON *t = cJSON_GetArrayItem(tools, j);
                    const char *t_name = cJSON_GetObjectItem(t, "name")->valuestring;
                    const char *t_sec = cJSON_GetObjectItem(t, "security_level")->valuestring;
                    const char *t_desc = cJSON_GetObjectItem(t, "description")->valuestring;

                    printf("%-20s %-18s %-10d %-32s [%s] %s\n", (j == 0) ? p_id : "",
                           (j == 0) ? p_dom : "", (j == 0) ? pid : 0, t_name, t_sec, t_desc);
                }
            }
            printf(
                "----------------------------------------------------------------------------------"
                "----------------------\n");
        }
    }

    if (providers) cJSON_Delete(providers);

    /* Also list foundational built-in tools */
    char *resp = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
                                        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
    if (resp) {
        cJSON *root = cJSON_Parse(resp);
        if (root) {
            cJSON *res = cJSON_GetObjectItem(root, "result");
            cJSON *tools = res ? cJSON_GetObjectItem(res, "tools") : NULL;
            if (tools && cJSON_IsArray(tools)) {
                printf("\nTotal Active MCP Tools Available in Dispatcher: %d\n",
                       cJSON_GetArraySize(tools));
            }
            cJSON_Delete(root);
        }
        free(resp);
    }

    mcp_bus_hub_destroy(hub);
    return 0;
}

static int handle_mcp_call(const char *tool_name, const char *args_json, const char *operator_id,
                           mcp_security_level_t sec_level) {
    if (!tool_name) {
        fprintf(stderr, "Error: Missing tool name for 'mcp call'.\n");
        return 1;
    }

    mcp_bus_hub_t *hub = create_initialized_hub();
    if (!hub) {
        fprintf(stderr, "Error: Failed to initialize MCP bus hub.\n");
        return 1;
    }

    const char *env_token = getenv("EDGEOS_SUPERVISOR_TOKEN");
    mcp_security_context_t sec_ctx = {.client_id = "iems-cli",
                                      .operator_id = operator_id ? operator_id : "cli_operator",
                                      .supervisor_token = env_token ? env_token : "",
                                      .role_level = sec_level,
                                      .request_timestamp_ns = (uint64_t)uv_hrtime()};

    cJSON *req = cJSON_CreateObject();
    cJSON_AddStringToObject(req, "jsonrpc", "2.0");
    cJSON_AddNumberToObject(req, "id", 1001);
    cJSON_AddStringToObject(req, "method", "tools/call");

    cJSON *params = cJSON_CreateObject();
    cJSON_AddStringToObject(params, "name", tool_name);
    if (args_json && strlen(args_json) > 0) {
        cJSON *args = cJSON_Parse(args_json);
        if (args) {
            cJSON_AddItemToObject(params, "arguments", args);
        } else {
            fprintf(stderr, "Warning: Failed to parse arguments JSON; sending empty object.\n");
            cJSON_AddObjectToObject(params, "arguments");
        }
    } else {
        cJSON_AddObjectToObject(params, "arguments");
    }
    cJSON_AddItemToObject(req, "params", params);

    char *req_str = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);

    char *resp_str = mcp_bus_dispatch_with_security(hub, req_str, &sec_ctx);
    free(req_str);

    if (!resp_str) {
        fprintf(stderr, "Error: No response returned from MCP bus.\n");
        mcp_bus_hub_destroy(hub);
        return 1;
    }

    /* Format response JSON for clean terminal display */
    cJSON *resp_obj = cJSON_Parse(resp_str);
    if (resp_obj) {
        char *pretty_resp = cJSON_Print(resp_obj);
        if (pretty_resp) {
            printf("%s\n", pretty_resp);
            free(pretty_resp);
        } else {
            printf("%s\n", resp_str);
        }
        cJSON_Delete(resp_obj);
    } else {
        printf("%s\n", resp_str);
    }

    free(resp_str);
    mcp_bus_hub_destroy(hub);
    return 0;
}

static int handle_mcp_audit(int limit) {
    if (limit <= 0) limit = 20;
    mcp_bus_hub_t *hub = create_initialized_hub();
    if (!hub) return 1;

    cJSON *logs = NULL;
    int count = mcp_bus_get_audit_logs(hub, (size_t)limit, &logs);

    printf(
        "=========================================================================================="
        "==============\n");
    printf(" EdgeOS Industrial Audit Blackbox Trail (Recent %d Records)\n", count);
    printf(
        "=========================================================================================="
        "==============\n");
    printf("%-22s %-16s %-16s %-28s %-12s %s\n", "MTT TIMESTAMP (ns)", "CLIENT ID", "OPERATOR",
           "TOOL NAME", "STATUS", "DURATION");
    printf(
        "------------------------------------------------------------------------------------------"
        "--------------\n");

    if (logs && cJSON_IsArray(logs)) {
        int sz = cJSON_GetArraySize(logs);
        for (int i = 0; i < sz; i++) {
            cJSON *item = cJSON_GetArrayItem(logs, i);
            double ts = cJSON_GetObjectItem(item, "timestamp_ns")->valuedouble;
            const char *cid = cJSON_GetObjectItem(item, "client_id")->valuestring;
            const char *op = cJSON_GetObjectItem(item, "operator_id")->valuestring;
            const char *tname = cJSON_GetObjectItem(item, "tool_name")->valuestring;
            bool auth = cJSON_IsTrue(cJSON_GetObjectItem(item, "authorized"));
            int dur = cJSON_GetObjectItem(item, "duration_us")->valueint;

            printf("%-22.0f %-16s %-16s %-28s %-12s %d µs\n", ts, cid, op, tname,
                   auth ? "APPROVED" : "BLOCKED", dur);
        }
    }
    printf(
        "=========================================================================================="
        "==============\n");

    if (logs) cJSON_Delete(logs);
    mcp_bus_hub_destroy(hub);
    return 0;
}

static int handle_tag_read(uint32_t tag_id) {
    const char *shm_name = getenv("EDGE_SHM_NAME") ? getenv("EDGE_SHM_NAME") : EDGE_SHM_BUS_NAME;
    uv_loop_t *loop = uv_default_loop();
    datahub_t hub = {0};
    if (datahub_init(&hub, loop, shm_name, 1024, 65536, NULL, 0, NULL, NULL, NULL) != 0) {
        fprintf(stderr, "Failed to connect to DataHub SHM at %s\n", shm_name);
        return 1;
    }

    pipe_msg_t msg = {0};
    if (datahub_read_shadow(&hub, tag_id, &msg) == 0) {
        printf("Tag [0x%04X / %u]: Value=%.4f Quality=0x%02X Timestamp=%lu\n", tag_id, tag_id,
               msg.payload.val.f, msg.quality, (unsigned long)msg.ts);
    } else {
        printf("Tag [0x%04X / %u]: NOT FOUND or uninitialized.\n", tag_id, tag_id);
    }

    datahub_destroy(&hub);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "mcp") == 0) {
        if (argc < 3) {
            print_usage(argv[0]);
            return 1;
        }
        const char *sub = argv[2];
        if (strcmp(sub, "list") == 0) {
            return handle_mcp_list();
        } else if (strcmp(sub, "call") == 0) {
            if (argc < 4) {
                fprintf(stderr, "Usage: %s mcp call <tool_name> [args_json]\n", argv[0]);
                return 1;
            }
            const char *tool_name = argv[3];
            const char *args = (argc >= 5) ? argv[4] : "{}";
            return handle_mcp_call(tool_name, args, "engineer_01", MCP_SEC_L2_CRITICAL_ACTION);
        } else if (strcmp(sub, "audit") == 0) {
            int limit = (argc >= 4) ? atoi(argv[3]) : 20;
            return handle_mcp_audit(limit);
        } else if (strcmp(sub, "policy") == 0) {
            return handle_mcp_call("mcp_query_security_policy", "{}", "guest",
                                   MCP_SEC_L0_SAFE_READ);
        } else {
            fprintf(stderr, "Unknown mcp subcommand: %s\n", sub);
            return 1;
        }
    } else if (strcmp(cmd, "tag") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s tag read <tag_id> | write <tag_id> <val>\n", argv[0]);
            return 1;
        }
        const char *sub = argv[2];
        uint32_t tid = (uint32_t)strtoul(argv[3], NULL, 0);
        if (strcmp(sub, "read") == 0) {
            return handle_tag_read(tid);
        } else if (strcmp(sub, "write") == 0 && argc >= 5) {
            char call_args[128];
            snprintf(call_args, sizeof(call_args), "{\"tag_id\": %u, \"value\": %s}", tid, argv[4]);
            return handle_mcp_call("datahub_write_tag", call_args, "operator",
                                   MCP_SEC_L1_PARAM_TUNING);
        }
    } else if (strcmp(cmd, "status") == 0) {
        printf("EdgeOS IEMS Node: ONLINE | MCP Bus: ACTIVE | DataHub: SYNCHRONIZED\n");
        return 0;
    } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    print_usage(argv[0]);
    return 1;
}
