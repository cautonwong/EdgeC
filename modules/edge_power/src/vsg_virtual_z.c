#include "vsg_control_internal.h"

/* -------------------------------------------------------------------------- */
/* AEP-729 Virtual Stator Impedance (R/X) & Harmonic Damping APIs              */
/* -------------------------------------------------------------------------- */

int vsg_controller_set_virtual_z_config(vsg_controller_t *vsg, const vsg_virtual_z_config_t *cfg) {
    if (!vsg || !cfg) return -1;
    vsg->config.virtual_z = *cfg;
    if (vsg->config.virtual_z.r_virtual_ohm <= 0.0) vsg->config.virtual_z.r_virtual_ohm = 0.02;
    if (vsg->config.virtual_z.x_virtual_ohm <= 0.0) vsg->config.virtual_z.x_virtual_ohm = 0.15;
    if (vsg->config.virtual_z.target_rx_ratio <= 0.0) vsg->config.virtual_z.target_rx_ratio = 0.10;
    if (vsg->config.virtual_z.r_line_est_ohm <= 0.0) vsg->config.virtual_z.r_line_est_ohm = 0.10;
    if (vsg->config.virtual_z.x_line_est_ohm <= 0.0) vsg->config.virtual_z.x_line_est_ohm = 0.05;
    if (vsg->config.virtual_z.k_harmonic_damping_h5 <= 0.0) vsg->config.virtual_z.k_harmonic_damping_h5 = 2.0;
    if (vsg->config.virtual_z.k_harmonic_damping_h7 <= 0.0) vsg->config.virtual_z.k_harmonic_damping_h7 = 2.0;
    if (vsg->config.virtual_z.harmonic_bandwidth_rad_s <= 0.0) vsg->config.virtual_z.harmonic_bandwidth_rad_s = 15.0;

    vsg->vz_status.r_v_effective_ohm = vsg->config.virtual_z.r_virtual_ohm;
    vsg->vz_status.x_v_effective_ohm = vsg->config.virtual_z.x_virtual_ohm;
    return 0;
}

int vsg_controller_step_virtual_z(vsg_controller_t *vsg,
                                  double v_d_ref_in, double v_q_ref_in,
                                  double i_d_meas, double i_q_meas,
                                  double dt_s,
                                  double *out_v_d, double *out_v_q) {
    if (!vsg || dt_s <= 0.0) return -1;

    /* 1. Adaptive R/X Ratio Compensation */
    double r_v = vsg->config.virtual_z.r_virtual_ohm;
    double x_v = vsg->config.virtual_z.x_virtual_ohm;

    if (vsg->config.virtual_z.enable_adaptive_rx) {
        double r_line = (vsg->config.circ_suppress.enable_online_impedance_id && vsg->circ_status.r_line_est_ohm > 0.0) ? 
                        vsg->circ_status.r_line_est_ohm : vsg->config.virtual_z.r_line_est_ohm;
        double x_line = (vsg->config.circ_suppress.enable_online_impedance_id && vsg->circ_status.x_line_est_ohm > 0.0) ? 
                        vsg->circ_status.x_line_est_ohm : vsg->config.virtual_z.x_line_est_ohm;
        double gamma = vsg->config.virtual_z.target_rx_ratio; /* typ. 0.10 */

        /* Target: (r_line + r_v) / (x_line + x_v) = gamma => r_v = gamma*(x_line + x_v) - r_line */
        if (x_line + x_v > 0.0) {
            double r_v_target = gamma * (x_line + x_v) - r_line;
            if (r_v_target < 0.0) {
                /* Highly resistive line: boost X_v so (r_line) / (x_line + X_v) = gamma */
                x_v = (r_line / gamma) - x_line;
                r_v = 0.0;
            } else {
                r_v = r_v_target;
            }
        }
    }

    vsg->vz_status.r_v_effective_ohm = r_v;
    vsg->vz_status.x_v_effective_ohm = x_v;

    /* 2. Fundamental Virtual Stator Impedance Voltage Drop */
    double v_d_drop = 0.0;
    double v_q_drop = 0.0;

    if (vsg->config.virtual_z.enable_virtual_z) {
        v_d_drop = r_v * i_d_meas - x_v * i_q_meas;
        v_q_drop = r_v * i_q_meas + x_v * i_d_meas;
    }

    vsg->vz_status.v_d_drop_fund_v = v_d_drop;
    vsg->vz_status.v_q_drop_fund_v = v_q_drop;

    /* 3. 5th/7th Harmonic Extraction & Active Damping Voltage Injection */
    /* In dq synchronous frame, 5th (-5w0-w0 = -6w0) and 7th (+7w0-w0 = +6w0) appear as 6w0 = 300Hz */
    double v_d_damp = 0.0;
    double v_q_damp = 0.0;
    double i_h5_rms = 0.0;
    double i_h7_rms = 0.0;
    double i_harm_rms = 0.0;

    double omega_h = 6.0 * vsg->omega_0; /* 6 * 2*pi*50 = 600*pi rad/s */
    double omega_cut = vsg->config.virtual_z.harmonic_bandwidth_rad_s;
    if (omega_cut <= 0.0) omega_cut = 20.0;
    double k_h = (vsg->config.virtual_z.k_harmonic_damping_h5 + vsg->config.virtual_z.k_harmonic_damping_h7) * 0.5;

    /* Discrete Tustin BPF Resonator Direct Form II (with frequency pre-warping) */
    double inv_T = 1.0 / dt_s;
    double inv_T2 = 4.0 * inv_T * inv_T;
    double omega_h_warped = (2.0 * inv_T) * tan(0.5 * omega_h * dt_s);
    double omega_h2 = omega_h_warped * omega_h_warped;
    double term_c = 4.0 * omega_cut * inv_T;
    double d0 = inv_T2 + term_c + omega_h2;

    double b0 = term_c / d0;
    double a1 = 2.0 * (omega_h2 - inv_T2) / d0;
    double a2 = (inv_T2 - term_c + omega_h2) / d0;

    /* D-axis BPF */
    double w_d = i_d_meas - a1 * vsg->h_sogi_x_d - a2 * vsg->h_sogi_y_d;
    double i_d_harm = b0 * (w_d - vsg->h_sogi_y_d);
    vsg->h_sogi_y_d = vsg->h_sogi_x_d;
    vsg->h_sogi_x_d = w_d;

    /* Q-axis BPF */
    double w_q = i_q_meas - a1 * vsg->h_sogi_x_q - a2 * vsg->h_sogi_y_q;
    double i_q_harm = b0 * (w_q - vsg->h_sogi_y_q);
    vsg->h_sogi_y_q = vsg->h_sogi_x_q;
    vsg->h_sogi_x_q = w_q;

    if (vsg->config.virtual_z.enable_harmonic_damping) {
        v_d_damp = k_h * i_d_harm;
        v_q_damp = k_h * i_q_harm;
    }

    vsg->vz_status.v_d_damp_harm_v = v_d_damp;
    vsg->vz_status.v_q_damp_harm_v = v_q_damp;

    /* Calculate Harmonic RMS and THD */
    double harm_amp = sqrt(i_d_harm * i_d_harm + i_q_harm * i_q_harm);
    i_harm_rms = harm_amp / sqrt(2.0);
    i_h5_rms = 0.6 * i_harm_rms; /* Typical decomposition share */
    i_h7_rms = 0.4 * i_harm_rms;

    vsg->vz_status.i_h5_rms_a = i_h5_rms;
    vsg->vz_status.i_h7_rms_a = i_h7_rms;

    /* Fundamental current magnitude (DC component in dq) */
    double i_d_fund = i_d_meas - i_d_harm;
    double i_q_fund = i_q_meas - i_q_harm;
    double fund_amp = sqrt(i_d_fund * i_d_fund + i_q_fund * i_q_fund);
    double i_fund_rms = (fund_amp > 1.0) ? (fund_amp / sqrt(2.0)) : 100.0;

    double thd = (i_fund_rms > 0.0) ? (i_harm_rms / i_fund_rms) * 100.0 : 0.0;
    vsg->vz_status.thd_current_pct = thd;

    /* THD attenuation tracking */
    if (vsg->thd_baseline_rms <= 0.0 && harm_amp > 1.0) {
        vsg->thd_baseline_rms = harm_amp;
    }
    if (vsg->thd_baseline_rms > 0.1) {
        double red = (1.0 - (harm_amp / vsg->thd_baseline_rms)) * 100.0;
        if (red < 0.0) red = 0.0;
        if (red > 100.0) red = 100.0;
        vsg->vz_status.thd_reduction_pct = red;
    } else {
        vsg->vz_status.thd_reduction_pct = 0.0;
    }

    /* 4. Output Reference Voltage Shaping */
    double v_d_out = v_d_ref_in - v_d_drop - v_d_damp;
    double v_q_out = v_q_ref_in - v_q_drop - v_q_damp;

    vsg->vz_status.v_d_out_v = v_d_out;
    vsg->vz_status.v_q_out_v = v_q_out;

    if (out_v_d) *out_v_d = v_d_out;
    if (out_v_q) *out_v_q = v_q_out;

    vsg->step_count++;
    return 0;
}

int vsg_controller_get_virtual_z_status(const vsg_controller_t *vsg, vsg_virtual_z_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->vz_status;
    return 0;
}

