/**
 * @file test_ai_pinn_evaluator.c
 * @brief Unit tests for PINN Conservation Law Residual Engine (AEP-709 / Issue #242)
 */

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "ai_pinn_evaluator.h"

static void test_pinn_mass_and_energy_conservation(void **state) {
    (void)state;

    ai_pinn_evaluator_storage_t storage;
    ai_pinn_config_t config = {.lambda_mass = 1.0f,
                               .lambda_energy = 0.5f,
                               .lambda_circuit = 0.5f,
                               .specific_heat_capacity = 900.0f,
                               .density = 2400.0f,
                               .thermal_conductivity = 2.0f,
                               .convection_coeff = 15.0f,
                               .ambient_temperature_c = 25.0f};

    ai_pinn_evaluator_t *pinn = ai_pinn_evaluator_create_in(&storage, &config);
    assert_non_null(pinn);

    /* Test Case 1: Incompressible mass conservation (du/dx + dv/dy + dw/dz = 0) */
    ai_pinn_mass_state_t mass = {.du_dx = 1.25f, .dv_dy = -0.75f, .dw_dz = -0.50f};

    /* Test Case 2: Steady state thermal equilibrium */
    ai_pinn_thermal_state_t thermal = {
        .dt_dt = 0.0f,
        .laplacian_t = 0.0f,
        .current_a = 10.0f,
        .internal_resistance_ohm = 0.02f, /* Q_joule = 100 * 0.02 = 2.0 W */
        .reaction_heat_w = 1.0f,          /* Q_gen = 3.0 W */
        .surface_area_m2 = 0.01f,
        .surface_temp_c = 45.0f /* Delta_T = 20 C -> Q_loss = 15 * 0.01 * 20 = 3.0 W */
    };

    /* Test Case 3: Balanced Circuit (KCL sum = 0, KVL sum = 0) */
    ai_pinn_circuit_state_t circuit = {.num_branches = 3,
                                       .branch_currents = {5.0f, -3.0f, -2.0f},
                                       .branch_voltages = {12.0f, -8.0f, -4.0f}};

    ai_pinn_result_t result;
    assert_int_equal(ai_pinn_evaluator_evaluate(pinn, &mass, &thermal, &circuit, &result), 0);

    /* In perfect physics equilibrium, all residuals should be 0.0 */
    assert_float_equal(result.mass_residual, 0.0f, 0.001f);
    assert_float_equal(result.energy_residual, 0.0f, 0.001f);
    assert_float_equal(result.circuit_residual, 0.0f, 0.001f);
    assert_float_equal(result.total_physics_loss, 0.0f, 0.001f);
    assert_float_equal(result.conservation_compliance_pct, 100.0f, 0.001f);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_pinn_mass_and_energy_conservation),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
