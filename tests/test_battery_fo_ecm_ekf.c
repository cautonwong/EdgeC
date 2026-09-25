#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battery_fo_ecm_ekf.h"

static void test_initialization_and_validation(void) {
    printf("[TEST] Testing FO-ECM EKF Initialization and Bounds...\n");

    fo_ecm_params_t params = {.alpha = 0.75,
                              .r0 = 0.005,
                              .rp = 0.008,
                              .c_alpha = 1200.0,
                              .capacity_ah = 100.0,
                              .coulombic_eff = 0.998,
                              .temp_coeff_r0 = 0.004,
                              .temp_ref_c = 25.0};

    fo_ecm_ekf_storage_t store;
    fo_ecm_ekf_t *ekf = fo_ecm_ekf_create_in(&store, &params, 0.60, 25.0, 1.0);
    assert(ekf != NULL);
    assert(fabs(fo_ecm_ekf_get_soc(ekf) - 0.60) < 1e-6);
    assert(fabs(fo_ecm_ekf_get_up(ekf)) < 1e-6);
    assert(fo_ecm_ekf_get_gl_weight(ekf, 0) == 1.0);
    assert(fabs(fo_ecm_ekf_get_gl_weight(ekf, 1) - (-0.75)) < 1e-6);

    /* Invalid parameters checks */
    fo_ecm_params_t invalid_params = params;
    invalid_params.alpha = 1.5; /* Out of fractional bounds */
    assert(fo_ecm_ekf_create_in(&store, &invalid_params, 0.5, 25.0, 1.0) == NULL);

    invalid_params = params;
    invalid_params.capacity_ah = -10.0;
    assert(fo_ecm_ekf_create_in(&store, &invalid_params, 0.5, 25.0, 1.0) == NULL);

    printf("  --> Initialization & validation tests PASSED!\n");
}

static void test_ocv_and_derivative(void) {
    printf("[TEST] Testing OCV Lookup & Jacobian Derivative...\n");

    double ocv_0 = fo_ecm_ocv_lookup(0.0);
    double ocv_50 = fo_ecm_ocv_lookup(0.5);
    double ocv_100 = fo_ecm_ocv_lookup(1.0);

    /* LiFePO4 / NMC OCV should increase monotonically with SOC */
    assert(ocv_0 > 2.8 && ocv_0 < 3.2);
    assert(ocv_50 > 3.2 && ocv_50 < 3.4);
    assert(ocv_100 > 3.5 && ocv_100 <= 4.25);
    assert(ocv_100 > ocv_50 && ocv_50 > ocv_0);

    /* Verify derivative dOCV/dSOC is positive and consistent with finite difference */
    double eps = 1e-4;
    for (double s = 0.1; s <= 0.9; s += 0.2) {
        double d_analytical = fo_ecm_docv_dsoc(s);
        double d_numerical =
            (fo_ecm_ocv_lookup(s + eps) - fo_ecm_ocv_lookup(s - eps)) / (2.0 * eps);
        assert(d_analytical > 0.0);
        assert(fabs(d_analytical - d_numerical) < 0.05);
    }

    printf("  --> OCV lookup & derivative tests PASSED!\n");
}

static void test_constant_current_convergence(void) {
    printf("[TEST] Testing EKF SOC Convergence under CC Discharge with Initial Bias...\n");

    fo_ecm_params_t params = {.alpha = 0.80,
                              .r0 = 0.004,
                              .rp = 0.006,
                              .c_alpha = 1500.0,
                              .capacity_ah = 100.0,
                              .coulombic_eff = 1.0,
                              .temp_coeff_r0 = 0.003,
                              .temp_ref_c = 25.0};

    fo_ecm_ekf_storage_t store;
    /* True SOC starts at 0.70, but EKF guess is intentionally biased to 0.50 (20% error) */
    double true_soc = 0.70;
    double biased_guess_soc = 0.50;
    double dt = 1.0;
    double current = -20.0; /* 20A discharge */

    fo_ecm_ekf_t *ekf = fo_ecm_ekf_create_in(&store, &params, biased_guess_soc, 25.0, dt);
    assert(ekf != NULL);

    double true_history_up[FO_ECM_MEMORY_LENGTH] = {0};
    double gl_w[FO_ECM_MEMORY_LENGTH] = {0};
    gl_w[0] = 1.0;
    for (int j = 1; j < FO_ECM_MEMORY_LENGTH; ++j) {
        gl_w[j] = (1.0 - (1.0 + params.alpha) / (double)j) * gl_w[j - 1];
    }

    double dt_alpha = pow(dt, params.alpha);
    double denom = 1.0 + dt_alpha / (params.rp * params.c_alpha);

    for (int step = 0; step < 120; ++step) {
        /* Physical fractional cell simulator (Ground Truth) */
        true_soc += (params.coulombic_eff * dt * current) / (3600.0 * params.capacity_ah);

        double hist_sum = 0.0;
        int active = step < FO_ECM_MEMORY_LENGTH ? step : FO_ECM_MEMORY_LENGTH;
        for (int j = 1; j < active; ++j) {
            hist_sum += gl_w[j] * true_history_up[j - 1];
        }
        double true_up = (-hist_sum + (dt_alpha / params.c_alpha) * current) / denom;
        for (int j = FO_ECM_MEMORY_LENGTH - 1; j > 0; --j) {
            true_history_up[j] = true_history_up[j - 1];
        }
        true_history_up[0] = true_up;

        double true_vt = fo_ecm_ocv_lookup(true_soc) + true_up + current * params.r0;

        /* Add small sensor noise */
        double meas_vt = true_vt + 0.001 * ((step % 3) - 1.0);

        /* Step EKF */
        fo_ecm_ekf_step(ekf, current, meas_vt, 25.0, dt);
    }

    double final_soc_error = fabs(fo_ecm_ekf_get_soc(ekf) - true_soc);
    printf(
        "  --> Initial error: 20.0%%, Final error after 120s: %.3f%% (Estimated: %.4f, True: "
        "%.4f)\n",
        final_soc_error * 100.0, fo_ecm_ekf_get_soc(ekf), true_soc);

    /* Error must converge to within 1.5% */
    assert(final_soc_error < 0.015);
    assert(fo_ecm_ekf_is_converged(ekf) == true);
    printf("  --> Convergence from 20%% initial bias PASSED!\n");
}

static void test_fractional_relaxation_and_pulses(void) {
    printf("[TEST] Testing Fractional-Order Long-Tail Memory & Pulse Dynamics...\n");

    fo_ecm_params_t params = {.alpha = 0.70,
                              .r0 = 0.005,
                              .rp = 0.010,
                              .c_alpha = 800.0,
                              .capacity_ah = 50.0,
                              .coulombic_eff = 0.995,
                              .temp_coeff_r0 = 0.004,
                              .temp_ref_c = 25.0};

    fo_ecm_ekf_storage_t store;
    fo_ecm_ekf_t *ekf = fo_ecm_ekf_create_in(&store, &params, 0.80, 25.0, 0.5);
    assert(ekf != NULL);

    /* 1. Apply high discharge pulse (-50A for 10s) */
    for (int i = 0; i < 20; ++i) {
        double current = -50.0;
        double vt = fo_ecm_ocv_lookup(fo_ecm_ekf_get_soc(ekf)) - 0.05 + current * params.r0;
        fo_ecm_ekf_step(ekf, current, vt, 25.0, 0.5);
    }
    double up_after_pulse = fo_ecm_ekf_get_up(ekf);
    assert(up_after_pulse < 0.0); /* Negative polarization during discharge */

    /* 2. Rest period (0A current) -> Fractional diffusion relaxation */
    for (int i = 0; i < 30; ++i) {
        double current = 0.0;
        double vt = fo_ecm_ocv_lookup(fo_ecm_ekf_get_soc(ekf)) + fo_ecm_ekf_get_up(ekf);
        fo_ecm_ekf_step(ekf, current, vt, 25.0, 0.5);
    }

    /* Polarization should decay towards 0, but preserve long-tail fractional memory */
    assert(fabs(fo_ecm_ekf_get_up(ekf)) < fabs(up_after_pulse));
    printf("  --> Pulse polarization: %.4f V, Relaxed polarization: %.4f V\n", up_after_pulse,
           fo_ecm_ekf_get_up(ekf));
    printf("  --> Fractional memory relaxation PASSED!\n");
}

static void test_temperature_compensation(void) {
    printf("[TEST] Testing Arrhenius Temperature Compensation on Ohmic Resistance...\n");

    fo_ecm_params_t params = {.alpha = 0.75,
                              .r0 = 0.010,
                              .rp = 0.015,
                              .c_alpha = 1000.0,
                              .capacity_ah = 60.0,
                              .coulombic_eff = 1.0,
                              .temp_coeff_r0 = 0.005,
                              .temp_ref_c = 25.0};

    fo_ecm_ekf_storage_t store;
    fo_ecm_ekf_t *ekf = fo_ecm_ekf_create_in(&store, &params, 0.50, 25.0, 1.0);
    assert(ekf != NULL);

    /* Step at 25°C */
    fo_ecm_ekf_step(ekf, -10.0, 3.25, 25.0, 1.0);
    double v_est_25 = fo_ecm_ekf_get_terminal_voltage_est(ekf);

    /* Step at 0°C (Resistance higher -> larger IR drop -> lower terminal voltage during discharge)
     */
    fo_ecm_ekf_reset(ekf, 0.50, 0.0);
    fo_ecm_ekf_step(ekf, -10.0, 3.20, 0.0, 1.0);
    double v_est_0 = fo_ecm_ekf_get_terminal_voltage_est(ekf);

    /* Step at 45°C (Resistance lower -> smaller IR drop -> higher terminal voltage during
     * discharge) */
    fo_ecm_ekf_reset(ekf, 0.50, 45.0);
    fo_ecm_ekf_step(ekf, -10.0, 3.28, 45.0, 1.0);
    double v_est_45 = fo_ecm_ekf_get_terminal_voltage_est(ekf);

    assert(v_est_0 < v_est_25);
    assert(v_est_25 < v_est_45);

    printf("  --> Terminal voltages under -10A discharge: 0°C=%.4fV, 25°C=%.4fV, 45°C=%.4fV\n",
           v_est_0, v_est_25, v_est_45);
    printf("  --> Temperature compensation PASSED!\n");
}

int main(void) {
    printf(
        "=== EdgeOS Battery FO-ECM Adaptive Extended Kalman Filter Tests (AEP-731 / AEP-752) "
        "===\n");
    test_initialization_and_validation();
    test_ocv_and_derivative();
    test_constant_current_convergence();
    test_fractional_relaxation_and_pulses();
    test_temperature_compensation();
    printf("=== All Battery FO-ECM EKF Tests PASSED successfully! ===\n");
    return 0;
}
