#include "bia_estimator.h"
#include <math.h>
#include <string.h>

void bia_estimator_init(bia_result_t *res) {
    if (!res) return;
    memset(res, 0, sizeof(bia_result_t));
}

void bia_estimator_compute(bia_result_t *res, float height_cm, float weight_kg, float age_years, bool is_male, float z_50khz, float phase_deg) {
    if (!res || height_cm <= 0.0f || weight_kg <= 0.0f || z_50khz <= 1.0f) return;

    res->impedance_50khz_ohm = z_50khz;
    res->phase_angle_deg = phase_deg;

    // Standard BIA Formula for Total Body Water (TBW):
    // TBW = 0.396 * (height^2 / Z) + 0.143 * weight + 8.4 * sex - 0.088 * age + 2.09
    float h2_over_z = (height_cm * height_cm) / z_50khz;
    float sex_coeff = is_male ? 1.0f : 0.0f;

    float tbw = 0.396f * h2_over_z + 0.143f * weight_kg + 8.4f * sex_coeff - 0.088f * age_years + 2.09f;
    if (tbw < 10.0f) tbw = 10.0f;
    res->total_body_water_l = tbw;

    // Fat Free Mass (FFM) = TBW / 0.73
    float ffm = tbw / 0.73f;
    if (ffm > weight_kg) ffm = weight_kg * 0.95f;
    res->fat_free_mass_kg = ffm;

    // Body Fat % = ((weight - FFM) / weight) * 100%
    float fat_kg = weight_kg - ffm;
    if (fat_kg < 0.0f) fat_kg = 0.0f;

    res->body_fat_percentage = (fat_kg / weight_kg) * 100.0f;
}
