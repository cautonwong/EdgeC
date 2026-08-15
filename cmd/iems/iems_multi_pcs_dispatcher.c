/**
 * @file iems_multi_pcs_dispatcher.c
 * @brief Production Commercial Multi-PCS Dynamic Load Sharing & Ramp-Rate Limiting Implementation
 */

#include "iems_multi_pcs_dispatcher.h"
#include <string.h>
#include <math.h>

void iems_pcs_dispatcher_init(iems_multi_pcs_dispatcher_t* disp, double ramp_limit_kw_s) {
    if (!disp) return;
    memset(disp, 0, sizeof(iems_multi_pcs_dispatcher_t));
    disp->ramp_rate_limit_kw_per_sec = (ramp_limit_kw_s > 0.0) ? ramp_limit_kw_s : 50.0;
}

int iems_pcs_dispatcher_register_unit(iems_multi_pcs_dispatcher_t* disp, uint8_t pcs_id,
                                      const char* sn, double rated_kw) {
    if (!disp || pcs_id == 0 || pcs_id > IEMS_MAX_PCS_UNITS) return -1;
    uint8_t idx = pcs_id - 1;
    iems_pcs_unit_t* u = &disp->units[idx];
    u->pcs_id = pcs_id;
    if (sn) strncpy(u->pcs_sn, sn, sizeof(u->pcs_sn) - 1);
    u->rated_power_kw = (rated_kw > 0.0) ? rated_kw : 250.0;
    u->derating_factor = 1.0;
    u->is_online = true;
    u->is_fault = false;

    if (pcs_id > disp->unit_count) {
        disp->unit_count = pcs_id;
    }

    // Recompute total capacity
    disp->total_rated_capacity_kw = 0.0;
    for (uint8_t i = 0; i < disp->unit_count; i++) {
        disp->total_rated_capacity_kw += disp->units[i].rated_power_kw;
    }

    return 0;
}

int iems_pcs_dispatcher_update_status(iems_multi_pcs_dispatcher_t* disp, uint8_t pcs_id,
                                      bool is_online, bool is_fault, double cur_p, double temp_c) {
    if (!disp || pcs_id == 0 || pcs_id > IEMS_MAX_PCS_UNITS) return -1;
    uint8_t idx = pcs_id - 1;
    iems_pcs_unit_t* u = &disp->units[idx];
    u->is_online = is_online;
    u->is_fault = is_fault;
    u->current_power_kw = cur_p;
    u->temp_c = temp_c;

    // Thermal Derating Strategy:
    // If IGBT temp > 75C, linearly derate power from 75C (100%) to 85C (0%)
    if (temp_c > 85.0) {
        u->derating_factor = 0.0;
    } else if (temp_c > 75.0) {
        u->derating_factor = 1.0 - (temp_c - 75.0) / 10.0;
    } else {
        u->derating_factor = 1.0;
    }

    return 0;
}

int iems_pcs_dispatcher_dispatch(iems_multi_pcs_dispatcher_t* disp, double total_demand_kw, double dt_s) {
    if (!disp || disp->unit_count == 0) return -1;

    // 1. Ramp-Rate Limiting:
    // Restrict change rate: |dPower| <= ramp_rate * dt
    double max_delta_kw = disp->ramp_rate_limit_kw_per_sec * (dt_s > 0.0 ? dt_s : 0.1);
    double target_ramp_kw = total_demand_kw;
    double delta_demand = total_demand_kw - disp->last_total_dispatched_kw;

    if (delta_demand > max_delta_kw) {
        target_ramp_kw = disp->last_total_dispatched_kw + max_delta_kw;
    } else if (delta_demand < -max_delta_kw) {
        target_ramp_kw = disp->last_total_dispatched_kw - max_delta_kw;
    }
    disp->last_total_dispatched_kw = target_ramp_kw;

    // 2. Compute Total Available Effective Capacity
    double total_effective_capacity = 0.0;
    for (uint8_t i = 0; i < disp->unit_count; i++) {
        iems_pcs_unit_t* u = &disp->units[i];
        if (u->is_online && !u->is_fault && u->derating_factor > 0.0) {
            total_effective_capacity += u->rated_power_kw * u->derating_factor;
        }
    }

    // 3. Proportional Dispatch
    if (total_effective_capacity <= 0.001) {
        // No units available
        for (uint8_t i = 0; i < disp->unit_count; i++) {
            disp->units[i].target_power_kw = 0.0;
        }
        return -2;
    }

    for (uint8_t i = 0; i < disp->unit_count; i++) {
        iems_pcs_unit_t* u = &disp->units[i];
        if (u->is_online && !u->is_fault && u->derating_factor > 0.0) {
            double effective_cap = u->rated_power_kw * u->derating_factor;
            double share = effective_cap / total_effective_capacity;
            u->target_power_kw = target_ramp_kw * share;
        } else {
            u->target_power_kw = 0.0;
        }
    }

    return 0;
}
