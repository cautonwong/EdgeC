/**
 * @file iems_pcs_controller.c
 * @brief Production Power Conversion System (PCS) Controller Implementation
 */

#include "iems_pcs_controller.h"

#include <math.h>
#include <string.h>

void iems_pcs_init(iems_pcs_controller_t* pcs, double max_power_kw) {
    if (!pcs) return;
    memset(pcs, 0, sizeof(iems_pcs_controller_t));

    double p_max = (max_power_kw > 0.0) ? max_power_kw : 250.0;
    pcs->max_charge_kw = p_max;
    pcs->max_discharge_kw = p_max;
    pcs->max_reactive_kvar = p_max * 0.6;  // 0.6 S_rated for VAR capability

    pcs->rated_voltage_v = 400.0;
    pcs->rated_frequency_hz = 50.00;

    // Standard VSG parameters
    pcs->vsg_inertia_j = 5.0;    // J
    pcs->vsg_damping_dp = 20.0;  // 20 kW per Hz droop
    pcs->vsg_droop_dq = 2.5;     // 2.5 kVAR per V droop

    pcs->mode = PCS_MODE_STANDBY;
    pcs->is_grid_connected = true;
}

int iems_pcs_set_mode(iems_pcs_controller_t* pcs, iems_pcs_mode_t target_mode) {
    if (!pcs) return -1;
    pcs->mode = target_mode;
    if (target_mode == PCS_MODE_ISLAND_VF) {
        pcs->is_grid_connected = false;
    } else if (target_mode == PCS_MODE_GRID_PQ || target_mode == PCS_MODE_VSG) {
        pcs->is_grid_connected = true;
    }
    return 0;
}

int iems_pcs_exec_pq_control(iems_pcs_controller_t* pcs, double target_p_kw, double target_q_kvar) {
    if (!pcs || pcs->mode != PCS_MODE_GRID_PQ) return -1;

    // Clamp Active Power
    if (target_p_kw > pcs->max_discharge_kw) target_p_kw = pcs->max_discharge_kw;
    if (target_p_kw < -pcs->max_charge_kw) target_p_kw = -pcs->max_charge_kw;

    // Clamp Reactive Power
    if (target_q_kvar > pcs->max_reactive_kvar) target_q_kvar = pcs->max_reactive_kvar;
    if (target_q_kvar < -pcs->max_reactive_kvar) target_q_kvar = -pcs->max_reactive_kvar;

    pcs->current_active_kw = target_p_kw;
    pcs->current_reactive_kvar = target_q_kvar;

    return 0;
}

int iems_pcs_exec_vsg_cycle(iems_pcs_controller_t* pcs, double v_meas, double f_meas, double df_dt,
                            double p_setpoint, double q_setpoint) {
    if (!pcs || pcs->mode != PCS_MODE_VSG) return -1;

    pcs->measured_voltage_v = v_meas;
    pcs->measured_freq_hz = f_meas;

    // 1. VSG Active Power - Frequency Response:
    // Delta_P = - Dp * (f_meas - f_nom) - J * (df/dt)
    double delta_f = f_meas - pcs->rated_frequency_hz;
    double delta_p_droop = -pcs->vsg_damping_dp * delta_f;
    double delta_p_inertia = -pcs->vsg_inertia_j * df_dt;
    double target_p = p_setpoint + delta_p_droop + delta_p_inertia;

    // 2. VSG Reactive Power - Voltage Response:
    // Delta_Q = - Dq * (V_meas - V_nom)
    double delta_v = v_meas - pcs->rated_voltage_v;
    double delta_q_droop = -pcs->vsg_droop_dq * delta_v;
    double target_q = q_setpoint + delta_q_droop;

    // Clamp to hardware boundaries
    if (target_p > pcs->max_discharge_kw) target_p = pcs->max_discharge_kw;
    if (target_p < -pcs->max_charge_kw) target_p = -pcs->max_charge_kw;
    if (target_q > pcs->max_reactive_kvar) target_q = pcs->max_reactive_kvar;
    if (target_q < -pcs->max_reactive_kvar) target_q = -pcs->max_reactive_kvar;

    pcs->current_active_kw = target_p;
    pcs->current_reactive_kvar = target_q;

    return 0;
}

int iems_pcs_exec_black_start(iems_pcs_controller_t* pcs, double target_v, double target_f) {
    if (!pcs || pcs->mode != PCS_MODE_ISLAND_VF) return -1;

    pcs->vsg_voltage_cmd_v = (target_v > 0.0) ? target_v : pcs->rated_voltage_v;
    pcs->vsg_freq_cmd_hz = (target_f > 0.0) ? target_f : pcs->rated_frequency_hz;
    pcs->is_black_start_ready = true;
    pcs->is_grid_connected = false;

    return 0;
}
