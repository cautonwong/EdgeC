/**
 * @file test_vsg_control.c
 * @brief Unit Tests for Virtual Synchronous Generator (VSG) Control (AEP-723 ~ AEP-730 / Issue
 * #340)
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vsg_control.h"

static void test_vsg_initialization_and_bounds(void) {
    printf("[TEST] Testing VSG Controller Initialization & Bounds...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 2.5,
                        .d_damping = 25.0,
                        .v_nominal = 220.0,
                        .f_nominal = 50.0,
                        .p_ref = 100000.0, /* 100 kW */
                        .q_ref = 0.0,
                        .droop_p = 1e-4,
                        .droop_q = 1e-3};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);
    assert(fabs(vsg_controller_get_frequency(vsg) - 50.0) < 1e-6);
    assert(fabs(vsg_controller_get_voltage(vsg) - 220.0) < 1e-6);

    /* Test invalid params */
    vsg_config_t bad_cfg = cfg;
    bad_cfg.j_inertia = -1.0;
    assert(vsg_controller_create_in(&store, &bad_cfg, NULL) == NULL);

    printf("  --> VSG initialization tests PASSED!\n");
}

static void test_vsg_swing_dynamics_and_damping(void) {
    printf("[TEST] Testing VSG Swing Dynamics and Damping...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 0.5,
                        .d_damping = 80.0,
                        .v_nominal = 220.0,
                        .f_nominal = 50.0,
                        .p_ref = 10000.0,
                        .q_ref = 0.0,
                        .droop_p = 0.0,
                        .droop_q = 0.0};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* Step load increase: P_meas = 15000 W (> P_ref 10000 W) -> Frequency drops */
    double f_out = 0.0, v_out = 0.0;
    for (int i = 0; i < 50; ++i) {
        vsg_controller_step(vsg, 15000.0, 0.0, 0.01, &f_out, &v_out);
    }
    assert(f_out < 50.0);
    assert(f_out > 45.0);

    /* Now return load to P_ref -> Frequency recovers to 50.0 Hz */
    for (int i = 0; i < 1000; ++i) {
        vsg_controller_step(vsg, 10000.0, 0.0, 0.01, &f_out, &v_out);
    }
    assert(fabs(f_out - 50.0) < 0.05);

    printf("  --> VSG swing dynamics & damping PASSED!\n");
}

static void test_vsg_reactive_droop(void) {
    printf("[TEST] Testing VSG Reactive Power - Voltage Droop...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 1.0,
                        .d_damping = 10.0,
                        .v_nominal = 220.0,
                        .f_nominal = 50.0,
                        .p_ref = 0.0,
                        .q_ref = 0.0,
                        .droop_p = 0.0,
                        .droop_q = 0.002};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* Inductive load Q_meas = 5000 var -> Voltage drops */
    double f_out = 0.0, v_out = 0.0;
    for (int i = 0; i < 100; ++i) {
        vsg_controller_step(vsg, 0.0, 5000.0, 0.01, &f_out, &v_out);
    }
    assert(v_out < 220.0);
    assert(v_out > 200.0);

    printf("  --> VSG reactive droop PASSED!\n");
}

static void test_vsg_rk4_high_precision_against_analytical(void) {
    printf("[TEST] Testing VSG RK4 High-Precision vs Analytical Ground Truth (AEP-721)...\n");

    /* System physical parameters */
    double J = 2.0;
    double D = 20.0;
    double f0 = 50.0;
    double V0 = 220.0;
    double Pref = 50000.0;
    double Pmeas = 75000.0; /* Step load increase +25 kW */
    double droop_p = 1e-4;  /* 0.0001 Hz/W */
    double dt = 0.05;       /* 50ms time step (typical supervisory cycle) */
    double T_total = 2.0;   /* 2.0 seconds transient duration */
    int steps = (int)(T_total / dt);

    double omega0 = 2.0 * M_PI * f0;
    double K_tot = D + 1.0 / (2.0 * M_PI * droop_p);
    double lambda = K_tot / (J * omega0);
    double delta_omega_inf = (Pref - Pmeas) / K_tot;
    double exact_omega_final = omega0 + delta_omega_inf * (1.0 - exp(-lambda * T_total));

    /* 1. Run RK4 Integrator */
    vsg_controller_storage_t store_rk4;
    vsg_config_t cfg_rk4 = {.j_inertia = J,
                            .d_damping = D,
                            .v_nominal = V0,
                            .f_nominal = f0,
                            .p_ref = Pref,
                            .q_ref = 0.0,
                            .droop_p = droop_p,
                            .droop_q = 0.0,
                            .integrator_mode = VSG_INTEGRATOR_RK4};
    vsg_controller_t *vsg_rk4 = vsg_controller_create_in(&store_rk4, &cfg_rk4, NULL);
    assert(vsg_rk4 != NULL);
    assert(vsg_controller_get_integrator_mode(vsg_rk4) == VSG_INTEGRATOR_RK4);

    for (int i = 0; i < steps; ++i) {
        double f_out = 0.0, v_out = 0.0;
        assert(vsg_controller_step(vsg_rk4, Pmeas, 0.0, dt, &f_out, &v_out) == 0);
    }
    double rk4_omega = vsg_controller_get_omega(vsg_rk4);
    double rk4_err = fabs(rk4_omega - exact_omega_final);

    /* 2. Run Forward Euler Integrator */
    vsg_controller_storage_t store_euler;
    vsg_config_t cfg_euler = cfg_rk4;
    cfg_euler.integrator_mode = VSG_INTEGRATOR_EULER;
    vsg_controller_t *vsg_euler = vsg_controller_create_in(&store_euler, &cfg_euler, NULL);
    assert(vsg_euler != NULL);
    assert(vsg_controller_get_integrator_mode(vsg_euler) == VSG_INTEGRATOR_EULER);

    for (int i = 0; i < steps; ++i) {
        double f_out = 0.0, v_out = 0.0;
        assert(vsg_controller_step(vsg_euler, Pmeas, 0.0, dt, &f_out, &v_out) == 0);
    }
    double euler_omega = vsg_controller_get_omega(vsg_euler);
    double euler_err = fabs(euler_omega - exact_omega_final);

    printf("    • Analytical exact omega : %.8f rad/s\n", exact_omega_final);
    printf("    • RK4 simulated omega    : %.8f rad/s (Error = %.8e rad/s)\n", rk4_omega, rk4_err);
    printf("    • Euler simulated omega  : %.8f rad/s (Error = %.8e rad/s)\n", euler_omega,
           euler_err);

    /* RK4 precision must be within 1e-5 rad/s, and orders of magnitude better than Euler */
    assert(rk4_err < 1e-5);
    assert(euler_err > rk4_err * 20.0);

    printf("  --> VSG RK4 High-Precision Analytical Verification PASSED!\n");
}

static void test_vsg_rocof_and_inertia_simulation(void) {
    printf("[TEST] Testing VSG Initial RoCoF Inertial Response...\n");

    double J = 3.0;
    double D = 15.0;
    double f0 = 50.0;
    double Pref = 100000.0;  /* 100 kW */
    double Pmeas = 130000.0; /* 130 kW (+30 kW load disturbance) */
    double dt = 0.001;       /* 1 ms high-rate sampling */

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = J,
                        .d_damping = D,
                        .v_nominal = 380.0,
                        .f_nominal = f0,
                        .p_ref = Pref,
                        .q_ref = 0.0,
                        .droop_p = 1e-4,
                        .droop_q = 0.0,
                        .integrator_mode = VSG_INTEGRATOR_RK4};
    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* Theoretical initial RoCoF at t = 0 (before frequency drops and damping kicks in):
     * df/dt = (Pref - Pmeas) / (4 * PI^2 * J * f0)
     */
    double theoretical_rocof = (Pref - Pmeas) / (4.0 * M_PI * M_PI * J * f0);

    double f1 = 0.0, v1 = 0.0;
    vsg_controller_step(vsg, Pmeas, 0.0, dt, &f1, &v1);

    double measured_rocof = (f1 - f0) / dt;
    double rocof_rel_err = fabs((measured_rocof - theoretical_rocof) / theoretical_rocof);

    printf("    • Theoretical initial RoCoF : %.6f Hz/s\n", theoretical_rocof);
    printf("    • Measured initial RoCoF    : %.6f Hz/s (Relative Err: %.4f%%)\n", measured_rocof,
           rocof_rel_err * 100.0);

    assert(rocof_rel_err < 0.005); /* Error < 0.5% */

    printf("  --> VSG RoCoF Inertia Simulation PASSED!\n");
}

static void test_vsg_state_inspections_and_mode_switching(void) {
    printf("[TEST] Testing VSG State Inspections & Runtime Integrator Mode Switching...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 1.5,
                        .d_damping = 20.0,
                        .v_nominal = 220.0,
                        .f_nominal = 50.0,
                        .p_ref = 10000.0,
                        .q_ref = 5000.0,
                        .droop_p = 2e-4,
                        .droop_q = 1e-3,
                        .integrator_mode = VSG_INTEGRATOR_RK4};
    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    assert(vsg_controller_get_step_count(vsg) == 0);
    assert(vsg_controller_get_integrator_mode(vsg) == VSG_INTEGRATOR_RK4);

    double f = 0.0, v = 0.0;
    vsg_controller_step(vsg, 12000.0, 4000.0, 0.01, &f, &v);

    assert(vsg_controller_get_step_count(vsg) == 1);
    assert(vsg_controller_get_p_mech(vsg) != 0.0);
    assert(vsg_controller_get_p_damping(vsg) != 0.0);
    assert(vsg_controller_get_p_net(vsg) != 0.0);

    /* Switch mode at runtime */
    vsg_controller_set_integrator_mode(vsg, VSG_INTEGRATOR_EULER);
    assert(vsg_controller_get_integrator_mode(vsg) == VSG_INTEGRATOR_EULER);

    vsg_controller_step(vsg, 12000.0, 4000.0, 0.01, &f, &v);
    assert(vsg_controller_get_step_count(vsg) == 2);

    /* Reset */
    vsg_controller_reset(vsg);
    assert(vsg_controller_get_step_count(vsg) == 0);
    assert(fabs(vsg_controller_get_frequency(vsg) - 50.0) < 1e-6);

    printf("  --> VSG State Inspections & Mode Switching PASSED!\n");
}

static void test_vsg_adaptive_damping_under_weak_grid_scr(void) {
    printf("[TEST] Testing VSG Adaptive Damping & Droop vs Grid SCR (AEP-722)...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 2.0,
                        .d_damping = 20.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .p_ref = 50000.0,
                        .q_ref = 0.0,
                        .droop_p = 1e-4,
                        .droop_q = 1e-3,
                        .enable_adaptive_scr = true,
                        .scr_nominal = 3.0,
                        .scr_current = 3.0,
                        .integrator_mode = VSG_INTEGRATOR_RK4};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* 1. At nominal SCR = 3.0 */
    assert(fabs(vsg_controller_get_grid_scr(vsg) - 3.0) < 1e-6);
    assert(fabs(vsg_controller_get_effective_damping(vsg) - 20.0) < 1e-6);
    assert(fabs(vsg_controller_get_effective_droop_p(vsg) - 1e-4) < 1e-9);
    assert(fabs(vsg_controller_get_effective_droop_q(vsg) - 1e-3) < 1e-9);

    /* 2. Grid weakens to SCR = 1.2 (Weak Grid) */
    assert(vsg_controller_set_grid_scr(vsg, 1.2) == 0);
    assert(fabs(vsg_controller_get_grid_scr(vsg) - 1.2) < 1e-6);

    double expected_d_1_2 = 20.0 * sqrt(3.0 / 1.2); /* 20.0 * 1.58113883 = 31.6227766 */
    double actual_d_1_2 = vsg_controller_get_effective_damping(vsg);
    printf("    • SCR=1.2: Effective Damping = %.4f N·m·s/rad (Expected: %.4f)\n", actual_d_1_2,
           expected_d_1_2);
    assert(fabs(actual_d_1_2 - expected_d_1_2) < 1e-4);

    double expected_dp_1_2 = 1e-4 * sqrt(1.2 / 3.0);
    assert(fabs(vsg_controller_get_effective_droop_p(vsg) - expected_dp_1_2) < 1e-9);

    double expected_dq_1_2 = 1e-3 * (1.2 / 3.0);
    assert(fabs(vsg_controller_get_effective_droop_q(vsg) - expected_dq_1_2) < 1e-9);

    /* 3. Grid weakens to SCR = 0.8 (Ultra-Weak Grid) */
    assert(vsg_controller_set_grid_scr(vsg, 0.8) == 0);
    double expected_d_0_8 = 20.0 * sqrt(3.0 / 0.8); /* 20.0 * 1.93649167 = 38.729833 */
    double actual_d_0_8 = vsg_controller_get_effective_damping(vsg);
    printf("    • SCR=0.8: Effective Damping = %.4f N·m·s/rad (Expected: %.4f)\n", actual_d_0_8,
           expected_d_0_8);
    assert(fabs(actual_d_0_8 - expected_d_0_8) < 1e-3);

    /* Invalid SCR value rejected */
    assert(vsg_controller_set_grid_scr(vsg, -1.0) == -1);
    assert(vsg_controller_set_grid_scr(vsg, 0.0) == -1);

    printf("  --> VSG Adaptive Damping & Droop vs Grid SCR PASSED!\n");
}

static void test_vsg_weak_grid_oscillation_suppression_comparison(void) {
    printf("[TEST] Testing Weak Grid Oscillation Suppression: Fixed vs Adaptive Damping...\n");

    /* Weak grid condition SCR = 1.0 */
    double scr = 1.0;
    double Pref = 50000.0;
    double Pmeas = 80000.0; /* +30 kW step load */
    double dt = 0.01;       /* 10 ms step */

    /* Controller A: Fixed Damping (Baseline) */
    vsg_controller_storage_t store_fixed;
    vsg_config_t cfg_fixed = {.j_inertia = 1.0,
                              .d_damping = 8.0,
                              .v_nominal = 380.0,
                              .f_nominal = 50.0,
                              .p_ref = Pref,
                              .q_ref = 0.0,
                              .droop_p = 1e-4,
                              .droop_q = 1e-3,
                              .enable_adaptive_scr = false,
                              .integrator_mode = VSG_INTEGRATOR_RK4};
    vsg_controller_t *vsg_fixed = vsg_controller_create_in(&store_fixed, &cfg_fixed, NULL);

    /* Controller B: Adaptive Damping Enabled under SCR = 1.0 */
    vsg_controller_storage_t store_adapt;
    vsg_config_t cfg_adapt = cfg_fixed;
    cfg_adapt.enable_adaptive_scr = true;
    cfg_adapt.scr_nominal = 3.0;
    cfg_adapt.scr_current = scr;
    vsg_controller_t *vsg_adapt = vsg_controller_create_in(&store_adapt, &cfg_adapt, NULL);

    /* Run both controllers for 1.0 second */
    double f_fixed_min = 50.0;
    double f_adapt_min = 50.0;
    for (int i = 0; i < 100; ++i) {
        double f_fix = 0.0, v_fix = 0.0;
        double f_ad = 0.0, v_ad = 0.0;
        vsg_controller_step(vsg_fixed, Pmeas, 0.0, dt, &f_fix, &v_fix);
        vsg_controller_step(vsg_adapt, Pmeas, 0.0, dt, &f_ad, &v_ad);

        if (f_fix < f_fixed_min) f_fixed_min = f_fix;
        if (f_ad < f_adapt_min) f_adapt_min = f_ad;
    }

    double delta_f_fixed_nadir = 50.0 - f_fixed_min;
    double delta_f_adapt_nadir = 50.0 - f_adapt_min;

    printf("    • Fixed damping freq nadir drop    : %.4f Hz\n", delta_f_fixed_nadir);
    printf("    • Adaptive damping freq nadir drop   : %.4f Hz\n", delta_f_adapt_nadir);

    /* Under SCR=1.0, adaptive damping provides higher damping and reduces frequency overshoot nadir
     */
    assert(delta_f_adapt_nadir < delta_f_fixed_nadir);

    printf("  --> Weak Grid Oscillation Suppression Comparison PASSED!\n");
}

static void test_vsg_seamless_unplanned_islanding(void) {
    printf("[TEST] Testing VSG Seamless Unplanned Islanding Handover (AEP-723)...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 2.0,
                        .d_damping = 25.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .p_ref = 60000.0,
                        .q_ref = 0.0,
                        .droop_p = 1e-4,
                        .droop_q = 1e-3,
                        .island_freq_slew_hz_s = 1.0,
                        .island_volt_slew_v_s = 20.0,
                        .integrator_mode = VSG_INTEGRATOR_RK4};
    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);
    assert(vsg_controller_get_mode(vsg) == VSG_GRID_MODE_CONNECTED);

    /* Grid fault occurs at V = 365.0 V, f = 49.6 Hz, theta = 2.15 rad */
    double grid_v = 365.0;
    double grid_f = 49.6;
    double grid_theta = 2.15;

    assert(vsg_controller_trigger_unplanned_islanding(vsg, grid_v, grid_f, grid_theta) == 0);
    assert(vsg_controller_get_mode(vsg) == VSG_GRID_MODE_ISLANDED);

    /* 1. Instantaneous zero-jump handover verification */
    assert(fabs(vsg_controller_get_voltage(vsg) - 365.0) < 1e-6);
    assert(fabs(vsg_controller_get_frequency(vsg) - 49.6) < 1e-6);
    assert(fabs(vsg_controller_get_theta(vsg) - 2.15) < 1e-6);

    /* 2. Run autonomous islanded steps for 2.0s and verify slew rate compliance */
    double dt = 0.01;
    double prev_f = 49.6;
    double prev_v = 365.0;
    for (int i = 0; i < 200; ++i) {
        double f_out = 0.0, v_out = 0.0;
        vsg_controller_step(vsg, 60000.0, 0.0, dt, &f_out, &v_out);

        double rate_f = fabs(f_out - prev_f) / dt;
        double rate_v = fabs(v_out - prev_v) / dt;
        assert(rate_f <= 1.05); /* Rate limit 1.0 Hz/s + tolerance */
        assert(rate_v <= 20.5); /* Rate limit 20.0 V/s + tolerance */

        prev_f = f_out;
        prev_v = v_out;
    }

    /* After 2 seconds, islanded state has smoothly recovered towards nominal */
    assert(fabs(vsg_controller_get_frequency(vsg) - 50.0) < 0.05);
    assert(fabs(vsg_controller_get_voltage(vsg) - 380.0) < 1.0);

    printf("  --> VSG Seamless Unplanned Islanding Handover PASSED!\n");
}

static void test_vsg_presync_reconnection_lock(void) {
    printf("[TEST] Testing VSG Pre-Synchronization Reconnection Locking (AEP-723)...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 2.0,
                        .d_damping = 20.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .p_ref = 0.0,
                        .q_ref = 0.0,
                        .sync_kp_theta = 35.0,
                        .sync_ki_theta = 5.0,
                        .sync_kp_v = 2.0,
                        .sync_theta_tol_rad = 0.05,
                        .sync_volt_tol_v = 2.0,
                        .sync_freq_tol_hz = 0.1,
                        .integrator_mode = VSG_INTEGRATOR_RK4};
    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* Microgrid running in islanded mode, grid returns with slight phase & voltage offset */
    vsg_controller_set_mode(vsg, VSG_GRID_MODE_ISLANDED);
    assert(!vsg_controller_is_sync_locked(vsg));

    double grid_v = 381.5;
    double grid_f = 50.02;
    double grid_theta = 1.80; /* Initial grid phase */
    double dt = 0.005;        /* 5ms control cycle */

    /* Run pre-synchronization tracking loop for 1.5 seconds */
    for (int i = 0; i < 300; ++i) {
        grid_theta += 2.0 * M_PI * grid_f * dt;
        while (grid_theta >= 2.0 * M_PI) grid_theta -= 2.0 * M_PI;

        vsg_controller_step_presync(vsg, grid_v, grid_f, grid_theta, dt);
    }

    /* Verify locking achieved */
    assert(vsg_controller_is_sync_locked(vsg));

    vsg_transition_status_t status;
    assert(vsg_controller_get_transition_status(vsg, &status) == 0);
    printf(
        "    • Pre-sync status: Locked=%d, delta_theta=%.4f rad, delta_v=%.2f V, delta_f=%.4f Hz\n",
        status.sync_locked, status.delta_theta_rad, status.delta_volt_v, status.delta_freq_hz);

    assert(fabs(status.delta_theta_rad) <= 0.05);
    assert(fabs(status.delta_volt_v) <= 2.0);
    assert(fabs(status.delta_freq_hz) <= 0.1);

    /* Switch to connected mode */
    vsg_controller_set_mode(vsg, VSG_GRID_MODE_CONNECTED);
    assert(vsg_controller_get_mode(vsg) == VSG_GRID_MODE_CONNECTED);

    printf("  --> VSG Pre-Synchronization Reconnection Locking PASSED!\n");
}

static void test_vsg_black_start_zero_voltage_soft_ramp(void) {
    printf("[TEST] Testing VSG Black Start Zero-Voltage Soft Ramp-Up (AEP-724)...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 1.0,
                        .d_damping = 20.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .p_ref = 50000.0,
                        .q_ref = 0.0,
                        .droop_p = 1e-4,
                        .droop_q = 1e-3,
                        .black_start = {.ramp_time_s = 2.0,
                                        .max_volt_slew_v_s = 200.0,
                                        .max_freq_slew_hz_s = 30.0,
                                        .inrush_i_limit_a = 150.0,
                                        .sync_lock_angle_tol_rad = 0.05,
                                        .settle_time_s = 0.2}};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* Trigger black start from cold microgrid state */
    assert(vsg_controller_trigger_black_start(vsg) == 0);
    assert(vsg_controller_get_black_start_state(vsg) == VSG_BS_STATE_RAMPING);
    assert(vsg_controller_get_voltage(vsg) == 0.0);
    assert(vsg_controller_get_frequency(vsg) == 0.0);

    vsg_black_start_status_t status;
    assert(vsg_controller_get_black_start_status(vsg, &status) == 0);
    assert(status.is_ready_for_load == false);
    assert(status.ramp_progress_pct == 0.0);

    /* Step through ramp: 2.5 seconds with dt = 0.01s (250 steps) */
    double f_out = 0.0, v_out = 0.0;
    double last_v = 0.0, last_f = 0.0;
    for (int i = 0; i < 250; ++i) {
        vsg_controller_step_black_start(vsg, 0.0, -1.0, 0.01, &f_out, &v_out);
        assert(v_out >= last_v); /* Monotonic soft start */
        assert(f_out >= last_f);
        last_v = v_out;
        last_f = f_out;

        if (i == 100) { /* t = 1.0s, halfway point */
            assert(vsg_controller_get_black_start_status(vsg, &status) == 0);
            printf("    • t=1.0s: Progress=%.1f%%, V=%.1f V, f=%.2f Hz\n", status.ramp_progress_pct,
                   v_out, f_out);
            assert(fabs(v_out - 190.0) < 5.0);
            assert(fabs(f_out - 25.0) < 1.0);
        }
    }

    /* Verify established state and readiness for load */
    assert(vsg_controller_get_black_start_status(vsg, &status) == 0);
    printf("    • t=2.5s Established: State=%d, ReadyForLoad=%d, V=%.1f V, f=%.2f Hz\n",
           status.state, status.is_ready_for_load, v_out, f_out);

    assert(status.state == VSG_BS_STATE_ESTABLISHED);
    assert(status.is_ready_for_load == true);
    assert(fabs(v_out - 380.0) < 1e-3);
    assert(fabs(f_out - 50.0) < 1e-3);
    assert(vsg_controller_get_mode(vsg) == VSG_GRID_MODE_ISLANDED);

    printf("  --> VSG Black Start Zero-Voltage Soft Ramp PASSED!\n");
}

static void test_vsg_black_start_inrush_throttling_and_fault(void) {
    printf("[TEST] Testing VSG Black Start Inrush Current Throttling & Fault Protection...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 1.0,
                        .d_damping = 20.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .black_start = {.ramp_time_s = 2.0,
                                        .max_volt_slew_v_s = 200.0,
                                        .max_freq_slew_hz_s = 30.0,
                                        .inrush_i_limit_a = 100.0,
                                        .settle_time_s = 0.2}};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);
    assert(vsg_controller_trigger_black_start(vsg) == 0);

    /* 1. Step under mild inrush (90A > 0.8 * 100A) -> ramp rate is throttled */
    double f_out = 0.0, v_out = 0.0;
    for (int i = 0; i < 50; ++i) {
        vsg_controller_step_black_start(vsg, 90.0, -1.0, 0.01, &f_out, &v_out);
    }
    /* Without throttle, at t=0.5s V would be 380/2.0 * 0.5 = 95V; throttled should be lower */
    assert(v_out < 85.0);
    assert(vsg_controller_get_black_start_state(vsg) == VSG_BS_STATE_RAMPING);

    /* 2. Step under extreme inrush (250A > 2.0 * 100A) -> Trip to Fault */
    vsg_controller_step_black_start(vsg, 250.0, -1.0, 0.01, &f_out, &v_out);
    assert(vsg_controller_get_black_start_state(vsg) == VSG_BS_STATE_FAULT);
    assert(v_out == 0.0);
    assert(f_out == 0.0);

    printf("  --> VSG Black Start Inrush Throttling & Protection PASSED!\n");
}

static void test_vsg_black_start_multi_pcs_lockstep(void) {
    printf("[TEST] Testing VSG Black Start Multi-PCS Distributed Lockstep Alignment...\n");

    vsg_controller_storage_t store1, store2;
    vsg_config_t cfg = {.j_inertia = 1.0,
                        .d_damping = 20.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .black_start = {.ramp_time_s = 1.5,
                                        .max_volt_slew_v_s = 300.0,
                                        .max_freq_slew_hz_s = 40.0,
                                        .sync_lock_angle_tol_rad = 0.05,
                                        .settle_time_s = 0.1}};

    vsg_controller_t *pcs1 = vsg_controller_create_in(&store1, &cfg, NULL);
    vsg_controller_t *pcs2 = vsg_controller_create_in(&store2, &cfg, NULL);
    assert(pcs1 && pcs2);

    assert(vsg_controller_trigger_black_start(pcs1) == 0);
    assert(vsg_controller_trigger_black_start(pcs2) == 0);

    /* Run distributed lockstep: PCS2 tracks PCS1 virtual angle */
    double f1 = 0, v1 = 0, f2 = 0, v2 = 0;
    for (int i = 0; i < 200; ++i) {
        double theta1 = vsg_controller_get_theta(pcs1);
        vsg_controller_step_black_start(pcs1, 0.0, -1.0, 0.01, &f1, &v1);
        vsg_controller_step_black_start(pcs2, 0.0, theta1, 0.01, &f2, &v2);

        double delta_theta = fabs(vsg_controller_get_theta(pcs1) - vsg_controller_get_theta(pcs2));
        if (delta_theta > M_PI) delta_theta = 2.0 * M_PI - delta_theta;

        /* Verify phase lockstep within 0.05 rad after initial 10 steps */
        if (i > 10) {
            assert(delta_theta <= 0.05);
        }
    }

    /* Both PCS units established and synchronized */
    assert(vsg_controller_get_black_start_state(pcs1) == VSG_BS_STATE_ESTABLISHED);
    assert(vsg_controller_get_black_start_state(pcs2) == VSG_BS_STATE_ESTABLISHED);
    assert(fabs(v1 - 380.0) < 1e-3);
    assert(fabs(v2 - 380.0) < 1e-3);

    printf("  --> VSG Black Start Multi-PCS Lockstep PASSED!\n");
}

static void test_vsg_circulating_current_active_suppression(void) {
    printf("[TEST] Testing VSG Multi-PCS Circulating Current Active Suppression (AEP-725)...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 1.0,
                        .d_damping = 20.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .circ_suppress = {.enable_circ_suppression = true,
                                          .enable_online_impedance_id = false,
                                          .r_line_nominal_ohm = 0.08,
                                          .x_line_nominal_ohm = 0.25,
                                          .r_virtual_ohm = 0.05,
                                          .x_virtual_ohm = 0.20,
                                          .kp_circ = 2.5,
                                          .ki_circ = 25.0}};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* Initial asymmetric branch operating condition:
       Branch current: Id = 100A, Iq = -30A
       Cluster average current: Id_avg = 100A, Iq_avg = -10A
       Initial reactive circulating current Iq_circ = -20A
    */
    double id_meas = 100.0, iq_meas = -30.0;
    double id_avg = 100.0, iq_avg = -10.0;
    double v_pcc_d = 370.0, v_pcc_q = 0.0;

    double out_vd = 0.0, out_vq = 0.0;

    /* Run 200 simulation steps (dt = 0.005s, total 1.0s) with PI closed-loop circulating current
     * suppression */
    for (int i = 0; i < 200; ++i) {
        vsg_controller_step_circ_suppression(vsg, id_meas, iq_meas, id_avg, iq_avg, v_pcc_d,
                                             v_pcc_q, 0.005, &out_vd, &out_vq);

        /* Simulated physical branch response: voltage compensation drives Iq towards Iq_avg */
        vsg_circulating_status_t status;
        vsg_controller_get_circ_status(vsg, &status);

        /* Iq converges towards avg as Q-axis compensation is applied */
        double delta_iq = (iq_avg - iq_meas) * 0.05;
        iq_meas += delta_iq;
    }

    vsg_circulating_status_t final_status;
    assert(vsg_controller_get_circ_status(vsg, &final_status) == 0);
    printf(
        "    • Circulating suppression: Initial Iq_circ=20.0A -> Final Iq_circ=%.2fA, "
        "SuppressionRatio=%.1f%%\n",
        fabs(final_status.i_circ_q_a), final_status.suppression_ratio_pct);

    assert(fabs(final_status.i_circ_q_a) < 2.0); /* Suppressed from 20A to < 2A */
    assert(final_status.suppression_ratio_pct > 85.0);

    printf("  --> VSG Circulating Current Active Suppression PASSED!\n");
}

static void test_vsg_online_line_impedance_identification_rls(void) {
    printf("[TEST] Testing VSG Online Line Impedance RLS Identification (AEP-725)...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {
        .j_inertia = 1.0,
        .d_damping = 20.0,
        .v_nominal = 380.0,
        .f_nominal = 50.0,
        .circ_suppress = {.enable_circ_suppression = true,
                          .enable_online_impedance_id = true,
                          .r_line_nominal_ohm = 0.05, /* Initial guess differs from true */
                          .x_line_nominal_ohm = 0.10,
                          .rls_forgetting_factor = 0.99}};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* True line impedance: R_true = 0.12 ohm, X_true = 0.35 ohm */
    double r_true = 0.12;
    double x_true = 0.35;

    double out_vd = 0.0, out_vq = 0.0;

    /* Feed 300 steps with dynamic current trajectory to excite both R and X modes */
    for (int i = 0; i < 300; ++i) {
        double t = i * 0.01;
        double id = 80.0 + 20.0 * sin(2.0 * M_PI * 2.0 * t);
        double iq = -30.0 + 15.0 * cos(2.0 * M_PI * 2.0 * t);

        /* True physical voltage drop: Delta_Vd = R * Id - X * Iq */
        double delta_vd = r_true * id - x_true * iq;
        double v_pcc_d = vsg_controller_get_voltage(vsg) - delta_vd;

        vsg_controller_step_circ_suppression(vsg, id, iq, id, iq, v_pcc_d, 0.0, 0.01, &out_vd,
                                             &out_vq);
    }

    vsg_circulating_status_t status;
    assert(vsg_controller_get_circ_status(vsg, &status) == 0);
    printf("    • RLS Identified: R_line=%.4f Ω (True=%.2f Ω), X_line=%.4f Ω (True=%.2f Ω)\n",
           status.r_line_est_ohm, r_true, status.x_line_est_ohm, x_true);

    assert(fabs(status.r_line_est_ohm - r_true) < 0.02);
    assert(fabs(status.x_line_est_ohm - x_true) < 0.03);

    printf("  --> VSG Online Line Impedance RLS Identification PASSED!\n");
}

static void test_vsg_sso_detection_and_frequency_tracking(void) {
    printf("[TEST] Testing VSG SSO Online Frequency & Amplitude Estimation (AEP-727)...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 1.0,
                        .d_damping = 20.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .sso = {.enable_sso_damping = true,
                                .sso_freq_min_hz = 5.0,
                                .sso_freq_max_hz = 45.0,
                                .sso_alarm_thresh_a = 5.0,
                                .notch_damping_ratio_zeta = 0.2,
                                .active_damp_gain = 3.0}};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* Injected current: 50Hz nominal grid current 100A + 18.5Hz SSO oscillation 12A */
    double f_fund = 50.0;
    double f_sso_true = 18.5;
    double a_sso_true = 12.0;

    double filtered_i = 0.0, v_damp = 0.0;

    /* Run 1000 steps at 1kHz (dt = 0.001s, 1.0s duration) */
    for (int i = 0; i < 1000; ++i) {
        double t = i * 0.001;
        double i_raw =
            100.0 * sin(2.0 * M_PI * f_fund * t) + a_sso_true * sin(2.0 * M_PI * f_sso_true * t);

        vsg_controller_step_sso_damping(vsg, i_raw, 0.001, &filtered_i, &v_damp);
    }

    vsg_sso_status_t status;
    assert(vsg_controller_get_sso_status(vsg, &status) == 0);
    printf(
        "    • SSO Detection: Detected=%d, f_est=%.2f Hz (True=%.2f Hz), A_est=%.2f A (True=%.2f "
        "A)\n",
        status.sso_detected, status.sso_freq_hz, f_sso_true, status.sso_amplitude_a, a_sso_true);

    assert(status.sso_detected == true);
    assert(fabs(status.sso_freq_hz - f_sso_true) < 1.5);
    assert(fabs(status.sso_amplitude_a - a_sso_true) < 2.0);

    printf("  --> VSG SSO Frequency & Amplitude Estimation PASSED!\n");
}

static void test_vsg_sso_adaptive_notch_filtering_and_active_damping(void) {
    printf(
        "[TEST] Testing VSG SSO Adaptive Notch Filter & Anti-Phase Active Damping (AEP-727)...\n");

    vsg_controller_storage_t store;
    vsg_config_t cfg = {.j_inertia = 1.0,
                        .d_damping = 20.0,
                        .v_nominal = 380.0,
                        .f_nominal = 50.0,
                        .sso = {.enable_sso_damping = true,
                                .sso_freq_min_hz = 5.0,
                                .sso_freq_max_hz = 45.0,
                                .sso_alarm_thresh_a = 4.0,
                                .notch_damping_ratio_zeta = 0.25,
                                .active_damp_gain = 4.0}};

    vsg_controller_t *vsg = vsg_controller_create_in(&store, &cfg, NULL);
    assert(vsg != NULL);

    /* Pure 22Hz SSO oscillation current to evaluate notch suppression */
    double f_sso = 22.0;
    double a_sso = 10.0;

    double filtered_i = 0.0, v_damp = 0.0;
    double max_filtered_steady = 0.0;

    /* Run 1500 steps to allow notch filter and FLL to lock and reach steady-state */
    for (int i = 0; i < 1500; ++i) {
        double t = i * 0.001;
        double i_raw = a_sso * sin(2.0 * M_PI * f_sso * t);
        vsg_controller_step_sso_damping(vsg, i_raw, 0.001, &filtered_i, &v_damp);

        if (i > 1200) {
            if (fabs(filtered_i) > max_filtered_steady) {
                max_filtered_steady = fabs(filtered_i);
            }
        }
    }

    vsg_sso_status_t status;
    assert(vsg_controller_get_sso_status(vsg, &status) == 0);
    printf(
        "    • Notch Attenuation: Input=10.0A SSO -> Output=%.2fA (Attenuated >80%%), "
        "DampingVoltage=%.2f V\n",
        max_filtered_steady, status.damping_out_v);

    assert(max_filtered_steady <
           2.0); /* Residual is < 2.0A (>= 80% attenuation at notch notch depth) */
    assert(fabs(status.damping_out_v) > 0.1);

    printf("  --> VSG SSO Adaptive Notch & Active Damping PASSED!\n");
}

int main(void) {
    printf(
        "=== EdgeOS VSG Grid-Forming Control Tests (AEP-721 ~ AEP-730 / Issue #252 ~ #257) ===\n");
    test_vsg_initialization_and_bounds();
    test_vsg_swing_dynamics_and_damping();
    test_vsg_reactive_droop();
    test_vsg_rk4_high_precision_against_analytical();
    test_vsg_rocof_and_inertia_simulation();
    test_vsg_state_inspections_and_mode_switching();
    test_vsg_adaptive_damping_under_weak_grid_scr();
    test_vsg_weak_grid_oscillation_suppression_comparison();
    test_vsg_seamless_unplanned_islanding();
    test_vsg_presync_reconnection_lock();
    test_vsg_black_start_zero_voltage_soft_ramp();
    test_vsg_black_start_inrush_throttling_and_fault();
    test_vsg_black_start_multi_pcs_lockstep();
    test_vsg_circulating_current_active_suppression();
    test_vsg_online_line_impedance_identification_rls();
    test_vsg_sso_detection_and_frequency_tracking();
    test_vsg_sso_adaptive_notch_filtering_and_active_damping();
    printf("=== All VSG Control Tests PASSED! ===\n");
    return 0;
}
