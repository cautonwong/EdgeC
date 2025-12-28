#ifndef EDGE_ALGO_BATTERY_STRATEGY_H
#define EDGE_ALGO_BATTERY_STRATEGY_H

#include <stdbool.h>
#include <stdint.h>

/**
 * 电池实时状态
 */
typedef struct {
    float soc;             // 0.0 - 100.0
    float voltage;         // V
    float current;         // A (正为充电，负为放电)
    float temperature;     // °C
    float rated_capacity;  // Wh
} battery_state_t;

/**
 * 电池安全约束
 */
typedef struct {
    float min_soc;            // 最小允许SOC (如 10.0)
    float max_soc;            // 最大允许SOC (如 95.0)
    float max_charge_pwr;     // 最大充电功率 (W)
    float max_discharge_pwr;  // 最大放电功率 (W)
} battery_limit_t;

/**
 * 计算推荐的执行功率
 * @return 修正后的安全功率 (W)
 */
float algo_battery_safety_clamp(const battery_state_t *state, const battery_limit_t *limits,
                                float target_pwr);

#endif  // EDGE_ALGO_BATTERY_STRATEGY_H
