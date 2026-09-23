/**
 * @file iems_bms_cluster_engine.c
 * @brief Production Commercial BMS Multi-Rack Cluster Aggregation & Thermal Runaway Safety Engine
 * Implementation
 */

#include "iems_bms_cluster_engine.h"

#include <math.h>
#include <string.h>

void iems_bms_cluster_init(iems_bms_cluster_engine_t* engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(iems_bms_cluster_engine_t));
}

int iems_bms_update_rack(iems_bms_cluster_engine_t* engine, uint8_t rack_id, const char* sn,
                         double u_v, double i_a, double soc, double soh, double v_max, double v_min,
                         double t_max, double t_min, double dt_rate, double r_iso) {
    if (!engine || rack_id == 0 || rack_id > IEMS_MAX_BMS_RACKS) return -1;

    uint8_t idx = rack_id - 1;
    iems_bms_rack_t* r = &engine->racks[idx];
    r->rack_id = rack_id;
    if (sn) strncpy(r->rack_sn, sn, sizeof(r->rack_sn) - 1);
    r->is_online = true;
    r->total_voltage_v = u_v;
    r->current_a = i_a;
    r->soc_pct = soc;
    r->soh_pct = soh;
    r->cell_v_max_v = v_max;
    r->cell_v_min_v = v_min;
    r->cell_t_max_c = t_max;
    r->cell_t_min_c = t_min;
    r->temp_rate_c_per_min = dt_rate;
    r->pos_insulation_kohm = r_iso;
    r->neg_insulation_kohm = r_iso;

    // 1. Cell unbalance check: Delta_V > 50mV (0.050 V) triggers balancing
    double cell_diff_v = v_max - v_min;
    r->is_balancing_active = (cell_diff_v > 0.050);

    // 2. Rack Alarm Level
    if (t_max > 55.0 || dt_rate > 1.0 || r_iso < 50.0) {
        r->alarm_level = BMS_ALARM_LEVEL_3;  // Thermal runaway / severe insulation fault
    } else if (t_max > 45.0 || cell_diff_v > 0.100) {
        r->alarm_level = BMS_ALARM_LEVEL_2;
    } else if (cell_diff_v > 0.050) {
        r->alarm_level = BMS_ALARM_LEVEL_1;
    } else {
        r->alarm_level = BMS_ALARM_NONE;
    }

    if (rack_id > engine->rack_count) {
        engine->rack_count = rack_id;
    }

    return 0;
}

int iems_bms_evaluate_cluster_safety(iems_bms_cluster_engine_t* engine) {
    if (!engine || engine->rack_count == 0) return -1;

    double sum_v = 0.0;
    double sum_i = 0.0;
    double sum_soc = 0.0;
    double sum_soh = 0.0;
    double min_rack_v = 99999.0;
    double max_rack_v = 0.0;

    engine->sys_max_cell_v = 0.0;
    engine->sys_min_cell_v = 99999.0;
    engine->sys_max_cell_t = -999.0;
    engine->sys_max_cell_temp_diff = 0.0;
    engine->active_rack_count = 0;
    engine->is_emergency_trip = false;

    for (uint8_t i = 0; i < engine->rack_count; i++) {
        iems_bms_rack_t* r = &engine->racks[i];
        if (!r->is_online) continue;

        engine->active_rack_count++;
        sum_v += r->total_voltage_v;
        sum_i += r->current_a;
        sum_soc += r->soc_pct;
        sum_soh += r->soh_pct;

        if (r->total_voltage_v > max_rack_v) max_rack_v = r->total_voltage_v;
        if (r->total_voltage_v < min_rack_v) min_rack_v = r->total_voltage_v;

        if (r->cell_v_max_v > engine->sys_max_cell_v) engine->sys_max_cell_v = r->cell_v_max_v;
        if (r->cell_v_min_v < engine->sys_min_cell_v) engine->sys_min_cell_v = r->cell_v_min_v;
        if (r->cell_t_max_c > engine->sys_max_cell_t) engine->sys_max_cell_t = r->cell_t_max_c;

        double temp_diff = r->cell_t_max_c - r->cell_t_min_c;
        if (temp_diff > engine->sys_max_cell_temp_diff) {
            engine->sys_max_cell_temp_diff = temp_diff;
        }

        // Check if any rack has Level 3 emergency
        if (r->alarm_level == BMS_ALARM_LEVEL_3) {
            engine->is_emergency_trip = true;
        }
    }

    if (engine->active_rack_count > 0) {
        engine->sys_total_voltage_v = sum_v / engine->active_rack_count;
        engine->sys_total_current_a = sum_i;
        engine->sys_total_power_kw =
            (engine->sys_total_voltage_v * engine->sys_total_current_a) / 1000.0;
        engine->sys_avg_soc_pct = sum_soc / engine->active_rack_count;
        engine->sys_avg_soh_pct = sum_soh / engine->active_rack_count;

        // Check circulating current risk: delta_V between parallel racks > 10.0 V
        engine->is_circulating_risk =
            (engine->active_rack_count > 1 && (max_rack_v - min_rack_v) > 10.0);
    }

    return 0;
}
