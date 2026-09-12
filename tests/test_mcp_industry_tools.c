/**
 * @file test_mcp_industry_tools.c
 * @brief Unit tests for Industry Specialized MCP Algorithm Tools (AEP-508)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmocka.h>
#include <cjson/cJSON.h>

#include "edgeos/edge_mcp_dispatcher.h"
#include "edgeos/edge_mcp_industry_tools.h"

/* 1. Test DCU Power Quality Analysis Tool */
static void test_mcp_tool_power_quality_normal(void **state) {
    (void)state;
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert_non_null(disp);

    const char *params = "{\"phase_a_v\": 220.0, \"phase_b_v\": 220.8, \"phase_c_v\": 219.4, \"current_thd\": 2.3}";
    char *result_json = NULL;
    int rc = edge_mcp_call_tool(disp, "dcu_analyze_power_quality", params, &result_json);
    assert_int_equal(rc, 0);
    assert_non_null(result_json);

    cJSON *res = cJSON_Parse(result_json);
    assert_non_null(res);

    cJSON *unb_pct = cJSON_GetObjectItem(res, "voltage_unbalance_pct");
    assert_non_null(unb_pct);
    assert_true(unb_pct->valuedouble < 2.0);

    cJSON *unb_status = cJSON_GetObjectItem(res, "unbalance_status");
    assert_non_null(unb_status);
    assert_string_equal(unb_status->valuestring, "NORMAL");

    cJSON *thd_status = cJSON_GetObjectItem(res, "thd_status");
    assert_non_null(thd_status);
    assert_string_equal(thd_status->valuestring, "COMPLIANT");

    cJSON *harmonics = cJSON_GetObjectItem(res, "harmonic_spectrum_31st");
    assert_non_null(harmonics);
    assert_true(cJSON_IsArray(harmonics));
    assert_int_equal(cJSON_GetArraySize(harmonics), 15);

    cJSON_Delete(res);
    free(result_json);
    edge_mcp_dispatcher_destroy(disp);
}

static void test_mcp_tool_power_quality_critical_commutation(void **state) {
    (void)state;
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert_non_null(disp);

    const char *params = "{\"phase_a_v\": 238.0, \"phase_b_v\": 218.0, \"phase_c_v\": 204.0, \"current_thd\": 7.2}";
    char *result_json = NULL;
    int rc = edge_mcp_call_tool(disp, "dcu_analyze_power_quality", params, &result_json);
    assert_int_equal(rc, 0);
    assert_non_null(result_json);

    cJSON *res = cJSON_Parse(result_json);
    assert_non_null(res);

    cJSON *unb_pct = cJSON_GetObjectItem(res, "voltage_unbalance_pct");
    assert_non_null(unb_pct);
    assert_true(unb_pct->valuedouble > 4.0);

    cJSON *unb_status = cJSON_GetObjectItem(res, "unbalance_status");
    assert_non_null(unb_status);
    assert_string_equal(unb_status->valuestring, "CRITICAL");

    cJSON *thd_status = cJSON_GetObjectItem(res, "thd_status");
    assert_non_null(thd_status);
    assert_string_equal(thd_status->valuestring, "VIOLATION");

    cJSON *plan = cJSON_GetObjectItem(res, "phase_commutation_plan");
    assert_non_null(plan);
    cJSON *act_req = cJSON_GetObjectItem(plan, "action_required");
    assert_true(cJSON_IsTrue(act_req));

    cJSON *transfer_p = cJSON_GetObjectItem(plan, "suggested_transfer_power_kw");
    assert_non_null(transfer_p);
    assert_true(transfer_p->valuedouble > 0.0);

    cJSON_Delete(res);
    free(result_json);
    edge_mcp_dispatcher_destroy(disp);
}

/* 2. Test SmartBox Transformer Thermal & Life Evaluation Tool */
static void test_mcp_tool_transformer_life_normal(void **state) {
    (void)state;
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert_non_null(disp);

    const char *params = "{\"load_ratio\": 0.65, \"ambient_temp_c\": 25.0, \"current_top_oil_c\": 52.0}";
    char *result_json = NULL;
    int rc = edge_mcp_call_tool(disp, "smartbox_eval_transformer_life", params, &result_json);
    assert_int_equal(rc, 0);
    assert_non_null(result_json);

    cJSON *res = cJSON_Parse(result_json);
    assert_non_null(res);

    cJSON *hotspot = cJSON_GetObjectItem(res, "hotspot_temp_c");
    assert_non_null(hotspot);
    assert_true(hotspot->valuedouble < 105.0);

    cJSON *faa = cJSON_GetObjectItem(res, "aging_acceleration_factor_faa");
    assert_non_null(faa);
    assert_true(faa->valuedouble < 1.0);

    cJSON *status = cJSON_GetObjectItem(res, "thermal_status");
    assert_non_null(status);
    assert_string_equal(status->valuestring, "NORMAL");

    cJSON *fan = cJSON_GetObjectItem(res, "cooling_fan_override_recommended");
    assert_false(cJSON_IsTrue(fan));

    cJSON_Delete(res);
    free(result_json);
    edge_mcp_dispatcher_destroy(disp);
}

static void test_mcp_tool_transformer_life_overload_critical(void **state) {
    (void)state;
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert_non_null(disp);

    const char *params = "{\"load_ratio\": 1.25, \"ambient_temp_c\": 42.0, \"current_top_oil_c\": 88.0}";
    char *result_json = NULL;
    int rc = edge_mcp_call_tool(disp, "smartbox_eval_transformer_life", params, &result_json);
    assert_int_equal(rc, 0);
    assert_non_null(result_json);

    cJSON *res = cJSON_Parse(result_json);
    assert_non_null(res);

    cJSON *hotspot = cJSON_GetObjectItem(res, "hotspot_temp_c");
    assert_non_null(hotspot);
    assert_true(hotspot->valuedouble >= 120.0);

    cJSON *faa = cJSON_GetObjectItem(res, "aging_acceleration_factor_faa");
    assert_non_null(faa);
    assert_true(faa->valuedouble > 5.0);

    cJSON *status = cJSON_GetObjectItem(res, "thermal_status");
    assert_non_null(status);
    assert_string_equal(status->valuestring, "CRITICAL");

    cJSON *fan = cJSON_GetObjectItem(res, "cooling_fan_override_recommended");
    assert_true(cJSON_IsTrue(fan));

    cJSON *shed = cJSON_GetObjectItem(res, "load_shedding_recommended_kw");
    assert_non_null(shed);
    assert_true(shed->valuedouble > 0.0);

    cJSON_Delete(res);
    free(result_json);
    edge_mcp_dispatcher_destroy(disp);
}

/* 3. Test IEMS TOU Arbitrage Scheduler Tool */
static void test_mcp_tool_tou_arbitrage_optimization(void **state) {
    (void)state;
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert_non_null(disp);

    const char *params = "{\"soc_pct\": 45.0, \"battery_capacity_kwh\": 200.0, \"pcs_max_power_kw\": 100.0, \"pv_forecast_kw\": 50.0, \"load_forecast_kw\": 80.0}";
    char *result_json = NULL;
    int rc = edge_mcp_call_tool(disp, "iems_optimize_tou_schedule", params, &result_json);
    assert_int_equal(rc, 0);
    assert_non_null(result_json);

    cJSON *res = cJSON_Parse(result_json);
    assert_non_null(res);

    cJSON *profit = cJSON_GetObjectItem(res, "daily_arbitrage_profit_cny");
    assert_non_null(profit);
    assert_true(profit->valuedouble > 0.0);

    cJSON *savings = cJSON_GetObjectItem(res, "daily_cost_savings_pct");
    assert_non_null(savings);
    assert_true(savings->valuedouble > 0.0);

    cJSON *sched = cJSON_GetObjectItem(res, "schedule_24h");
    assert_non_null(sched);
    assert_true(cJSON_IsArray(sched));
    assert_int_equal(cJSON_GetArraySize(sched), 24);

    /* Check valley hour charges (Hour 0 charges until full) */
    cJSON *slot_valley = cJSON_GetArrayItem(sched, 0);
    assert_non_null(slot_valley);
    cJSON *act_v = cJSON_GetObjectItem(slot_valley, "action");
    assert_string_equal(act_v->valuestring, "CHARGE");

    /* Check peak hour discharges (Hour 8 morning peak) */
    cJSON *slot_peak = cJSON_GetArrayItem(sched, 8);
    assert_non_null(slot_peak);
    cJSON *act_p = cJSON_GetObjectItem(slot_peak, "action");
    assert_string_equal(act_p->valuestring, "DISCHARGE");

    cJSON_Delete(res);
    free(result_json);
    edge_mcp_dispatcher_destroy(disp);
}

/* 4. Test Electrical Five-Anti Safety Interlocking Simulator */
static void test_mcp_tool_electrical_five_anti_interlock(void **state) {
    (void)state;
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert_non_null(disp);

    /* Case A: Disconnector operation under heavy load (Violates Rule 2) */
    {
        const char *p_load = "{\"breaker_id\": \"DS-101-1\", \"target_state\": \"OPEN\", \"current_load_amps\": 350.0, \"associated_cb_closed\": true}";
        char *out = NULL;
        int rc = edge_mcp_call_tool(disp, "security_simulate_interlock", p_load, &out);
        assert_int_equal(rc, 0);
        cJSON *res = cJSON_Parse(out);
        assert_non_null(res);
        assert_false(cJSON_IsTrue(cJSON_GetObjectItem(res, "operation_allowed")));
        cJSON *rules = cJSON_GetObjectItem(res, "violated_rules");
        assert_true(cJSON_GetArraySize(rules) > 0);
        cJSON_Delete(res);
        free(out);
    }

    /* Case B: Earth switch closing on energized line (Violates Rule 3) */
    {
        const char *p_live = "{\"breaker_id\": \"ES-101\", \"target_state\": \"CLOSE\", \"line_voltage_v\": 10000.0}";
        char *out = NULL;
        int rc = edge_mcp_call_tool(disp, "security_simulate_interlock", p_live, &out);
        assert_int_equal(rc, 0);
        cJSON *res = cJSON_Parse(out);
        assert_non_null(res);
        assert_false(cJSON_IsTrue(cJSON_GetObjectItem(res, "operation_allowed")));
        cJSON_Delete(res);
        free(out);
    }

    /* Case C: Breaker closing when Earth switch is active (Violates Rule 4) */
    {
        const char *p_earth = "{\"breaker_id\": \"CB-101\", \"target_state\": \"CLOSE\", \"associated_es_closed\": true}";
        char *out = NULL;
        int rc = edge_mcp_call_tool(disp, "security_simulate_interlock", p_earth, &out);
        assert_int_equal(rc, 0);
        cJSON *res = cJSON_Parse(out);
        assert_non_null(res);
        assert_false(cJSON_IsTrue(cJSON_GetObjectItem(res, "operation_allowed")));
        cJSON_Delete(res);
        free(out);
    }

    /* Case D: Safe Breaker Opening with zero violation */
    {
        const char *p_safe = "{\"breaker_id\": \"CB-101\", \"target_state\": \"OPEN\", \"current_load_amps\": 0.0, \"line_voltage_v\": 0.0}";
        char *out = NULL;
        int rc = edge_mcp_call_tool(disp, "security_simulate_interlock", p_safe, &out);
        assert_int_equal(rc, 0);
        cJSON *res = cJSON_Parse(out);
        assert_non_null(res);
        assert_true(cJSON_IsTrue(cJSON_GetObjectItem(res, "operation_allowed")));
        cJSON *sop = cJSON_GetObjectItem(res, "sequential_sop_steps");
        assert_true(cJSON_IsArray(sop));
        assert_true(cJSON_GetArraySize(sop) >= 3);
        cJSON_Delete(res);
        free(out);
    }

    edge_mcp_dispatcher_destroy(disp);
}

/* 5. Test JSON-RPC 2.0 Full List & Call Dispatch Integration */
static void test_mcp_json_rpc_tools_list_and_call(void **state) {
    (void)state;
    edge_mcp_dispatcher_t *disp = edge_mcp_dispatcher_create(NULL);
    assert_non_null(disp);

    /* Test tools/list */
    const char *list_req = "{\"jsonrpc\":\"2.0\",\"id\":101,\"method\":\"tools/list\"}";
    char *list_resp = edge_mcp_dispatch_json(disp, list_req);
    assert_non_null(list_resp);

    cJSON *resp_obj = cJSON_Parse(list_resp);
    assert_non_null(resp_obj);
    cJSON *result = cJSON_GetObjectItem(resp_obj, "result");
    assert_non_null(result);
    cJSON *tools = cJSON_GetObjectItem(result, "tools");
    assert_non_null(tools);
    assert_true(cJSON_IsArray(tools));
    /* 13 default tools + 4 industry algorithm tools = 17 tools */
    assert_int_equal(cJSON_GetArraySize(tools), 17);

    cJSON_Delete(resp_obj);
    free(list_resp);

    /* Test tools/call JSON-RPC for dcu_analyze_power_quality */
    const char *call_req = "{\"jsonrpc\":\"2.0\",\"id\":102,\"method\":\"tools/call\",\"params\":{\"name\":\"dcu_analyze_power_quality\",\"arguments\":{\"phase_a_v\":220.0,\"phase_b_v\":220.0,\"phase_c_v\":220.0}}}";
    char *call_resp = edge_mcp_dispatch_json(disp, call_req);
    assert_non_null(call_resp);

    cJSON *call_obj = cJSON_Parse(call_resp);
    assert_non_null(call_obj);
    cJSON *call_res = cJSON_GetObjectItem(call_obj, "result");
    assert_non_null(call_res);
    cJSON *content = cJSON_GetObjectItem(call_res, "content");
    assert_non_null(content);
    assert_true(cJSON_IsArray(content));

    cJSON_Delete(call_obj);
    free(call_resp);
    edge_mcp_dispatcher_destroy(disp);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_mcp_tool_power_quality_normal),
        cmocka_unit_test(test_mcp_tool_power_quality_critical_commutation),
        cmocka_unit_test(test_mcp_tool_transformer_life_normal),
        cmocka_unit_test(test_mcp_tool_transformer_life_overload_critical),
        cmocka_unit_test(test_mcp_tool_tou_arbitrage_optimization),
        cmocka_unit_test(test_mcp_tool_electrical_five_anti_interlock),
        cmocka_unit_test(test_mcp_json_rpc_tools_list_and_call),
    };

    printf("=== Running EdgeOS Industry Specialized MCP Algorithm Tools (AEP-508) Tests ===\n");
    return cmocka_run_group_tests(tests, NULL, NULL);
}
