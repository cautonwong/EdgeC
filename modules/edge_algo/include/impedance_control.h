/**
 * @file impedance_control.h
 * @brief Rehabilitation Robotic Impedance & Active/Passive Torque Control Loop Engine
 */

#ifndef IMPEDANCE_CONTROL_H
#define IMPEDANCE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float mass;              /* M (kg) */
    float damping;           /* B (N*s/m) */
    float stiffness;         /* K (N/m) */
    float target_pos;        /* x0 (m) */
    float max_torque_limit;  /* Torque safety limit (N*m) */
} impedance_params_t;

typedef struct {
    float current_pos;      /* x (m) */
    float current_vel;      /* v (m/s) */
    float external_force;   /* F_ext (N) */
    float output_torque;    /* Tau (N*m) */
    bool active_assistance; /* True if assisting patient, False if resistive */
} impedance_state_t;

typedef struct impedance_controller impedance_controller_t;

typedef struct {
    alignas(max_align_t) uint8_t raw[128];
} impedance_controller_storage_t;

impedance_controller_t *impedance_controller_create_in(impedance_controller_storage_t *store,
                                                      const impedance_params_t *params);

float impedance_controller_get_torque(const impedance_controller_t *ctrl);
bool impedance_controller_is_assisted(const impedance_controller_t *ctrl);
int impedance_controller_step(impedance_controller_t *ctrl, float pos, float vel, float force, float dt_s);

/**
 * @brief Initializes impedance control parameters
 * @param params Pointer to parameter structure
 * @param M Virtual Mass (kg)
 * @param B Virtual Damping (N*s/m)
 * @param K Virtual Stiffness (N/m)
 * @param target_pos Target Equilibrium Position (m)
 * @param max_torque Maximum Safe Torque Limit (N*m)
 * @return 0 on success, negative error code on failure
 */
int impedance_control_init(impedance_params_t *params, float M, float B, float K, float target_pos, float max_torque);

/**
 * @brief Computes 1ms control loop output torque
 * @param params Pointer to parameter structure
 * @param state Pointer to state structure
 * @param dt_s Loop delta time in seconds (e.g., 0.001f for 1kHz)
 * @return 0 on success, negative error code on failure
 */
int impedance_control_step(const impedance_params_t *params, impedance_state_t *state, float dt_s);

#ifdef __cplusplus
}
#endif

#endif /* IMPEDANCE_CONTROL_H */
