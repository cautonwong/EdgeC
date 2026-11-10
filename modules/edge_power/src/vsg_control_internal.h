/**
 * @file vsg_control_internal.h
 * @brief Shared VSG controller state and helpers for the split AEP-723..730 files.
 */

#ifndef VSG_CONTROL_INTERNAL_H
#define VSG_CONTROL_INTERNAL_H

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

    /* AEP-730 Computing-Power Integrated Dynamic Tracking */
    vsg_compute_tracking_status_t compute_status;
    double prev_p_compute_meas_w;

    /* Integrator state */
    uint64_t step_count;
};

static inline double _wrap_pi(double angle) {
    while (angle > M_PI) angle -= 2.0 * M_PI;
    while (angle < -M_PI) angle += 2.0 * M_PI;
    return angle;
}

#endif /* VSG_CONTROL_INTERNAL_H */
