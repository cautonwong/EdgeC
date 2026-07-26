/**
 * @file facility_coordinator.c
 * @brief Facility Microgrid & Environment Coordinator Engine Implementation
 */

#include "facility_coordinator.h"
#include <string.h>

int facility_coordinator_init(facility_control_cmd_t *cmd) {
    if (!cmd) {
        return -1;
    }

    memset(cmd, 0, sizeof(facility_control_cmd_t));
    cmd->hvac_fan_speed_pct = 30.0f; /* Base ventilation */
    return 0;
}

int facility_coordinator_step(const facility_telemetry_t *telem, facility_control_cmd_t *cmd) {
    if (!telem || !cmd) {
        return -1;
    }

    /* Peak Shaving Logic: Discharge battery when grid power exceeds threshold */
    if (telem->grid_power_kw > telem->peak_shave_threshold_kw && telem->battery_soc_pct > 20.0f) {
        float deficit = telem->grid_power_kw - telem->peak_shave_threshold_kw;
        cmd->battery_charge_cmd_kw = -deficit; /* Discharge */
        cmd->peak_shaving_active = true;
    } else if (telem->grid_power_kw < telem->peak_shave_threshold_kw * 0.7f && telem->battery_soc_pct < 95.0f) {
        cmd->battery_charge_cmd_kw = 10.0f; /* Off-peak charge */
        cmd->peak_shaving_active = false;
    } else {
        cmd->battery_charge_cmd_kw = 0.0f;
        cmd->peak_shaving_active = false;
    }

    /* Environment CO2 Air Quality Control */
    if (telem->indoor_co2_ppm > 1000.0f) {
        cmd->hvac_fan_speed_pct = 100.0f;
        cmd->fresh_air_boost = true;
    } else if (telem->indoor_co2_ppm > 800.0f) {
        cmd->hvac_fan_speed_pct = 60.0f;
        cmd->fresh_air_boost = false;
    } else {
        cmd->hvac_fan_speed_pct = 30.0f;
        cmd->fresh_air_boost = false;
    }

    return 0;
}
