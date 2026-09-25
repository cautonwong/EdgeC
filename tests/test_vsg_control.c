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

int main(void) {
    printf("=== EdgeOS VSG Grid-Forming Control Tests (AEP-723 ~ AEP-730 / AEP-752) ===\n");
    test_vsg_initialization_and_bounds();
    test_vsg_swing_dynamics_and_damping();
    test_vsg_reactive_droop();
    printf("=== All VSG Control Tests PASSED! ===\n");
    return 0;
}
