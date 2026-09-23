/**
 * @file test_mcp_bus_security.c
 * @brief 共享 MCP 工业神经总线与五防安全仲裁门禁单元测试 (ARCH-MCP-02 / Issue #297)
 */

#include <cjson/cJSON.h>
#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "edgeos/edge_mcp_bus.h"
#include "edgeos/edge_mcp_dispatcher.h"

static int mock_transformer_tool_handler(edge_mcp_dispatcher_t *disp, const cJSON *params,
                                         cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp;
    (void)err_msg;
    (void)err_len;
    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "hotspot_temp_c", 88.5);
    cJSON_AddNumberToObject(res, "aging_factor_faa", 1.05);
    cJSON_AddNumberToObject(res, "remaining_life_years", 28.4);
    if (params) {
        cJSON_AddStringToObject(res, "echo_params", "OK");
    }
    *out_result = res;
    return 0;
}

static int mock_breaker_action_handler(edge_mcp_dispatcher_t *disp, const cJSON *params,
                                       cJSON **out_result, char *err_msg, size_t err_len) {
    (void)disp;
    (void)err_msg;
    (void)err_len;
    (void)params;
    cJSON *res = cJSON_CreateObject();
    cJSON_AddStringToObject(res, "action_status", "EXECUTED");
    cJSON_AddStringToObject(res, "breaker_state", "OPEN");
    *out_result = res;
    return 0;
}

static void test_mcp_bus_create_and_destroy(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);
    assert_non_null(hub);
    assert_non_null(mcp_bus_get_dispatcher(hub));

    /* 验证核心管理方法已就绪 */
    char *resp = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
                                        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
    assert_non_null(resp);
    assert_non_null(strstr(resp, "mcp_register_provider"));
    assert_non_null(strstr(resp, "mcp_unregister_provider"));
    assert_non_null(strstr(resp, "mcp_get_audit_logs"));
    assert_non_null(strstr(resp, "mcp_query_security_policy"));
    free(resp);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_bus_dynamic_provider_registration(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);
    assert_non_null(hub);

    mcp_provider_desc_t provider = {
        .provider_id = "smartbox_service",
        .pid = 12345,
        .security_domain = "grid_distribution",
        .tool_count = 2,
        .client_fd = 10,
        .tools = {{.name = "smartbox_eval_transformer_life",
                   .description = "Evaluates transformer thermal life",
                   .input_schema_json = "{\"type\":\"object\"}",
                   .security_level = MCP_SEC_L0_SAFE_READ,
                   .interlock_rule = MCP_INTERLOCK_RULE_NONE,
                   .local_handler = mock_transformer_tool_handler},
                  {.name = "smartbox_trip_breaker",
                   .description = "Trip 10kV breaker",
                   .input_schema_json = "{\"type\":\"object\"}",
                   .security_level = MCP_SEC_L2_CRITICAL_ACTION,
                   .interlock_rule = MCP_INTERLOCK_RULE_PREVENT_LOAD_DISCONNECT,
                   .local_handler = mock_breaker_action_handler}}};

    /* 注册 Provider */
    int ret = mcp_bus_register_provider(hub, &provider);
    assert_int_equal(ret, 0);

    /* 验证 Provider 列表 */
    cJSON *providers_json = NULL;
    int p_count = mcp_bus_get_providers_json(hub, &providers_json);
    assert_int_equal(p_count, 1);
    assert_non_null(providers_json);
    char *p_str = cJSON_PrintUnformatted(providers_json);
    assert_non_null(strstr(p_str, "smartbox_service"));
    assert_non_null(strstr(p_str, "smartbox_eval_transformer_life"));
    free(p_str);
    cJSON_Delete(providers_json);

    /* 验证动态调用工具 */
    mcp_security_context_t sec_ctx = {
        .client_id = "test_agent", .operator_id = "op_01", .role_level = MCP_SEC_L0_SAFE_READ};

    char *call_resp = mcp_bus_dispatch_with_security(
        hub,
        "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"smartbox_eval_transformer_life\",\"arguments\":{}}}",
        &sec_ctx);
    assert_non_null(call_resp);
    assert_non_null(strstr(call_resp, "hotspot_temp_c"));
    free(call_resp);

    /* 注销 Provider */
    ret = mcp_bus_unregister_provider(hub, "smartbox_service");
    assert_int_equal(ret, 0);

    p_count = mcp_bus_get_providers_json(hub, &providers_json);
    assert_int_equal(p_count, 0);
    cJSON_Delete(providers_json);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_bus_l0_safe_read_passthrough(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);

    mcp_security_context_t sec_ctx = {
        .client_id = "guest_copilot", .operator_id = "guest", .role_level = MCP_SEC_L0_SAFE_READ};

    /* L0 只读工具 (datahub_read_tags) 直接放行 */
    char *resp = mcp_bus_dispatch_with_security(hub,
                                                "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/"
                                                "call\",\"params\":{\"name\":\"datahub_read_tags\","
                                                "\"arguments\":{\"tag_ids\":[1001, 1002]}}}",
                                                &sec_ctx);
    assert_non_null(resp);
    assert_null(strstr(resp, "error"));
    assert_non_null(strstr(resp, "tags"));
    free(resp);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_bus_l1_param_tuning_boundary_checks(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);

    mcp_security_context_t low_sec = {
        .client_id = "unauth_client", .role_level = MCP_SEC_L0_SAFE_READ /* 权限不足 (L0 < L1) */
    };

    /* 权限不足时拦截 */
    char *resp = mcp_bus_dispatch_with_security(hub,
                                                "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/"
                                                "call\",\"params\":{\"name\":\"datahub_write_tag\","
                                                "\"arguments\":{\"tag_id\":1001,\"value\":220.0}}}",
                                                &low_sec);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "-32002"));
    assert_non_null(strstr(resp, "Privilege Insufficient"));
    free(resp);

    /* 权限足够时 (L1) 放行 */
    mcp_security_context_t ok_sec = {.client_id = "auth_tuning_agent",
                                     .role_level = MCP_SEC_L1_PARAM_TUNING};
    resp = mcp_bus_dispatch_with_security(hub,
                                          "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/"
                                          "call\",\"params\":{\"name\":\"datahub_write_tag\","
                                          "\"arguments\":{\"tag_id\":1001,\"value\":220.0}}}",
                                          &ok_sec);
    assert_non_null(resp);
    assert_null(strstr(resp, "Privilege Insufficient"));
    assert_non_null(strstr(resp, "written_value"));
    free(resp);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_bus_l2_five_anti_interlock_enforcement(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);

    mcp_provider_desc_t provider = {
        .provider_id = "grid_breaker_driver",
        .tool_count = 3,
        .tools = {{.name = "breaker_operate_disconnector",
                   .description = "Open/close knife disconnector",
                   .security_level = MCP_SEC_L2_CRITICAL_ACTION,
                   .interlock_rule = MCP_INTERLOCK_RULE_PREVENT_LOAD_DISCONNECT,
                   .local_handler = mock_breaker_action_handler},
                  {.name = "breaker_operate_earth_switch",
                   .description = "Close grounding earth switch",
                   .security_level = MCP_SEC_L2_CRITICAL_ACTION,
                   .interlock_rule = MCP_INTERLOCK_RULE_PREVENT_LIVE_EARTHING,
                   .local_handler = mock_breaker_action_handler},
                  {.name = "breaker_close_main_breaker",
                   .description = "Close main 10kV circuit breaker",
                   .security_level = MCP_SEC_L2_CRITICAL_ACTION,
                   .interlock_rule = MCP_INTERLOCK_RULE_PREVENT_EARTHED_CLOSING,
                   .local_handler = mock_breaker_action_handler}}};
    mcp_bus_register_provider(hub, &provider);

    mcp_security_context_t sec_ctx = {.client_id = "scada_control_agent",
                                      .operator_id = "engineer_wkt",
                                      .role_level = MCP_SEC_L2_CRITICAL_ACTION};

    /* 测试 1: 五防规则 1 - 带负荷分刀闸 (I = 15.0A > 0.5A) -> 必须 100% 阻断 */
    char *resp = mcp_bus_dispatch_with_security(
        hub,
        "{\"jsonrpc\":\"2.0\",\"id\":100,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"breaker_operate_disconnector\",\"arguments\":{\"load_"
        "current_a\":15.0,\"target_state\":\"OPEN\"}}}",
        &sec_ctx);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "-32001"));
    assert_non_null(strstr(resp, "Five-Anti Violation [Rule 1]"));
    free(resp);

    /* 负荷电流归零 (I = 0.0A) -> 五防放行 */
    resp = mcp_bus_dispatch_with_security(
        hub,
        "{\"jsonrpc\":\"2.0\",\"id\":101,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"breaker_operate_disconnector\",\"arguments\":{\"load_"
        "current_a\":0.0,\"target_state\":\"OPEN\"}}}",
        &sec_ctx);
    assert_non_null(resp);
    assert_null(strstr(resp, "Five-Anti Violation"));
    assert_non_null(strstr(resp, "EXECUTED"));
    free(resp);

    /* 测试 2: 五防规则 2 - 带电合接地刀闸 (U = 220.0V > 10V) -> 必须阻断 */
    resp = mcp_bus_dispatch_with_security(hub,
                                          "{\"jsonrpc\":\"2.0\",\"id\":102,\"method\":\"tools/"
                                          "call\",\"params\":{\"name\":\"breaker_operate_earth_"
                                          "switch\",\"arguments\":{\"bus_voltage_v\":220.0}}}",
                                          &sec_ctx);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "-32001"));
    assert_non_null(strstr(resp, "Five-Anti Violation [Rule 2]"));
    free(resp);

    /* 测试 3: 五防规则 3 - 带接地刀闸合断路器 -> 必须阻断 */
    resp = mcp_bus_dispatch_with_security(hub,
                                          "{\"jsonrpc\":\"2.0\",\"id\":103,\"method\":\"tools/"
                                          "call\",\"params\":{\"name\":\"breaker_close_main_"
                                          "breaker\",\"arguments\":{\"is_earthed\":true}}}",
                                          &sec_ctx);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "-32001"));
    assert_non_null(strstr(resp, "Five-Anti Violation [Rule 3]"));
    free(resp);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_bus_l3_dual_authorization_requirement(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);

    mcp_provider_desc_t provider = {
        .provider_id = "firmware_service",
        .tool_count = 1,
        .tools = {{.name = "system_firmware_upgrade",
                   .description = "Perform dual-bank DFU firmware upgrade",
                   .security_level = MCP_SEC_L3_SYSTEM_CRITICAL,
                   .interlock_rule = MCP_INTERLOCK_RULE_NONE,
                   .local_handler = mock_breaker_action_handler}}};
    mcp_bus_register_provider(hub, &provider);

    /* 场景 1: 无监护人 Supervisor Token -> 阻断 */
    mcp_security_context_t sec_no_token = {.client_id = "admin_client",
                                           .operator_id = "admin",
                                           .role_level = MCP_SEC_L3_SYSTEM_CRITICAL,
                                           .supervisor_token = NULL};
    char *resp =
        mcp_bus_dispatch_with_security(hub,
                                       "{\"jsonrpc\":\"2.0\",\"id\":200,\"method\":\"tools/"
                                       "call\",\"params\":{\"name\":\"system_firmware_upgrade\","
                                       "\"arguments\":{\"firmware_url\":\"https://ota/fw.bin\"}}}",
                                       &sec_no_token);
    assert_non_null(resp);
    assert_non_null(strstr(resp, "-32002"));
    assert_non_null(strstr(resp, "Dual-Authorization Required"));
    free(resp);

    /* 场景 2: 携带有效 Supervisor Token -> 放行 */
    mcp_security_context_t sec_with_token = {.client_id = "admin_client",
                                             .operator_id = "admin",
                                             .role_level = MCP_SEC_L3_SYSTEM_CRITICAL,
                                             .supervisor_token = "SUPERVISOR_KEY_TOKEN_SECRET_999"};
    resp =
        mcp_bus_dispatch_with_security(hub,
                                       "{\"jsonrpc\":\"2.0\",\"id\":201,\"method\":\"tools/"
                                       "call\",\"params\":{\"name\":\"system_firmware_upgrade\","
                                       "\"arguments\":{\"firmware_url\":\"https://ota/fw.bin\"}}}",
                                       &sec_with_token);
    assert_non_null(resp);
    assert_null(strstr(resp, "Dual-Authorization Required"));
    assert_non_null(strstr(resp, "EXECUTED"));
    free(resp);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_bus_audit_blackbox_logging(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);

    /* 触发若干调用 (包含放行与阻断) */
    mcp_security_context_t guest = {.client_id = "guest", .role_level = MCP_SEC_L0_SAFE_READ};
    char *r1 = mcp_bus_dispatch_with_security(
        hub,
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/"
        "call\",\"params\":{\"name\":\"datahub_read_tags\",\"arguments\":{\"tag_ids\":[1001]}}}",
        &guest);
    free(r1);

    char *r2 = mcp_bus_dispatch_with_security(hub,
                                              "{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/"
                                              "call\",\"params\":{\"name\":\"datahub_write_tag\","
                                              "\"arguments\":{\"tag_id\":1001,\"value\":50.0}}}",
                                              &guest /* L0 写操作被阻断 */
    );
    free(r2);

    /* 查询审计黑匣子日志 */
    cJSON *logs = NULL;
    int count = mcp_bus_get_audit_logs(hub, 10, &logs);
    assert_true(count >= 2);
    assert_non_null(logs);

    char *logs_str = cJSON_PrintUnformatted(logs);
    assert_non_null(strstr(logs_str, "timestamp_ns"));
    assert_non_null(strstr(logs_str, "datahub_read_tags"));
    assert_non_null(strstr(logs_str, "Privilege Insufficient"));
    free(logs_str);
    cJSON_Delete(logs);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_bus_client_disconnect_cleanup(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);

    mcp_provider_desc_t p1 = {.provider_id = "p1", .client_fd = 20, .tool_count = 0};
    mcp_provider_desc_t p2 = {.provider_id = "p2", .client_fd = 21, .tool_count = 0};
    mcp_bus_register_provider(hub, &p1);
    mcp_bus_register_provider(hub, &p2);

    cJSON *providers = NULL;
    int cnt = mcp_bus_get_providers_json(hub, &providers);
    assert_int_equal(cnt, 2);
    cJSON_Delete(providers);

    /* 模拟 FD 20 客户端连接异常断开 */
    int removed = mcp_bus_unregister_by_fd(hub, 20);
    assert_int_equal(removed, 1);

    cnt = mcp_bus_get_providers_json(hub, &providers);
    assert_int_equal(cnt, 1);
    cJSON_Delete(providers);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_bus_security_defaults(void **state) {
    (void)state;
    /* 验证统一的默认 UDS 路径与 Localhost 绑定常量 (Issue #324) */
    assert_string_equal(EDGEOS_MCP_DEFAULT_UDS_PATH, "/tmp/edgeos_mcp_bus.sock");
    assert_string_equal(EDGEOS_MCP_DEFAULT_BIND_IP, "127.0.0.1");
    assert_int_equal(EDGEOS_MCP_DEFAULT_PORT, 8089);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mcp_bus_create_and_destroy),
        cmocka_unit_test(test_mcp_bus_dynamic_provider_registration),
        cmocka_unit_test(test_mcp_bus_l0_safe_read_passthrough),
        cmocka_unit_test(test_mcp_bus_l1_param_tuning_boundary_checks),
        cmocka_unit_test(test_mcp_bus_l2_five_anti_interlock_enforcement),
        cmocka_unit_test(test_mcp_bus_l3_dual_authorization_requirement),
        cmocka_unit_test(test_mcp_bus_audit_blackbox_logging),
        cmocka_unit_test(test_mcp_bus_client_disconnect_cleanup),
        cmocka_unit_test(test_mcp_bus_security_defaults),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
