/**
 * @file dcu_topology_lineloss.c
 * @brief Production DCU Substation Topology, 3-Phase Unbalance & Dynamic Line Loss Analytics Engine
 * Implementation
 */

#include "dcu_topology_lineloss.h"

#include <math.h>
#include <string.h>

void dcu_topology_init(dcu_topology_engine_t* engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(dcu_topology_engine_t));
}

int dcu_topology_update_meter(dcu_topology_engine_t* engine, const char* addr, uint32_t dev_id,
                              uint32_t branch_id, dcu_phase_t phase, double energy_kwh,
                              double u_val, double i_val, double pf, int8_t snr) {
    if (!engine || !addr) return -1;

    // Check if meter already exists
    for (uint32_t i = 0; i < engine->meter_count; i++) {
        if (strncmp(engine->meters[i].meter_addr, addr, sizeof(engine->meters[i].meter_addr)) ==
            0) {
            engine->meters[i].device_id = dev_id;
            engine->meters[i].branch_id = branch_id;
            engine->meters[i].phase = phase;
            engine->meters[i].energy_kwh = energy_kwh;
            engine->meters[i].voltage_v = u_val;
            engine->meters[i].current_a = i_val;
            engine->meters[i].power_factor = pf;
            engine->meters[i].snr_db = snr;
            engine->meters[i].is_online = true;
            return 0;
        }
    }

    // Insert new meter node
    if (engine->meter_count >= DCU_MAX_TOPOLOGY_METERS) {
        return -2;  // Capacity exceeded
    }

    uint32_t idx = engine->meter_count++;
    strncpy(engine->meters[idx].meter_addr, addr, sizeof(engine->meters[idx].meter_addr) - 1);
    engine->meters[idx].device_id = dev_id;
    engine->meters[idx].branch_id = branch_id;
    engine->meters[idx].phase = phase;
    engine->meters[idx].energy_kwh = energy_kwh;
    engine->meters[idx].voltage_v = u_val;
    engine->meters[idx].current_a = i_val;
    engine->meters[idx].power_factor = pf;
    engine->meters[idx].snr_db = snr;
    engine->meters[idx].is_online = true;

    return 0;
}

int dcu_topology_compute_analytics(dcu_topology_engine_t* engine, double master_input_kwh,
                                   double master_ia, double master_ib, double master_ic,
                                   double master_ua, double master_ub, double master_uc,
                                   uint64_t now_ms) {
    if (!engine) return -1;

    dcu_substation_analytics_t* res = &engine->analytics;
    res->total_input_energy_kwh = master_input_kwh;
    res->last_calc_timestamp_ms = now_ms;

    // 1. Sum up all slave meters' active energy
    double sum_consumed = 0.0;
    uint32_t online_cnt = 0;
    uint32_t offline_cnt = 0;

    for (uint32_t i = 0; i < engine->meter_count; i++) {
        if (engine->meters[i].is_online) {
            sum_consumed += engine->meters[i].energy_kwh;
            online_cnt++;
        } else {
            offline_cnt++;
        }
    }

    res->total_consumed_kwh = sum_consumed;
    res->online_meter_count = online_cnt;
    res->offline_meter_count = offline_cnt;

    // 2. Line Loss Calculation
    res->line_loss_kwh = master_input_kwh - sum_consumed;
    if (master_input_kwh > 0.0001) {
        res->line_loss_rate_pct = (res->line_loss_kwh / master_input_kwh) * 100.0;
    } else {
        res->line_loss_rate_pct = 0.0;
    }

    // Classify Health Status
    if (res->line_loss_rate_pct < 0.0) {
        res->health = LINELOSS_HEALTH_NEGATIVE;  // Reverse energy / theft alarm
    } else if (res->line_loss_rate_pct <= 4.0) {
        res->health = LINELOSS_HEALTH_NORMAL;
    } else if (res->line_loss_rate_pct <= 8.0) {
        res->health = LINELOSS_HEALTH_ATTENTION;
    } else {
        res->health = LINELOSS_HEALTH_HIGH_LOSS;  // Abnormal high dissipation
    }

    // 3. 3-Phase Current Unbalance Calculation (GB/T 15543)
    res->phase_a_current_a = master_ia;
    res->phase_b_current_a = master_ib;
    res->phase_c_current_a = master_ic;

    double i_avg = (master_ia + master_ib + master_ic) / 3.0;
    if (i_avg > 0.001) {
        double diff_a = fabs(master_ia - i_avg);
        double diff_b = fabs(master_ib - i_avg);
        double diff_c = fabs(master_ic - i_avg);
        double max_diff = diff_a;
        if (diff_b > max_diff) max_diff = diff_b;
        if (diff_c > max_diff) max_diff = diff_c;
        res->current_unbalance_pct = (max_diff / i_avg) * 100.0;
    } else {
        res->current_unbalance_pct = 0.0;
    }

    // 4. 3-Phase Voltage Unbalance Calculation
    res->phase_a_voltage_v = master_ua;
    res->phase_b_voltage_v = master_ub;
    res->phase_c_voltage_v = master_uc;

    double u_avg = (master_ua + master_ub + master_uc) / 3.0;
    if (u_avg > 0.001) {
        double diff_ua = fabs(master_ua - u_avg);
        double diff_ub = fabs(master_ub - u_avg);
        double diff_uc = fabs(master_uc - u_avg);
        double max_u_diff = diff_ua;
        if (diff_ub > max_u_diff) max_u_diff = diff_ub;
        if (diff_uc > max_u_diff) max_u_diff = diff_uc;
        res->voltage_unbalance_pct = (max_u_diff / u_avg) * 100.0;
    } else {
        res->voltage_unbalance_pct = 0.0;
    }

    return 0;
}
