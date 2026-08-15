/**
 * @file iems_commissioning_tool.c
 * @brief Production Commercial Energy Storage Site Commissioning & Pre-Flight Self-Check Implementation
 */

#include "iems_commissioning_tool.h"
#include <string.h>
#include <stdio.h>

void iems_commissioning_set_transformer(iems_meter_transformer_cfg_t* cfg,
                                        uint32_t ct_pri, uint32_t ct_sec,
                                        uint32_t pt_pri, uint32_t pt_sec) {
    if (!cfg) return;
    memset(cfg, 0, sizeof(iems_meter_transformer_cfg_t));

    cfg->ct_primary = (ct_pri > 0) ? ct_pri : 1000;
    cfg->ct_secondary = (ct_sec > 0) ? ct_sec : 5;
    cfg->pt_primary = (pt_pri > 0) ? pt_pri : 10000;
    cfg->pt_secondary = (pt_sec > 0) ? pt_sec : 100;

    cfg->calculated_ct_ratio = (double)cfg->ct_primary / (double)cfg->ct_secondary;
    cfg->calculated_pt_ratio = (double)cfg->pt_primary / (double)cfg->pt_secondary;
    cfg->total_power_multiplier = cfg->calculated_ct_ratio * cfg->calculated_pt_ratio;
}

double iems_commissioning_scale_power(const iems_meter_transformer_cfg_t* cfg, double raw_power_w) {
    if (!cfg) return raw_power_w;
    return raw_power_w * cfg->total_power_multiplier;
}

int iems_commissioning_run_preflight_check(iems_commissioning_report_t* report,
                                          double u_grid_v, double f_grid_hz,
                                          double bms_r_iso_kohm, double bms_max_rack_v_diff,
                                          bool pcs_fault, bool fire_alarm, double bus_drop_rate_pct) {
    if (!report) return -1;
    memset(report, 0, sizeof(iems_commissioning_report_t));

    uint8_t score = 100;
    bool is_go = true;
    report->failure_reason[0] = '\0';

    // 1. Grid Voltage Check (342V ~ 440V, nominal 380V +- 10% / 400V)
    report->is_grid_voltage_ok = (u_grid_v >= 342.0 && u_grid_v <= 440.0);
    if (!report->is_grid_voltage_ok) {
        score = (uint8_t)(score >= 20 ? score - 20 : 0);
        is_go = false;
        snprintf(report->failure_reason, sizeof(report->failure_reason), "Grid Voltage Out-of-Bounds (%.1fV)", u_grid_v);
    }

    // 2. Grid Frequency Check (49.5Hz ~ 50.5Hz)
    report->is_grid_frequency_ok = (f_grid_hz >= 49.5 && f_grid_hz <= 50.5);
    if (!report->is_grid_frequency_ok) {
        score = (uint8_t)(score >= 20 ? score - 20 : 0);
        is_go = false;
        if (report->failure_reason[0] == '\0') {
            snprintf(report->failure_reason, sizeof(report->failure_reason), "Grid Frequency Unstable (%.2fHz)", f_grid_hz);
        }
    }

    // 3. BMS Insulation Check (>= 500 kOhm)
    report->is_bms_insulation_ok = (bms_r_iso_kohm >= 500.0);
    if (!report->is_bms_insulation_ok) {
        score = (uint8_t)(score >= 25 ? score - 25 : 0);
        is_go = false;
        if (report->failure_reason[0] == '\0') {
            snprintf(report->failure_reason, sizeof(report->failure_reason), "BMS Low Insulation Resistance (%.1fkOhm)", bms_r_iso_kohm);
        }
    }

    // 4. BMS Rack Voltage Matching (<= 10.0V)
    report->is_bms_voltage_matched = (bms_max_rack_v_diff <= 10.0);
    if (!report->is_bms_voltage_matched) {
        score = (uint8_t)(score >= 15 ? score - 15 : 0);
        is_go = false;
        if (report->failure_reason[0] == '\0') {
            snprintf(report->failure_reason, sizeof(report->failure_reason), "BMS Inter-Rack Voltage Mismatch (%.1fV > 10V)", bms_max_rack_v_diff);
        }
    }

    // 5. PCS Ready Check
    report->is_pcs_standby_ready = !pcs_fault;
    if (!report->is_pcs_standby_ready) {
        score = (uint8_t)(score >= 30 ? score - 30 : 0);
        is_go = false;
        if (report->failure_reason[0] == '\0') {
            strncpy(report->failure_reason, "PCS Hardware Fault Active", sizeof(report->failure_reason) - 1);
        }
    }

    // 6. Fire Loop Check
    report->is_fire_loop_normal = !fire_alarm;
    if (!report->is_fire_loop_normal) {
        score = 0; // Critical blocker
        is_go = false;
        strncpy(report->failure_reason, "Fire Safety Interlock Tripped", sizeof(report->failure_reason) - 1);
    }

    // 7. Bus Health (< 1% drop rate)
    report->is_comm_bus_healthy = (bus_drop_rate_pct < 1.0);
    if (!report->is_comm_bus_healthy) {
        score = (uint8_t)(score >= 10 ? score - 10 : 0);
    }

    report->total_score = score;
    report->is_go_for_grid_tie = is_go;

    return 0;
}
