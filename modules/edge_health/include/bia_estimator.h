#ifndef BIA_ESTIMATOR_H
#define BIA_ESTIMATOR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float impedance_50khz_ohm;  // Z at 50kHz (e.g. 500 Ohm)
    float phase_angle_deg;      // Phase angle (e.g. 6.5 deg)
    float total_body_water_l;   // TBW in Liters
    float fat_free_mass_kg;     // FFM in kg
    float body_fat_percentage;  // BF%
} bia_result_t;

void bia_estimator_init(bia_result_t *res);

void bia_estimator_compute(bia_result_t *res, float height_cm, float weight_kg, float age_years, bool is_male, float z_50khz, float phase_deg);

#ifdef __cplusplus
}
#endif

#endif // BIA_ESTIMATOR_H
