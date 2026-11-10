#include "vsg_control_internal.h"

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

