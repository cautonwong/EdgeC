/**
 * @file gait_safety.h
 * @brief Multimodal Gait Analysis & Fall Hazard Active Safety Lock Engine
 */

#ifndef GAIT_SAFETY_H
#define GAIT_SAFETY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float cop_x_cm;         /* Center of Pressure X */
    float cop_y_cm;         /* Center of Pressure Y */
    float body_tilt_deg;    /* Trunk tilt angle from IMU */
    float tilt_velocity;    /* Angular velocity (deg/s) */
    float harness_tension_n;/* Safety harness load cell tension */
} gait_sensory_input_t;

typedef struct {
    bool fall_hazard_detected;
    bool emergency_lock_triggered;
    float hazard_score;     /* 0.0 to 1.0 risk level */
} gait_safety_status_t;

/**
 * @brief Initializes gait active safety engine
 * @param status Pointer to status structure
 * @return 0 on success, negative error code on failure
 */
int gait_safety_init(gait_safety_status_t *status);

/**
 * @brief Evaluates 5ms multimodal sensor frame for active fall protection
 * @param input Pointer to sensory input structure
 * @param status Output safety status
 * @return 0 on success, negative error code on failure
 */
int gait_safety_eval(const gait_sensory_input_t *input, gait_safety_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* GAIT_SAFETY_H */
