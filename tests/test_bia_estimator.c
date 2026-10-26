/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

#include "bia_estimator.h"

int main(void) {
    printf("[TEST] Testing Bio-Impedance Analysis (BIA) Estimator...\n");

    bia_result_t res;
    bia_estimator_init(&res);

    // Height 175cm, Weight 70kg, Age 30, Male, Z=500 Ohm, Phase=6.5 deg
    bia_estimator_compute(&res, 175.0f, 70.0f, 30.0f, true, 500.0f, 6.5f);

    printf("TBW: %.1f L, FFM: %.1f kg, Body Fat: %.1f%%\n", res.total_body_water_l,
           res.fat_free_mass_kg, res.body_fat_percentage);

    assert(res.total_body_water_l > 30.0f && res.total_body_water_l < 55.0f);
    assert(res.fat_free_mass_kg > 40.0f && res.fat_free_mass_kg < 70.0f);
    assert(res.body_fat_percentage > 5.0f && res.body_fat_percentage < 35.0f);

    printf("[TEST] Bio-Impedance Analysis (BIA) Estimator PASSED!\n");
    return 0;
}
