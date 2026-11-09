#include "vsg_control_internal.h"

/* -------------------------------------------------------------------------- */
/* AEP-730 Computing-Power Integrated Dynamic Tracking APIs                  */
/* -------------------------------------------------------------------------- */

int vsg_controller_set_compute_tracking_config(vsg_controller_t *vsg, const vsg_compute_tracking_config_t *cfg) {
    if (!vsg || !cfg) return -1;
    vsg->config.compute_tracking = *cfg;
    if (vsg->config.compute_tracking.p_compute_min_w <= 0.0) vsg->config.compute_tracking.p_compute_min_w = 1000.0;
    if (vsg->config.compute_tracking.p_compute_max_w <= 0.0) vsg->config.compute_tracking.p_compute_max_w = 10000.0;
    if (vsg->config.compute_tracking.tracking_efficiency <= 0.0) vsg->config.compute_tracking.tracking_efficiency = 0.95;
    if (vsg->config.compute_tracking.k_feedforward_power <= 0.0) vsg->config.compute_tracking.k_feedforward_power = 1.0;
    if (vsg->config.compute_tracking.t_feedforward_deriv_s <= 0.0) vsg->config.compute_tracking.t_feedforward_deriv_s = 0.02;
    if (vsg->config.compute_tracking.carbon_factor_kg_kwh <= 0.0) vsg->config.compute_tracking.carbon_factor_kg_kwh = 0.5810;
    return 0;
}

int vsg_controller_step_compute_tracking(vsg_controller_t *vsg,
                                         double p_renewable_gen_w,
                                         double p_base_load_w,
                                         double p_ess_charge_avail_w,
                                         double p_compute_meas_w,
                                         double dt_s,
                                         double *out_p_compute_target_w,
                                         double *out_p_vsg_feedforward_w) {
    if (!vsg || dt_s <= 0.0) return -1;

    double p_min = vsg->config.compute_tracking.p_compute_min_w;
    double p_max = vsg->config.compute_tracking.p_compute_max_w;
    double eta = vsg->config.compute_tracking.tracking_efficiency;

    /* 1. Green Surplus Power Calculation */
    double p_green_avail = p_renewable_gen_w - p_base_load_w;
    double p_surplus = p_green_avail - p_ess_charge_avail_w;
    if (p_surplus < 0.0) p_surplus = 0.0;

    vsg->compute_status.p_green_surplus_w = p_surplus;

    /* 2. Target Compute Load Mapping & DVFS Frequency Scaling */
    double p_target = p_min;
    if (vsg->config.compute_tracking.enable_compute_tracking) {
        p_target = p_min + eta * p_surplus;
        if (p_target > p_max) p_target = p_max;
        if (p_target < p_min) p_target = p_min;
    }

    double freq_scale = 10.0;
    if (p_max > p_min) {
        freq_scale = 10.0 + 90.0 * ((p_target - p_min) / (p_max - p_min));
    }
    if (freq_scale > 100.0) freq_scale = 100.0;
    if (freq_scale < 10.0) freq_scale = 10.0;

    vsg->compute_status.p_compute_target_w = p_target;
    vsg->compute_status.dvfs_freq_scale_pct = freq_scale;

    /* 3. Transient Power Feedforward to VSG Mechanical Torque */
    double delta_p_compute = p_compute_meas_w - vsg->prev_p_compute_meas_w;
    double p_dot_compute = delta_p_compute / dt_s;
    vsg->prev_p_compute_meas_w = p_compute_meas_w;

    double p_ff = 0.0;
    if (vsg->config.compute_tracking.enable_compute_tracking) {
        p_ff = vsg->config.compute_tracking.k_feedforward_power * delta_p_compute +
               vsg->config.compute_tracking.t_feedforward_deriv_s * p_dot_compute;
    }
    vsg->compute_status.p_feedforward_vsg_w = p_ff;

    /* Injected directly into VSG accelerating power */
    vsg->p_mech_last += p_ff;

    /* 4. Green Energy Utilization & Carbon Accounting */
    double p_green_consumed = (p_green_avail > 0.0) ? 
                              ((p_compute_meas_w < p_green_avail) ? p_compute_meas_w : p_green_avail) : 0.0;
    double green_ratio = (p_compute_meas_w > 1.0) ? (p_green_consumed / p_compute_meas_w) * 100.0 : 0.0;
    if (green_ratio > 100.0) green_ratio = 100.0;
    vsg->compute_status.green_utilization_pct = green_ratio;

    double energy_kwh = (p_green_consumed * dt_s) / 3600000.0;
    vsg->compute_status.cumulative_green_kwh += energy_kwh;
    vsg->compute_status.cumulative_carbon_saved_kg += energy_kwh * vsg->config.compute_tracking.carbon_factor_kg_kwh;

    if (out_p_compute_target_w) *out_p_compute_target_w = p_target;
    if (out_p_vsg_feedforward_w) *out_p_vsg_feedforward_w = p_ff;

    vsg->step_count++;
    return 0;
}

int vsg_controller_get_compute_tracking_status(const vsg_controller_t *vsg, vsg_compute_tracking_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->compute_status;
    return 0;
}


