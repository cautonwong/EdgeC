#include "algo_peak_shaving.h"

#include <math.h>

double algo_peak_shave_calculate(double grid_power, const peak_shave_config_t *cfg) {
    if (!cfg) return 0.0;

    // 削峰逻辑
    if (grid_power > cfg->upper_threshold) {
        double gap = grid_power - cfg->upper_threshold;
        if (gap > cfg->max_discharge) gap = cfg->max_discharge;
        return gap;
    }

    // 填谷逻辑
    if (grid_power < cfg->lower_threshold) {
        double surplus = grid_power - cfg->lower_threshold;  // 负值
        if (surplus < -cfg->max_charge) surplus = -cfg->max_charge;
        return surplus;
    }

    return 0.0;
}

/**
 * @brief 带迟滞保护的工业级削峰填谷算法
 * @param last_state 上次动作状态 (1:放电, -1:充电, 0:静默)
 */
double algo_peak_shave_with_hysteresis(double grid_power, const peak_shave_config_t *cfg,
                                       int last_state) {
    if (!cfg) return 0.0;

    double deadband = cfg->upper_threshold * 0.02;  // 默认 2% 迟滞带

    // 1. 削峰逻辑 (放电)
    // 启动条件：功率 > 阈值 + 迟滞
    // 维持条件：功率 > 阈值 - 迟滞 (如果已经在放电)
    double discharge_start = cfg->upper_threshold + deadband;
    double discharge_hold = cfg->upper_threshold - deadband;

    if (grid_power > discharge_start || (last_state == 1 && grid_power > discharge_hold)) {
        double gap = grid_power - cfg->upper_threshold;
        return (gap > cfg->max_discharge) ? cfg->max_discharge : gap;
    }

    // 2. 填谷逻辑 (充电)
    double charge_start = cfg->lower_threshold - deadband;
    double charge_hold = cfg->lower_threshold + deadband;

    if (grid_power < charge_start || (last_state == -1 && grid_power < charge_hold)) {
        double surplus = grid_power - cfg->lower_threshold;  // 负值
        return (surplus < -cfg->max_charge) ? -cfg->max_charge : surplus;
    }

    return 0.0;
}
