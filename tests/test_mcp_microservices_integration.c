/**
 * @file test_mcp_microservices_integration.c
 * @brief 微服务 Provider 动态挂载与工业原子工具全链路测试 (ARCH-MCP-03 / Issue #302)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <cmocka.h>
#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>

#include "edgeos/edge_mcp_bus.h"
#include "edgeos/edge_mcp_dispatcher.h"
#include "edgeos/edge_mcp_industry_tools.h"

static void test_mcp_microservices_registration_and_aggregation(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);
    assert_non_null(hub);

    /* 挂载三大微服务 Provider */
    int rc = edge_mcp_register_all_microservices(hub, NULL);
    assert_int_equal(rc, 0);

    /* 验证 Provider 列表 */
    cJSON *providers = NULL;
    int pcount = mcp_bus_get_providers_json(hub, &providers);
    assert_int_equal(pcount, 3);
    assert_non_null(providers);
    assert_int_equal(cJSON_GetArraySize(providers), 3);
    cJSON_Delete(providers);

    /* 验证 tools/list 全量聚合 */
    char *resp = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/list\"}");
    assert_non_null(resp);

    /* 核心管理工具 */
    assert_non_null(strstr(resp, "mcp_register_provider"));
    assert_non_null(strstr(resp, "mcp_get_audit_logs"));

    /* IEMS 储能工具 */
    assert_non_null(strstr(resp, "iems_optimize_tou_schedule"));
    assert_non_null(strstr(resp, "ess_emergency_stop"));
    assert_non_null(strstr(resp, "iems_get_battery_status"));

    /* SmartBox 配电工具 */
    assert_non_null(strstr(resp, "smartbox_eval_transformer_life"));
    assert_non_null(strstr(resp, "smartbox_get_topology"));
    assert_non_null(strstr(resp, "security_simulate_interlock"));

    /* DCU 采集工具 */
    assert_non_null(strstr(resp, "dcu_analyze_power_quality"));
    assert_non_null(strstr(resp, "tag_batch_read_scaled"));

    free(resp);
    mcp_bus_hub_destroy(hub);
}

static void test_mcp_microservices_cascade_execution(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);
    assert_non_null(hub);
    assert_int_equal(edge_mcp_register_all_microservices(hub, NULL), 0);

    /* 1. 调用 SmartBox 变压器热点寿命评估 */
    char *resp1 = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":101,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"smartbox_eval_transformer_life\","
        "\"arguments\":{\"load_ratio\":1.15,\"ambient_temp_c\":35.0,\"current_top_oil_c\":78.0}}}");
    assert_non_null(resp1);
    assert_non_null(strstr(resp1, "hotspot_temp_c"));
    assert_non_null(strstr(resp1, "aging_acceleration_factor_faa"));
    assert_null(strstr(resp1, "\"error\""));
    free(resp1);

    /* 2. 调用 DCU 电能质量三相不平衡与谐波分析 */
    char *resp2 = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":102,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"dcu_analyze_power_quality\","
        "\"arguments\":{\"phase_a_v\":230.0,\"phase_b_v\":220.0,\"phase_c_v\":215.0,\"current_thd\":4.5}}}");
    assert_non_null(resp2);
    assert_non_null(strstr(resp2, "voltage_unbalance_pct"));
    assert_non_null(strstr(resp2, "phase_commutation_plan"));
    assert_null(strstr(resp2, "\"error\""));
    free(resp2);

    /* 3. 调用 IEMS 峰谷套利优化 */
    char *resp3 = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":103,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"iems_optimize_tou_schedule\","
        "\"arguments\":{\"soc_pct\":75.0,\"pv_forecast_kw\":45.0,\"load_forecast_kw\":28.0}}}");
    assert_non_null(resp3);
    assert_non_null(strstr(resp3, "schedule_24h"));
    assert_non_null(strstr(resp3, "daily_arbitrage_profit_cny"));
    assert_null(strstr(resp3, "\"error\""));
    free(resp3);

    /* 4. 调用 IEMS 电池簇物理状态查询 (Fast Track SHM) */
    char *resp4 = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":104,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"iems_get_battery_status\","
        "\"arguments\":{\"cluster_id\":\"RACK_01\",\"fast_track\":true}}}");
    assert_non_null(resp4);
    assert_non_null(strstr(resp4, "shm://datahub/offset=0x2000"));
    assert_non_null(strstr(resp4, "soc_pct"));
    assert_non_null(strstr(resp4, "cell_max_v"));
    assert_null(strstr(resp4, "\"error\""));
    free(resp4);

    /* 5. 调用 SmartBox 拓扑与分支相位查询 */
    char *resp5 = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":105,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"smartbox_get_topology\","
        "\"arguments\":{\"transformer_id\":\"TRANSFORMER_T01\",\"include_branches\":true}}}");
    assert_non_null(resp5);
    assert_non_null(strstr(resp5, "total_branches"));
    assert_non_null(strstr(resp5, "Phase_A"));
    assert_non_null(strstr(resp5, "shm://datahub/offset=0x3000"));
    assert_null(strstr(resp5, "\"error\""));
    free(resp5);

    /* 6. 调用 DCU 点位批量读取与字节序重排 (CDAB) */
    char *resp6 = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":106,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"tag_batch_read_scaled\","
        "\"arguments\":{\"tag_ids\":[1001, 1002, 2001],\"byte_order\":\"CDAB\",\"deadband\":0.01}}}");
    assert_non_null(resp6);
    assert_non_null(strstr(resp6, "scaled_value"));
    assert_non_null(strstr(resp6, "raw_hex"));
    assert_non_null(strstr(resp6, "shm://datahub/offset=0x1000"));
    assert_null(strstr(resp6, "\"error\""));
    free(resp6);

    /* 7. 调用 ESS 储能急停动作 (L2 特权) */
    mcp_security_context_t sec_l2 = {
        .client_id = "iems_emergency_copilot",
        .operator_id = "engineer_01",
        .role_level = MCP_SEC_L2_CRITICAL_ACTION,
        .request_timestamp_ns = (uint64_t)uv_hrtime()
    };
    char *resp7 = mcp_bus_dispatch_with_security(hub,
        "{\"jsonrpc\":\"2.0\",\"id\":107,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"ess_emergency_stop\","
        "\"arguments\":{\"system_id\":\"ESS_01\",\"reason\":\"GRID_SURGE_DETECTED\"}}}", &sec_l2);
    assert_non_null(resp7);
    assert_non_null(strstr(resp7, "EMERGENCY_STOP"));
    assert_non_null(strstr(resp7, "TRIPPED"));
    assert_null(strstr(resp7, "\"error\""));
    free(resp7);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_microservices_security_interlock_and_audit(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);
    assert_non_null(hub);
    assert_int_equal(edge_mcp_register_all_microservices(hub, NULL), 0);

    /* 1. L0 低权限客户端尝试触发 L2 级五防带负荷分闸 */
    mcp_security_context_t sec_low = {
        .client_id = "untrusted_web_guest",
        .operator_id = "guest",
        .role_level = MCP_SEC_L0_SAFE_READ,
        .request_timestamp_ns = (uint64_t)uv_hrtime()
    };

    char *resp_block = mcp_bus_dispatch_with_security(hub,
        "{\"jsonrpc\":\"2.0\",\"id\":201,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"security_simulate_interlock\","
        "\"arguments\":{\"breaker_id\":\"QS_01_ISOLATOR\",\"target_state\":\"OPEN\","
        "\"current_load_amps\":150.0,\"associated_cb_closed\":true}}}", &sec_low);
    assert_non_null(resp_block);
    /* 必须被安全门禁 100% 拦截 */
    assert_non_null(strstr(resp_block, "-32001") ? strstr(resp_block, "-32001") : strstr(resp_block, "-32002"));
    free(resp_block);

    /* 2. 检查黑匣子审计日志 */
    cJSON *audit_logs = NULL;
    int nlogs = mcp_bus_get_audit_logs(hub, 10, &audit_logs);
    assert_true(nlogs >= 1);
    assert_non_null(audit_logs);

    char *audit_str = cJSON_PrintUnformatted(audit_logs);
    assert_non_null(audit_str);
    assert_non_null(strstr(audit_str, "untrusted_web_guest"));
    assert_non_null(strstr(audit_str, "security_simulate_interlock"));
    free(audit_str);
    cJSON_Delete(audit_logs);

    mcp_bus_hub_destroy(hub);
}

static void test_mcp_microservices_dynamic_unmount(void **state) {
    (void)state;
    mcp_bus_hub_t *hub = mcp_bus_hub_create(NULL);
    assert_non_null(hub);
    assert_int_equal(edge_mcp_register_all_microservices(hub, NULL), 0);

    /* 注销 DCU Provider */
    int ret = mcp_bus_unregister_provider(hub, "dcu_service");
    assert_int_equal(ret, 0);

    /* 查询 tools/list：DCU 工具已自动下架，IEMS 和 SmartBox 依然正常 */
    char *resp = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":301,\"method\":\"tools/list\"}");
    assert_non_null(resp);
    assert_null(strstr(resp, "dcu_analyze_power_quality"));
    assert_null(strstr(resp, "tag_batch_read_scaled"));
    assert_non_null(strstr(resp, "iems_optimize_tou_schedule"));
    assert_non_null(strstr(resp, "smartbox_eval_transformer_life"));
    free(resp);

    /* 再次调用 DCU 工具应返回 METHOD_NOT_FOUND (-32601) */
    char *resp_dead = edge_mcp_dispatch_json(mcp_bus_get_dispatcher(hub),
        "{\"jsonrpc\":\"2.0\",\"id\":302,\"method\":\"tools/call\",\"params\":{"
        "\"name\":\"dcu_analyze_power_quality\","
        "\"arguments\":{\"phase_a_v\":220.0,\"phase_b_v\":220.0,\"phase_c_v\":220.0}}}");
    assert_non_null(resp_dead);
    assert_non_null(strstr(resp_dead, "-32601"));
    free(resp_dead);

    mcp_bus_hub_destroy(hub);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mcp_microservices_registration_and_aggregation),
        cmocka_unit_test(test_mcp_microservices_cascade_execution),
        cmocka_unit_test(test_mcp_microservices_security_interlock_and_audit),
        cmocka_unit_test(test_mcp_microservices_dynamic_unmount),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
