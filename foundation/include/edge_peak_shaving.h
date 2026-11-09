/**
 * @file edge_peak_shaving.h
 * @brief 削峰填谷纯算法与配置值对象。
 */
#ifndef EDGE_PEAK_SHAVING_H
#define EDGE_PEAK_SHAVING_H

#include <stdint.h>

typedef struct {
    double upper_threshold;
    double lower_threshold;
    double max_discharge;
    double max_charge;
} peak_shave_config_t;

double algo_peak_shave_calculate(double grid_power, const peak_shave_config_t *cfg);
double algo_peak_shave_with_hysteresis(double grid_power, const peak_shave_config_t *cfg,
                                       int last_state);

#endif /* EDGE_PEAK_SHAVING_H */
