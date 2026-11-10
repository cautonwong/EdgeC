#ifndef REHAB_KINEMATICS_H
#define REHAB_KINEMATICS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float shoulder_angle_deg; // 0..180 deg
    float elbow_angle_deg;    // 0..150 deg
    float wrist_angle_deg;    // -90..90 deg
    float rom_score;          // Range of Motion score 0..100%
    bool hyperextension_alert;
} rehab_kinematics_t;

void rehab_kinematics_init(rehab_kinematics_t *kin);

void rehab_kinematics_compute(rehab_kinematics_t *kin, float arm_accel[3], float forearm_accel[3]);

#ifdef __cplusplus
}
#endif

#endif // REHAB_KINEMATICS_H
