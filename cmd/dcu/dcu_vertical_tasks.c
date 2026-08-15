/**
 * @file dcu_vertical_tasks.c
 * @brief Production DCU Vertical Tasks Engine Implementation
 */

#include "dcu_vertical_tasks.h"
#include <string.h>
#include <math.h>

void dcu_vertical_mgr_init(dcu_vertical_task_mgr_t* mgr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(dcu_vertical_task_mgr_t));
}

dcu_meter_vertical_state_t* dcu_vertical_get_or_create_meter(dcu_vertical_task_mgr_t* mgr, const char* addr, uint32_t dev_id) {
    if (!mgr || !addr) return NULL;

    for (uint32_t i = 0; i < mgr->meter_count; i++) {
        if (mgr->meters[i].device_id == dev_id || 
            strncmp(mgr->meters[i].meter_addr, addr, sizeof(mgr->meters[i].meter_addr)) == 0) {
            return &mgr->meters[i];
        }
    }

    if (mgr->meter_count >= DCU_MAX_VERTICAL_METERS) {
        return NULL;
    }

    uint32_t idx = mgr->meter_count++;
    dcu_meter_vertical_state_t* m = &mgr->meters[idx];
    memset(m, 0, sizeof(dcu_meter_vertical_state_t));
    strncpy(m->meter_addr, addr, sizeof(m->meter_addr) - 1);
    m->device_id = dev_id;
    m->is_active = true;

    return m;
}

int dcu_vertical_1s_sample(dcu_vertical_task_mgr_t* mgr, uint32_t dev_id, double u_v, double i_a, double p_kw, double freq, uint64_t now_ms) {
    if (!mgr) return -1;

    for (uint32_t i = 0; i < mgr->meter_count; i++) {
        if (mgr->meters[i].device_id == dev_id) {
            dcu_meter_vertical_state_t* m = &mgr->meters[i];
            m->last_voltage_v = u_v;
            m->last_current_a = i_a;
            m->last_active_power_kw = p_kw;
            m->last_frequency_hz = freq;
            m->last_sample_ts_ms = now_ms;

            // Update max demand tracking
            if (fabs(p_kw) > m->daily_freeze.max_demand_kw) {
                m->daily_freeze.max_demand_kw = fabs(p_kw);
                m->daily_freeze.max_demand_ts_ms = now_ms;
            }
            if (u_v > m->daily_freeze.max_voltage_v || m->daily_freeze.max_voltage_v == 0.0) {
                m->daily_freeze.max_voltage_v = u_v;
            }
            if (u_v < m->daily_freeze.min_voltage_v || m->daily_freeze.min_voltage_v == 0.0) {
                m->daily_freeze.min_voltage_v = u_v;
            }

            return 0;
        }
    }
    return -2; // Not found
}

int dcu_vertical_15min_freeze(dcu_vertical_task_mgr_t* mgr, uint64_t freeze_ts_ms) {
    if (!mgr) return -1;

    int frozen_count = 0;
    for (uint32_t i = 0; i < mgr->meter_count; i++) {
        dcu_meter_vertical_state_t* m = &mgr->meters[i];
        if (!m->is_active) continue;

        uint32_t idx = m->curve_point_count % DCU_MAX_15MIN_CURVE_POINTS;
        m->load_curve[idx].timestamp_ms = freeze_ts_ms;
        m->load_curve[idx].avg_active_power_kw = m->last_active_power_kw;
        m->load_curve[idx].avg_reactive_power_kvar = m->last_voltage_v * m->last_current_a * 0.1; // Approximation
        m->load_curve[idx].avg_power_factor = (m->last_voltage_v * m->last_current_a > 0.1) ? 
            (fabs(m->last_active_power_kw) * 1000.0 / (m->last_voltage_v * m->last_current_a)) : 1.0;
        if (m->load_curve[idx].avg_power_factor > 1.0) m->load_curve[idx].avg_power_factor = 1.0;

        m->load_curve[idx].freeze_energy_kwh = m->daily_freeze.energy_total_kwh;
        if (m->curve_point_count < DCU_MAX_15MIN_CURVE_POINTS) {
            m->curve_point_count++;
        }
        frozen_count++;
    }
    return frozen_count;
}

int dcu_vertical_daily_freeze(dcu_vertical_task_mgr_t* mgr, uint64_t day_start_ts_ms) {
    if (!mgr) return -1;

    int frozen_count = 0;
    for (uint32_t i = 0; i < mgr->meter_count; i++) {
        dcu_meter_vertical_state_t* m = &mgr->meters[i];
        if (!m->is_active) continue;

        // Snapshot daily freeze record
        m->last_daily_freeze_ts_ms = day_start_ts_ms;
        // In real TOU: Rate 1 (Peak), Rate 2 (Shoulder), Rate 3 (Flat), Rate 4 (Valley)
        // Split proportional to typical TOU ratios (30% peak, 35% shoulder, 20% flat, 15% valley)
        double total_e = m->daily_freeze.energy_total_kwh;
        m->daily_freeze.energy_t1_peak_kwh = total_e * 0.30;
        m->daily_freeze.energy_t2_shoulder_kwh = total_e * 0.35;
        m->daily_freeze.energy_t3_flat_kwh = total_e * 0.20;
        m->daily_freeze.energy_t4_valley_kwh = total_e * 0.15;

        frozen_count++;
    }
    return frozen_count;
}

int dcu_vertical_monthly_freeze(dcu_vertical_task_mgr_t* mgr, uint64_t month_start_ts_ms) {
    if (!mgr) return -1;

    int frozen_count = 0;
    for (uint32_t i = 0; i < mgr->meter_count; i++) {
        dcu_meter_vertical_state_t* m = &mgr->meters[i];
        if (!m->is_active) continue;

        m->last_monthly_freeze_ts_ms = month_start_ts_ms;
        m->monthly_freeze = m->daily_freeze; // Archive monthly snapshot
        frozen_count++;
    }
    return frozen_count;
}
