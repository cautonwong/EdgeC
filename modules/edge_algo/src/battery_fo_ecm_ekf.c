#include "battery_fo_ecm_ekf.h"

#include <math.h>
#include <string.h>

/* Monotonic LiFePO4 / NMC OCV-SOC 5th-order polynomial coefficients */
static const double OCV_POLY[6] = {
    3.008315084,  /* c0: OCV at 0% SOC */
    3.182119151,  /* c1 */
    -13.57559473, /* c2 */
    29.25277462,  /* c3 */
    -30.86924070, /* c4 */
    13.20932326   /* c5 */
};

double fo_ecm_ocv_lookup(double soc) {
    if (soc < 0.0) soc = 0.0;
    if (soc > 1.0) soc = 1.0;
    
    double val = OCV_POLY[5];
    for (int i = 4; i >= 0; --i) {
        val = val * soc + OCV_POLY[i];
    }
    return val;
}

double fo_ecm_docv_dsoc(double soc) {
    if (soc < 0.0) soc = 0.0;
    if (soc > 1.0) soc = 1.0;
    
    double val = 5.0 * OCV_POLY[5];
    for (int i = 4; i >= 1; --i) {
        val = val * soc + (double)i * OCV_POLY[i];
    }
    return val;
}

static void compute_gl_weights(double alpha, double *weights, size_t len) {
    if (!weights || len == 0) return;
    weights[0] = 1.0;
    for (size_t j = 1; j < len; ++j) {
        weights[j] = (1.0 - (1.0 + alpha) / (double)j) * weights[j - 1];
    }
}

bool fo_ecm_ekf_init(fo_ecm_ekf_t *ekf, const fo_ecm_params_t *params,
                     double initial_soc, double initial_temp_c, double dt_s) {
    if (!ekf || !params || dt_s <= 0.0) return false;
    if (params->alpha <= 0.0 || params->alpha > 1.0) return false;
    if (params->capacity_ah <= 0.0 || params->r0 <= 0.0 || params->rp <= 0.0 || params->c_alpha <= 0.0) {
        return false;
    }

    memset(ekf, 0, sizeof(fo_ecm_ekf_t));
    ekf->params = *params;
    
    /* Clamp initial SOC */
    if (initial_soc < 0.0) initial_soc = 0.0;
    if (initial_soc > 1.0) initial_soc = 1.0;
    ekf->soc = initial_soc;
    ekf->up = 0.0;
    ekf->temperature_c = initial_temp_c;
    
    /* Precompute Grünwald-Letnikov weights */
    compute_gl_weights(ekf->params.alpha, ekf->gl_weights, FO_ECM_MEMORY_LENGTH);
    
    /* Initial Covariance Matrix P */
    ekf->p[0][0] = 0.04; /* Standard 20% initial SOC uncertainty */
    ekf->p[0][1] = 0.0;
    ekf->p[1][0] = 0.0;
    ekf->p[1][1] = 1e-3; /* Initial polarization voltage covariance */
    
    /* Process Noise Covariance Q */
    ekf->q[0][0] = 1e-6; /* SOC drift process noise */
    ekf->q[0][1] = 0.0;
    ekf->q[1][0] = 0.0;
    ekf->q[1][1] = 1e-5; /* Fractional Up process noise */
    
    /* Measurement Noise Covariance R */
    ekf->r_meas = 5e-4;  /* Voltage sensor variance */
    
    /* Initialize history window */
    for (int i = 0; i < FO_ECM_MEMORY_LENGTH; ++i) {
        ekf->history_up[i] = 0.0;
    }
    ekf->memory_count = 0;
    ekf->moving_residual_sq = 0.0;
    ekf->step_count = 0;
    ekf->is_converged = false;
    
    return true;
}

void fo_ecm_ekf_reset(fo_ecm_ekf_t *ekf, double soc, double temp_c) {
    if (!ekf) return;
    if (soc < 0.0) soc = 0.0;
    if (soc > 1.0) soc = 1.0;
    ekf->soc = soc;
    ekf->up = 0.0;
    ekf->temperature_c = temp_c;
    
    ekf->p[0][0] = 0.04;
    ekf->p[0][1] = 0.0;
    ekf->p[1][0] = 0.0;
    ekf->p[1][1] = 1e-3;
    
    for (int i = 0; i < FO_ECM_MEMORY_LENGTH; ++i) {
        ekf->history_up[i] = 0.0;
    }
    ekf->memory_count = 0;
    ekf->moving_residual_sq = 0.0;
    ekf->step_count = 0;
    ekf->is_converged = false;
}

bool fo_ecm_ekf_step(fo_ecm_ekf_t *ekf, double current_a, double voltage_v,
                     double temp_c, double dt_s) {
    if (!ekf || dt_s <= 0.0) return false;

    ekf->current_meas = current_a;
    ekf->terminal_voltage_meas = voltage_v;
    ekf->temperature_c = temp_c;
    ekf->step_count++;

    /* 1. Temperature-compensated Ohmic resistance (lower temp -> higher internal resistance) */
    double temp_diff = temp_c - ekf->params.temp_ref_c;
    double r0_actual = ekf->params.r0 * (1.0 - ekf->params.temp_coeff_r0 * temp_diff);
    if (r0_actual < 1e-5) r0_actual = 1e-5;

    /* 2. Fractional Time Factor Ts^alpha */
    double dt_alpha = pow(dt_s, ekf->params.alpha);
    double denom = 1.0 + dt_alpha / (ekf->params.rp * ekf->params.c_alpha);
    if (denom < 1e-6) denom = 1e-6;

    /* Fractional History Sum: sum_{j=1}^{L-1} w_j * U_p(k-j) */
    double history_sum = 0.0;
    uint32_t active_len = ekf->memory_count < FO_ECM_MEMORY_LENGTH ? ekf->memory_count : FO_ECM_MEMORY_LENGTH;
    for (uint32_t j = 1; j < active_len; ++j) {
        history_sum += ekf->gl_weights[j] * ekf->history_up[j - 1];
    }

    /* A. State Prediction: x_prior = [SOC_prior, Up_prior]^T */
    /* Coulomb counting: current_a > 0 is charge (SOC increases) */
    double delta_soc = (ekf->params.coulombic_eff * dt_s * current_a) / (3600.0 * ekf->params.capacity_ah);
    double soc_prior = ekf->soc + delta_soc;
    if (soc_prior < 0.0) soc_prior = 0.0;
    if (soc_prior > 1.0) soc_prior = 1.0;

    /* Fractional polarization voltage prediction */
    double up_prior = (-history_sum + (dt_alpha / ekf->params.c_alpha) * current_a) / denom;

    /* B. State Transition Jacobian Matrix A */
    /* w_1 = -alpha, so -w_1 = alpha */
    double a11 = (-ekf->gl_weights[1]) / denom;
    double p00_prior = ekf->p[0][0] + ekf->q[0][0];
    double p01_prior = ekf->p[0][1] * a11;
    double p10_prior = a11 * ekf->p[1][0];
    double p11_prior = a11 * ekf->p[1][1] * a11 + ekf->q[1][1];

    /* C. Output Prediction & Measurement Jacobian H */
    double ocv_est = fo_ecm_ocv_lookup(soc_prior);
    ekf->ocv_est = ocv_est;
    
    /* V_terminal = OCV + Up + I * R0 */
    double v_est = ocv_est + up_prior + current_a * r0_actual;
    ekf->terminal_voltage_est = v_est;

    /* Observation Jacobian H = [dOCV/dSOC, 1.0] */
    double h0 = fo_ecm_docv_dsoc(soc_prior);
    double h1 = 1.0;

    /* D. Innovation (Residual) */
    double residual = voltage_v - v_est;
    ekf->residual = residual;

    /* Innovation Covariance S = H * P_prior * H^T + R */
    double s = h0 * (p00_prior * h0 + p01_prior * h1) +
               h1 * (p10_prior * h0 + p11_prior * h1) + ekf->r_meas;
    if (s < 1e-9) s = 1e-9;

    /* E. Kalman Gain K = P_prior * H^T / S */
    double k0 = (p00_prior * h0 + p01_prior * h1) / s;
    double k1 = (p10_prior * h0 + p11_prior * h1) / s;

    /* F. Posterior State Update */
    double soc_post = soc_prior + k0 * residual;
    if (soc_post < 0.0) soc_post = 0.0;
    if (soc_post > 1.0) soc_post = 1.0;
    double up_post = up_prior + k1 * residual;

    ekf->soc = soc_post;
    ekf->up = up_post;

    /* G. Posterior Covariance Update: P_post = (I - K * H) * P_prior */
    double ikh00 = 1.0 - k0 * h0;
    double ikh01 = -k0 * h1;
    double ikh10 = -k1 * h0;
    double ikh11 = 1.0 - k1 * h1;

    ekf->p[0][0] = ikh00 * p00_prior + ikh01 * p10_prior;
    ekf->p[0][1] = ikh00 * p01_prior + ikh01 * p11_prior;
    ekf->p[1][0] = ikh10 * p00_prior + ikh11 * p10_prior;
    ekf->p[1][1] = ikh10 * p01_prior + ikh11 * p11_prior;

    /* Ensure P remains positive semi-definite */
    if (ekf->p[0][0] < 1e-9) ekf->p[0][0] = 1e-9;
    if (ekf->p[1][1] < 1e-9) ekf->p[1][1] = 1e-9;

    /* H. Update Fractional History Window (Shift Right & Insert New Up) */
    for (int i = FO_ECM_MEMORY_LENGTH - 1; i > 0; --i) {
        ekf->history_up[i] = ekf->history_up[i - 1];
    }
    ekf->history_up[0] = up_post;
    if (ekf->memory_count < FO_ECM_MEMORY_LENGTH) {
        ekf->memory_count++;
    }

    /* I. Sage-Husa Adaptive Measurement Noise Covariance Update (steady state) */
    double res_sq = residual * residual;
    double alpha_adapt = 0.02; /* Adaptive forget factor */
    ekf->moving_residual_sq = (1.0 - alpha_adapt) * ekf->moving_residual_sq + alpha_adapt * res_sq;
    
    if (ekf->step_count > 20) {
        double r_adaptive = ekf->moving_residual_sq - (h0 * p00_prior * h0 + h1 * p11_prior * h1);
        if (r_adaptive > 1e-4 && r_adaptive < 0.01) {
            ekf->r_meas = (1.0 - alpha_adapt) * ekf->r_meas + alpha_adapt * r_adaptive;
        }
    }

    /* Convergence Check: low residual variance over consecutive steps */
    if (ekf->step_count > 10 && fabs(residual) < 0.015) {
        ekf->is_converged = true;
    }

    return true;
}
