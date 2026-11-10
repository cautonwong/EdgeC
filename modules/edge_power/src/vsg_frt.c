#include "vsg_control_internal.h"

/* -------------------------------------------------------------------------- */
/* AEP-728 Fault Ride-Through (HVRT/LVRT) & Dynamic Reactive Support APIs     */
/* -------------------------------------------------------------------------- */

int vsg_controller_set_frt_config(vsg_controller_t *vsg, const vsg_frt_config_t *cfg) {
    if (!vsg || !cfg) return -1;
    vsg->config.frt = *cfg;
    if (vsg->config.frt.u_lvrt_thresh_pu <= 0.0) vsg->config.frt.u_lvrt_thresh_pu = 0.90;
    if (vsg->config.frt.u_hvrt_thresh_pu <= 0.0) vsg->config.frt.u_hvrt_thresh_pu = 1.10;
    if (vsg->config.frt.k_lvrt_reactive_support <= 0.0) vsg->config.frt.k_lvrt_reactive_support = 2.0;
    if (vsg->config.frt.k_hvrt_reactive_support <= 0.0) vsg->config.frt.k_hvrt_reactive_support = 2.0;
    if (vsg->config.frt.i_nominal_a <= 0.0) vsg->config.frt.i_nominal_a = 150.0;
    if (vsg->config.frt.i_max_limit_a <= 0.0) vsg->config.frt.i_max_limit_a = 1.2 * vsg->config.frt.i_nominal_a;
    if (vsg->config.frt.k_trans_virtual_z <= 0.0) vsg->config.frt.k_trans_virtual_z = 0.5;
    if (vsg->config.frt.recovery_ramp_rate_pu_s <= 0.0) vsg->config.frt.recovery_ramp_rate_pu_s = 0.5;
    return 0;
}

int vsg_controller_step_frt(vsg_controller_t *vsg, double grid_volt_v,
                            double id_meas, double iq_meas, double dt_s,
                            double *out_iq_support, double *out_p_derate,
                            double *out_v_trans_drop) {
    if (!vsg || dt_s <= 0.0) return -1;

    double v_nom = vsg->config.v_nominal;
    double u_pu = (v_nom > 0.0) ? (grid_volt_v / v_nom) : 1.0;
    double i_nom = vsg->config.frt.i_nominal_a;
    double i_max = vsg->config.frt.i_max_limit_a;

    double i_q_support = 0.0;
    double i_d_limit = i_max;
    double p_derate = 1.0;
    vsg_frt_state_t state = VSG_FRT_STATE_NORMAL;

    /* 1. Fault Detection and State Transition */
    if (u_pu < vsg->config.frt.u_lvrt_thresh_pu) {
        /* LVRT Low Voltage Ride-Through */
        state = VSG_FRT_STATE_LVRT;
        vsg->frt_recovery_ramp = 0.0;

        /* Reactive current support: Iq_support = K_lvrt * (0.9 - U_pu) * I_N */
        double delta_u = vsg->config.frt.u_lvrt_thresh_pu - u_pu;
        i_q_support = vsg->config.frt.k_lvrt_reactive_support * delta_u * i_nom;

        /* Deep sag ceiling */
        if (u_pu < 0.20) {
            i_q_support = 1.05 * i_nom;
        }
        if (i_q_support > i_max) i_q_support = i_max;

        /* Reactive priority: Id_limit = sqrt(I_max^2 - Iq^2) */
        double iq_sq = i_q_support * i_q_support;
        double id_avail_sq = (i_max * i_max > iq_sq) ? (i_max * i_max - iq_sq) : 0.0;
        i_d_limit = sqrt(id_avail_sq);

        p_derate = (i_nom > 0.0) ? (i_d_limit / i_nom) * u_pu : 0.0;
        if (p_derate > 1.0) p_derate = 1.0;
        if (p_derate < 0.0) p_derate = 0.0;
    } else if (u_pu > vsg->config.frt.u_hvrt_thresh_pu) {
        /* HVRT High Voltage Ride-Through */
        state = VSG_FRT_STATE_HVRT;
        vsg->frt_recovery_ramp = 0.0;

        /* Reactive current absorption: Iq_support = -K_hvrt * (U_pu - 1.1) * I_N */
        double delta_u = u_pu - vsg->config.frt.u_hvrt_thresh_pu;
        i_q_support = -vsg->config.frt.k_hvrt_reactive_support * delta_u * i_nom;
        if (fabs(i_q_support) > i_max) i_q_support = -i_max;

        double iq_sq = i_q_support * i_q_support;
        double id_avail_sq = (i_max * i_max > iq_sq) ? (i_max * i_max - iq_sq) : 0.0;
        i_d_limit = sqrt(id_avail_sq);

        p_derate = (i_nom > 0.0) ? (i_d_limit / i_nom) : 0.0;
        if (p_derate > 1.0) p_derate = 1.0;
        if (p_derate < 0.0) p_derate = 0.0;
    } else {
        /* Grid voltage within normal band [0.9, 1.1] */
        if (vsg->frt_status.state == VSG_FRT_STATE_LVRT ||
            vsg->frt_status.state == VSG_FRT_STATE_HVRT ||
            vsg->frt_status.state == VSG_FRT_STATE_RECOVERY) {
            state = VSG_FRT_STATE_RECOVERY;
            vsg->frt_recovery_ramp += vsg->config.frt.recovery_ramp_rate_pu_s * dt_s;
            if (vsg->frt_recovery_ramp >= 1.0) {
                vsg->frt_recovery_ramp = 1.0;
                state = VSG_FRT_STATE_NORMAL;
            }
            i_q_support = 0.0;
            i_d_limit = i_max;
            p_derate = vsg->frt_recovery_ramp;
        } else {
            state = VSG_FRT_STATE_NORMAL;
            vsg->frt_recovery_ramp = 1.0;
            i_q_support = 0.0;
            i_d_limit = i_max;
            p_derate = 1.0;
        }
    }

    /* 2. Transient Virtual Impedance Current Clamping */
    double i_mag = sqrt(id_meas * id_meas + iq_meas * iq_meas);
    double v_trans_drop = 0.0;
    bool clamped = false;

    if (i_mag > i_max) {
        double delta_i = i_mag - i_max;
        double r_trans = vsg->config.frt.k_trans_virtual_z * delta_i;
        v_trans_drop = r_trans * i_mag;
        clamped = true;
    }

    /* 3. Update Status */
    vsg->frt_status.state = state;
    vsg->frt_status.u_grid_pu = u_pu;
    vsg->frt_status.i_q_support_a = i_q_support;
    vsg->frt_status.i_d_limit_a = i_d_limit;
    vsg->frt_status.v_trans_drop_v = v_trans_drop;
    vsg->frt_status.recovery_progress_pct = vsg->frt_recovery_ramp * 100.0;
    vsg->frt_status.is_current_clamped = clamped;

    if (out_iq_support) *out_iq_support = i_q_support;
    if (out_p_derate) *out_p_derate = p_derate;
    if (out_v_trans_drop) *out_v_trans_drop = v_trans_drop;

    vsg->step_count++;
    return 0;
}

vsg_frt_state_t vsg_controller_get_frt_state(const vsg_controller_t *vsg) {
    return vsg ? vsg->frt_status.state : VSG_FRT_STATE_NORMAL;
}

int vsg_controller_get_frt_status(const vsg_controller_t *vsg, vsg_frt_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->frt_status;
    return 0;
}

