/**
 * @file edge_battery_types.h
 * @brief 电池域值对象，供算法、流处理与适配器共享。
 */
#ifndef EDGE_BATTERY_TYPES_H
#define EDGE_BATTERY_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float soc;
    float voltage;
    float current;
    float temperature;
    float rated_capacity;
} battery_state_t;

typedef struct battery_limit_s {
    float min_soc;
    float max_soc;
    float max_charge_pwr;
    float max_discharge_pwr;
} battery_limit_t;

#endif /* EDGE_BATTERY_TYPES_H */
