/**
 * @file facility_coordinator.h
 * @brief Facility Microgrid & Environment Coordinator Engine
 */

#ifndef FACILITY_COORDINATOR_H
#define FACILITY_COORDINATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float grid_power_kw;       /* Current grid demand (kW) */
    float battery_soc_pct;     /* Storage Battery SOC (%) */
    float indoor_temp_c;       /* Indoor Temperature (C) */
    float indoor_co2_ppm;      /* Indoor CO2 concentration (ppm) */
    float peak_shave_threshold_kw;
} facility_telemetry_t;

typedef struct {
    float battery_charge_cmd_kw;  /* + Charge, - Discharge */
    float hvac_fan_speed_pct;     /* HVAC Fan speed (0-100%) */
    bool peak_shaving_active;
    bool fresh_air_boost;
} facility_control_cmd_t;

/**
 * @brief Initializes facility microgrid & environment coordinator
 * @param cmd Pointer to control command structure
 * @return 0 on success, negative error code on failure
 */
int facility_coordinator_init(facility_control_cmd_t *cmd);

/**
 * @brief Runs periodic 15min/60min MTT control loop iteration
 * @param telem Pointer to facility telemetry structure
 * @param cmd Output control commands
 * @return 0 on success, negative error code on failure
 */
int facility_coordinator_step(const facility_telemetry_t *telem, facility_control_cmd_t *cmd);

#ifdef __cplusplus
}
#endif

#endif /* FACILITY_COORDINATOR_H */
