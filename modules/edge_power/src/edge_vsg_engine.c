/**
 * @file edge_vsg_engine.c
 * @brief Virtual Synchronous Generator (VSG) & Fast Frequency Response Engine Implementation
 */

#include "edge_vsg_engine.h"
#include <string.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int edge_vsg_engine_init(edge_vsg_engine_t *engine, const edge_vsg_config_t *config) {
    if (!engine || !config) return -1;
    memset(engine, 0, sizeof(edge_vsg_engine_t));
    memcpy(&engine->config, config, sizeof(edge_vsg_config_t));

    if (engine->config.j_inertia <= 0.0f) engine->config.j_inertia = 5.0f;
    if (engine->config.d_damping <= 0.0f) engine->config.d_damping = 20.0f;
    if (engine->config.nominal_voltage_v <= 0.0f) engine->config.nominal_voltage_v = 380.0f;
    if (engine->config.nominal_freq_hz <= 0.0f) engine->config.nominal_freq_hz = 50.0f;
    if (engine->config.max_power_kw <= 0.0f) engine->config.max_power_kw = 500.0f;
    if (engine->config.max_current_a <= 0.0f) engine->config.max_current_a = 800.0f;

    edge_vsg_engine_reset(engine, engine->config.nominal_freq_hz, engine->config.nominal_voltage_v);
    engine->is_initialized = true;
    return 0;
}

void edge_vsg_engine_reset(edge_vsg_engine_t *engine, float initial_freq_hz, float initial_voltage_v) {
    if (!engine) return;
    engine->state.grid_freq_hz = initial_freq_hz;
    engine->state.grid_volt_rms = initial_voltage_v;
    engine->state.omega_virtual = 2.0f * (float)M_PI * initial_freq_hz;
    engine->state.theta_virtual = 0.0f;
    engine->state.p_active_kw = 0.0f;
    engine->state.q_reactive_kvar = 0.0f;
    engine->state.ffr_active_power_kw = 0.0f;
    engine->state.id_ref = 0.0f;
    engine->state.iq_ref = 0.0f;
    engine->state.step_count = 0;
}

int edge_vsg_engine_step(edge_vsg_engine_t *engine, float dt_s, float p_ref_kw,
                         float q_ref_kvar, float grid_freq_hz, float grid_volt_rms) {
    if (!engine || !engine->is_initialized || dt_s <= 0.0f) return -1;

    edge_vsg_config_t *cfg = &engine->config;
    edge_vsg_state_t *st = &engine->state;

    st->grid_freq_hz = grid_freq_hz;
    st->grid_volt_rms = grid_volt_rms;

    float omega_0 = 2.0f * (float)M_PI * cfg->nominal_freq_hz;
    float omega_grid = 2.0f * (float)M_PI * grid_freq_hz;

    /* 1. Fast Frequency Response (FFR) & Droop */
    float delta_f = grid_freq_hz - cfg->nominal_freq_hz;
    float p_droop = -cfg->droop_p_f * delta_f;
    float p_ffr = 0.0f;

    if (cfg->enable_ffr && fabsf(delta_f) > 0.05f) {
        /* FFR fast dynamic inertial boost */
        p_ffr = -cfg->droop_p_f * 2.0f * delta_f;
    }
    st->ffr_active_power_kw = p_ffr;

    float p_target = p_ref_kw + p_droop + p_ffr;
    if (p_target > cfg->max_power_kw) p_target = cfg->max_power_kw;
    if (p_target < -cfg->max_power_kw) p_target = -cfg->max_power_kw;

    /* 2. 2nd-order virtual rotor swing equation:
     * J * d(omega)/dt = (P_target - P_e)/omega_0 - D * (omega - omega_0)
     */
    float delta_p = p_target - st->p_active_kw;
    float torque_mech = (delta_p * 1000.0f) / omega_0; /* Nm */
    float torque_damping = cfg->d_damping * (st->omega_virtual - omega_0);

    float d_omega = ((torque_mech - torque_damping) / cfg->j_inertia) * dt_s;
    st->omega_virtual += d_omega;

    /* Prevent runaway */
    if (st->omega_virtual < 0.8f * omega_0) st->omega_virtual = 0.8f * omega_0;
    if (st->omega_virtual > 1.2f * omega_0) st->omega_virtual = 1.2f * omega_0;

    /* Phase integration */
    st->theta_virtual += st->omega_virtual * dt_s;
    if (st->theta_virtual > 2.0f * (float)M_PI) {
        st->theta_virtual -= 2.0f * (float)M_PI;
    }

    /* 3. Virtual Stator Voltage & Q-V Droop */
    float delta_v = grid_volt_rms - cfg->nominal_voltage_v;
    float q_droop = -cfg->droop_q_v * delta_v;
    float q_target = q_ref_kvar + q_droop;

    /* 4. Converter Current Reference (id, iq) calculation */
    /* Id corresponds to active power (P = 1.5 * Vd * Id) */
    float vd = (grid_volt_rms > 10.0f) ? grid_volt_rms * sqrtf(2.0f / 3.0f) : 10.0f;
    st->id_ref = (p_target * 1000.0f) / (1.5f * vd);
    st->iq_ref = -(q_target * 1000.0f) / (1.5f * vd);

    /* Current limit clamping */
    float i_mag = sqrtf(st->id_ref * st->id_ref + st->iq_ref * st->iq_ref);
    if (i_mag > cfg->max_current_a && i_mag > 0.0f) {
        float scale = cfg->max_current_a / i_mag;
        st->id_ref *= scale;
        st->iq_ref *= scale;
    }

    /* First order lag update for actual electrical power response */
    float tau = 0.005f; /* 5ms converter lag */
    float alpha = dt_s / (tau + dt_s);
    st->p_active_kw += alpha * (p_target - st->p_active_kw);
    st->q_reactive_kvar += alpha * (q_target - st->q_reactive_kvar);

    st->step_count++;
    return 0;
}

int edge_vsg_engine_get_state(const edge_vsg_engine_t *engine, edge_vsg_state_t *out_state) {
    if (!engine || !out_state) return -1;
    memcpy(out_state, &engine->state, sizeof(edge_vsg_state_t));
    return 0;
}
