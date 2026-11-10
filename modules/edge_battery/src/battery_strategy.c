#include "battery_strategy.h"

float algo_battery_safety_clamp(const battery_state_t *state, const battery_limit_t *limits,
                                float target_pwr) {
    float safe_pwr = target_pwr;

    // 1. SoC 保护：如果 SoC 已经过高，禁止充电
    if (state->soc >= limits->max_soc && target_pwr > 0) {
        safe_pwr = 0;
    }

    // 2. SoC 保护：如果 SoC 已经过低，禁止放电
    if (state->soc <= limits->min_soc && target_pwr < 0) {
        safe_pwr = 0;
    }

    // 3. 物理功率限值
    if (safe_pwr > limits->max_charge_pwr) {
        safe_pwr = limits->max_charge_pwr;
    }
    if (safe_pwr < -limits->max_discharge_pwr) {
        safe_pwr = -limits->max_discharge_pwr;
    }

    // 4. 温度保护示例 (简化版)
    if (state->temperature > 55.0f || state->temperature < -5.0f) {
        safe_pwr = 0;
    }

    return safe_pwr;
}
