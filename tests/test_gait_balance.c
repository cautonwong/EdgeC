/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

#include "gait_balance.h"

int main(void) {
    printf("[TEST] Testing Plantar Pressure Gait Balance Analyzer...\n");

    gait_balance_result_t res;
    gait_balance_init(&res);

    // Balanced weight distribution (50N left, 50N right)
    float left[SENSOR_ARRAY_SIZE] = {6.25f, 6.25f, 6.25f, 6.25f, 6.25f, 6.25f, 6.25f, 6.25f};
    float right[SENSOR_ARRAY_SIZE] = {6.25f, 6.25f, 6.25f, 6.25f, 6.25f, 6.25f, 6.25f, 6.25f};

    gait_balance_compute(&res, left, right);

    printf("Asymmetry Index: %.1f%%, CoP Offset: %.1f mm, Fall Risk Alert: %s\n",
           res.balance_asymmetry_index, res.cop_x_offset_mm,
           res.fall_risk_asymmetry_alert ? "YES" : "NO");

    assert(res.balance_asymmetry_index == 0.0f);
    assert(!res.fall_risk_asymmetry_alert);

    // Asymmetric distribution (75N left, 25N right -> Asymmetry = 50%)
    float left_unbalanced[SENSOR_ARRAY_SIZE] = {9.375f, 9.375f, 9.375f, 9.375f,
                                                9.375f, 9.375f, 9.375f, 9.375f};
    float right_unbalanced[SENSOR_ARRAY_SIZE] = {3.125f, 3.125f, 3.125f, 3.125f,
                                                 3.125f, 3.125f, 3.125f, 3.125f};

    gait_balance_compute(&res, left_unbalanced, right_unbalanced);
    assert(res.balance_asymmetry_index == 50.0f);
    assert(res.fall_risk_asymmetry_alert);

    printf("[TEST] Plantar Pressure Gait Balance Analyzer PASSED!\n");
    return 0;
}
