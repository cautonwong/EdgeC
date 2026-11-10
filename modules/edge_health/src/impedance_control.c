/**
 * @file impedance_control.c
 * @brief Rehabilitation Robotic Impedance & Active/Passive Torque Control Loop Implementation
 */

#include "impedance_control.h"
#include <string.h>
#include <math.h>

struct impedance_controller {
    impedance_params_t params;
    impedance_state_t state;
};

_Static_assert(sizeof(struct impedance_controller) <= sizeof(impedance_controller_storage_t),
               "impedance_controller_storage_t size is insufficient");

impedance_controller_t *impedance_controller_create_in(impedance_controller_storage_t *store,
                                                      const impedance_params_t *params) {
    if (!store || !params) return NULL;
    impedance_controller_t *ctrl = (impedance_controller_t *)store->raw;
    memset(ctrl, 0, sizeof(struct impedance_controller));
    ctrl->params = *params;
    return ctrl;
}

float impedance_controller_get_torque(const impedance_controller_t *ctrl) {
    return ctrl ? ctrl->state.output_torque : 0.0f;
}

bool impedance_controller_is_assisted(const impedance_controller_t *ctrl) {
    return ctrl ? ctrl->state.active_assistance : false;
}

int impedance_controller_step(impedance_controller_t *ctrl, float pos, float vel, float force, float dt_s) {
    if (!ctrl) return -1;
    ctrl->state.current_pos = pos;
    ctrl->state.current_vel = vel;
    ctrl->state.external_force = force;
    return impedance_control_step(&ctrl->params, &ctrl->state, dt_s);
}

int impedance_control_init(impedance_params_t *params, float M, float B, float K, float target_pos, float max_torque) {
    if (!params || M <= 0.0f || B < 0.0f || K < 0.0f || max_torque <= 0.0f) {
        return -1;
    }

    params->mass = M;
    params->damping = B;
    params->stiffness = K;
    params->target_pos = target_pos;
    params->max_torque_limit = max_torque;
    return 0;
}

int impedance_control_step(const impedance_params_t *params, impedance_state_t *state, float dt_s) {
    if (!params || !state || dt_s <= 0.0f) {
        return -1;
    }

    /* Spring-Damper-Mass Impedance Equation: F_spring = K * (target_pos - current_pos) */
    float pos_error = params->target_pos - state->current_pos;
    float spring_force = params->stiffness * pos_error;
    float damping_force = -params->damping * state->current_vel;

    float desired_force = spring_force + damping_force + state->external_force;

    /* Torque conversion (assuming lever arm r = 1.0m for simplicity) */
    float raw_torque = desired_force;

    /* Safety torque saturation limit */
    if (raw_torque > params->max_torque_limit) {
        raw_torque = params->max_torque_limit;
    } else if (raw_torque < -params->max_torque_limit) {
        raw_torque = -params->max_torque_limit;
    }

    state->output_torque = raw_torque;
    state->active_assistance = (state->external_force * pos_error > 0.0f);

    return 0;
}
