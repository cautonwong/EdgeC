/**
 * @file vsg_control.h
 * @brief Virtual Synchronous Generator (VSG) Grid-Forming Control (AEP-723 ~ AEP-730 / AEP-752)
 *
 * Implements Virtual Synchronous Machine swing equation and reactive-voltage droop
 * for grid-forming energy storage converters, conforming to:
 * 1. Physical Opaque Encapsulation (Opaque struct pointer)
 * 2. Caller-Allocated Storage (_Alignas(max_align_t) placeholder)
 * 3. Consumer-Defined Interfaces (vsg_ports_t)
 * 4. Aggregate Root State Invariant Management
 */

#ifndef EDGE_ALGO_VSG_CONTROL_H
#define EDGE_ALGO_VSG_CONTROL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include "edge_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VSG Rotor Motion Integrator Algorithm Type (AEP-721)
 */
typedef enum {
    VSG_INTEGRATOR_RK4 = 0,   /**< 4th-order Runge-Kutta high-precision integrator (default) */
    VSG_INTEGRATOR_EULER = 1, /**< Forward Euler integrator (legacy / baseline comparison) */
} vsg_integrator_mode_t;

/**
 * @brief VSG Grid Operation Modes (AEP-723 Seamless Transition)
 */
typedef enum {
    VSG_GRID_MODE_CONNECTED   = 0,  /**< Grid-connected mode (active/reactive dispatch + grid forming) */
    VSG_GRID_MODE_ISLANDED    = 1,  /**< Autonomous islanded microgrid (V/f master) */
    VSG_GRID_MODE_PRE_SYNC    = 2,  /**< Pre-synchronization mode (phase/voltage tracking for reconnection) */
    VSG_GRID_MODE_BLACK_START = 3,  /**< Black start zero-voltage soft excitation mode (AEP-724) */
} vsg_grid_mode_t;

/**
 * @brief VSG Black Start State Machine (AEP-724)
 */
typedef enum {
    VSG_BS_STATE_IDLE        = 0,   /**< Black start idle / unexcited state */
    VSG_BS_STATE_RAMPING     = 1,   /**< Zero-voltage soft ramp-up excitation phase */
    VSG_BS_STATE_SYNC_LOCK   = 2,   /**< Multi-unit virtual lockstep phase alignment */
    VSG_BS_STATE_ESTABLISHED = 3,   /**< Stable microgrid bus voltage/frequency established */
    VSG_BS_STATE_FAULT       = 4,   /**< Black start fault (inrush overcurrent / timeout) */
} vsg_black_start_state_t;

/**
 * @brief Black Start Engine Status & Telemetry (AEP-724)
 */
typedef struct {
    vsg_black_start_state_t state; /**< Current state of black start engine */
    double ramp_progress_pct;      /**< Ramp completion percentage [0.0, 100.0] */
    double v_ref_v;                /**< Current instantaneous soft-start voltage reference (V) */
    double f_ref_hz;               /**< Current instantaneous soft-start frequency reference (Hz) */
    double elapsed_time_s;         /**< Time elapsed since black start trigger (s) */
    bool is_ready_for_load;        /**< True when voltage and frequency are stable and ready for load pickup */
} vsg_black_start_status_t;

/**
 * @brief Black Start Physical Configuration Parameters (AEP-724)
 */
typedef struct {
    double ramp_time_s;            /**< Target duration for 0 -> V_nominal soft-start ramp (s, typ. 2.0 ~ 10.0) */
    double max_volt_slew_v_s;      /**< Max voltage rise rate dV/dt (V/s, typ. 50.0 ~ 100.0) */
    double max_freq_slew_hz_s;     /**< Max frequency rise rate df/dt (Hz/s, typ. 10.0 ~ 25.0) */
    double inrush_i_limit_a;       /**< Maximum permissible inrush current during excitation (A, 0.0=disabled) */
    double sync_lock_angle_tol_rad;/**< Inter-unit lockstep angle alignment tolerance (rad, typ. 0.05) */
    double settle_time_s;          /**< Required duration to hold target V/f before ESTABLISHED (s, typ. 0.5) */
} vsg_black_start_config_t;

/**
 * @brief Multi-PCS Line Impedance & Circulating Current Suppression Configuration (AEP-725)
 */
typedef struct {
    bool enable_circ_suppression;     /**< Enable active circulating current suppression */
    bool enable_online_impedance_id;  /**< Enable online line impedance RLS estimation */
    double r_line_nominal_ohm;        /**< Nominal/initial branch line resistance (Ω, typ. 0.05 ~ 0.5) */
    double x_line_nominal_ohm;        /**< Nominal/initial branch line reactance (Ω, typ. 0.1 ~ 1.0) */
    double r_virtual_ohm;             /**< Adaptive virtual stator resistance R_v (Ω, typ. 0.05 ~ 0.5) */
    double x_virtual_ohm;             /**< Adaptive virtual stator reactance X_v (Ω, typ. 0.1 ~ 1.0) */
    double kp_circ;                   /**< Reactive circulating current suppression proportional gain */
    double ki_circ;                   /**< Reactive circulating current suppression integral gain */
    double rls_forgetting_factor;     /**< RLS recursive least squares forgetting factor lambda (typ. 0.98 ~ 0.999) */
} vsg_circulating_suppress_config_t;

/**
 * @brief Circulating Current & Line Impedance Status Telemetry (AEP-725)
 */
typedef struct {
    double r_line_est_ohm;            /**< Online identified line resistance (Ω) */
    double x_line_est_ohm;            /**< Online identified line reactance (Ω) */
    double i_circ_d_a;                /**< Active circulating current component (A) */
    double i_circ_q_a;                /**< Reactive circulating current component (A) */
    double v_comp_d_v;                /**< D-axis virtual impedance voltage compensation (V) */
    double v_comp_q_v;                /**< Q-axis virtual impedance voltage compensation (V) */
    double suppression_ratio_pct;     /**< Circulating current suppression ratio (%) */
} vsg_circulating_status_t;

/**
 * @brief Subsynchronous Oscillation (SSO) & Supersynchronous Resonance (SSR) Damping Config (AEP-727)
 */
typedef struct {
    bool enable_sso_damping;          /**< Enable active SSO/SSR detection and damping injection */
    double sso_freq_min_hz;           /**< Lower search bound for SSO frequency band (Hz, typ. 5.0) */
    double sso_freq_max_hz;           /**< Upper search bound for SSO frequency band (Hz, typ. 45.0) */
    double sso_alarm_thresh_a;        /**< Current oscillation amplitude threshold to trigger alarm (A, typ. 2.0 ~ 10.0) */
    double notch_damping_ratio_zeta;  /**< Damping ratio zeta of adaptive notch filter (typ. 0.1 ~ 0.3) */
    double active_damp_gain;          /**< Anti-phase active damping injection gain K_damp (V/A, typ. 1.0 ~ 10.0) */
} vsg_sso_config_t;

/**
 * @brief SSO/SSR Online Detection & Damping Status (AEP-727)
 */
typedef struct {
    bool sso_detected;                /**< True when oscillation amplitude exceeds alarm threshold */
    double sso_freq_hz;               /**< Online identified dominant oscillation frequency (Hz) */
    double sso_amplitude_a;           /**< Estimated peak amplitude of subsynchronous current oscillation (A) */
    double damping_out_v;             /**< Active anti-phase damping compensation voltage output (V) */
    double energy_reduction_ratio_pct;/**< Oscillation energy suppression reduction percentage (%) */
} vsg_sso_status_t;

/**
 * @brief VSG Fault Ride-Through (FRT) State Machine (AEP-728)
 */
typedef enum {
    VSG_FRT_STATE_NORMAL   = 0,  /**< Grid voltage within normal bounds (0.9 ~ 1.1 p.u.) */
    VSG_FRT_STATE_LVRT     = 1,  /**< Low Voltage Ride-Through active (sag < 0.9 p.u.) */
    VSG_FRT_STATE_HVRT     = 2,  /**< High Voltage Ride-Through active (swell > 1.1 p.u.) */
    VSG_FRT_STATE_RECOVERY = 3,  /**< Post-fault smooth recovery state */
} vsg_frt_state_t;

/**
 * @brief VSG Fault Ride-Through (FRT) Physical Configuration (AEP-728)
 */
typedef struct {
    bool enable_frt;                  /**< Enable dynamic FRT state machine and reactive support */
    double u_lvrt_thresh_pu;          /**< LVRT activation threshold (p.u., typ. 0.90) */
    double u_hvrt_thresh_pu;          /**< HVRT activation threshold (p.u., typ. 1.10) */
    double k_lvrt_reactive_support;   /**< LVRT reactive current support slope K_lvrt (typ. 1.5 ~ 2.0) */
    double k_hvrt_reactive_support;   /**< HVRT reactive current support slope K_hvrt (typ. 1.5 ~ 2.0) */
    double i_nominal_a;               /**< Rated converter output current RMS (A) */
    double i_max_limit_a;             /**< Maximum allowable converter current limit (A, typ. 1.2 * I_N) */
    double k_trans_virtual_z;         /**< Transient virtual impedance gain for current clamping */
    double recovery_ramp_rate_pu_s;   /**< Active power recovery ramp rate after fault clearing (p.u./s, typ. 0.5) */
} vsg_frt_config_t;

/**
 * @brief VSG Fault Ride-Through (FRT) Status Telemetry (AEP-728)
 */
typedef struct {
    vsg_frt_state_t state;            /**< Current FRT state */
    double u_grid_pu;                 /**< Grid voltage amplitude (p.u.) */
    double i_q_support_a;             /**< Dynamic reactive current support injection setpoint (A) */
    double i_d_limit_a;               /**< Active current ceiling allowed during reactive priority (A) */
    double v_trans_drop_v;            /**< Transient virtual impedance voltage drop for current clamping (V) */
    double recovery_progress_pct;     /**< Post-fault power recovery progress (0.0% ~ 100.0%) */
    bool is_current_clamped;          /**< True when transient virtual impedance is actively clamping current */
} vsg_frt_status_t;

/**
 * @brief VSG Virtual Stator Impedance & Harmonic Damping Configuration (AEP-729)
 */
typedef struct {
    bool enable_virtual_z;            /**< Enable fundamental virtual stator impedance drop */
    bool enable_adaptive_rx;          /**< Enable online R/X ratio shaping decoupling */
    bool enable_harmonic_damping;     /**< Enable 5th/7th harmonic virtual impedance extraction and damping */
    double r_virtual_ohm;             /**< Fundamental virtual stator resistance R_v (Ω, typ. 0.01 ~ 0.5) */
    double x_virtual_ohm;             /**< Fundamental virtual stator reactance X_v (Ω, typ. 0.05 ~ 1.0) */
    double target_rx_ratio;           /**< Target equivalent (R_total / X_total) ratio for P-Q decoupling (typ. 0.1) */
    double r_line_est_ohm;            /**< Estimated grid line resistance R_line (Ω) */
    double x_line_est_ohm;            /**< Estimated grid line reactance X_line (Ω) */
    double k_harmonic_damping_h5;     /**< 5th harmonic virtual damping gain R_v5 (V/A, typ. 0.5 ~ 5.0) */
    double k_harmonic_damping_h7;     /**< 7th harmonic virtual damping gain R_v7 (V/A, typ. 0.5 ~ 5.0) */
    double harmonic_bandwidth_rad_s;  /**< Resonant filter bandwidth omega_cut (rad/s, typ. 5.0 ~ 20.0) */
} vsg_virtual_z_config_t;

/**
 * @brief VSG Virtual Stator Impedance & Harmonic Damping Status (AEP-729)
 */
typedef struct {
    double r_v_effective_ohm;         /**< Effective fundamental virtual resistance R_v */
    double x_v_effective_ohm;         /**< Effective fundamental virtual reactance X_v */
    double v_d_drop_fund_v;           /**< Fundamental D-axis virtual impedance voltage drop (V) */
    double v_q_drop_fund_v;           /**< Fundamental Q-axis virtual impedance voltage drop (V) */
    double v_d_damp_harm_v;           /**< Harmonic D-axis damping voltage compensation (V) */
    double v_q_damp_harm_v;           /**< Harmonic Q-axis damping voltage compensation (V) */
    double v_d_out_v;                 /**< Final shaped D-axis output reference voltage Vd* (V) */
    double v_q_out_v;                 /**< Final shaped Q-axis output reference voltage Vq* (V) */
    double i_h5_rms_a;                /**< Extracted 5th harmonic current RMS (A) */
    double i_h7_rms_a;                /**< Extracted 7th harmonic current RMS (A) */
    double thd_current_pct;           /**< Estimated output current Total Harmonic Distortion (%) */
    double thd_reduction_pct;         /**< Harmonic THD attenuation percentage achieved (%) */
} vsg_virtual_z_status_t;

/**
 * @brief Pre-synchronization & Seamless Transition Status (AEP-723)
 */
typedef struct {
    vsg_grid_mode_t current_mode;   /**< Current operation mode */
    bool sync_locked;               /**< True when phase, voltage, and frequency are locked within tolerances */
    double delta_theta_rad;         /**< Phase angle error vs grid (rad, [-pi, pi]) */
    double delta_volt_v;            /**< Voltage amplitude error vs grid (V) */
    double delta_freq_hz;           /**< Frequency error vs grid (Hz) */
    uint32_t lock_hold_cycles;      /**< Number of consecutive cycles tolerance held */
} vsg_transition_status_t;

/**
 * @brief Virtual Synchronous Generator Configuration & Physical Parameters
 */
typedef struct {
    double j_inertia;       /**< Virtual moment of inertia (kg·m² or s, typ. 0.1 ~ 5.0) */
    double d_damping;       /**< Damping factor D (N·m·s/rad, typ. 5.0 ~ 50.0) */
    double v_nominal;       /**< Nominal line-to-neutral or RMS voltage (V, typ. 220.0 or 380.0) */
    double f_nominal;       /**< Nominal grid frequency (Hz, typ. 50.0 or 60.0) */
    double p_ref;           /**< Active power setpoint (W) */
    double q_ref;           /**< Reactive power setpoint (var) */
    double droop_p;         /**< Frequency-Active power droop coefficient (Hz/W) */
    double droop_q;         /**< Voltage-Reactive power droop coefficient (V/var) */
    vsg_integrator_mode_t integrator_mode; /**< Numerical integrator mode (default RK4) */

    /* AEP-722 Adaptive Droop & Dynamic Damping Parameters */
    bool enable_adaptive_scr;     /**< Enable dynamic SCR-based adaptation */
    double scr_nominal;           /**< Baseline nominal grid Short-Circuit Ratio (typ. 3.0) */
    double scr_current;           /**< Current grid Short-Circuit Ratio (typ. 1.0 ~ 10.0) */
    double d_transient;           /**< Transient acceleration damping coefficient D_trans (s) */

    /* AEP-723 Seamless Transition & Pre-Synchronization Parameters */
    vsg_grid_mode_t grid_mode;      /**< Initial operating mode (default CONNECTED) */
    double sync_kp_theta;           /**< Pre-sync phase angle tracking proportional gain */
    double sync_ki_theta;           /**< Pre-sync phase angle tracking integral gain */
    double sync_kp_v;               /**< Pre-sync voltage magnitude tracking gain */
    double sync_theta_tol_rad;      /**< Sync lock tolerance for delta_theta (rad, typ. 0.05) */
    double sync_volt_tol_v;         /**< Sync lock tolerance for delta_V (V, typ. 2.0) */
    double sync_freq_tol_hz;        /**< Sync lock tolerance for delta_f (Hz, typ. 0.1) */
    double island_freq_slew_hz_s;   /**< Max frequency ramp rate when islanding (Hz/s, typ. 1.0) */
    double island_volt_slew_v_s;    /**< Max voltage ramp rate when islanding (V/s, typ. 20.0) */

    /* AEP-724 Microgrid Black Start & Zero-Voltage Soft Excitation */
    vsg_black_start_config_t black_start; /**< Black start physical configuration */

    /* AEP-725 Multi-PCS Line Impedance Identification & Circulating Current Suppression */
    vsg_circulating_suppress_config_t circ_suppress; /**< Circulating current suppression configuration */

    /* AEP-727 Subsynchronous Oscillation (SSO) Detection & Active Notch Damping */
    vsg_sso_config_t sso; /**< SSO/SSR detection and damping configuration */

    /* AEP-728 Fault Ride-Through (HVRT/LVRT) & Transient Current Limiting */
    vsg_frt_config_t frt; /**< Fault Ride-Through physical configuration */

    /* AEP-729 Virtual Stator Impedance (R/X) Decoupling & Harmonic Damping */
    vsg_virtual_z_config_t virtual_z; /**< Virtual stator impedance configuration */
} vsg_config_t;

/**
 * @brief Consumer-Defined Port Interfaces for VSG Controller
 */
typedef struct {
    edge_reader_port_t reader; /**< Telemetry input port */
    edge_writer_port_t writer; /**< Actuation output port */
    edge_clock_port_t  clock;  /**< Monotonic clock port */
} vsg_ports_t;

/**
 * @brief Opaque Aggregate Root Type for VSG Controller
 */
typedef struct vsg_controller vsg_controller_t;

/**
 * @brief Caller-Allocated Storage for VSG Controller (Physical Opaque Encapsulation)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[2048];
} vsg_controller_storage_t;

/**
 * @brief In-place placement constructor for VSG Controller
 *
 * @param store Caller-provided storage buffer
 * @param cfg Physical parameters and control setpoints
 * @param ports Injected consumer-defined ports bundle (optional, may be NULL)
 * @return vsg_controller_t* handle pointing into store, or NULL on error
 */
vsg_controller_t *vsg_controller_create_in(vsg_controller_storage_t *store,
                                           const vsg_config_t *cfg,
                                           const vsg_ports_t *ports);

/**
 * @brief Execute one control step of VSG Swing Equation & Droop
 *
 * Swing Equation:
 *   J * omega_0 * (d(omega)/dt) = P_ref + D_p*(f_0 - f) - P_meas - D * (omega - omega_0)
 *   d(theta)/dt = omega
 *
 * Voltage Control:
 *   E = V_nominal + D_q * (Q_ref - Q_meas)
 *
 * @param vsg Pointer to VSG controller instance
 * @param p_meas Measured active power from grid/PCS (W)
 * @param q_meas Measured reactive power from grid/PCS (var)
 * @param dt_s Sampling interval (seconds)
 * @param out_freq_hz Output virtual frequency setpoint (Hz)
 * @param out_voltage_v Output virtual voltage magnitude setpoint (V)
 * @return 0 on success, negative error code on invalid parameters
 */
int vsg_controller_step(vsg_controller_t *vsg, double p_meas, double q_meas,
                        double dt_s, double *out_freq_hz, double *out_voltage_v);

/**
 * @brief Step VSG controller using Consumer-Defined Ports
 */
int vsg_controller_step_ports(vsg_controller_t *vsg,
                              uint32_t tag_p_meas, uint32_t tag_q_meas,
                              uint32_t tag_freq_out, uint32_t tag_volt_out,
                              double dt_s);

/**
 * @brief Reset VSG internal virtual rotor state and integrator
 */
void vsg_controller_reset(vsg_controller_t *vsg);

/* -------------------------------------------------------------------------- */
/* State Inspectors                                                           */
/* -------------------------------------------------------------------------- */
double vsg_controller_get_frequency(const vsg_controller_t *vsg);
double vsg_controller_get_voltage(const vsg_controller_t *vsg);
double vsg_controller_get_omega(const vsg_controller_t *vsg);
double vsg_controller_get_theta(const vsg_controller_t *vsg);
vsg_integrator_mode_t vsg_controller_get_integrator_mode(const vsg_controller_t *vsg);
void vsg_controller_set_integrator_mode(vsg_controller_t *vsg, vsg_integrator_mode_t mode);
double vsg_controller_get_p_mech(const vsg_controller_t *vsg);
double vsg_controller_get_p_damping(const vsg_controller_t *vsg);
double vsg_controller_get_p_net(const vsg_controller_t *vsg);
uint64_t vsg_controller_get_step_count(const vsg_controller_t *vsg);

/* -------------------------------------------------------------------------- */
/* AEP-722 Adaptive Droop & Dynamic Damping Compensator APIs                   */
/* -------------------------------------------------------------------------- */
int vsg_controller_set_grid_scr(vsg_controller_t *vsg, double scr);
double vsg_controller_get_grid_scr(const vsg_controller_t *vsg);
double vsg_controller_get_effective_damping(const vsg_controller_t *vsg);
double vsg_controller_get_effective_droop_p(const vsg_controller_t *vsg);
double vsg_controller_get_effective_droop_q(const vsg_controller_t *vsg);

/* -------------------------------------------------------------------------- */
/* AEP-723 Seamless Grid-Forming Transition & Pre-Sync APIs                   */
/* -------------------------------------------------------------------------- */
int vsg_controller_set_mode(vsg_controller_t *vsg, vsg_grid_mode_t mode);
vsg_grid_mode_t vsg_controller_get_mode(const vsg_controller_t *vsg);
int vsg_controller_trigger_unplanned_islanding(vsg_controller_t *vsg, double grid_v, double grid_f, double grid_theta);
int vsg_controller_step_presync(vsg_controller_t *vsg, double grid_v, double grid_f, double grid_theta, double dt_s);
bool vsg_controller_is_sync_locked(const vsg_controller_t *vsg);
int vsg_controller_get_transition_status(const vsg_controller_t *vsg, vsg_transition_status_t *status);

/* -------------------------------------------------------------------------- */
/* AEP-724 Microgrid Black Start & Multi-Unit Lockstep Engine APIs            */
/* -------------------------------------------------------------------------- */
int vsg_controller_trigger_black_start(vsg_controller_t *vsg);
int vsg_controller_step_black_start(vsg_controller_t *vsg, double i_meas_a, double sync_angle_rad,
                                    double dt_s, double *out_freq_hz, double *out_voltage_v);
vsg_black_start_state_t vsg_controller_get_black_start_state(const vsg_controller_t *vsg);
int vsg_controller_get_black_start_status(const vsg_controller_t *vsg, vsg_black_start_status_t *status);
void vsg_controller_abort_black_start(vsg_controller_t *vsg);

/* -------------------------------------------------------------------------- */
/* AEP-725 Multi-PCS Line Impedance & Circulating Suppression APIs            */
/* -------------------------------------------------------------------------- */
int vsg_controller_set_circ_suppress_config(vsg_controller_t *vsg, const vsg_circulating_suppress_config_t *cfg);
int vsg_controller_step_circ_suppression(vsg_controller_t *vsg,
                                         double i_d_meas, double i_q_meas,
                                         double i_d_avg, double i_q_avg,
                                         double v_pcc_d, double v_pcc_q,
                                         double dt_s,
                                         double *out_v_d, double *out_v_q);
int vsg_controller_get_circ_status(const vsg_controller_t *vsg, vsg_circulating_status_t *status);

/* -------------------------------------------------------------------------- */
/* AEP-727 Subsynchronous Oscillation (SSO) Detection & Active Damping APIs   */
/* -------------------------------------------------------------------------- */
int vsg_controller_set_sso_config(vsg_controller_t *vsg, const vsg_sso_config_t *cfg);
int vsg_controller_step_sso_damping(vsg_controller_t *vsg, double i_raw_a, double dt_s,
                                    double *out_filtered_i_a, double *out_damping_v);
int vsg_controller_get_sso_status(const vsg_controller_t *vsg, vsg_sso_status_t *status);

/* -------------------------------------------------------------------------- */
/* AEP-728 Fault Ride-Through (HVRT/LVRT) & Dynamic Reactive Support APIs     */
/* -------------------------------------------------------------------------- */
int vsg_controller_set_frt_config(vsg_controller_t *vsg, const vsg_frt_config_t *cfg);
int vsg_controller_step_frt(vsg_controller_t *vsg, double grid_volt_v,
                            double id_meas, double iq_meas, double dt_s,
                            double *out_iq_support, double *out_p_derate,
                            double *out_v_trans_drop);
vsg_frt_state_t vsg_controller_get_frt_state(const vsg_controller_t *vsg);
int vsg_controller_get_frt_status(const vsg_controller_t *vsg, vsg_frt_status_t *status);

/* -------------------------------------------------------------------------- */
/* AEP-729 Virtual Stator Impedance (R/X) & Harmonic Damping APIs              */
/* -------------------------------------------------------------------------- */
int vsg_controller_set_virtual_z_config(vsg_controller_t *vsg, const vsg_virtual_z_config_t *cfg);
int vsg_controller_step_virtual_z(vsg_controller_t *vsg,
                                  double v_d_ref_in, double v_q_ref_in,
                                  double i_d_meas, double i_q_meas,
                                  double dt_s,
                                  double *out_v_d, double *out_v_q);
int vsg_controller_get_virtual_z_status(const vsg_controller_t *vsg, vsg_virtual_z_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALGO_VSG_CONTROL_H */

