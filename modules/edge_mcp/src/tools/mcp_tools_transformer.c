/**
 * @file mcp_tools_transformer.c
 * @brief SmartBox Transformer Life & Thermal Rise MCP Tool (AEP-508)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 *
 * Implements IEEE C57.91 / IEC 60076-7 thermal models for transformer hot-spot
 * temperature calculation, Arrhenius insulation aging factor (FAA), and residual life.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cjson/cJSON.h>

#include "edgeos/edge_mcp_industry_tools.h"
#include "edge_log.h"

int handle_smartbox_eval_transformer_life(edge_mcp_dispatcher_t *disp,
                                         const cJSON *params,
                                         cJSON **out_result,
                                         char *err_msg,
                                         size_t err_len) {
    (void)disp;
    if (!params) {
        snprintf(err_msg, err_len, "Missing required parameters for smartbox_eval_transformer_life");
        return MCP_RPC_INVALID_PARAMS;
    }

    const cJSON *load_item = cJSON_GetObjectItem(params, "load_ratio");
    const cJSON *amb_item = cJSON_GetObjectItem(params, "ambient_temp_c");
    const cJSON *top_oil_item = cJSON_GetObjectItem(params, "current_top_oil_c");

    if (!load_item || !cJSON_IsNumber(load_item) ||
        !amb_item || !cJSON_IsNumber(amb_item) ||
        !top_oil_item || !cJSON_IsNumber(top_oil_item)) {
        snprintf(err_msg, err_len, "Params must include numeric 'load_ratio', 'ambient_temp_c', and 'current_top_oil_c'");
        return MCP_RPC_INVALID_PARAMS;
    }

    double load_ratio = load_item->valuedouble;
    double ambient_temp = amb_item->valuedouble;
    double top_oil_temp = top_oil_item->valuedouble;

    if (load_ratio < 0.0) {
        snprintf(err_msg, err_len, "load_ratio cannot be negative");
        return MCP_RPC_INVALID_PARAMS;
    }

    /* Optional rating parameters */
    double rated_hotspot_rise = 65.0; /* K */
    const cJSON *hrise_item = cJSON_GetObjectItem(params, "rated_hotspot_rise");
    if (hrise_item && cJSON_IsNumber(hrise_item)) {
        rated_hotspot_rise = hrise_item->valuedouble;
    }

    /* 1. Hot-spot Temperature Rise Calculation (IEEE C57.91) */
    /* Exponent m = 0.8 for ONAN cooling mode -> 2m = 1.6 */
    double hotspot_gradient = rated_hotspot_rise * pow(load_ratio, 1.6);
    double hotspot_temp_c = top_oil_temp + hotspot_gradient;

    /* 2. Arrhenius Aging Acceleration Factor (FAA) */
    /* Reference hotspot: 110 deg C (383.15 K) where FAA = 1.0 */
    double hotspot_kelvin = hotspot_temp_c + 273.15;
    double faa = exp((15000.0 / 383.15) - (15000.0 / hotspot_kelvin));
    if (faa < 0.001) faa = 0.001;

    /* 3. Loss of Life & Lifespan estimation */
    /* Normal expected insulation life: 180,000 hours (~20.55 years) */
    double loss_of_life_daily_hrs = 24.0 * faa;
    double normal_life_years = 20.55;
    double estimated_remaining_years = (faa > 0.0) ? (normal_life_years / faa) : 100.0;
    if (estimated_remaining_years > 50.0) estimated_remaining_years = 50.0;

    /* 4. Thermal & Risk Assessment */
    const char *thermal_status = "NORMAL";
    bool fan_override = false;
    double load_shedding_kw = 0.0;

    if (hotspot_temp_c >= 120.0) {
        thermal_status = "CRITICAL";
        fan_override = true;
        /* Recommend shedding load to bring load_ratio below 0.9 */
        if (load_ratio > 0.9) {
            load_shedding_kw = (load_ratio - 0.9) * 1000.0; /* Based on 1000 kVA base */
        }
    } else if (hotspot_temp_c >= 105.0) {
        thermal_status = "ELEVATED";
        fan_override = true;
    }

    /* 5. Assemble JSON response */
    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "hotspot_temp_c", round(hotspot_temp_c * 100.0) / 100.0);
    cJSON_AddNumberToObject(res, "top_oil_temp_c", round(top_oil_temp * 100.0) / 100.0);
    cJSON_AddNumberToObject(res, "ambient_temp_c", round(ambient_temp * 100.0) / 100.0);
    cJSON_AddNumberToObject(res, "load_ratio", round(load_ratio * 1000.0) / 1000.0);
    cJSON_AddNumberToObject(res, "hotspot_gradient_k", round(hotspot_gradient * 100.0) / 100.0);

    cJSON_AddNumberToObject(res, "aging_acceleration_factor_faa", round(faa * 1000.0) / 1000.0);
    cJSON_AddNumberToObject(res, "loss_of_life_per_day_hours", round(loss_of_life_daily_hrs * 100.0) / 100.0);
    cJSON_AddNumberToObject(res, "estimated_remaining_life_years", round(estimated_remaining_years * 10.0) / 10.0);

    cJSON_AddStringToObject(res, "thermal_status", thermal_status);
    cJSON_AddBoolToObject(res, "cooling_fan_override_recommended", fan_override);
    cJSON_AddNumberToObject(res, "load_shedding_recommended_kw", round(load_shedding_kw * 10.0) / 10.0);

    char rec_buf[512];
    if (hotspot_temp_c >= 120.0) {
        snprintf(rec_buf, sizeof(rec_buf),
                 "CRITICAL: Hot-spot temp %.1f°C exceeds 120°C limit! FAA=%.2f accelerates insulation degradation by %.1fx. Force cooling fans ON and shed %.1f kW load immediately.",
                 hotspot_temp_c, faa, faa, load_shedding_kw);
    } else if (hotspot_temp_c >= 105.0) {
        snprintf(rec_buf, sizeof(rec_buf),
                 "WARNING: Hot-spot temp %.1f°C is elevated (FAA=%.2f). Engage auxiliary cooling to prevent accelerated aging.",
                 hotspot_temp_c, faa);
    } else {
        snprintf(rec_buf, sizeof(rec_buf),
                 "NORMAL: Transformer thermal state is optimal (Hot-spot: %.1f°C, FAA=%.2f). Projected life: %.1f years.",
                 hotspot_temp_c, faa, estimated_remaining_years);
    }
    cJSON_AddStringToObject(res, "recommendation", rec_buf);

    *out_result = res;
    return 0;
}
