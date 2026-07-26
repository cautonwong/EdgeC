#include "gait_balance.h"
#include <math.h>
#include <string.h>

void gait_balance_init(gait_balance_result_t *res) {
    if (!res) return;
    memset(res, 0, sizeof(gait_balance_result_t));
}

void gait_balance_compute(gait_balance_result_t *res, const float left_sensors[SENSOR_ARRAY_SIZE], const float right_sensors[SENSOR_ARRAY_SIZE]) {
    if (!res || !left_sensors || !right_sensors) return;

    float total_left = 0.0f;
    float total_right = 0.0f;

    for (size_t i = 0; i < SENSOR_ARRAY_SIZE; i++) {
        res->left_foot_pressure[i] = left_sensors[i];
        res->right_foot_pressure[i] = right_sensors[i];
        total_left += left_sensors[i];
        total_right += right_sensors[i];
    }

    float total_force = total_left + total_right;
    if (total_force <= 1e-4f) {
        res->balance_asymmetry_index = 0.0f;
        res->fall_risk_asymmetry_alert = false;
        return;
    }

    // Asymmetry Index Formula = |Left - Right| / (Left + Right) * 100%
    float asym = (fabsf(total_left - total_right) / total_force) * 100.0f;
    res->balance_asymmetry_index = asym;

    // Center of Pressure (CoP) Lateral Offset
    res->cop_x_offset_mm = ((total_right - total_left) / total_force) * 100.0f;

    // Fall Risk Alert if asymmetry > 25%
    if (asym > 25.0f) {
        res->fall_risk_asymmetry_alert = true;
    } else {
        res->fall_risk_asymmetry_alert = false;
    }
}
