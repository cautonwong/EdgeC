#ifndef GAIT_BALANCE_H
#define GAIT_BALANCE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_ARRAY_SIZE 8

typedef struct {
    float left_foot_pressure[SENSOR_ARRAY_SIZE];
    float right_foot_pressure[SENSOR_ARRAY_SIZE];
    float balance_asymmetry_index; // 0..100% asymmetry (0 = perfect balance)
    float cop_x_offset_mm;         // Center of Pressure X offset
    float cop_y_offset_mm;         // Center of Pressure Y offset
    bool fall_risk_asymmetry_alert;
} gait_balance_result_t;

void gait_balance_init(gait_balance_result_t *res);

void gait_balance_compute(gait_balance_result_t *res, const float left_sensors[SENSOR_ARRAY_SIZE], const float right_sensors[SENSOR_ARRAY_SIZE]);

#ifdef __cplusplus
}
#endif

#endif // GAIT_BALANCE_H
