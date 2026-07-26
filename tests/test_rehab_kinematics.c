#include <stdio.h>
#include <assert.h>
#include "rehab_kinematics.h"

int main(void) {
    printf("[TEST] Testing Rehabilitation Joint Angle Kinematics Estimator...\n");

    rehab_kinematics_t kin;
    rehab_kinematics_init(&kin);

    // Test orthogonal vectors (90 degree elbow bend)
    float arm[3] = {0.0f, 0.0f, 9.8f};
    float forearm[3] = {9.8f, 0.0f, 0.0f};

    rehab_kinematics_compute(&kin, arm, forearm);

    printf("Elbow Angle: %.1f deg, ROM Score: %.1f%%, Hyperextension: %s\n",
           kin.elbow_angle_deg, kin.rom_score, kin.hyperextension_alert ? "YES" : "NO");

    assert(kin.elbow_angle_deg >= 89.0f && kin.elbow_angle_deg <= 91.0f);
    assert(!kin.hyperextension_alert);

    printf("[TEST] Rehabilitation Joint Angle Kinematics Estimator PASSED!\n");
    return 0;
}
