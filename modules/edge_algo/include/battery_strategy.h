#ifndef EDGE_ALGO_BATTERY_STRATEGY_H
#define EDGE_ALGO_BATTERY_STRATEGY_H

#include "edge_battery_types.h"

/**
 * 计算推荐的执行功率
 * @return 修正后的安全功率 (W)
 */
float algo_battery_safety_clamp(const battery_state_t *state, const battery_limit_t *limits,
                                float target_pwr);

#endif  // EDGE_ALGO_BATTERY_STRATEGY_H