/**
 * @file vsg_control.c
 * @brief Virtual Synchronous Generator (VSG) Implementation (AEP-723 ~ AEP-730 / AEP-752)
 */

#include "vsg_control.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct vsg_controller {
    vsg_config_t config;
    vsg_ports_t  ports;
    
    /* Dynamic State Variables */
    double omega;         /**< Virtual rotor angular velocity (rad/s) */
    double theta;         /**< Virtual rotor phase angle (rad, [0, 2*PI)) */
    double frequency_hz;  /**< Output frequency (Hz) */
    double voltage_v;     /**< Output voltage amplitude (V) */
    double omega_0;       /**< Nominal angular velocity = 2*PI*f_nominal (rad/s) */

    /* Dynamic Power Telemetry & Intermediate Quantities */
    double p_mech_last;   /**< Last calculated mechanical power (W) */
    double p_damping_last;/**< Last calculated damping power (W) */
    double p_net_last;    /**< Last net accelerating power (W) */

    /* AEP-723 Seamless Transition & Pre-Sync State */
    vsg_transition_status_t transition;
    double sync_theta_integ;   /**< Pre-sync integral state */

    /* AEP-724 Black Start Engine State */
    vsg_black_start_status_t bs_status;
    double bs_settle_timer_s;

    /* AEP-725 Multi-PCS Circulating Suppression & Impedance ID */
    vsg_circulating_status_t circ_status;
    double rls_theta[2];
    double rls_p[2][2];
    double circ_integ_q;
    double baseline_circ_q;

    /* AEP-727 SSO Detection & Active Notch Damping */
    vsg_sso_status_t sso_status;
    double fund_notch_w1;
    double fund_notch_w2;
    double notch_w1;
    double notch_w2;
    double sso_sogi_x;
    double sso_sogi_y;
    double sso_omega_est;
    double sso_energy_est;
    double sso_baseline_energy;

    /* AEP-728 Fault Ride-Through (FRT) State & Control */
    vsg_frt_status_t frt_status;
    double frt_recovery_ramp;
    
    /* AEP-729 Virtual Stator Impedance (R/X) & Harmonic Damping */
    vsg_virtual_z_status_t vz_status;
    double h_sogi_x_d;
    double h_sogi_y_d;
    double h_sogi_x_q;
    double h_sogi_y_q;
    double thd_baseline_rms;
    double thd_current_rms;

    /* Integrator state */
    uint64_t step_count;
};

/* -------------------------------------------------------------------------- */
/* Compile-Time Size & Alignment Assertions (AEP-752)                          */
/* -------------------------------------------------------------------------- */
_Static_assert(sizeof(struct vsg_controller) <= sizeof(vsg_controller_storage_t),
               "vsg_controller_storage_t buffer too small for struct vsg_controller");
_Static_assert(_Alignof(struct vsg_controller) <= _Alignof(vsg_controller_storage_t),
               "vsg_controller_storage_t alignment mismatch for struct vsg_controller");

vsg_controller_t *vsg_controller_create_in(vsg_controller_storage_t *store,
                                           const vsg_config_t *cfg,
                                           const vsg_ports_t *ports) {
    if (!store || !cfg) return NULL;
    if (cfg->j_inertia <= 0.0 || cfg->d_damping <= 0.0 || cfg->f_nominal <= 0.0 || cfg->v_nominal <= 0.0) {
        return NULL;
    }

    vsg_controller_t *vsg = (vsg_controller_t *)store->raw;
    memset(vsg, 0, sizeof(struct vsg_controller));

    vsg->config = *cfg;
    if (ports) {
        vsg->ports = *ports;
    }

    vsg->omega_0 = 2.0 * M_PI * cfg->f_nominal;
    vsg->omega = vsg->omega_0;
    vsg->frequency_hz = cfg->f_nominal;
    vsg->voltage_v = cfg->v_nominal;
    vsg->theta = 0.0;
    vsg->step_count = 0;

    if (vsg->config.scr_nominal <= 0.0) {
        vsg->config.scr_nominal = 3.0;
    }
    if (vsg->config.scr_current <= 0.0) {
        vsg->config.scr_current = vsg->config.scr_nominal;
    }

    if (vsg->config.sync_kp_theta <= 0.0) vsg->config.sync_kp_theta = 20.0;
    if (vsg->config.sync_ki_theta <= 0.0) vsg->config.sync_ki_theta = 2.0;
    if (vsg->config.sync_kp_v <= 0.0) vsg->config.sync_kp_v = 1.0;
    if (vsg->config.sync_theta_tol_rad <= 0.0) vsg->config.sync_theta_tol_rad = 0.05;
    if (vsg->config.sync_volt_tol_v <= 0.0) vsg->config.sync_volt_tol_v = 2.0;
    if (vsg->config.sync_freq_tol_hz <= 0.0) vsg->config.sync_freq_tol_hz = 0.1;
    if (vsg->config.island_freq_slew_hz_s <= 0.0) vsg->config.island_freq_slew_hz_s = 1.0;
    if (vsg->config.island_volt_slew_v_s <= 0.0) vsg->config.island_volt_slew_v_s = 20.0;

    /* AEP-724 Black Start Defaults */
    if (vsg->config.black_start.ramp_time_s <= 0.0) vsg->config.black_start.ramp_time_s = 5.0;
    if (vsg->config.black_start.max_volt_slew_v_s <= 0.0) vsg->config.black_start.max_volt_slew_v_s = 50.0;
    if (vsg->config.black_start.max_freq_slew_hz_s <= 0.0) vsg->config.black_start.max_freq_slew_hz_s = 10.0;
    if (vsg->config.black_start.sync_lock_angle_tol_rad <= 0.0) vsg->config.black_start.sync_lock_angle_tol_rad = 0.05;
    if (vsg->config.black_start.settle_time_s <= 0.0) vsg->config.black_start.settle_time_s = 0.5;

    vsg->bs_status.state = VSG_BS_STATE_IDLE;
    vsg->bs_status.ramp_progress_pct = 0.0;
    vsg->bs_status.v_ref_v = 0.0;
    vsg->bs_status.f_ref_hz = 0.0;
    vsg->bs_status.elapsed_time_s = 0.0;
    vsg->bs_status.is_ready_for_load = false;
    vsg->bs_settle_timer_s = 0.0;

    /* AEP-725 Circulating Current Suppression Defaults */
    if (vsg->config.circ_suppress.r_line_nominal_ohm <= 0.0) vsg->config.circ_suppress.r_line_nominal_ohm = 0.05;
    if (vsg->config.circ_suppress.x_line_nominal_ohm <= 0.0) vsg->config.circ_suppress.x_line_nominal_ohm = 0.15;
    if (vsg->config.circ_suppress.r_virtual_ohm <= 0.0) vsg->config.circ_suppress.r_virtual_ohm = 0.05;
    if (vsg->config.circ_suppress.x_virtual_ohm <= 0.0) vsg->config.circ_suppress.x_virtual_ohm = 0.20;
    if (vsg->config.circ_suppress.kp_circ <= 0.0) vsg->config.circ_suppress.kp_circ = 2.0;
    if (vsg->config.circ_suppress.ki_circ <= 0.0) vsg->config.circ_suppress.ki_circ = 20.0;
    if (vsg->config.circ_suppress.rls_forgetting_factor <= 0.0) vsg->config.circ_suppress.rls_forgetting_factor = 0.995;

    vsg->rls_theta[0] = vsg->config.circ_suppress.r_line_nominal_ohm;
    vsg->rls_theta[1] = vsg->config.circ_suppress.x_line_nominal_ohm;
    vsg->rls_p[0][0] = 100.0; vsg->rls_p[0][1] = 0.0;
    vsg->rls_p[1][0] = 0.0;   vsg->rls_p[1][1] = 100.0;
    vsg->circ_integ_q = 0.0;
    vsg->baseline_circ_q = 0.0;

    vsg->circ_status.r_line_est_ohm = vsg->rls_theta[0];
    vsg->circ_status.x_line_est_ohm = vsg->rls_theta[1];

    /* AEP-727 SSO Defaults */
    if (vsg->config.sso.sso_freq_min_hz <= 0.0) vsg->config.sso.sso_freq_min_hz = 5.0;
    if (vsg->config.sso.sso_freq_max_hz <= 0.0) vsg->config.sso.sso_freq_max_hz = 45.0;
    if (vsg->config.sso.sso_alarm_thresh_a <= 0.0) vsg->config.sso.sso_alarm_thresh_a = 5.0;
    if (vsg->config.sso.notch_damping_ratio_zeta <= 0.0) vsg->config.sso.notch_damping_ratio_zeta = 0.2;
    if (vsg->config.sso.active_damp_gain <= 0.0) vsg->config.sso.active_damp_gain = 3.0;

    vsg->sso_omega_est = 2.0 * M_PI * 15.0;
    vsg->notch_w1 = 0.0;
    vsg->notch_w2 = 0.0;
    vsg->sso_sogi_x = 0.0;
    vsg->sso_sogi_y = 0.0;
    vsg->sso_energy_est = 0.0;
    vsg->sso_baseline_energy = 0.0;
    vsg->sso_status.sso_detected = false;
    vsg->sso_status.sso_freq_hz = 15.0;
    vsg->sso_status.sso_amplitude_a = 0.0;
    vsg->sso_status.damping_out_v = 0.0;
    vsg->sso_status.energy_reduction_ratio_pct = 0.0;

    /* AEP-728 FRT Defaults */
    if (vsg->config.frt.u_lvrt_thresh_pu <= 0.0) vsg->config.frt.u_lvrt_thresh_pu = 0.90;
    if (vsg->config.frt.u_hvrt_thresh_pu <= 0.0) vsg->config.frt.u_hvrt_thresh_pu = 1.10;
    if (vsg->config.frt.k_lvrt_reactive_support <= 0.0) vsg->config.frt.k_lvrt_reactive_support = 2.0;
    if (vsg->config.frt.k_hvrt_reactive_support <= 0.0) vsg->config.frt.k_hvrt_reactive_support = 2.0;
    if (vsg->config.frt.i_nominal_a <= 0.0) vsg->config.frt.i_nominal_a = 150.0;
    if (vsg->config.frt.i_max_limit_a <= 0.0) vsg->config.frt.i_max_limit_a = 1.2 * vsg->config.frt.i_nominal_a;
    if (vsg->config.frt.k_trans_virtual_z <= 0.0) vsg->config.frt.k_trans_virtual_z = 0.5;
    if (vsg->config.frt.recovery_ramp_rate_pu_s <= 0.0) vsg->config.frt.recovery_ramp_rate_pu_s = 0.5;

    vsg->frt_recovery_ramp = 1.0;
    vsg->frt_status.state = VSG_FRT_STATE_NORMAL;
    vsg->frt_status.u_grid_pu = 1.0;
    vsg->frt_status.i_q_support_a = 0.0;
    vsg->frt_status.i_d_limit_a = vsg->config.frt.i_max_limit_a;

    /* AEP-729 Virtual Stator Impedance & Harmonic Damping Defaults */
    if (vsg->config.virtual_z.r_virtual_ohm <= 0.0) vsg->config.virtual_z.r_virtual_ohm = 0.02;
    if (vsg->config.virtual_z.x_virtual_ohm <= 0.0) vsg->config.virtual_z.x_virtual_ohm = 0.15;
    if (vsg->config.virtual_z.target_rx_ratio <= 0.0) vsg->config.virtual_z.target_rx_ratio = 0.10;
    if (vsg->config.virtual_z.r_line_est_ohm <= 0.0) vsg->config.virtual_z.r_line_est_ohm = 0.10;
    if (vsg->config.virtual_z.x_line_est_ohm <= 0.0) vsg->config.virtual_z.x_line_est_ohm = 0.05;
    if (vsg->config.virtual_z.k_harmonic_damping_h5 <= 0.0) vsg->config.virtual_z.k_harmonic_damping_h5 = 2.0;
    if (vsg->config.virtual_z.k_harmonic_damping_h7 <= 0.0) vsg->config.virtual_z.k_harmonic_damping_h7 = 2.0;
    if (vsg->config.virtual_z.harmonic_bandwidth_rad_s <= 0.0) vsg->config.virtual_z.harmonic_bandwidth_rad_s = 15.0;

    vsg->h_sogi_x_d = 0.0;
    vsg->h_sogi_y_d = 0.0;
    vsg->h_sogi_x_q = 0.0;
    vsg->h_sogi_y_q = 0.0;
    vsg->thd_baseline_rms = 0.0;
    vsg->thd_current_rms = 0.0;
    vsg->vz_status.r_v_effective_ohm = vsg->config.virtual_z.r_virtual_ohm;
    vsg->vz_status.x_v_effective_ohm = vsg->config.virtual_z.x_virtual_ohm;

    vsg->frt_status.v_trans_drop_v = 0.0;
    vsg->frt_status.recovery_progress_pct = 100.0;
    vsg->frt_status.is_current_clamped = false;

    vsg->transition.current_mode = cfg->grid_mode;
    vsg->transition.sync_locked = false;
    vsg->transition.lock_hold_cycles = 0;
    vsg->sync_theta_integ = 0.0;

    return vsg;
}

static inline void _vsg_calc_adaptive_params(const struct vsg_controller *vsg,
                                             double *out_d_eff, double *out_droop_p,
                                             double *out_droop_q) {
    double d_base = vsg->config.d_damping;
    double dp_base = vsg->config.droop_p;
    double dq_base = vsg->config.droop_q;

    if (!vsg->config.enable_adaptive_scr) {
        *out_d_eff = d_base;
        *out_droop_p = dp_base;
        *out_droop_q = dq_base;
        return;
    }

    double scr = vsg->config.scr_current;
    double scr_nom = vsg->config.scr_nominal > 0.0 ? vsg->config.scr_nominal : 3.0;
    if (scr <= 0.1) scr = 0.1; /* Physical lower bound */

    if (scr < scr_nom) {
        /* In weak grid, synchronizing power Ks decreases proportionally with SCR.
         * To maintain damping ratio zeta = D / (2 * sqrt(J * omega0 * Ks)) >= target,
         * D must scale inversely with sqrt(SCR / SCR_nom): D_eff = D_0 * sqrt(SCR_nom / SCR).
         */
        double ratio = sqrt(scr_nom / scr);
        if (ratio > 4.0) ratio = 4.0; /* Clamped to 4x to avoid overdamping */
        *out_d_eff = d_base * ratio;

        /* Adaptive Droop: scale down droop slopes to avoid excessive angle and voltage deviations */
        double droop_scale = sqrt(scr / scr_nom);
        *out_droop_p = dp_base * droop_scale;
        *out_droop_q = dq_base * (scr / scr_nom);
    } else {
        *out_d_eff = d_base;
        *out_droop_p = dp_base;
        *out_droop_q = dq_base;
    }
}

static inline double _vsg_eval_domega(const struct vsg_controller *vsg, double omega,
                                      double p_meas, double *out_p_mech, double *out_p_damp) {
    double d_eff = 0.0, droop_p_eff = 0.0, droop_q_eff = 0.0;
    _vsg_calc_adaptive_params(vsg, &d_eff, &droop_p_eff, &droop_q_eff);

    double p_mech = vsg->config.p_ref;
    if (droop_p_eff > 0.0) {
        double delta_f = (omega - vsg->omega_0) / (2.0 * M_PI);
        p_mech -= (delta_f / droop_p_eff);
    }
    double delta_omega = omega - vsg->omega_0;
    double p_damping = d_eff * delta_omega;
    double p_net = p_mech - p_meas - p_damping;

    /* Transient acceleration damping D_trans (if enabled) */
    if (vsg->config.d_transient > 0.0) {
        double d_trans_power = vsg->config.d_transient * p_net;
        p_damping += d_trans_power;
        p_net -= d_trans_power;
    }

    if (out_p_mech) *out_p_mech = p_mech;
    if (out_p_damp) *out_p_damp = p_damping;

    double j_omega0 = vsg->config.j_inertia * vsg->omega_0;
    if (j_omega0 < 1e-6) j_omega0 = 1e-6;

    return p_net / j_omega0;
}

int vsg_controller_step(vsg_controller_t *vsg, double p_meas, double q_meas,
                        double dt_s, double *out_freq_hz, double *out_voltage_v) {
    if (!vsg || dt_s <= 0.0) return -1;

    double prev_f = vsg->frequency_hz;
    double prev_v = vsg->voltage_v;

    /* 1 & 2. Swing Equation (Rotor Motion Integration) */
    if (vsg->config.integrator_mode == VSG_INTEGRATOR_EULER) {
        /* Forward Euler Integration (Baseline / Fast) */
        double p_mech = 0.0, p_damp = 0.0;
        double d_omega_dt = _vsg_eval_domega(vsg, vsg->omega, p_meas, &p_mech, &p_damp);
        vsg->p_mech_last = p_mech;
        vsg->p_damping_last = p_damp;
        vsg->p_net_last = p_mech - p_meas - p_damp;

        vsg->omega += d_omega_dt * dt_s;
        vsg->theta += vsg->omega * dt_s;
    } else {
        /* 4th-Order Runge-Kutta (RK4) High-Precision Integrator (AEP-721) */
        /* k1: initial state */
        double k1_w = _vsg_eval_domega(vsg, vsg->omega, p_meas, &vsg->p_mech_last, &vsg->p_damping_last);
        vsg->p_net_last = vsg->p_mech_last - p_meas - vsg->p_damping_last;
        double k1_th = vsg->omega;

        /* k2: midpoint step */
        double w_k2 = vsg->omega + 0.5 * dt_s * k1_w;
        double k2_w = _vsg_eval_domega(vsg, w_k2, p_meas, NULL, NULL);
        double k2_th = w_k2;

        /* k3: second midpoint step */
        double w_k3 = vsg->omega + 0.5 * dt_s * k2_w;
        double k3_w = _vsg_eval_domega(vsg, w_k3, p_meas, NULL, NULL);
        double k3_th = w_k3;

        /* k4: full step */
        double w_k4 = vsg->omega + dt_s * k3_w;
        double k4_w = _vsg_eval_domega(vsg, w_k4, p_meas, NULL, NULL);
        double k4_th = w_k4;

        /* Weighted Simpson's rule accumulation */
        vsg->omega += (dt_s / 6.0) * (k1_w + 2.0 * k2_w + 2.0 * k3_w + k4_w);
        vsg->theta += (dt_s / 6.0) * (k1_th + 2.0 * k2_th + 2.0 * k3_th + k4_th);
    }

    /* Safeguard virtual frequency within plausible grid bounds (+/- 5 Hz) */
    double min_omega = 2.0 * M_PI * (vsg->config.f_nominal - 5.0);
    double max_omega = 2.0 * M_PI * (vsg->config.f_nominal + 5.0);
    if (vsg->omega < min_omega) vsg->omega = min_omega;
    if (vsg->omega > max_omega) vsg->omega = max_omega;

    vsg->frequency_hz = vsg->omega / (2.0 * M_PI);

    /* Update post-step physical power metrics at current rotor speed */
    double d_eff_post = 0.0, dp_eff_post = 0.0, dq_eff_post = 0.0;
    _vsg_calc_adaptive_params(vsg, &d_eff_post, &dp_eff_post, &dq_eff_post);

    double delta_omega_post = vsg->omega - vsg->omega_0;
    vsg->p_damping_last = d_eff_post * delta_omega_post;
    double p_mech_post = vsg->config.p_ref;
    if (dp_eff_post > 0.0) {
        double delta_f_post = (vsg->omega - vsg->omega_0) / (2.0 * M_PI);
        p_mech_post -= (delta_f_post / dp_eff_post);
    }
    vsg->p_mech_last = p_mech_post;
    vsg->p_net_last = p_mech_post - p_meas - vsg->p_damping_last;

    /* 3. Rotor Phase Angle Normalization [0, 2*PI) */
    vsg->theta = fmod(vsg->theta, 2.0 * M_PI);
    if (vsg->theta < 0.0) {
        vsg->theta += 2.0 * M_PI;
    }

    /* 4. Reactive Power - Voltage Droop Control:
     *    E = V_nominal + Droop_Q * (Q_ref - Q_meas)
     */
    double delta_q = vsg->config.q_ref - q_meas;
    double v_target = vsg->config.v_nominal + dq_eff_post * delta_q;

    /* Low-pass filtering on voltage adjustment */
    double tau_v = 0.02; /* 20ms voltage filter time constant */
    double alpha_v = dt_s / (tau_v + dt_s);
    vsg->voltage_v = (1.0 - alpha_v) * vsg->voltage_v + alpha_v * v_target;

    /* Voltage protection clamp (0.8 ~ 1.2 pu) */
    double min_v = 0.80 * vsg->config.v_nominal;
    double max_v = 1.20 * vsg->config.v_nominal;
    if (vsg->voltage_v < min_v) vsg->voltage_v = min_v;
    if (vsg->voltage_v > max_v) vsg->voltage_v = max_v;

    /* AEP-723 Autonomous Islanding Voltage & Frequency Slew Rate Control */
    if (vsg->transition.current_mode == VSG_GRID_MODE_ISLANDED) {
        double df_max = vsg->config.island_freq_slew_hz_s * dt_s;
        double df = vsg->config.f_nominal - prev_f;
        if (fabs(df) > df_max) {
            vsg->frequency_hz = prev_f + (df > 0.0 ? df_max : -df_max);
        } else {
            vsg->frequency_hz = vsg->config.f_nominal;
        }
        vsg->omega = 2.0 * M_PI * vsg->frequency_hz;

        double dv_max = vsg->config.island_volt_slew_v_s * dt_s;
        double dv = vsg->config.v_nominal - prev_v;
        if (fabs(dv) > dv_max) {
            vsg->voltage_v = prev_v + (dv > 0.0 ? dv_max : -dv_max);
        } else {
            vsg->voltage_v = vsg->config.v_nominal;
        }
    }

    vsg->step_count++;

    if (out_freq_hz) *out_freq_hz = vsg->frequency_hz;
    if (out_voltage_v) *out_voltage_v = vsg->voltage_v;

    return 0;
}

int vsg_controller_step_ports(vsg_controller_t *vsg,
                              uint32_t tag_p_meas, uint32_t tag_q_meas,
                              uint32_t tag_freq_out, uint32_t tag_volt_out,
                              double dt_s) {
    if (!vsg) return -1;

    double p_meas = 0.0;
    double q_meas = 0.0;

    if (vsg->ports.reader.read_tag) {
        if (tag_p_meas) {
            vsg->ports.reader.read_tag(vsg->ports.reader.ctx, tag_p_meas, &p_meas, NULL);
        }
        if (tag_q_meas) {
            vsg->ports.reader.read_tag(vsg->ports.reader.ctx, tag_q_meas, &q_meas, NULL);
        }
    }

    double freq_out = 0.0;
    double volt_out = 0.0;
    int rc = vsg_controller_step(vsg, p_meas, q_meas, dt_s, &freq_out, &volt_out);
    if (rc != 0) return rc;

    uint64_t now_us = 0;
    if (vsg->ports.clock.now_us) {
        now_us = vsg->ports.clock.now_us(vsg->ports.clock.ctx);
    }

    if (vsg->ports.writer.write_tag) {
        if (tag_freq_out) {
            vsg->ports.writer.write_tag(vsg->ports.writer.ctx, tag_freq_out, freq_out, now_us);
        }
        if (tag_volt_out) {
            vsg->ports.writer.write_tag(vsg->ports.writer.ctx, tag_volt_out, volt_out, now_us);
        }
    }

    return 0;
}

void vsg_controller_reset(vsg_controller_t *vsg) {
    if (!vsg) return;
    vsg->omega = vsg->omega_0;
    vsg->frequency_hz = vsg->config.f_nominal;
    vsg->voltage_v = vsg->config.v_nominal;
    vsg->theta = 0.0;
    vsg->step_count = 0;
}

double vsg_controller_get_frequency(const vsg_controller_t *vsg) {
    return vsg ? vsg->frequency_hz : 0.0;
}

double vsg_controller_get_voltage(const vsg_controller_t *vsg) {
    return vsg ? vsg->voltage_v : 0.0;
}

double vsg_controller_get_omega(const vsg_controller_t *vsg) {
    return vsg ? vsg->omega : 0.0;
}

double vsg_controller_get_theta(const vsg_controller_t *vsg) {
    return vsg ? vsg->theta : 0.0;
}

vsg_integrator_mode_t vsg_controller_get_integrator_mode(const vsg_controller_t *vsg) {
    return vsg ? vsg->config.integrator_mode : VSG_INTEGRATOR_RK4;
}

void vsg_controller_set_integrator_mode(vsg_controller_t *vsg, vsg_integrator_mode_t mode) {
    if (vsg) {
        vsg->config.integrator_mode = mode;
    }
}

double vsg_controller_get_p_mech(const vsg_controller_t *vsg) {
    return vsg ? vsg->p_mech_last : 0.0;
}

double vsg_controller_get_p_damping(const vsg_controller_t *vsg) {
    return vsg ? vsg->p_damping_last : 0.0;
}

double vsg_controller_get_p_net(const vsg_controller_t *vsg) {
    return vsg ? vsg->p_net_last : 0.0;
}

uint64_t vsg_controller_get_step_count(const vsg_controller_t *vsg) {
    return vsg ? vsg->step_count : 0;
}

int vsg_controller_set_grid_scr(vsg_controller_t *vsg, double scr) {
    if (!vsg || scr <= 0.0) return -1;
    vsg->config.scr_current = scr;
    return 0;
}

double vsg_controller_get_grid_scr(const vsg_controller_t *vsg) {
    return vsg ? vsg->config.scr_current : 0.0;
}

double vsg_controller_get_effective_damping(const vsg_controller_t *vsg) {
    if (!vsg) return 0.0;
    double d_eff = 0.0, dp = 0.0, dq = 0.0;
    _vsg_calc_adaptive_params(vsg, &d_eff, &dp, &dq);
    return d_eff;
}

double vsg_controller_get_effective_droop_p(const vsg_controller_t *vsg) {
    if (!vsg) return 0.0;
    double d_eff = 0.0, dp = 0.0, dq = 0.0;
    _vsg_calc_adaptive_params(vsg, &d_eff, &dp, &dq);
    return dp;
}

double vsg_controller_get_effective_droop_q(const vsg_controller_t *vsg) {
    if (!vsg) return 0.0;
    double d_eff = 0.0, dp = 0.0, dq = 0.0;
    _vsg_calc_adaptive_params(vsg, &d_eff, &dp, &dq);
    return dq;
}

/* -------------------------------------------------------------------------- */
/* AEP-723 Seamless Grid-Forming Transition & Pre-Sync Implementation         */
/* -------------------------------------------------------------------------- */
static inline double _wrap_pi(double angle) {
    while (angle > M_PI) angle -= 2.0 * M_PI;
    while (angle < -M_PI) angle += 2.0 * M_PI;
    return angle;
}

int vsg_controller_set_mode(vsg_controller_t *vsg, vsg_grid_mode_t mode) {
    if (!vsg) return -1;
    vsg->transition.current_mode = mode;
    if (mode == VSG_GRID_MODE_CONNECTED) {
        vsg->transition.sync_locked = false;
        vsg->transition.lock_hold_cycles = 0;
        vsg->sync_theta_integ = 0.0;
    }
    return 0;
}

vsg_grid_mode_t vsg_controller_get_mode(const vsg_controller_t *vsg) {
    return vsg ? vsg->transition.current_mode : VSG_GRID_MODE_CONNECTED;
}

int vsg_controller_trigger_unplanned_islanding(vsg_controller_t *vsg,
                                              double grid_v, double grid_f, double grid_theta) {
    if (!vsg) return -1;

    /* Microsecond-level state handover to ensure zero voltage/phase jump */
    if (grid_v > 0.0) {
        vsg->voltage_v = grid_v;
    }
    if (grid_f > 0.0) {
        vsg->frequency_hz = grid_f;
        vsg->omega = 2.0 * M_PI * grid_f;
    }
    vsg->theta = fmod(grid_theta, 2.0 * M_PI);
    if (vsg->theta < 0.0) vsg->theta += 2.0 * M_PI;

    vsg->transition.current_mode = VSG_GRID_MODE_ISLANDED;
    vsg->transition.sync_locked = false;
    vsg->transition.lock_hold_cycles = 0;
    vsg->sync_theta_integ = 0.0;

    return 0;
}

int vsg_controller_step_presync(vsg_controller_t *vsg,
                                double grid_v, double grid_f, double grid_theta, double dt_s) {
    if (!vsg || dt_s <= 0.0 || grid_v <= 0.0 || grid_f <= 0.0) return -1;

    vsg->transition.current_mode = VSG_GRID_MODE_PRE_SYNC;

    /* 1. Calculate phase, voltage, and frequency errors */
    double delta_theta = _wrap_pi(grid_theta - vsg->theta);
    double delta_v = grid_v - vsg->voltage_v;
    double delta_f = grid_f - vsg->frequency_hz;

    vsg->transition.delta_theta_rad = delta_theta;
    vsg->transition.delta_volt_v = delta_v;
    vsg->transition.delta_freq_hz = delta_f;

    /* 2. PI phase tracking trim */
    vsg->sync_theta_integ += delta_theta * dt_s;
    if (vsg->sync_theta_integ > 10.0) vsg->sync_theta_integ = 10.0;
    if (vsg->sync_theta_integ < -10.0) vsg->sync_theta_integ = -10.0;

    double omega_trim = vsg->config.sync_kp_theta * delta_theta +
                        vsg->config.sync_ki_theta * vsg->sync_theta_integ;

    vsg->omega = 2.0 * M_PI * grid_f + omega_trim;
    vsg->frequency_hz = vsg->omega / (2.0 * M_PI);

    /* 3. Voltage magnitude tracking trim */
    double v_trim = vsg->config.sync_kp_v * delta_v;
    vsg->voltage_v += v_trim * dt_s * 10.0;
    if (vsg->voltage_v > 1.2 * vsg->config.v_nominal) vsg->voltage_v = 1.2 * vsg->config.v_nominal;
    if (vsg->voltage_v < 0.8 * vsg->config.v_nominal) vsg->voltage_v = 0.8 * vsg->config.v_nominal;

    /* 4. Update phase angle */
    vsg->theta += vsg->omega * dt_s;
    vsg->theta = fmod(vsg->theta, 2.0 * M_PI);
    if (vsg->theta < 0.0) vsg->theta += 2.0 * M_PI;

    /* 5. Check if tolerances are satisfied for lock hold */
    bool in_tol = (fabs(delta_theta) <= vsg->config.sync_theta_tol_rad) &&
                  (fabs(delta_v) <= vsg->config.sync_volt_tol_v) &&
                  (fabs(delta_f) <= vsg->config.sync_freq_tol_hz);

    if (in_tol) {
        vsg->transition.lock_hold_cycles++;
        if (vsg->transition.lock_hold_cycles >= 5) {
            vsg->transition.sync_locked = true;
        }
    } else {
        vsg->transition.lock_hold_cycles = 0;
        vsg->transition.sync_locked = false;
    }

    vsg->step_count++;
    return 0;
}

bool vsg_controller_is_sync_locked(const vsg_controller_t *vsg) {
    return vsg ? vsg->transition.sync_locked : false;
}

int vsg_controller_get_transition_status(const vsg_controller_t *vsg,
                                         vsg_transition_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->transition;
    return 0;
}

/* -------------------------------------------------------------------------- */
/* AEP-724 Microgrid Black Start & Multi-Unit Lockstep Engine APIs            */
/* -------------------------------------------------------------------------- */

int vsg_controller_trigger_black_start(vsg_controller_t *vsg) {
    if (!vsg) return -1;

    vsg->transition.current_mode = VSG_GRID_MODE_BLACK_START;
    vsg->bs_status.state = VSG_BS_STATE_RAMPING;
    vsg->bs_status.ramp_progress_pct = 0.0;
    vsg->bs_status.v_ref_v = 0.0;
    vsg->bs_status.f_ref_hz = 0.0;
    vsg->bs_status.elapsed_time_s = 0.0;
    vsg->bs_status.is_ready_for_load = false;
    vsg->bs_settle_timer_s = 0.0;

    vsg->voltage_v = 0.0;
    vsg->frequency_hz = 0.0;
    vsg->omega = 0.0;
    vsg->theta = 0.0;
    vsg->transition.sync_locked = false;
    vsg->transition.lock_hold_cycles = 0;

    return 0;
}

int vsg_controller_step_black_start(vsg_controller_t *vsg, double i_meas_a, double sync_angle_rad,
                                    double dt_s, double *out_freq_hz, double *out_voltage_v) {
    if (!vsg || dt_s <= 0.0) return -1;

    if (vsg->bs_status.state == VSG_BS_STATE_IDLE || vsg->bs_status.state == VSG_BS_STATE_FAULT) {
        if (out_freq_hz) *out_freq_hz = 0.0;
        if (out_voltage_v) *out_voltage_v = 0.0;
        return 0;
    }

    vsg->bs_status.elapsed_time_s += dt_s;

    /* 1. Inrush current detection and soft throttling */
    double throttle_factor = 1.0;
    if (vsg->config.black_start.inrush_i_limit_a > 0.0) {
        double i_lim = vsg->config.black_start.inrush_i_limit_a;
        if (i_meas_a > 2.0 * i_lim) {
            /* Severe inrush fault trip */
            vsg->bs_status.state = VSG_BS_STATE_FAULT;
            vsg->voltage_v = 0.0;
            vsg->frequency_hz = 0.0;
            vsg->omega = 0.0;
            if (out_freq_hz) *out_freq_hz = 0.0;
            if (out_voltage_v) *out_voltage_v = 0.0;
            return 0;
        } else if (i_meas_a > 0.8 * i_lim) {
            /* Soft throttle ramp rate */
            double excess = (i_meas_a - 0.8 * i_lim) / (0.4 * i_lim);
            if (excess > 1.0) excess = 1.0;
            throttle_factor = 1.0 - 0.8 * excess;
            if (throttle_factor < 0.1) throttle_factor = 0.1;
        }
    }

    /* 2. Soft ramp-up profile */
    double target_v = vsg->config.v_nominal;
    double target_f = vsg->config.f_nominal;

    double dv_rate = (target_v / vsg->config.black_start.ramp_time_s) * throttle_factor;
    if (dv_rate > vsg->config.black_start.max_volt_slew_v_s) {
        dv_rate = vsg->config.black_start.max_volt_slew_v_s;
    }

    double df_rate = (target_f / vsg->config.black_start.ramp_time_s) * throttle_factor;
    if (df_rate > vsg->config.black_start.max_freq_slew_hz_s) {
        df_rate = vsg->config.black_start.max_freq_slew_hz_s;
    }

    vsg->bs_status.v_ref_v += dv_rate * dt_s;
    if (vsg->bs_status.v_ref_v > target_v) {
        vsg->bs_status.v_ref_v = target_v;
    }

    vsg->bs_status.f_ref_hz += df_rate * dt_s;
    if (vsg->bs_status.f_ref_hz > target_f) {
        vsg->bs_status.f_ref_hz = target_f;
    }

    double prog_v = (target_v > 0.0) ? (vsg->bs_status.v_ref_v / target_v) * 100.0 : 100.0;
    double prog_f = (target_f > 0.0) ? (vsg->bs_status.f_ref_hz / target_f) * 100.0 : 100.0;
    vsg->bs_status.ramp_progress_pct = (prog_v < prog_f) ? prog_v : prog_f;
    if (vsg->bs_status.ramp_progress_pct > 100.0) {
        vsg->bs_status.ramp_progress_pct = 100.0;
    }

    /* 3. Multi-unit lockstep virtual rotor phase alignment */
    double delta_theta = 0.0;
    double omega_ref = 2.0 * M_PI * vsg->bs_status.f_ref_hz;
    double omega_trim = 0.0;

    if (sync_angle_rad >= 0.0) {
        delta_theta = _wrap_pi(sync_angle_rad - vsg->theta);
        if (fabs(delta_theta) > vsg->config.black_start.sync_lock_angle_tol_rad &&
            vsg->bs_status.state != VSG_BS_STATE_ESTABLISHED) {
            vsg->bs_status.state = VSG_BS_STATE_SYNC_LOCK;
        } else if (vsg->bs_status.state == VSG_BS_STATE_SYNC_LOCK) {
            vsg->bs_status.state = VSG_BS_STATE_RAMPING;
        }
        omega_trim = 10.0 * delta_theta;
    }

    vsg->omega = omega_ref + omega_trim;
    vsg->frequency_hz = (vsg->omega > 0.0) ? (vsg->omega / (2.0 * M_PI)) : 0.0;
    vsg->voltage_v = vsg->bs_status.v_ref_v;

    vsg->theta += vsg->omega * dt_s;
    vsg->theta = fmod(vsg->theta, 2.0 * M_PI);
    if (vsg->theta < 0.0) vsg->theta += 2.0 * M_PI;

    /* 4. Check stabilization condition */
    bool v_reached = (target_v - vsg->voltage_v) <= 0.01 * target_v;
    bool f_reached = (target_f - vsg->frequency_hz) <= 0.01 * target_f;
    bool sync_locked = (sync_angle_rad < 0.0) ||
                       (fabs(delta_theta) <= vsg->config.black_start.sync_lock_angle_tol_rad);

    if (v_reached && f_reached && sync_locked) {
        vsg->bs_settle_timer_s += dt_s;
        if (vsg->bs_settle_timer_s >= vsg->config.black_start.settle_time_s) {
            vsg->bs_status.state = VSG_BS_STATE_ESTABLISHED;
            vsg->bs_status.is_ready_for_load = true;
            vsg->transition.current_mode = VSG_GRID_MODE_ISLANDED;
        }
    } else {
        vsg->bs_settle_timer_s = 0.0;
    }

    if (out_freq_hz) *out_freq_hz = vsg->frequency_hz;
    if (out_voltage_v) *out_voltage_v = vsg->voltage_v;

    vsg->step_count++;
    return 0;
}

vsg_black_start_state_t vsg_controller_get_black_start_state(const vsg_controller_t *vsg) {
    return vsg ? vsg->bs_status.state : VSG_BS_STATE_IDLE;
}

int vsg_controller_get_black_start_status(const vsg_controller_t *vsg, vsg_black_start_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->bs_status;
    return 0;
}

void vsg_controller_abort_black_start(vsg_controller_t *vsg) {
    if (!vsg) return;
    vsg->bs_status.state = VSG_BS_STATE_FAULT;
    vsg->bs_status.is_ready_for_load = false;
    vsg->voltage_v = 0.0;
    vsg->frequency_hz = 0.0;
    vsg->omega = 0.0;
}

/* -------------------------------------------------------------------------- */
/* AEP-725 Multi-PCS Line Impedance & Circulating Suppression APIs            */
/* -------------------------------------------------------------------------- */

int vsg_controller_set_circ_suppress_config(vsg_controller_t *vsg, const vsg_circulating_suppress_config_t *cfg) {
    if (!vsg || !cfg) return -1;
    vsg->config.circ_suppress = *cfg;
    if (vsg->config.circ_suppress.r_line_nominal_ohm <= 0.0) vsg->config.circ_suppress.r_line_nominal_ohm = 0.05;
    if (vsg->config.circ_suppress.x_line_nominal_ohm <= 0.0) vsg->config.circ_suppress.x_line_nominal_ohm = 0.15;
    if (vsg->config.circ_suppress.r_virtual_ohm <= 0.0) vsg->config.circ_suppress.r_virtual_ohm = 0.05;
    if (vsg->config.circ_suppress.x_virtual_ohm <= 0.0) vsg->config.circ_suppress.x_virtual_ohm = 0.20;
    if (vsg->config.circ_suppress.kp_circ <= 0.0) vsg->config.circ_suppress.kp_circ = 2.0;
    if (vsg->config.circ_suppress.ki_circ <= 0.0) vsg->config.circ_suppress.ki_circ = 20.0;
    if (vsg->config.circ_suppress.rls_forgetting_factor <= 0.0) vsg->config.circ_suppress.rls_forgetting_factor = 0.995;

    vsg->rls_theta[0] = vsg->config.circ_suppress.r_line_nominal_ohm;
    vsg->rls_theta[1] = vsg->config.circ_suppress.x_line_nominal_ohm;
    vsg->circ_status.r_line_est_ohm = vsg->rls_theta[0];
    vsg->circ_status.x_line_est_ohm = vsg->rls_theta[1];
    return 0;
}

int vsg_controller_step_circ_suppression(vsg_controller_t *vsg,
                                         double i_d_meas, double i_q_meas,
                                         double i_d_avg, double i_q_avg,
                                         double v_pcc_d, double v_pcc_q,
                                         double dt_s,
                                         double *out_v_d, double *out_v_q) {
    if (!vsg || dt_s <= 0.0) return -1;
    (void)v_pcc_q;

    /* 1. Calculate branch circulating currents */
    double i_circ_d = i_d_meas - i_d_avg;
    double i_circ_q = i_q_meas - i_q_avg;

    vsg->circ_status.i_circ_d_a = i_circ_d;
    vsg->circ_status.i_circ_q_a = i_circ_q;

    if (vsg->baseline_circ_q <= 0.0 && fabs(i_circ_q) > 0.01) {
        vsg->baseline_circ_q = fabs(i_circ_q);
    }

    /* 2. Online RLS Line Impedance Identification */
    if (vsg->config.circ_suppress.enable_online_impedance_id) {
        /* Physical relationship across feeder line:
           Delta V_d = V_vsg_d - V_pcc_d = R_line * I_d - X_line * I_q
        */
        double delta_v_d = vsg->voltage_v - v_pcc_d;
        double phi[2] = {i_d_meas, -i_q_meas};

        double i_sq = i_d_meas * i_d_meas + i_q_meas * i_q_meas;
        if (i_sq > 1.0) { /* Update only when non-trivial current flows */
            double lambda = vsg->config.circ_suppress.rls_forgetting_factor;

            /* P * phi */
            double p_phi[2];
            p_phi[0] = vsg->rls_p[0][0] * phi[0] + vsg->rls_p[0][1] * phi[1];
            p_phi[1] = vsg->rls_p[1][0] * phi[0] + vsg->rls_p[1][1] * phi[1];

            /* denom = lambda + phi^T * P * phi */
            double denom = lambda + phi[0] * p_phi[0] + phi[1] * p_phi[1];
            if (denom > 1e-6) {
                /* Gain vector K = p_phi / denom */
                double k_gain[2] = {p_phi[0] / denom, p_phi[1] / denom};

                /* Prediction error epsilon = y - phi^T * theta */
                double y_pred = phi[0] * vsg->rls_theta[0] + phi[1] * vsg->rls_theta[1];
                double epsilon = delta_v_d - y_pred;

                /* Parameter update */
                vsg->rls_theta[0] += k_gain[0] * epsilon;
                vsg->rls_theta[1] += k_gain[1] * epsilon;

                /* Physical bounds clamping */
                if (vsg->rls_theta[0] < 0.001) vsg->rls_theta[0] = 0.001;
                if (vsg->rls_theta[0] > 2.0)   vsg->rls_theta[0] = 2.0;
                if (vsg->rls_theta[1] < 0.001) vsg->rls_theta[1] = 0.001;
                if (vsg->rls_theta[1] > 5.0)   vsg->rls_theta[1] = 5.0;

                /* Covariance update: P = (P - K * p_phi^T) / lambda */
                double p00 = (vsg->rls_p[0][0] - k_gain[0] * p_phi[0]) / lambda;
                double p01 = (vsg->rls_p[0][1] - k_gain[0] * p_phi[1]) / lambda;
                double p10 = (vsg->rls_p[1][0] - k_gain[1] * p_phi[0]) / lambda;
                double p11 = (vsg->rls_p[1][1] - k_gain[1] * p_phi[1]) / lambda;

                vsg->rls_p[0][0] = p00; vsg->rls_p[0][1] = p01;
                vsg->rls_p[1][0] = p10; vsg->rls_p[1][1] = p11;
            }
        }
    }

    vsg->circ_status.r_line_est_ohm = vsg->rls_theta[0];
    vsg->circ_status.x_line_est_ohm = vsg->rls_theta[1];

    /* 3. Virtual Stator Impedance & Circulating Regulator */
    double r_v = vsg->config.circ_suppress.r_virtual_ohm;
    double x_v = vsg->config.circ_suppress.x_virtual_ohm;

    double v_comp_d = 0.0;
    double v_comp_q = 0.0;

    if (vsg->config.circ_suppress.enable_circ_suppression) {
        /* Virtual impedance voltage drop:
           Delta V_v,d = R_v * I_d - X_v * I_q
           Delta V_v,q = R_v * I_q + X_v * I_d
        */
        v_comp_d = r_v * i_d_meas - x_v * i_q_meas;
        v_comp_q = r_v * i_q_meas + x_v * i_d_meas;

        /* PI regulation on reactive circulating current */
        vsg->circ_integ_q += i_circ_q * dt_s;
        if (vsg->circ_integ_q > 50.0) vsg->circ_integ_q = 50.0;
        if (vsg->circ_integ_q < -50.0) vsg->circ_integ_q = -50.0;

        double v_pi_q = vsg->config.circ_suppress.kp_circ * i_circ_q +
                        vsg->config.circ_suppress.ki_circ * vsg->circ_integ_q;
        v_comp_q += v_pi_q;
    }

    vsg->circ_status.v_comp_d_v = v_comp_d;
    vsg->circ_status.v_comp_q_v = v_comp_q;

    /* 4. Calculate Suppression Efficiency Ratio */
    if (vsg->baseline_circ_q > 0.01) {
        double current_abs = fabs(i_circ_q);
        double ratio = (1.0 - (current_abs / vsg->baseline_circ_q)) * 100.0;
        if (ratio < 0.0) ratio = 0.0;
        if (ratio > 100.0) ratio = 100.0;
        vsg->circ_status.suppression_ratio_pct = ratio;
    } else {
        vsg->circ_status.suppression_ratio_pct = 100.0;
    }

    /* Output compensated voltage references */
    double v_ref_d = vsg->voltage_v - v_comp_d;
    double v_ref_q = 0.0 - v_comp_q;

    if (out_v_d) *out_v_d = v_ref_d;
    if (out_v_q) *out_v_q = v_ref_q;

    vsg->step_count++;
    return 0;
}

int vsg_controller_get_circ_status(const vsg_controller_t *vsg, vsg_circulating_status_t *status) {
    if (!vsg || !status) return -1;
    *status = vsg->circ_status;
    return 0;
}

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
        double r_line = (vsg->circ_status.r_line_est_ohm > 0.0) ? 
                        vsg->circ_status.r_line_est_ohm : vsg->config.virtual_z.r_line_est_ohm;
        double x_line = (vsg->circ_status.x_line_est_ohm > 0.0) ? 
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
    double k_h = (vsg->config.virtual_z.k_harmonic_damping_h5 + vsg->config.virtual_z.k_harmonic_damping_h7) * 0.5;

    /* SOGI Resonator on D-axis current */
    double err_d = i_d_meas - vsg->h_sogi_x_d;
    vsg->h_sogi_x_d += (2.0 * omega_cut * err_d - omega_h * vsg->h_sogi_y_d) * dt_s;
    vsg->h_sogi_y_d += (omega_h * vsg->h_sogi_x_d) * dt_s;

    /* SOGI Resonator on Q-axis current */
    double err_q = i_q_meas - vsg->h_sogi_x_q;
    vsg->h_sogi_x_q += (2.0 * omega_cut * err_q - omega_h * vsg->h_sogi_y_q) * dt_s;
    vsg->h_sogi_y_q += (omega_h * vsg->h_sogi_x_q) * dt_s;

    double i_d_harm = vsg->h_sogi_x_d;
    double i_q_harm = vsg->h_sogi_x_q;

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

