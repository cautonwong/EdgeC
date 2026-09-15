/**
 * @file mcp_tools_tou.c
 * @brief IEMS Photovoltaic & Energy Storage TOU Arbitrage MCP Tool (AEP-508)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 *
 * Implements 24-hour Time-of-Use (TOU) dynamic tariff optimization, battery
 * charge/discharge dispatch scheduling, and arbitrage revenue prediction.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cjson/cJSON.h>

#include "edgeos/edge_mcp_industry_tools.h"
#include "edge_log.h"

/* Standard Industrial TOU Tariff Template (¥/kWh) */
typedef struct {
    int hour;
    double tariff_rate;
    const char *period_type; /* "VALLEY", "FLAT", "PEAK", "DEEP_PEAK" */
} hourly_tariff_t;

static hourly_tariff_t g_default_tariffs[24] = {
    {0, 0.35, "VALLEY"}, {1, 0.35, "VALLEY"}, {2, 0.35, "VALLEY"}, {3, 0.35, "VALLEY"},
    {4, 0.35, "VALLEY"}, {5, 0.35, "VALLEY"}, {6, 0.35, "VALLEY"}, {7, 0.72, "FLAT"},
    {8, 1.25, "PEAK"},   {9, 1.25, "PEAK"},   {10, 1.25, "PEAK"},  {11, 0.72, "FLAT"},
    {12, 0.72, "FLAT"},  {13, 0.72, "FLAT"},  {14, 0.72, "FLAT"},  {15, 1.68, "DEEP_PEAK"},
    {16, 1.68, "DEEP_PEAK"}, {17, 1.68, "DEEP_PEAK"}, {18, 1.25, "PEAK"}, {19, 1.25, "PEAK"},
    {20, 1.25, "PEAK"},  {21, 0.72, "FLAT"},  {22, 0.72, "FLAT"},  {23, 0.35, "VALLEY"}
};

int handle_iems_optimize_tou_schedule(edge_mcp_dispatcher_t *disp,
                                      const cJSON *params,
                                      cJSON **out_result,
                                      char *err_msg,
                                      size_t err_len) {
    (void)disp;
    if (!params) {
        snprintf(err_msg, err_len, "Missing parameters for iems_optimize_tou_schedule");
        return MCP_RPC_INVALID_PARAMS;
    }

    const cJSON *soc_item = cJSON_GetObjectItem(params, "soc_pct");
    if (!soc_item || !cJSON_IsNumber(soc_item)) {
        snprintf(err_msg, err_len, "Params must include numeric 'soc_pct' (0..100)");
        return MCP_RPC_INVALID_PARAMS;
    }

    double current_soc = soc_item->valuedouble;
    if (!isfinite(current_soc) || current_soc < 0.0 || current_soc > 100.0) {
        snprintf(err_msg, err_len, "'soc_pct' must be a valid finite number within [0, 100]");
        return MCP_RPC_INVALID_PARAMS;
    }

    /* Capacity and Power constraints */
    double batt_cap_kwh = 200.0;
    double pcs_max_kw = 100.0;
    double soc_min = 10.0;
    double soc_max = 95.0;
    double round_trip_eff = 0.92;

    const cJSON *cap_item = cJSON_GetObjectItem(params, "battery_capacity_kwh");
    if (cap_item && cJSON_IsNumber(cap_item)) {
        batt_cap_kwh = cap_item->valuedouble;
        if (!isfinite(batt_cap_kwh) || batt_cap_kwh <= 0.0) {
            snprintf(err_msg, err_len, "'battery_capacity_kwh' must be positive finite number");
            return MCP_RPC_INVALID_PARAMS;
        }
    }

    const cJSON *pcs_item = cJSON_GetObjectItem(params, "pcs_max_power_kw");
    if (pcs_item && cJSON_IsNumber(pcs_item)) {
        pcs_max_kw = pcs_item->valuedouble;
        if (!isfinite(pcs_max_kw) || pcs_max_kw <= 0.0) {
            snprintf(err_msg, err_len, "'pcs_max_power_kw' must be positive finite number");
            return MCP_RPC_INVALID_PARAMS;
        }
    }

    double pv_avg_kw = 45.0;
    double load_avg_kw = 80.0;
    const cJSON *pv_item = cJSON_GetObjectItem(params, "pv_forecast_kw");
    if (pv_item && cJSON_IsNumber(pv_item)) {
        pv_avg_kw = pv_item->valuedouble;
        if (!isfinite(pv_avg_kw) || pv_avg_kw < 0.0) {
            snprintf(err_msg, err_len, "'pv_forecast_kw' must be non-negative finite number");
            return MCP_RPC_INVALID_PARAMS;
        }
    }

    const cJSON *load_item = cJSON_GetObjectItem(params, "load_forecast_kw");
    if (load_item && cJSON_IsNumber(load_item)) {
        load_avg_kw = load_item->valuedouble;
        if (!isfinite(load_avg_kw) || load_avg_kw < 0.0) {
            snprintf(err_msg, err_len, "'load_forecast_kw' must be non-negative finite number");
            return MCP_RPC_INVALID_PARAMS;
        }
    }

    /* 1. Simulate 24-Hour Schedule Optimization */
    double baseline_daily_cost = 0.0;
    double optimized_daily_cost = 0.0;
    double total_charged_kwh = 0.0;
    double total_discharged_kwh = 0.0;
    double max_peak_shaved_kw = 0.0;

    double sim_soc = current_soc;
    cJSON *schedule_arr = cJSON_CreateArray();

    for (int h = 0; h < 24; h++) {
        hourly_tariff_t t_info = g_default_tariffs[h];
        double rate = t_info.tariff_rate;

        /* Standard baseline load and solar curve model */
        double h_pv = 0.0;
        if (h >= 6 && h <= 18) {
            h_pv = pv_avg_kw * sin((double)(h - 6) * M_PI / 12.0) * 1.5;
            if (h_pv < 0.0) h_pv = 0.0;
        }
        double h_load = load_avg_kw * (0.7 + 0.5 * sin((double)(h - 2) * M_PI / 12.0));
        if (h_load < 20.0) h_load = 20.0;

        /* Baseline cost without ESS */
        double net_grid_base = (h_load > h_pv) ? (h_load - h_pv) : 0.0;
        baseline_daily_cost += net_grid_base * rate;

        /* ESS Decision Logic */
        const char *action = "STANDBY";
        double ess_power_kw = 0.0; /* Positive = Discharge, Negative = Charge */

        if (strcmp(t_info.period_type, "VALLEY") == 0) {
            /* Charge during valley */
            double max_charge_energy = (soc_max - sim_soc) / 100.0 * batt_cap_kwh;
            double charge_p = fmin(pcs_max_kw, max_charge_energy);
            if (charge_p > 1.0) {
                action = "CHARGE";
                ess_power_kw = -charge_p;
                sim_soc += (charge_p * sqrt(round_trip_eff) / batt_cap_kwh) * 100.0;
                total_charged_kwh += charge_p;
            }
        } else if (strcmp(t_info.period_type, "DEEP_PEAK") == 0 || strcmp(t_info.period_type, "PEAK") == 0) {
            /* Discharge during peak / deep peak to offset net grid purchase */
            double net_load = (h_load > h_pv) ? (h_load - h_pv) : 0.0;
            double max_discharge_energy = (sim_soc - soc_min) / 100.0 * batt_cap_kwh;
            double discharge_p = fmin(pcs_max_kw, fmin(net_load, max_discharge_energy));
            if (discharge_p > 1.0) {
                action = "DISCHARGE";
                ess_power_kw = discharge_p;
                sim_soc -= (discharge_p / (sqrt(round_trip_eff) * batt_cap_kwh)) * 100.0;
                total_discharged_kwh += discharge_p;
                if (discharge_p > max_peak_shaved_kw) max_peak_shaved_kw = discharge_p;
            }
        } else {
            /* Flat period: absorb excess solar generation for free */
            if (h_pv > h_load && sim_soc < soc_max) {
                double excess_pv = h_pv - h_load;
                double max_charge = (soc_max - sim_soc) / 100.0 * batt_cap_kwh;
                double charge_p = fmin(pcs_max_kw, fmin(excess_pv, max_charge));
                if (charge_p > 1.0) {
                    action = "CHARGE_SOLAR";
                    ess_power_kw = -charge_p;
                    sim_soc += (charge_p * sqrt(round_trip_eff) / batt_cap_kwh) * 100.0;
                    total_charged_kwh += charge_p;
                }
            }
        }

        /* Clamp SOC */
        if (sim_soc < soc_min) sim_soc = soc_min;
        if (sim_soc > soc_max) sim_soc = soc_max;

        /* Optimized grid cost */
        double net_grid_opt = h_load - h_pv - ess_power_kw;
        if (net_grid_opt < 0.0) net_grid_opt = 0.0;
        optimized_daily_cost += net_grid_opt * rate;

        /* Append schedule slot */
        cJSON *slot = cJSON_CreateObject();
        cJSON_AddNumberToObject(slot, "hour", h);
        cJSON_AddStringToObject(slot, "period", t_info.period_type);
        cJSON_AddNumberToObject(slot, "tariff_rate_cny", rate);
        cJSON_AddStringToObject(slot, "action", action);
        cJSON_AddNumberToObject(slot, "ess_power_kw", round(ess_power_kw * 10.0) / 10.0);
        cJSON_AddNumberToObject(slot, "soc_pct", round(sim_soc * 10.0) / 10.0);
        cJSON_AddNumberToObject(slot, "load_forecast_kw", round(h_load * 10.0) / 10.0);
        cJSON_AddNumberToObject(slot, "pv_forecast_kw", round(h_pv * 10.0) / 10.0);
        cJSON_AddItemToArray(schedule_arr, slot);
    }

    double daily_profit = baseline_daily_cost - optimized_daily_cost;
    if (daily_profit < 0.0) daily_profit = 0.0;
    double savings_pct = (baseline_daily_cost > 0.0) ? (daily_profit / baseline_daily_cost * 100.0) : 0.0;
    double annualized_profit = daily_profit * 365.0;

    /* 2. Assemble Result */
    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "daily_arbitrage_profit_cny", round(daily_profit * 100.0) / 100.0);
    cJSON_AddNumberToObject(res, "daily_cost_savings_pct", round(savings_pct * 10.0) / 10.0);
    cJSON_AddNumberToObject(res, "annualized_profit_projection_cny", round(annualized_profit));
    cJSON_AddNumberToObject(res, "baseline_daily_cost_cny", round(baseline_daily_cost * 100.0) / 100.0);
    cJSON_AddNumberToObject(res, "optimized_daily_cost_cny", round(optimized_daily_cost * 100.0) / 100.0);

    cJSON_AddNumberToObject(res, "total_daily_charge_kwh", round(total_charged_kwh * 10.0) / 10.0);
    cJSON_AddNumberToObject(res, "total_daily_discharge_kwh", round(total_discharged_kwh * 10.0) / 10.0);
    cJSON_AddNumberToObject(res, "peak_shaving_achieved_kw", round(max_peak_shaved_kw * 10.0) / 10.0);

    cJSON_AddItemToObject(res, "schedule_24h", schedule_arr);

    char summary_buf[512];
    snprintf(summary_buf, sizeof(summary_buf),
             "TOU Optimization complete: Daily profit ¥%.2f (%.1f%% savings, Annualized ¥%.0f). Discharged %.1f kWh during Peak/Deep-Peak, charged %.1f kWh during Valley.",
             daily_profit, savings_pct, annualized_profit, total_discharged_kwh, total_charged_kwh);
    cJSON_AddStringToObject(res, "summary", summary_buf);

    *out_result = res;
    return 0;
}
