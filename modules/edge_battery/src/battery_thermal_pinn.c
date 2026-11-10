/**
 * @file battery_thermal_pinn.c
 * @brief Megawatt ESS Battery Thermal Runaway Early Warning & PINN Digital Twin Implementation
 */

#include "battery_thermal_pinn.h"
#include <string.h>
#include <math.h>

int battery_thermal_pinn_init(battery_thermal_pinn_t *engine, const pinn_config_t *config) {
    if (!engine || !config) return -1;
    memset(engine, 0, sizeof(battery_thermal_pinn_t));
    memcpy(&engine->config, config, sizeof(pinn_config_t));

    if (engine->config.nominal_capacity_ah <= 0.0f) engine->config.nominal_capacity_ah = 280.0f;
    if (engine->config.c_th_joule_per_k <= 0.0f) engine->config.c_th_joule_per_k = 1500.0f;
    if (engine->config.r_th_k_per_w <= 0.0f) engine->config.r_th_k_per_w = 0.8f;
    if (engine->config.r_isc_alarm_ohm <= 0.0f) engine->config.r_isc_alarm_ohm = 50.0f;
    if (engine->config.r_isc_trip_ohm <= 0.0f) engine->config.r_isc_trip_ohm = 5.0f;
    if (engine->config.q_abnormal_trip_w <= 0.0f) engine->config.q_abnormal_trip_w = 15.0f;

    return 0;
}

int battery_thermal_pinn_register_cell(battery_thermal_pinn_t *engine, uint32_t cell_id) {
    if (!engine || cell_id == 0) return -1;
    if (engine->channel_count >= PINN_MAX_CELLS) return -2;

    for (uint32_t i = 0; i < engine->channel_count; i++) {
        if (engine->channels[i].cell_id == cell_id) {
            return 0; /* Already registered */
        }
    }

    uint32_t idx = engine->channel_count++;
    pinn_cell_channel_t *ch = &engine->channels[idx];
    memset(ch, 0, sizeof(pinn_cell_channel_t));
    ch->cell_id = cell_id;
    ch->current_risk.cell_id = cell_id;
    ch->current_risk.r0_mohm = 0.5f;
    ch->current_risk.r_isc_ohm = 10000.0f; /* Normal mega-ohm scale */
    ch->current_risk.warning_level = PINN_WARN_NORMAL;

    engine->telemetry.active_cells_monitored = engine->channel_count;
    return 0;
}

int battery_thermal_pinn_feed_sample(battery_thermal_pinn_t *engine, uint32_t cell_id,
                                     float voltage_v, float current_a, float temp_c,
                                     uint64_t timestamp_ns, cell_thermal_risk_t *out_risk) {
    if (!engine) return -1;

    int ch_idx = -1;
    for (uint32_t i = 0; i < engine->channel_count; i++) {
        if (engine->channels[i].cell_id == cell_id) {
            ch_idx = (int)i;
            break;
        }
    }

    if (ch_idx < 0) {
        if (battery_thermal_pinn_register_cell(engine, cell_id) != 0) return -2;
        ch_idx = (int)(engine->channel_count - 1);
    }

    pinn_cell_channel_t *ch = &engine->channels[ch_idx];
    pinn_config_t *cfg = &engine->config;

    /* Push sample into window */
    uint32_t s_idx = ch->sample_count % PINN_WINDOW_SIZE;
    ch->window[s_idx].voltage_v = voltage_v;
    ch->window[s_idx].current_a = current_a;
    ch->window[s_idx].temp_c = temp_c;
    ch->sample_count++;

    /* 1. Calculate Joule heating: Q_joule = I^2 * R0 (W) */
    float r0_ohm = (ch->current_risk.r0_mohm > 0.0f) ? (ch->current_risk.r0_mohm / 1000.0f) : 0.0005f;
    float q_joule = current_a * current_a * r0_ohm;

    /* 2. Physics thermal PDE equilibrium:
     * C_th * dT/dt + (T - T_amb) / R_th = Q_joule + Q_abnormal
     */
    float dt = 0.1f; /* 100ms standard sampling period */
    float temp_diff = temp_c - cfg->temp_ambient_c;
    float q_dissipation = temp_diff / cfg->r_th_k_per_w;

    float dT_dt = 0.0f;
    if (ch->sample_count >= 2) {
        uint32_t prev_idx = (ch->sample_count - 2) % PINN_WINDOW_SIZE;
        dT_dt = (temp_c - ch->window[prev_idx].temp_c) / dt;
    }

    /* Solved abnormal thermal generation */
    float q_total_measured = cfg->c_th_joule_per_k * dT_dt + q_dissipation;
    float q_abnormal = q_total_measured - q_joule;
    if (q_abnormal < 0.0f) q_abnormal = 0.0f;

    /* 3. Electro-chemical Micro-Internal Short Circuit (ISC) resistance estimation:
     * P_isc = V^2 / R_isc -> R_isc = V^2 / Q_abnormal
     */
    float r_isc = 10000.0f;
    if (q_abnormal > 0.05f && voltage_v > 2.0f) {
        r_isc = (voltage_v * voltage_v) / q_abnormal;
    }

    /* 4. PINN 1-hour ahead temperature projection:
     * T_1h = T_ambient + R_th * (Q_joule + Q_abnormal)
     */
    float predicted_temp_1h = cfg->temp_ambient_c + cfg->r_th_k_per_w * (q_joule + q_abnormal);

    /* 5. Comprehensive Risk Scoring & Level Arbitration */
    float risk_score = 0.0f;

    /* Risk contributors */
    float r_score_isc = (r_isc < cfg->r_isc_alarm_ohm) ? (1.0f - (r_isc / cfg->r_isc_alarm_ohm)) : 0.0f;
    float r_score_q = (q_abnormal / cfg->q_abnormal_trip_w);
    float r_score_temp = (temp_c > 45.0f) ? ((temp_c - 45.0f) / 35.0f) : 0.0f;

    risk_score = 0.5f * r_score_isc + 0.3f * r_score_q + 0.2f * r_score_temp;
    if (risk_score > 1.0f) risk_score = 1.0f;
    if (risk_score < 0.0f) risk_score = 0.0f;

    pinn_warning_level_t level = PINN_WARN_NORMAL;
    if (r_isc <= cfg->r_isc_trip_ohm || q_abnormal >= cfg->q_abnormal_trip_w || temp_c >= 75.0f) {
        level = PINN_WARN_TRIP;
    } else if (r_isc <= cfg->r_isc_alarm_ohm || q_abnormal >= (cfg->q_abnormal_trip_w * 0.5f) || temp_c >= 55.0f) {
        level = PINN_WARN_ALARM;
    } else if (risk_score >= 0.25f || temp_c >= 45.0f) {
        level = PINN_WARN_NOTICE;
    }

    /* Update current state */
    cell_thermal_risk_t *risk = &ch->current_risk;
    risk->r_isc_ohm = r_isc;
    risk->thermal_generation_w = q_abnormal;
    risk->predicted_temp_1h = predicted_temp_1h;
    risk->runaway_risk_score = risk_score;
    risk->warning_level = level;
    risk->update_timestamp_ns = timestamp_ns;

    if (out_risk) {
        memcpy(out_risk, risk, sizeof(cell_thermal_risk_t));
    }

    /* Update engine telemetry */
    engine->telemetry.total_inference_cycles++;
    if (risk_score > engine->telemetry.max_risk_score) {
        engine->telemetry.max_risk_score = risk_score;
    }
    if (q_abnormal > engine->telemetry.max_q_abnormal_w) {
        engine->telemetry.max_q_abnormal_w = q_abnormal;
    }

    return 0;
}

int battery_thermal_pinn_get_cell_risk(const battery_thermal_pinn_t *engine, uint32_t cell_id,
                                       cell_thermal_risk_t *out_risk) {
    if (!engine || !out_risk) return -1;
    for (uint32_t i = 0; i < engine->channel_count; i++) {
        if (engine->channels[i].cell_id == cell_id) {
            memcpy(out_risk, &engine->channels[i].current_risk, sizeof(cell_thermal_risk_t));
            return 0;
        }
    }
    return -2;
}

void battery_thermal_pinn_get_telemetry(const battery_thermal_pinn_t *engine, pinn_telemetry_t *out_telem) {
    if (engine && out_telem) {
        memcpy(out_telem, &engine->telemetry, sizeof(pinn_telemetry_t));

        /* Count cell warning levels */
        out_telem->cells_in_notice = 0;
        out_telem->cells_in_alarm = 0;
        out_telem->cells_in_trip = 0;
        for (uint32_t i = 0; i < engine->channel_count; i++) {
            switch (engine->channels[i].current_risk.warning_level) {
                case PINN_WARN_NOTICE: out_telem->cells_in_notice++; break;
                case PINN_WARN_ALARM:  out_telem->cells_in_alarm++; break;
                case PINN_WARN_TRIP:   out_telem->cells_in_trip++; break;
                default: break;
            }
        }
    }
}
