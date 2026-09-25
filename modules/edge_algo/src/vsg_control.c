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

    return vsg;
}

int vsg_controller_step(vsg_controller_t *vsg, double p_meas, double q_meas,
                        double dt_s, double *out_freq_hz, double *out_voltage_v) {
    if (!vsg || dt_s <= 0.0) return -1;

    /* 1. Active Power - Frequency Droop Adjustment */
    double p_mech_ref = vsg->config.p_ref;
    if (vsg->config.droop_p > 0.0) {
        double delta_f = vsg->frequency_hz - vsg->config.f_nominal;
        p_mech_ref -= (delta_f / vsg->config.droop_p);
    }

    /* 2. Swing Equation (Rotor Motion Equation):
     *    J * omega_0 * (d(omega)/dt) = P_mech_ref - P_meas - D * (omega - omega_0)
     */
    double delta_omega = vsg->omega - vsg->omega_0;
    double p_damping = vsg->config.d_damping * delta_omega;
    double p_net = p_mech_ref - p_meas - p_damping;

    double j_omega0 = vsg->config.j_inertia * vsg->omega_0;
    if (j_omega0 < 1e-6) j_omega0 = 1e-6;

    double d_omega_dt = p_net / j_omega0;
    vsg->omega += d_omega_dt * dt_s;

    /* Safeguard virtual frequency within plausible grid bounds (+/- 5 Hz) */
    double min_omega = 2.0 * M_PI * (vsg->config.f_nominal - 5.0);
    double max_omega = 2.0 * M_PI * (vsg->config.f_nominal + 5.0);
    if (vsg->omega < min_omega) vsg->omega = min_omega;
    if (vsg->omega > max_omega) vsg->omega = max_omega;

    vsg->frequency_hz = vsg->omega / (2.0 * M_PI);

    /* 3. Rotor Phase Angle Integration */
    vsg->theta += vsg->omega * dt_s;
    while (vsg->theta >= 2.0 * M_PI) {
        vsg->theta -= 2.0 * M_PI;
    }
    while (vsg->theta < 0.0) {
        vsg->theta += 2.0 * M_PI;
    }

    /* 4. Reactive Power - Voltage Droop Control:
     *    E = V_nominal + Droop_Q * (Q_ref - Q_meas)
     */
    double delta_q = vsg->config.q_ref - q_meas;
    double v_target = vsg->config.v_nominal + vsg->config.droop_q * delta_q;

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
