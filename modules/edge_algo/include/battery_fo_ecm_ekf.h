#ifndef EDGE_ALGO_BATTERY_FO_ECM_EKF_H
#define EDGE_ALGO_BATTERY_FO_ECM_EKF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum Grünwald-Letnikov memory length for fractional derivation */
#define FO_ECM_MEMORY_LENGTH 24

/**
 * @brief Fractional-Order Equivalent Circuit Model (FO-ECM) Physical Parameters
 */
typedef struct {
    double alpha;            /**< Fractional order (0.0 < alpha <= 1.0, typ. 0.75) */
    double r0;               /**< Nominal Ohmic resistance at 25°C (Ohms, typ. 0.001 ~ 0.015) */
    double rp;               /**< Polarization resistance (Ohms, typ. 0.002 ~ 0.020) */
    double c_alpha;          /**< Fractional capacitance (F·s^(alpha-1), typ. 500 ~ 5000) */
    double capacity_ah;      /**< Nominal battery cell capacity (Ah) */
    double coulombic_eff;    /**< Coulombic efficiency (typ. 0.995 ~ 1.0) */
    double temp_coeff_r0;    /**< Arrhenius thermal sensitivity coefficient (1/K) */
    double temp_ref_c;       /**< Reference temperature (°C, typ. 25.0) */
} fo_ecm_params_t;

/**
 * @brief Opaque Aggregate Root Type for FO-ECM Adaptive EKF (AEP-752)
 *
 * Internal state fields are strictly encapsulated within battery_fo_ecm_ekf.c.
 */
typedef struct fo_ecm_ekf fo_ecm_ekf_t;

/**
 * @brief Caller-Allocated Storage for FO-ECM EKF (Physical Opaque Encapsulation)
 *
 * Callers allocate this aligned placeholder on the stack or static .bss section.
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[1024];
} fo_ecm_ekf_storage_t;

/**
 * @brief In-place constructor for FO-ECM EKF using Caller-Allocated Storage
 *
 * @param store Pointer to caller-allocated storage buffer
 * @param params Physical cell model parameters
 * @param initial_soc Initial guess for SOC (0.0 ~ 1.0)
 * @param initial_temp_c Initial cell temperature (°C)
 * @param dt_s Sampling interval (seconds)
 * @return fo_ecm_ekf_t* handle pointing into store, or NULL on error
 */
fo_ecm_ekf_t *fo_ecm_ekf_create_in(fo_ecm_ekf_storage_t *store,
                                   const fo_ecm_params_t *params,
                                   double initial_soc, double initial_temp_c, double dt_s);

/**
 * @brief Initialize FO-ECM Adaptive Extended Kalman Filter (In-place on existing instance)
 */
bool fo_ecm_ekf_init(fo_ecm_ekf_t *ekf, const fo_ecm_params_t *params,
                     double initial_soc, double initial_temp_c, double dt_s);

/**
 * @brief Compute Open Circuit Voltage (OCV) from SOC using high-precision 7th-order polynomial
 */
double fo_ecm_ocv_lookup(double soc);

/**
 * @brief Compute derivative d(OCV)/d(SOC) for Jacobian observation matrix H
 */
double fo_ecm_docv_dsoc(double soc);

/**
 * @brief Execute one time-step update of FO-ECM Adaptive EKF
 */
bool fo_ecm_ekf_step(fo_ecm_ekf_t *ekf, double current_a, double voltage_v,
                     double temp_c, double dt_s);

/**
 * @brief Reset or re-seed EKF state without losing physical parameters
 */
void fo_ecm_ekf_reset(fo_ecm_ekf_t *ekf, double soc, double temp_c);

/* -------------------------------------------------------------------------- */
/* State & Telemetry Inspectors (Opaque Encapsulation Getters)                 */
/* -------------------------------------------------------------------------- */
double fo_ecm_ekf_get_soc(const fo_ecm_ekf_t *ekf);
double fo_ecm_ekf_get_up(const fo_ecm_ekf_t *ekf);
double fo_ecm_ekf_get_terminal_voltage_est(const fo_ecm_ekf_t *ekf);
double fo_ecm_ekf_get_ocv_est(const fo_ecm_ekf_t *ekf);
double fo_ecm_ekf_get_residual(const fo_ecm_ekf_t *ekf);
bool fo_ecm_ekf_is_converged(const fo_ecm_ekf_t *ekf);
double fo_ecm_ekf_get_gl_weight(const fo_ecm_ekf_t *ekf, size_t index);
uint32_t fo_ecm_ekf_get_step_count(const fo_ecm_ekf_t *ekf);

/* -------------------------------------------------------------------------- */
/* Consumer-Defined Port Driven Interface (AEP-751)                          */
/* -------------------------------------------------------------------------- */

struct edge_reader_port;
struct edge_writer_port;
struct edge_clock_port;

typedef struct {
    uint32_t tag_current_a;
    uint32_t tag_voltage_v;
    uint32_t tag_temp_c;
    uint32_t tag_soc_est;
    uint32_t tag_ocv_est;
} fo_ecm_port_binding_t;

/**
 * @brief Consumer-Port driven execution: reads inputs from Reader Port, executes EKF, writes output to Writer Port
 */
bool fo_ecm_ekf_step_ports(fo_ecm_ekf_t *ekf,
                           const struct edge_reader_port *reader,
                           const struct edge_writer_port *writer,
                           const struct edge_clock_port *clock,
                           const fo_ecm_port_binding_t *binding,
                           double dt_s);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALGO_BATTERY_FO_ECM_EKF_H */
