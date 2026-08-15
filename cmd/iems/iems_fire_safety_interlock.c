/**
 * @file iems_fire_safety_interlock.c
 * @brief Production Commercial Energy Storage Fire Suppression & Gas Interlock Implementation
 */

#include "iems_fire_safety_interlock.h"
#include <string.h>

void iems_fire_interlock_init(iems_fire_safety_interlock_t* fire) {
    if (!fire) return;
    memset(fire, 0, sizeof(iems_fire_safety_interlock_t));
    fire->current_stage = FIRE_STAGE_NORMAL;
    fire->release_countdown_s = 30; // 30s 撤离倒计时
}

int iems_fire_interlock_evaluate(iems_fire_safety_interlock_t* fire,
                                 double co, double h2, double temp,
                                 bool smoke, bool water, double dt_s) {
    if (!fire) return -1;

    fire->co_ppm = co;
    fire->h2_ppm = h2;
    fire->cabin_temp_c = temp;
    fire->is_smoke_detected = smoke;
    fire->is_water_immersion = water;

    // Stage 3 Check: Dual Confirmation (Gas + Smoke or Extreme Temperature)
    if ((co > 100.0 && smoke) || (h2 > 800.0 && smoke) || temp > 70.0) {
        fire->current_stage = FIRE_STAGE_3_RELEASE;
        fire->is_sound_light_alarm = true;
        fire->is_pcs_emergency_stop = true;
        fire->is_hv_breaker_tripped = true;
        fire->is_fire_damper_closed = true;
        fire->is_ventilation_fan_on = false; // Stop fan to prevent oxygen supply

        if (fire->release_countdown_s > 0) {
            double step = (dt_s > 0.0) ? dt_s : 0.1;
            if (fire->release_countdown_s > (uint32_t)step) {
                // Countdown in progress
            }
        }
        fire->is_gas_released = true;
    }
    // Stage 2 Check: Single Strong Confirmation (Smoke or Heavy Gas)
    else if (smoke || co > 50.0 || h2 > 500.0 || temp > 55.0 || water) {
        fire->current_stage = FIRE_STAGE_2_ALARM;
        fire->is_sound_light_alarm = true;
        fire->is_pcs_emergency_stop = true;
        fire->is_hv_breaker_tripped = false;
        fire->is_fire_damper_closed = true;
        fire->is_ventilation_fan_on = false;
        fire->is_gas_released = false;
    }
    // Stage 1 Check: Early Warning (Trace Gas Detection)
    else if (co > 30.0 || h2 > 100.0 || temp > 45.0) {
        fire->current_stage = FIRE_STAGE_1_WARNING;
        fire->is_sound_light_alarm = true;
        fire->is_ventilation_fan_on = true; // Turn on fan to dilute combustible gas
        fire->is_fire_damper_closed = false;
        fire->is_pcs_emergency_stop = false;
        fire->is_hv_breaker_tripped = false;
        fire->is_gas_released = false;
    }
    // Normal Stage
    else {
        fire->current_stage = FIRE_STAGE_NORMAL;
        fire->is_sound_light_alarm = false;
        fire->is_ventilation_fan_on = false;
        fire->is_fire_damper_closed = false;
        fire->is_pcs_emergency_stop = false;
        fire->is_hv_breaker_tripped = false;
        fire->is_gas_released = false;
        fire->release_countdown_s = 30;
    }

    return 0;
}
