/**
 * @file thermal_throttle.h
 * @brief Dynamic Thermal Throttle & Power Governor Engine for EdgeC
 */

#ifndef THERMAL_THROTTLE_H
#define THERMAL_THROTTLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    THERMAL_STATE_NORMAL = 0,
    THERMAL_STATE_WARNING = 1,
    THERMAL_STATE_THROTTLE = 2,
    THERMAL_STATE_CRITICAL = 3
} thermal_state_t;

typedef struct {
    float temp_celsius;
    float cpu_load_pct;
    float supply_voltage_v;
    thermal_state_t state;
    uint32_t cpu_freq_limit_mhz;
    uint32_t sample_decimation_factor;
} thermal_governor_t;

/**
 * @brief Initializes thermal governor with default hardware limits
 * @param gov Pointer to governor instance
 * @return 0 on success, negative error code on failure
 */
int thermal_governor_init(thermal_governor_t *gov);

/**
 * @brief Updates thermal metrics and evaluates throttling status
 * @param gov Pointer to governor instance
 * @param temp_c Current junction temperature in degrees C
 * @param cpu_load Current CPU load percentage (0-100%)
 * @param voltage Current supply voltage in Volts
 * @return 0 on success, negative error code on failure
 */
int thermal_governor_update(thermal_governor_t *gov, float temp_c, float cpu_load, float voltage);

#ifdef __cplusplus
}
#endif

#endif /* THERMAL_THROTTLE_H */
