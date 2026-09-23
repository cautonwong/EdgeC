/**
 * @file test_mcp_dispatcher.c
 * @brief Unit tests for Industrial MCP Dispatcher (AEP-506)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <assert.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <uv.h>

#include "datahub.h"
#include "edgeos/edge_mcp_dispatcher.h"

static datahub_t *create_test_hub(uv_loop_t *loop, const char *name) {
    datahub_t *hub = (datahub_t *)calloc(1, sizeof(datahub_t));
    if (!hub) return NULL;
    shm_unlink(name);
    if (datahub_init(hub, loop, name, 128, 65536, NULL, 0, NULL, NULL, NULL) != 0) {
        free(hub);
        return NULL;
    }
    return hub;
}

static void destroy_test_hub(datahub_t *hub) {
    if (!hub) return;
    datahub_destroy(hub);
    free(hub);
}

static void test_dispatcher_create_destroy(void) {
    printf("[TEST] Testing MCP Dispatcher lifecycle & default tools registration...\n");
    uv_loop_t *loop = uv_default_loop();
    datahub_t *hub = create_test_hub(loop, "/test_mcp_disp_1");
    assert(hub != NULL);

    edge_mcp_env_t env = {
        .hub = hub, .drv_mgr = NULL, .stream_eng = NULL, .storage_drv = NULL, .loop = loop};

    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(&env);
    assert(disp != NULL);

    cJSON *tools = edge_mcp_list_tools_json(disp);
    assert(tools != NULL);
    assert(cJSON_IsArray(tools));
    int tool_count = cJSON_GetArraySize(tools);
    assert(tool_count == 17);
    printf("  --> Verified %d default industrial MCP tools registered.\n", tool_count);
    cJSON_Delete(tools);

    edge_mcp_dispatcher_destroy(disp);
    destroy_test_hub(hub);
    printf("  --> Dispatcher create/destroy verified successfully!\n");
}

static void test_jsonrpc_initialize_and_ping(void) {
    printf("[TEST] Testing MCP JSON-RPC 2.0 initialize and ping methods...\n");
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert(disp != NULL);

    /* 1. initialize handshake */
    const char *init_req = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{}}";
    char *resp = edge_mcp_dispatch_json(disp, init_req);
    assert(resp != NULL);

    cJSON *resp_json = cJSON_Parse(resp);
    assert(resp_json != NULL);
    cJSON *id_item = cJSON_GetObjectItem(resp_json, "id");
    assert(id_item && id_item->valueint == 1);
    cJSON *result = cJSON_GetObjectItem(resp_json, "result");
    assert(result != NULL);
    cJSON *server_info = cJSON_GetObjectItem(result, "serverInfo");
    assert(server_info != NULL);
    cJSON *server_name = cJSON_GetObjectItem(server_info, "name");
    assert(server_name && strcmp(server_name->valuestring, "edge-mcp-server") == 0);
    cJSON_Delete(resp_json);
    free(resp);

    /* 2. ping */
    const char *ping_req = "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"ping\"}";
    resp = edge_mcp_dispatch_json(disp, ping_req);
    assert(resp != NULL);
    resp_json = cJSON_Parse(resp);
    assert(resp_json != NULL);
    assert(cJSON_GetObjectItem(resp_json, "result") != NULL);
    cJSON_Delete(resp_json);
    free(resp);

    edge_mcp_dispatcher_destroy(disp);
    printf("  --> JSON-RPC initialize & ping verified successfully!\n");
}

static void test_jsonrpc_tools_list_and_call(void) {
    printf("[TEST] Testing MCP tools/list and tools/call on physical DataHub...\n");
    uv_loop_t *loop = uv_default_loop();
    datahub_t *hub = create_test_hub(loop, "/test_mcp_disp_2");
    assert(hub != NULL);

    /* Seed DataHub with physical measurements */
    pipe_msg_t msg1 = {.tag_id = 0x1001,
                       .payload = {.val = {.f = 220.5f}},
                       .quality = 0,
                       .type = PIPEMSG_TYPE_SCALAR,
                       .ts = 1000000ULL};
    assert(datahub_write_shadow(hub, &msg1) == 0);

    edge_mcp_env_t env = {.hub = hub, .loop = loop};
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(&env);
    assert(disp != NULL);

    /* 1. tools/list */
    const char *list_req = "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tools/list\"}";
    char *resp = edge_mcp_dispatch_json(disp, list_req);
    assert(resp != NULL);
    cJSON *resp_json = cJSON_Parse(resp);
    assert(resp_json != NULL);
    cJSON *res_obj = cJSON_GetObjectItem(resp_json, "result");
    assert(res_obj != NULL);
    cJSON *tools_arr = cJSON_GetObjectItem(res_obj, "tools");
    assert(tools_arr && cJSON_GetArraySize(tools_arr) == 17);
    cJSON_Delete(resp_json);
    free(resp);

    /* 2. tools/call datahub_read_tags */
    const char *call_req =
        "{\"jsonrpc\":\"2.0\",\"id\":11,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"datahub_read_tags\",\"arguments\":{\"tag_ids\":[4097]}}}";
    resp = edge_mcp_dispatch_json(disp, call_req);
    assert(resp != NULL);
    resp_json = cJSON_Parse(resp);
    assert(resp_json != NULL);
    res_obj = cJSON_GetObjectItem(resp_json, "result");
    assert(res_obj != NULL);
    cJSON *content_arr = cJSON_GetObjectItem(res_obj, "content");
    assert(content_arr && cJSON_GetArraySize(content_arr) > 0);
    cJSON *item0 = cJSON_GetArrayItem(content_arr, 0);
    cJSON *text_item = cJSON_GetObjectItem(item0, "text");
    assert(text_item && strstr(text_item->valuestring, "220.5"));
    cJSON_Delete(resp_json);
    free(resp);

    /* 3. tools/call datahub_write_tag */
    const char *write_req =
        "{\"jsonrpc\":\"2.0\",\"id\":12,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"datahub_write_tag\",\"arguments\":{\"tag_id\":4098,"
        "\"value\":49.98,\"quality\":0}}}";
    resp = edge_mcp_dispatch_json(disp, write_req);
    assert(resp != NULL);
    resp_json = cJSON_Parse(resp);
    assert(resp_json != NULL);
    res_obj = cJSON_GetObjectItem(resp_json, "result");
    assert(res_obj != NULL);
    content_arr = cJSON_GetObjectItem(res_obj, "content");
    item0 = cJSON_GetArrayItem(content_arr, 0);
    text_item = cJSON_GetObjectItem(item0, "text");
    assert(text_item && strstr(text_item->valuestring, "\"success\":true"));
    cJSON_Delete(resp_json);
    free(resp);

    /* Verify DataHub value was actually updated */
    pipe_msg_t read_back = {0};
    assert(datahub_read_shadow(hub, 4098, &read_back) == 0);
    assert(read_back.payload.val.f > 49.97f && read_back.payload.val.f < 49.99f);

    /* 4. tools/call other domain tools (io, pipeline, dds, storage) */
    const char *io_req =
        "{\"jsonrpc\":\"2.0\",\"id\":13,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"io_list_channels\",\"arguments\":{}}}";
    resp = edge_mcp_dispatch_json(disp, io_req);
    assert(resp != NULL && strstr(resp, "RS485-1"));
    free(resp);

    const char *pipe_req =
        "{\"jsonrpc\":\"2.0\",\"id\":14,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"pipeline_query_metrics\",\"arguments\":{}}}";
    resp = edge_mcp_dispatch_json(disp, pipe_req);
    assert(resp != NULL && strstr(resp, "avg_latency_us"));
    free(resp);

    const char *dds_req =
        "{\"jsonrpc\":\"2.0\",\"id\":15,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"dds_inspect_topics\",\"arguments\":{}}}";
    resp = edge_mcp_dispatch_json(disp, dds_req);
    assert(resp != NULL && strstr(resp, "EdgeOS/DCU/Telemetry"));
    free(resp);

    const char *soe_req =
        "{\"jsonrpc\":\"2.0\",\"id\":16,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"storage_query_soe_events\",\"arguments\":{\"limit\":5}}}";
    resp = edge_mcp_dispatch_json(disp, soe_req);
    assert(resp != NULL && strstr(resp, "GRID_POWER_OUTAGE"));
    free(resp);

    edge_mcp_dispatcher_destroy(disp);
    destroy_test_hub(hub);
    printf("  --> Tools list and call verified successfully!\n");
}

static void test_jsonrpc_error_handling(void) {
    printf("[TEST] Testing JSON-RPC 2.0 error handling and edge cases...\n");
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert(disp != NULL);

    /* 1. Malformed JSON */
    char *resp = edge_mcp_dispatch_json(disp, "invalid json {");
    assert(resp != NULL);
    assert(strstr(resp, "\"-32700\"") || strstr(resp, "-32700"));
    free(resp);

    /* 2. Missing method */
    resp = edge_mcp_dispatch_json(disp, "{\"jsonrpc\":\"2.0\",\"id\":99}");
    assert(resp != NULL);
    assert(strstr(resp, "-32600"));
    free(resp);

    /* 3. Unknown method */
    resp = edge_mcp_dispatch_json(disp,
                                  "{\"jsonrpc\":\"2.0\",\"id\":99,\"method\":\"unknown_method\"}");
    assert(resp != NULL);
    assert(strstr(resp, "-32601"));
    free(resp);

    /* 4. Unknown tool in tools/call */
    resp = edge_mcp_dispatch_json(disp,
                                  "{\"jsonrpc\":\"2.0\",\"id\":100,\"method\":\"tools/"
                                  "call\",\"params\":{\"name\":\"non_existent_tool\"}}");
    assert(resp != NULL);
    assert(strstr(resp, "-32601"));
    free(resp);

    /* 5. Missing params in tools/call */
    resp =
        edge_mcp_dispatch_json(disp, "{\"jsonrpc\":\"2.0\",\"id\":101,\"method\":\"tools/call\"}");
    assert(resp != NULL);
    assert(strstr(resp, "-32602"));
    free(resp);

    edge_mcp_dispatcher_destroy(disp);
    printf("  --> Error handling verified successfully!\n");
}

static void test_direct_c_api_and_benchmark(void) {
    printf("[TEST] Testing direct C API tool invocation & latency benchmark...\n");
    uv_loop_t *loop = uv_default_loop();
    datahub_t *hub = create_test_hub(loop, "/test_mcp_disp_3");
    edge_mcp_env_t env = {.hub = hub, .loop = loop};
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(&env);

    /* Direct C call */
    char *res_str = NULL;
    int rc = edge_mcp_call_tool(disp, "datahub_get_tag_meta", "{\"tag_id\":4097}", &res_str);
    assert(rc == 0);
    assert(res_str != NULL);
    assert(strstr(res_str, "TAG_1001"));
    free(res_str);

    /* High frequency dispatch benchmark: 1000 calls */
    uint64_t start = uv_hrtime();
    int iterations = 1000;
    for (int i = 0; i < iterations; i++) {
        char req[128];
        snprintf(req, sizeof(req), "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"ping\"}", i);
        char *r = edge_mcp_dispatch_json(disp, req);
        free(r);
    }
    uint64_t elapsed_ns = uv_hrtime() - start;
    double avg_us = (double)elapsed_ns / (double)iterations / 1000.0;
    printf("  --> 1000 MCP JSON-RPC dispatches completed in %.2f ms (avg %.2f µs/op)\n",
           (double)elapsed_ns / 1000000.0, avg_us);
    assert(avg_us < 100.0); /* Must be sub-100µs industrial latency */

    edge_mcp_dispatcher_destroy(disp);
    destroy_test_hub(hub);
    printf("  --> Direct C API & benchmark verified successfully!\n");
}

int main(void) {
    printf("=== EdgeOS Industrial MCP Dispatcher Unit Tests (AEP-506) ===\n");
    test_dispatcher_create_destroy();
    test_jsonrpc_initialize_and_ping();
    test_jsonrpc_tools_list_and_call();
    test_jsonrpc_error_handling();
    test_direct_c_api_and_benchmark();
    printf("=== All MCP Dispatcher Tests PASSED ===\n");
    return 0;
}
