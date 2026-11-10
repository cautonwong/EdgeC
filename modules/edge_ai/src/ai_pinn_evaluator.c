/**
 * @file ai_pinn_evaluator.c
 * @brief Physics-Informed Neural Network (PINN) Conservation Law Residual Implementation (AEP-709 / Issue #242)
 */

#include "ai_pinn_evaluator.h"
#include <string.h>
#include <math.h>

struct ai_pinn_evaluator {
    ai_pinn_config_t config;
};

_Static_assert(sizeof(struct ai_pinn_evaluator) <= sizeof(ai_pinn_evaluator_storage_t),
               "ai_pinn_evaluator_storage_t size is smaller than internal struct ai_pinn_evaluator");

ai_pinn_evaluator_t *ai_pinn_evaluator_create_in(ai_pinn_evaluator_storage_t *storage,
                                                 const ai_pinn_config_t *config) {
    if (!storage || !config) return NULL;

    struct ai_pinn_evaluator *pinn = (struct ai_pinn_evaluator *)storage->raw;
    memset(pinn, 0, sizeof(*pinn));
    pinn->config = *config;

    if (pinn->config.specific_heat_capacity <= 0.0f) pinn->config.specific_heat_capacity = 1000.0f;
    if (pinn->config.density <= 0.0f) pinn->config.density = 2500.0f;
    if (pinn->config.thermal_conductivity <= 0.0f) pinn->config.thermal_conductivity = 1.5f;
    if (pinn->config.convection_coeff <= 0.0f) pinn->config.convection_coeff = 20.0f;
    if (pinn->config.lambda_mass <= 0.0f) pinn->config.lambda_mass = 1.0f;
    if (pinn->config.lambda_energy <= 0.0f) pinn->config.lambda_energy = 1.0f;
    if (pinn->config.lambda_circuit <= 0.0f) pinn->config.lambda_circuit = 1.0f;

    return pinn;
}

int ai_pinn_evaluator_evaluate(ai_pinn_evaluator_t *pinn,
                               const ai_pinn_mass_state_t *mass,
                               const ai_pinn_thermal_state_t *thermal,
                               const ai_pinn_circuit_state_t *circuit,
                               ai_pinn_result_t *result) {
    if (!pinn || !result) return -1;

    memset(result, 0, sizeof(*result));

    /* 1. Mass Conservation Residual (div(v) = 0) */
    if (mass) {
        result->mass_residual = mass->du_dx + mass->dv_dy + mass->dw_dz;
    }

    /* 2. Thermal Energy Conservation Residual */
    if (thermal) {
        float q_joule = thermal->current_a * thermal->current_a * thermal->internal_resistance_ohm;
        float q_gen = q_joule + thermal->reaction_heat_w;
        float delta_t = thermal->surface_temp_c - pinn->config.ambient_temperature_c;
        float q_loss = pinn->config.convection_coeff * thermal->surface_area_m2 * delta_t;

        float thermal_accum = pinn->config.density * pinn->config.specific_heat_capacity * thermal->dt_dt;
        float conduction = pinn->config.thermal_conductivity * thermal->laplacian_t;

        result->energy_residual = thermal_accum - conduction - (q_gen - q_loss);
    }

    /* 3. Circuit KCL/KVL Residual */
    if (circuit && circuit->num_branches > 0) {
        float sum_i = 0.0f;
        float sum_v = 0.0f;
        for (uint32_t i = 0; i < circuit->num_branches; ++i) {
            sum_i += circuit->branch_currents[i];
            sum_v += circuit->branch_voltages[i];
        }
        result->circuit_residual = fabsf(sum_i) + fabsf(sum_v);
    }

    /* Total Physics Penalty Loss */
    float loss_m = pinn->config.lambda_mass * (result->mass_residual * result->mass_residual);
    float loss_e = pinn->config.lambda_energy * (result->energy_residual * result->energy_residual);
    float loss_c = pinn->config.lambda_circuit * (result->circuit_residual * result->circuit_residual);

    result->total_physics_loss = loss_m + loss_e + loss_c;

    /* Compliance Percentage */
    float compliance = 100.0f * expf(-0.05f * result->total_physics_loss);
    if (compliance > 100.0f) compliance = 100.0f;
    if (compliance < 0.0f) compliance = 0.0f;
    result->conservation_compliance_pct = compliance;

    return 0;
}

int ai_pinn_evaluator_emit_results(const ai_pinn_result_t *result, const edge_algo_sink_t *sink) {
    if (!result || !sink) return -1;

    edge_algo_sink_emit_scalar(sink, 262176, (double)result->total_physics_loss, 0);
    edge_algo_sink_emit_scalar(sink, 262177, (double)result->conservation_compliance_pct, 0);

    return 0;
}
