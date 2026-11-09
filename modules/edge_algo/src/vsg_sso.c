#include "vsg_control_internal.h"

/* -------------------------------------------------------------------------- */
/* AEP-727 Subsynchronous Oscillation (SSO) Detection & Active Damping APIs   */
/* -------------------------------------------------------------------------- */

int vsg_controller_set_sso_config(vsg_controller_t *vsg, const vsg_sso_config_t *cfg) {
    if (!vsg || !cfg) return -1;
    vsg->config.sso = *cfg;
    if (vsg->config.sso.sso_freq_min_hz <= 0.0) vsg->config.sso.sso_freq_min_hz = 5.0;
    if (vsg->config.sso.sso_freq_max_hz <= 0.0) vsg->config.sso.sso_freq_max_hz = 45.0;
    if (vsg->config.sso.sso_alarm_thresh_a <= 0.0) vsg->config.sso.sso_alarm_thresh_a = 5.0;
    if (vsg->config.sso.notch_damping_ratio_zeta <= 0.0) vsg->config.sso.notch_damping_ratio_zeta = 0.2;
    if (vsg->config.sso.active_damp_gain <= 0.0) vsg->config.sso.active_damp_gain = 3.0;
    return 0;
}

int vsg_controller_step_sso_damping(vsg_controller_t *vsg, double i_raw_a, double dt_s,
                                    double *out_filtered_i_a, double *out_damping_v) {
    if (!vsg || dt_s <= 0.0) return -1;

    if (!vsg->config.sso.enable_sso_damping) {
        if (out_filtered_i_a) *out_filtered_i_a = i_raw_a;
        if (out_damping_v) *out_damping_v = 0.0;
        return 0;
    }

    /* 0. Fundamental Component Rejection Pre-Filter (Notch at nominal frequency f_nominal) */
    double w_fund = 2.0 * M_PI * vsg->config.f_nominal;
    double w_t2_f = tan(w_fund * dt_s * 0.5);
    double w_t2_f_sq = w_t2_f * w_t2_f;
    double zeta_f = 0.2;
    double denom_f = 1.0 + 2.0 * zeta_f * w_t2_f + w_t2_f_sq;
    double b0_f = (1.0 + w_t2_f_sq) / denom_f;
    double b1_f = (2.0 * (w_t2_f_sq - 1.0)) / denom_f;
    double b2_f = b0_f;
    double a1_f = b1_f;
    double a2_f = (1.0 - 2.0 * zeta_f * w_t2_f + w_t2_f_sq) / denom_f;

    double i_sub = b0_f * i_raw_a + vsg->fund_notch_w1;
    vsg->fund_notch_w1 = b1_f * i_raw_a - a1_f * i_sub + vsg->fund_notch_w2;
    vsg->fund_notch_w2 = b2_f * i_raw_a - a2_f * i_sub;

    /* 1. SOGI-based Online Frequency & Amplitude Estimator on sub-harmonic signal */
    double omega_min = 2.0 * M_PI * vsg->config.sso.sso_freq_min_hz;
    double omega_max = 2.0 * M_PI * vsg->config.sso.sso_freq_max_hz;
    double omega_c = vsg->sso_omega_est;
    if (omega_c < omega_min) omega_c = omega_min;
    if (omega_c > omega_max) omega_c = omega_max;

    double k_sogi = 1.414;
    double err_sogi = i_sub - vsg->sso_sogi_x;
    double d_x = (k_sogi * omega_c * err_sogi - omega_c * vsg->sso_sogi_y) * dt_s;
    double d_y = (omega_c * vsg->sso_sogi_x) * dt_s;

    vsg->sso_sogi_x += d_x;
    vsg->sso_sogi_y += d_y;

    /* Frequency Adaptation Law (Frequency Locked Loop / Phase Estimator) */
    double gamma_fll = 80.0;
    double err_freq = -err_sogi * vsg->sso_sogi_y;
    vsg->sso_omega_est += gamma_fll * err_freq * dt_s;
    if (vsg->sso_omega_est < omega_min) vsg->sso_omega_est = omega_min;
    if (vsg->sso_omega_est > omega_max) vsg->sso_omega_est = omega_max;

    double f_sso = vsg->sso_omega_est / (2.0 * M_PI);
    double amp_sso = sqrt(vsg->sso_sogi_x * vsg->sso_sogi_x + vsg->sso_sogi_y * vsg->sso_sogi_y);

    vsg->sso_status.sso_freq_hz = f_sso;
    vsg->sso_status.sso_amplitude_a = amp_sso;
    vsg->sso_status.sso_detected = (amp_sso >= vsg->config.sso.sso_alarm_thresh_a);

    /* 2. Adaptive Second-Order Direct Form II Transposed Notch Filter */
    double zeta = vsg->config.sso.notch_damping_ratio_zeta;
    double w_t2 = tan(vsg->sso_omega_est * dt_s * 0.5);
    double w_t2_sq = w_t2 * w_t2;
    double denom = 1.0 + 2.0 * zeta * w_t2 + w_t2_sq;

    double b0 = (1.0 + w_t2_sq) / denom;
    double b1 = (2.0 * (w_t2_sq - 1.0)) / denom;
    double b2 = b0;
    double a1 = b1;
    double a2 = (1.0 - 2.0 * zeta * w_t2 + w_t2_sq) / denom;

    double i_filtered = b0 * i_raw_a + vsg->notch_w1;
    vsg->notch_w1 = b1 * i_raw_a - a1 * i_filtered + vsg->notch_w2;
    vsg->notch_w2 = b2 * i_raw_a - a2 * i_filtered;

    /* 3. Extracted SSO Current & Anti-Phase Active Damping Voltage Injection */
    double i_sso_extracted = i_raw_a - i_filtered;
    double v_damping = -vsg->config.sso.active_damp_gain * i_sso_extracted;
    vsg->sso_status.damping_out_v = v_damping;

    /* 4. Energy Reduction Tracking */
    double inst_energy = i_sso_extracted * i_sso_extracted;
    vsg->sso_energy_est = 0.95 * vsg->sso_energy_est + 0.05 * inst_energy;
    if (vsg->sso_baseline_energy <= 0.0 && amp_sso > vsg->config.sso.sso_alarm_thresh_a) {
        vsg->sso_baseline_energy = inst_energy;
    }
    if (vsg->sso_baseline_energy > 0.01) {
        double red = (1.0 - (vsg->sso_energy_est / vsg->sso_baseline_energy)) * 100.0;
        if (red < 0.0) red = 0.0;
        if (red > 100.0) red = 100.0;
        vsg->sso_status.energy_reduction_ratio_pct = red;
    } else {
        vsg->sso_status.energy_reduction_ratio_pct = 0.0;
    }

    if (out_filtered_i_a) *out_filtered_i_a = i_filtered;
    if (out_damping_v) *out_damping_v = v_damping;

    vsg->step_count++;
    return 0;
}

int vsg_controller_get_sso_status(const vsg_controller_t *vsg, vsg_sso_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->sso_status;
    return 0;
}

