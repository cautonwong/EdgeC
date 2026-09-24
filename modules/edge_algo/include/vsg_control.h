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
    alignas(max_align_t) uint8_t raw[512];
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

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALGO_VSG_CONTROL_H */
