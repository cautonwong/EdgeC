#ifndef ALGO_PEAK_SHAVING_H
#define ALGO_PEAK_SHAVING_H

#include <stdint.h>

/**
 * @brief 削峰填谷决策上下文
 */
typedef struct {
    double upper_threshold;  // 削峰阈值 (W)
    double lower_threshold;  // 填谷阈值 (W)
    double max_discharge;    // 最大放电功率限制
    double max_charge;       // 最大充电功率限制
} peak_shave_config_t;

/**
 * @brief 削峰填谷计算函数 (无状态纯逻辑)
 * @param grid_power 当前电网有功功率 (正为消耗，负为反送)
 * @param cfg 策略配置
 * @return double 建议的电池功率指令 (正放负充)
 */
double algo_peak_shave_calculate(double grid_power, const peak_shave_config_t *cfg);

#endif
