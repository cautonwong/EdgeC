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

int main(void) {
    printf("=== EdgeOS VSG Grid-Forming Control Tests (AEP-721 ~ AEP-730 / Issue #252) ===\n");
    test_vsg_initialization_and_bounds();
    test_vsg_swing_dynamics_and_damping();
    test_vsg_reactive_droop();
    test_vsg_rk4_high_precision_against_analytical();
    test_vsg_rocof_and_inertia_simulation();
    test_vsg_state_inspections_and_mode_switching();
    printf("=== All VSG Control Tests PASSED! ===\n");
    return 0;
}
