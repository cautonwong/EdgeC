#ifndef EDGE_ALGO_BATTERY_FO_ECM_EKF_H
#define EDGE_ALGO_BATTERY_FO_ECM_EKF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
 * @brief Adaptive Fractional-Order Extended Kalman Filter (FO-EKF) State
 */
typedef struct {
    fo_ecm_params_t params;  /**< Model physical parameters */
    
    /* State Vector x = [SOC, Up]^T */
    double soc;              /**< Estimated State of Charge (0.0 to 1.0) */
    double up;               /**< Estimated fractional polarization voltage (V) */
    
    /* Voltage and Current Estimates */
    double terminal_voltage_est;  /**< Estimated terminal voltage (V) */
    double terminal_voltage_meas; /**< Measured terminal voltage (V) */
    double current_meas;          /**< Measured current (A, >0 charge, <0 discharge) */
    double temperature_c;         /**< Current cell temperature (°C) */
    double ocv_est;               /**< Estimated Open-Circuit Voltage (V) */
    double residual;              /**< Voltage innovation: y_k - y_est (V) */
    
    /* Covariance Matrices */
    double p[2][2];          /**< Error covariance matrix P */
    double q[2][2];          /**< Process noise covariance matrix Q */
    double r_meas;           /**< Measurement noise covariance R */
    
    /* Grünwald-Letnikov History Window */
    double history_up[FO_ECM_MEMORY_LENGTH]; /**< Past polarization voltages */
    double gl_weights[FO_ECM_MEMORY_LENGTH]; /**< Precomputed GL binomial coefficients */
    uint32_t memory_count;                   /**< Current filled memory count */
    
    /* Adaptive Sage-Husa / Innovation Tracking */
    double moving_residual_sq; /**< Low-pass filtered innovation square */
    uint32_t step_count;       /**< Total execution steps */
    bool is_converged;         /**< Convergence status */
} fo_ecm_ekf_t;

/**
 * @brief Initialize FO-ECM Adaptive Extended Kalman Filter
 * 
 * @param ekf Pointer to EKF instance
 * @param params Physical cell model parameters
 * @param initial_soc Initial guess for SOC (0.0 ~ 1.0)
 * @param initial_temp_c Initial cell temperature (°C)
 * @param dt_s Sampling interval (seconds)
 * @return true on success, false on invalid parameters
 */
bool fo_ecm_ekf_init(fo_ecm_ekf_t *ekf, const fo_ecm_params_t *params,
                     double initial_soc, double initial_temp_c, double dt_s);

/**
 * @brief Compute Open Circuit Voltage (OCV) from SOC using high-precision 7th-order polynomial
 * 
 * @param soc State of Charge (0.0 ~ 1.0)
 * @return OCV in Volts (V)
 */
double fo_ecm_ocv_lookup(double soc);

/**
 * @brief Compute derivative d(OCV)/d(SOC) for Jacobian observation matrix H
 * 
 * @param soc State of Charge (0.0 ~ 1.0)
 * @return Derivative in V/unit_SOC
 */
double fo_ecm_docv_dsoc(double soc);

/**
 * @brief Execute one time-step update of FO-ECM Adaptive EKF
 * 
 * Performs:
 * 1. Temperature-compensated Ohmic resistance calculation
 * 2. Grünwald-Letnikov fractional state prediction
 * 3. Terminal voltage observation & Jacobian evaluation
 * 4. Kalman gain calculation & state posterior update
 * 5. Sage-Husa adaptive measurement noise covariance adjustment
 * 
 * @param ekf Pointer to EKF instance
 * @param current_a Measured current (A, positive = charge, negative = discharge)
 * @param voltage_v Measured terminal voltage (V)
 * @param temp_c Measured temperature (°C)
 * @param dt_s Actual elapsed time step (seconds)
 * @return true if step executed cleanly
 */
bool fo_ecm_ekf_step(fo_ecm_ekf_t *ekf, double current_a, double voltage_v,
                     double temp_c, double dt_s);

/**
 * @brief Reset or re-seed EKF state without losing physical parameters
 * 
 * @param ekf Pointer to EKF instance
 * @param soc New SOC seed (0.0 ~ 1.0)
 * @param temp_c New temperature (°C)
 */
void fo_ecm_ekf_reset(fo_ecm_ekf_t *ekf, double soc, double temp_c);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALGO_BATTERY_FO_ECM_EKF_H */
