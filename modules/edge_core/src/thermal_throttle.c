/**
 * @file thermal_throttle.c
 * @brief Dynamic Thermal Throttle & Power Governor Implementation
 */

#include "thermal_throttle.h"
#include <string.h>

int thermal_governor_init(thermal_governor_t *gov) {
    if (!gov) {
        return -1;
    }

    memset(gov, 0, sizeof(thermal_governor_t));
    gov->state = THERMAL_STATE_NORMAL;
    gov->cpu_freq_limit_mhz = 1500;
    gov->sample_decimation_factor = 1;
    return 0;
}

int thermal_governor_update(thermal_governor_t *gov, float temp_c, float cpu_load, float voltage) {
    if (!gov) {
        return -1;
    }

    gov->temp_celsius = temp_c;
    gov->cpu_load_pct = cpu_load;
    gov->supply_voltage_v = voltage;

    if (temp_c >= 85.0f || cpu_load >= 95.0f) {
        gov->state = THERMAL_STATE_CRITICAL;
        gov->cpu_freq_limit_mhz = 600;
        gov->sample_decimation_factor = 4;
    } else if (temp_c >= 75.0f || cpu_load >= 85.0f) {
        gov->state = THERMAL_STATE_THROTTLE;
        gov->cpu_freq_limit_mhz = 1000;
        gov->sample_decimation_factor = 2;
    } else if (temp_c >= 65.0f) {
        gov->state = THERMAL_STATE_WARNING;
        gov->cpu_freq_limit_mhz = 1200;
        gov->sample_decimation_factor = 1;
    } else {
        gov->state = THERMAL_STATE_NORMAL;
        gov->cpu_freq_limit_mhz = 1500;
        gov->sample_decimation_factor = 1;
    }

    return 0;
}
