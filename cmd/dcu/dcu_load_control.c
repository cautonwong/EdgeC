/**
 * @file dcu_load_control.c
 * @brief Production DCU Transformer Overload Protection & Demand Response Load Shaving Engine Implementation
 */

#include "dcu_load_control.h"
#include <string.h>
#include <math.h>

void dcu_load_control_init(dcu_load_controller_t* ctrl, double rated_kva) {
    if (!ctrl) return;
    memset(ctrl, 0, sizeof(dcu_load_controller_t));
    ctrl->rated_capacity_kva = (rated_kva > 0.0) ? rated_kva : 630.0; // Default 630 kVA
    ctrl->state = LOAD_STATE_NORMAL;
}

dcu_load_state_t dcu_load_control_evaluate(dcu_load_controller_t* ctrl, double u_line,
                                           double i_line, double power_factor,
                                           uint32_t delta_time_s) {
    if (!ctrl || ctrl->rated_capacity_kva <= 0.0) return LOAD_STATE_NORMAL;

    // S = sqrt(3) * U_line * I_line / 1000.0 (kVA)
    double s_kva = (sqrt(3.0) * u_line * i_line) / 1000.0;
    ctrl->current_apparent_kva = s_kva;

    // P = S * cos(phi)
    ctrl->active_power_kw = s_kva * fabs(power_factor);
    // Q = sqrt(S^2 - P^2)
    if (s_kva >= ctrl->active_power_kw) {
        ctrl->reactive_power_kvar = sqrt(s_kva * s_kva - ctrl->active_power_kw * ctrl->active_power_kw);
    } else {
        ctrl->reactive_power_kvar = 0.0;
    }

    // Load ratio = (S / S_rated) * 100%
    ctrl->load_ratio_pct = (s_kva / ctrl->rated_capacity_kva) * 100.0;

    if (ctrl->load_ratio_pct > 110.0) {
        ctrl->overload_duration_s += delta_time_s;
        ctrl->state = LOAD_STATE_OVERLOAD;
        ctrl->is_curtailment_active = true;
        // Target reduction = Current Apparent - 95% Rated
        ctrl->target_reduction_kw = (s_kva - 0.95 * ctrl->rated_capacity_kva) * fabs(power_factor);
    } else if (ctrl->load_ratio_pct > 95.0) {
        ctrl->overload_duration_s = 0;
        ctrl->state = LOAD_STATE_HIGH;
        ctrl->is_curtailment_active = true;
        ctrl->target_reduction_kw = (s_kva - 0.85 * ctrl->rated_capacity_kva) * fabs(power_factor);
    } else if (ctrl->load_ratio_pct > 85.0) {
        ctrl->overload_duration_s = 0;
        ctrl->state = LOAD_STATE_WARNING;
        ctrl->is_curtailment_active = true;
        ctrl->target_reduction_kw = (s_kva - 0.80 * ctrl->rated_capacity_kva) * fabs(power_factor);
    } else {
        ctrl->overload_duration_s = 0;
        ctrl->state = LOAD_STATE_NORMAL;
        ctrl->is_curtailment_active = false;
        ctrl->target_reduction_kw = 0.0;
    }

    return ctrl->state;
}
