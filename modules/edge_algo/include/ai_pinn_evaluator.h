/**
 * @file ai_pinn_evaluator.h
 * @brief Physics-Informed Neural Network (PINN) Conservation Law Residual Engine (AEP-709 / Issue #242)
 *
 * Implements industrial edge PINN physics loss and conservation derivative evaluation:
 * - Mass Conservation (Continuity PDE residual: div(v) = 0);
 * - Energy Conservation (Battery/Motor thermal dynamics: rho*cp*dT/dt - k*laplacian(T) - Q_gen + Q_diss = 0);
 * - Kirchhoff Circuit Laws (KCL/KVL electrical branch residuals);
 * - Total physics residual loss & conservation compliance percentage;
 * - Zero-heap caller-allocated aggregate root;
 * - DataHub telemetry synchronization.
 */

#ifndef EDGE_AI_PINN_EVALUATOR_H
#define EDGE_AI_PINN_EVALUATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct datahub_s datahub_t;

/* -------------------------------------------------------------------------- */
/* Constants & Types                                                          */
/* -------------------------------------------------------------------------- */

typedef struct {
    float lambda_mass;                 /**< Weight for mass conservation penalty */
    float lambda_energy;               /**< Weight for energy conservation penalty */
    float lambda_circuit;              /**< Weight for electrical circuit KCL/KVL penalty */
    float specific_heat_capacity;      /**< cp (J/(kg*K)) */
    float density;                     /**< rho (kg/m^3) */
    float thermal_conductivity;        /**< k (W/(m*K)) */
    float convection_coeff;            /**< h (W/(m^2*K)) */
    float ambient_temperature_c;       /**< T_amb */
} ai_pinn_config_t;

/**
 * @brief Fluid & Flow Field Inputs for Mass Conservation Check
 */
typedef struct {
    float du_dx;                       /**< Velocity gradient du/dx */
    float dv_dy;                       /**< Velocity gradient dv/dy */
    float dw_dz;                       /**< Velocity gradient dw/dz */
} ai_pinn_mass_state_t;

/**
 * @brief Thermal & Electrochemical Inputs for Energy Conservation Check
 */
typedef struct {
    float dt_dt;                       /**< Temperature derivative dT/dt */
    float laplacian_t;                 /**< Spatial Laplacian nabla^2(T) */
    float current_a;                   /**< Current I */
    float internal_resistance_ohm;     /**< R */
    float reaction_heat_w;             /**< Q_reac */
    float surface_area_m2;             /**< A */
    float surface_temp_c;              /**< T */
} ai_pinn_thermal_state_t;

/**
 * @brief Circuit Branch Inputs for Kirchhoff Residual Check
 */
typedef struct {
    uint32_t num_branches;
    float branch_currents[16];
    float branch_voltages[16];
} ai_pinn_circuit_state_t;

typedef struct {
    float mass_residual;
    float energy_residual;
    float circuit_residual;
    float total_physics_loss;
    float conservation_compliance_pct;
} ai_pinn_result_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define AI_PINN_EVALUATOR_STORAGE_SIZE 8192

typedef struct {
    alignas(max_align_t) uint8_t raw[AI_PINN_EVALUATOR_STORAGE_SIZE];
} ai_pinn_evaluator_storage_t;

typedef struct ai_pinn_evaluator ai_pinn_evaluator_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize PINN Evaluator in caller-allocated buffer
 */
ai_pinn_evaluator_t *ai_pinn_evaluator_create_in(ai_pinn_evaluator_storage_t *storage,
                                                 const ai_pinn_config_t *config);

/**
 * @brief Evaluate Mass, Energy, and Electrical Conservation residuals
 */
int ai_pinn_evaluator_evaluate(ai_pinn_evaluator_t *pinn,
                               const ai_pinn_mass_state_t *mass,
                               const ai_pinn_thermal_state_t *thermal,
                               const ai_pinn_circuit_state_t *circuit,
                               ai_pinn_result_t *result);

#include "edge_algo_port.h"

/**
 * @brief Emit PINN evaluation metrics via Consumer-Defined Algo Sink
 */
int ai_pinn_evaluator_emit_results(const ai_pinn_result_t *result, const edge_algo_sink_t *sink);

/**
 * @brief Synchronize PINN evaluation metrics to DataHub (Adapter bridge)
 */
int ai_pinn_evaluator_sync_datahub(const ai_pinn_result_t *result, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_AI_PINN_EVALUATOR_H */
