/**
 * @file mcp_tools_registration.c
 * @brief Industry Specialized Tools Auto-Registration (AEP-508)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 */

#include "edgeos/edge_mcp_industry_tools.h"

static edge_mcp_tool_def_t g_industry_tools[] = {
    {
        .name = "dcu_analyze_power_quality",
        .description = "Analyzes 3-phase voltage unbalance (GB/T 15543), 31st harmonic total distortion rate (THD), and outputs optimal phase commutation recommendations",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"phase_a_v\":{\"type\":\"number\"},\"phase_b_v\":{\"type\":\"number\"},\"phase_c_v\":{\"type\":\"number\"},\"current_thd\":{\"type\":\"number\"}},\"required\":[\"phase_a_v\",\"phase_b_v\",\"phase_c_v\"]}",
        .handler = handle_dcu_analyze_power_quality
    },
    {
        .name = "smartbox_eval_transformer_life",
        .description = "Evaluates distribution transformer winding hot-spot temperature rise, insulation aging acceleration factor (FAA), and estimated remaining operational lifespan based on IEEE C57.91",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"load_ratio\":{\"type\":\"number\"},\"ambient_temp_c\":{\"type\":\"number\"},\"current_top_oil_c\":{\"type\":\"number\"}},\"required\":[\"load_ratio\",\"ambient_temp_c\",\"current_top_oil_c\"]}",
        .handler = handle_smartbox_eval_transformer_life
    },
    {
        .name = "iems_optimize_tou_schedule",
        .description = "Optimizes industrial energy storage (ESS) charge/discharge TOU schedule against peak/valley tariff rates, PV generation forecasts, and load forecasts to maximize daily arbitrage profits",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"soc_pct\":{\"type\":\"number\"},\"pv_forecast_kw\":{\"type\":\"number\"},\"load_forecast_kw\":{\"type\":\"number\"}},\"required\":[\"soc_pct\"]}",
        .handler = handle_iems_optimize_tou_schedule
    },
    {
        .name = "security_simulate_interlock",
        .description = "Simulates electrical switching operations against electrical 'Five-Anti' safety interlocking rules, preventing fatal arc flashes and misoperations",
        .input_schema_json = "{\"type\":\"object\",\"properties\":{\"breaker_id\":{\"type\":\"string\"},\"target_state\":{\"type\":\"string\",\"enum\":[\"OPEN\",\"CLOSE\"]}},\"required\":[\"breaker_id\",\"target_state\"]}",
        .handler = handle_security_simulate_interlock
    }
};

int edge_mcp_register_industry_tools(edge_mcp_dispatcher_t *disp) {
    if (!disp) return -1;
    size_t count = sizeof(g_industry_tools) / sizeof(g_industry_tools[0]);
    for (size_t i = 0; i < count; i++) {
        int rc = edge_mcp_register_tool(disp, &g_industry_tools[i]);
        if (rc != 0) return rc;
    }
    return 0;
}
