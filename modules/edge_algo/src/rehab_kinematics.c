#include "rehab_kinematics.h"
#include <math.h>
#include <string.h>

void rehab_kinematics_init(rehab_kinematics_t *kin) {
    if (!kin) return;
    memset(kin, 0, sizeof(rehab_kinematics_t));
}

void rehab_kinematics_compute(rehab_kinematics_t *kin, float arm_accel[3], float forearm_accel[3]) {
    if (!kin || !arm_accel || !forearm_accel) return;

    // Calculate dot product between upper arm and forearm acceleration vectors
    float dot = arm_accel[0] * forearm_accel[0] + arm_accel[1] * forearm_accel[1] + arm_accel[2] * forearm_accel[2];
    float mag_arm = sqrtf(arm_accel[0] * arm_accel[0] + arm_accel[1] * arm_accel[1] + arm_accel[2] * arm_accel[2]);
    float mag_forearm = sqrtf(forearm_accel[0] * forearm_accel[0] + forearm_accel[1] * forearm_accel[1] + forearm_accel[2] * forearm_accel[2]);

    float cos_theta = 0.0f;
    if (mag_arm > 1e-4f && mag_forearm > 1e-4f) {
        cos_theta = dot / (mag_arm * mag_forearm);
    }

    if (cos_theta > 1.0f) cos_theta = 1.0f;
    if (cos_theta < -1.0f) cos_theta = -1.0f;

    float angle_rad = acosf(cos_theta);
    float elbow_deg = angle_rad * 180.0f / (float)M_PI;

    kin->elbow_angle_deg = elbow_deg;
    kin->shoulder_angle_deg = (float)fabs(arm_accel[1]) * 45.0f;

    // Calculate Range of Motion (ROM) score based on 150 deg max extension
    float rom = (elbow_deg / 150.0f) * 100.0f;
    if (rom > 100.0f) rom = 100.0f;
    kin->rom_score = rom;

    // Hyperextension detection > 155 degrees
    if (elbow_deg > 155.0f) {
        kin->hyperextension_alert = true;
    } else {
        kin->hyperextension_alert = false;
    }
}
