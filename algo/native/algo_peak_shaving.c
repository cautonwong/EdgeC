#include "algo_peak_shaving.h"

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