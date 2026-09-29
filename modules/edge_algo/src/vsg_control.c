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
