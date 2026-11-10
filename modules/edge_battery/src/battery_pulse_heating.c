/**
 * @file battery_pulse_heating.c
 * @brief 极端低温环境电池脉冲自加热与热电耦合控制状态机实现 (AEP-738)
 */

#include "battery_pulse_heating.h"
#include <math.h>
#include <string.h>

#define GAS_CONSTANT_R 8.314462f
#define KELVIN_OFFSET 273.15f
#define REFERENCE_TEMP_K 298.15f /* 25.0 °C */

static const pulse_heat_config_t DEFAULT_CONFIG = {
    .target_temp_c = 10.0f,
    .min_start_temp_c = 0.0f,
    .max_temp_limit_c = 45.0f,
    .max_thermal_gradient_c = 6.0f,
    .max_cell_temp_spread_c = 4.0f,
    .min_soc = 0.15f,
    .max_soc = 0.90f,
    .max_current_amp_a = 150.0f,
    .nominal_frequency_hz = 500.0f,
    .sweep_freq_start_hz = 100.0f,
    .sweep_freq_end_hz = 1000.0f,
    .current_ramp_rate_a_per_s = 30.0f,
    .active_burst_duration_s = 20.0f,
    .homogenize_duration_s = 4.0f,
    .timeout_limit_s = 600.0f,
    .cell_v_min_nominal = 2.0f,
    .cell_v_max_nominal = 3.8f,
    .cell_heat_capacity_j_per_k = 450.0f,
    .core_surface_r_th = 0.8f,
    .surface_ambient_r_th = 3.5f,
    .baseline_resistance_ohm = 0.0012f,
    .arrhenius_activation_energy_j = 28000.0f
};

static float calculate_arrhenius_resistance(const pulse_heat_config_t *cfg, float temp_c) {
    float temp_k = temp_c + KELVIN_OFFSET;
    if (temp_k < 200.0f) {
        temp_k = 200.0f;
    }
    float exponent = (cfg->arrhenius_activation_energy_j / GAS_CONSTANT_R) *
                     ((1.0f / temp_k) - (1.0f / REFERENCE_TEMP_K));
    if (exponent > 5.0f) {
        exponent = 5.0f; /* 限制倍率上限，防止数值溢出 */
    }
    return cfg->baseline_resistance_ohm * expf(exponent);
}

#include <stdalign.h>

struct pulse_heat_engine {
    pulse_heat_config_t cfg;
    pulse_heat_state_t current_state;
    pulse_heat_fault_t active_fault;
    float current_cmd_a;
    float target_cmd_a;
    float optimal_freq_hz;
    float core_temp_est_c;
    float surf_temp_est_c;
    float state_timer_s;
    float total_elapsed_s;
    float cumulative_energy_wh;
    float last_dt_s;
    float prev_temp_for_rate_c;
    float rate_timer_s;
    float calculated_rate_c_min;
    bool initialized;
    bool temp_initialized;
};

_Static_assert(sizeof(struct pulse_heat_engine) <= sizeof(pulse_heat_engine_storage_t),
               "pulse_heat_engine_storage_t too small for struct pulse_heat_engine");
_Static_assert(alignof(struct pulse_heat_engine) <= alignof(pulse_heat_engine_storage_t),
               "pulse_heat_engine_storage_t bad alignment for struct pulse_heat_engine");

pulse_heat_engine_t *pulse_heat_engine_create_in(pulse_heat_engine_storage_t *store, const pulse_heat_config_t *cfg) {
    if (!store) {
        return NULL;
    }
    pulse_heat_engine_t *engine = (pulse_heat_engine_t *)store;
    if (pulse_heat_engine_init(engine, cfg) != 0) {
        return NULL;
    }
    return engine;
}

int pulse_heat_engine_init(pulse_heat_engine_t *engine, const pulse_heat_config_t *cfg) {
    if (!engine) {
        return -1;
    }
    memset(engine, 0, sizeof(*engine));
    if (cfg) {
        engine->cfg = *cfg;
    } else {
        engine->cfg = DEFAULT_CONFIG;
    }
    engine->current_state = PULSE_HEAT_STATE_IDLE;
    engine->active_fault = PULSE_HEAT_FAULT_NONE;
    engine->optimal_freq_hz = engine->cfg.nominal_frequency_hz;
    engine->initialized = true;
    engine->temp_initialized = false;
    return 0;
}

int pulse_heat_engine_reset(pulse_heat_engine_t *engine) {
    if (!engine || !engine->initialized) {
        return -1;
    }
    engine->current_state = PULSE_HEAT_STATE_IDLE;
    engine->active_fault = PULSE_HEAT_FAULT_NONE;
    engine->current_cmd_a = 0.0f;
    engine->target_cmd_a = 0.0f;
    engine->state_timer_s = 0.0f;
    engine->total_elapsed_s = 0.0f;
    engine->cumulative_energy_wh = 0.0f;
    engine->rate_timer_s = 0.0f;
    engine->calculated_rate_c_min = 0.0f;
    engine->temp_initialized = false;
    return 0;
}

int pulse_heat_engine_start(pulse_heat_engine_t *engine) {
    if (!engine || !engine->initialized) {
        return -1;
    }
    if (engine->current_state == PULSE_HEAT_STATE_FAULT) {
        return -2; /* 故障态必须先显式 reset */
    }
    if (engine->current_state == PULSE_HEAT_STATE_IDLE ||
        engine->current_state == PULSE_HEAT_STATE_COMPLETED) {
        engine->current_state = PULSE_HEAT_STATE_PRECHECK;
        engine->active_fault = PULSE_HEAT_FAULT_NONE;
        engine->state_timer_s = 0.0f;
        engine->total_elapsed_s = 0.0f;
        engine->current_cmd_a = 0.0f;
        return 0;
    }
    return 0;
}

int pulse_heat_engine_stop(pulse_heat_engine_t *engine) {
    if (!engine || !engine->initialized) {
        return -1;
    }
    engine->current_state = PULSE_HEAT_STATE_IDLE;
    engine->current_cmd_a = 0.0f;
    engine->target_cmd_a = 0.0f;
    return 0;
}

typedef struct {
    float v_min;
    float v_max;
    float t_max;
    float t_surf_avg;
    float temp_spread_c;
    float total_pack_heat_power_w;
    float temp_gradient_c;
    float plating_margin_v;
} pulse_heat_estimate_t;

static void pulse_heat_fill_fault(pulse_heat_engine_t *engine, pulse_heat_control_output_t *out,
                                  pulse_heat_fault_t fault) {
    engine->current_state = PULSE_HEAT_STATE_FAULT;
    engine->active_fault = fault;
    engine->current_cmd_a = 0.0f;
    engine->target_cmd_a = 0.0f;

    out->state = engine->current_state;
    out->fault = engine->active_fault;
    out->inverter_enable = false;
    out->target_current_amp_a = 0.0f;
    out->target_frequency_hz = engine->optimal_freq_hz;
    out->duty_ratio = 0.50f;
    out->estimated_core_temp_c = engine->core_temp_est_c;
    out->estimated_surface_temp_c = engine->surf_temp_est_c;
    out->temp_gradient_c = fabsf(engine->core_temp_est_c - engine->surf_temp_est_c);
    out->cell_temp_spread_c = 0.0f;
    out->estimated_heating_power_w = 0.0f;
    out->heating_rate_c_per_min = engine->calculated_rate_c_min;
    out->plating_safety_margin_v = 0.0f;
    out->elapsed_heating_time_s = engine->total_elapsed_s;
    out->total_energy_consumed_wh = engine->cumulative_energy_wh;
}

static bool pulse_heat_telemetry_valid(const pulse_heat_cell_telemetry_t *telem) {
    if (telem->cell_count == 0 || telem->cell_count > PULSE_HEAT_MAX_CELLS ||
        telem->temp_count == 0 || telem->temp_count > PULSE_HEAT_MAX_TEMPS ||
        !isfinite(telem->ambient_temp_c) || !isfinite(telem->pack_soc) ||
        !isfinite(telem->actual_current_rms_a) || !isfinite(telem->actual_frequency_hz)) {
        return false;
    }
    for (uint16_t i = 0; i < telem->cell_count; i++) {
        if (!isfinite(telem->cell_voltages[i])) return false;
    }
    for (uint16_t i = 0; i < telem->temp_count; i++) {
        if (!isfinite(telem->cell_temps[i])) return false;
    }
    return true;
}

static void pulse_heat_measure_cells(const pulse_heat_cell_telemetry_t *telem,
                                     pulse_heat_estimate_t *est) {
    float v_min = 1000.0f;
    float v_max = -1000.0f;
    uint16_t cells = telem->cell_count;
    for (uint16_t i = 0; i < cells; i++) {
        float v = telem->cell_voltages[i];
        if (v < v_min) v_min = v;
        if (v > v_max) v_max = v;
    }

    float t_min = 1000.0f;
    float t_max = -1000.0f;
    float t_sum = 0.0f;
    uint16_t temps = telem->temp_count;
    for (uint16_t i = 0; i < temps; i++) {
        float t = telem->cell_temps[i];
        if (t < t_min) t_min = t;
        if (t > t_max) t_max = t;
        t_sum += t;
    }

    est->v_min = v_min;
    est->v_max = v_max;
    est->t_max = t_max;
    est->t_surf_avg = t_sum / (float)temps;
    est->temp_spread_c = (temps > 1) ? (t_max - t_min) : 0.0f;
}

static void pulse_heat_update_thermal(pulse_heat_engine_t *engine,
                                      const pulse_heat_cell_telemetry_t *telem,
                                      float dt_s, pulse_heat_estimate_t *est) {
    const pulse_heat_config_t *cfg = &engine->cfg;

    if (!engine->temp_initialized) {
        engine->core_temp_est_c = est->t_surf_avg;
        engine->surf_temp_est_c = est->t_surf_avg;
        engine->prev_temp_for_rate_c = est->t_surf_avg;
        engine->temp_initialized = true;
    }

    float r_ac_cell = calculate_arrhenius_resistance(cfg, engine->core_temp_est_c);
    float i_rms = telem->actual_current_rms_a;
    if (i_rms <= 0.0f && engine->current_cmd_a > 0.0f) {
        i_rms = engine->current_cmd_a * 0.7071f;
    }
    float single_cell_heat_power_w = (i_rms * i_rms) * r_ac_cell;
    float total_pack_heat_power_w = single_cell_heat_power_w * (float)telem->cell_count;

    float c_core = cfg->cell_heat_capacity_j_per_k * 0.65f;
    float c_surf = cfg->cell_heat_capacity_j_per_k * 0.35f;
    float r_cs = (cfg->core_surface_r_th > 0.01f) ? cfg->core_surface_r_th : 0.8f;
    float r_sa = (cfg->surface_ambient_r_th > 0.01f) ? cfg->surface_ambient_r_th : 3.5f;

    float q_core_to_surf = (engine->core_temp_est_c - engine->surf_temp_est_c) / r_cs;
    float q_surf_to_amb = (engine->surf_temp_est_c - telem->ambient_temp_c) / r_sa;

    float d_t_core = ((single_cell_heat_power_w - q_core_to_surf) / c_core) * dt_s;
    float d_t_surf = ((q_core_to_surf - q_surf_to_amb) / c_surf) * dt_s;

    engine->core_temp_est_c += d_t_core;
    engine->surf_temp_est_c += d_t_surf;

    if (telem->temp_count > 0) {
        float observer_gain = 0.05f * (dt_s / 0.1f);
        if (observer_gain > 0.2f) observer_gain = 0.2f;
        engine->surf_temp_est_c += observer_gain * (est->t_surf_avg - engine->surf_temp_est_c);
    }

    est->temp_gradient_c = fabsf(engine->core_temp_est_c - engine->surf_temp_est_c);
    est->total_pack_heat_power_w = total_pack_heat_power_w;

    float temp_subzero_deg = (engine->core_temp_est_c < 0.0f) ? (-engine->core_temp_est_c) : 0.0f;
    float v_max_dynamic = cfg->cell_v_max_nominal - (0.015f * temp_subzero_deg);
    est->plating_margin_v = v_max_dynamic - est->v_max;

    engine->rate_timer_s += dt_s;
    if (engine->rate_timer_s >= 2.0f) {
        float dt_rate_min = engine->rate_timer_s / 60.0f;
        float inst_rate = (engine->core_temp_est_c - engine->prev_temp_for_rate_c) / dt_rate_min;
        engine->calculated_rate_c_min = 0.7f * engine->calculated_rate_c_min + 0.3f * inst_rate;
        engine->prev_temp_for_rate_c = engine->core_temp_est_c;
        engine->rate_timer_s = 0.0f;
    }
}

static void pulse_heat_supervise(pulse_heat_engine_t *engine,
                                 const pulse_heat_cell_telemetry_t *telem,
                                 const pulse_heat_estimate_t *est, float dt_s) {
    const pulse_heat_config_t *cfg = &engine->cfg;
    if (engine->current_state == PULSE_HEAT_STATE_IDLE ||
        engine->current_state == PULSE_HEAT_STATE_COMPLETED ||
        engine->current_state == PULSE_HEAT_STATE_FAULT) {
        return;
    }

    engine->total_elapsed_s += dt_s;
    engine->state_timer_s += dt_s;

    float power_kw = est->total_pack_heat_power_w / 1000.0f;
    engine->cumulative_energy_wh += (power_kw * (dt_s / 3600.0f)) * 1000.0f;

    if (engine->total_elapsed_s > cfg->timeout_limit_s) {
        engine->current_state = PULSE_HEAT_STATE_FAULT;
        engine->active_fault = PULSE_HEAT_FAULT_TIMEOUT;
    } else if (est->t_max > cfg->max_temp_limit_c ||
               engine->core_temp_est_c > cfg->max_temp_limit_c) {
        engine->current_state = PULSE_HEAT_STATE_FAULT;
        engine->active_fault = PULSE_HEAT_FAULT_OVERTEMP;
    } else if (est->v_max > cfg->cell_v_max_nominal + 0.05f) {
        engine->current_state = PULSE_HEAT_STATE_FAULT;
        engine->active_fault = PULSE_HEAT_FAULT_CELL_OVERVOLTAGE;
    } else if (est->v_min < cfg->cell_v_min_nominal - 0.05f) {
        engine->current_state = PULSE_HEAT_STATE_FAULT;
        engine->active_fault = PULSE_HEAT_FAULT_CELL_UNDERVOLTAGE;
    } else if (est->temp_spread_c > cfg->max_cell_temp_spread_c) {
        engine->current_state = PULSE_HEAT_STATE_FAULT;
        engine->active_fault = PULSE_HEAT_FAULT_HIGH_THERMAL_GRADIENT;
    } else if (telem->pack_soc < cfg->min_soc || telem->pack_soc > cfg->max_soc) {
        engine->current_state = PULSE_HEAT_STATE_FAULT;
        engine->active_fault = PULSE_HEAT_FAULT_SOC_OUT_OF_BOUNDS;
    }
}

static void pulse_heat_advance(pulse_heat_engine_t *engine,
                               const pulse_heat_cell_telemetry_t *telem,
                               const pulse_heat_estimate_t *est, float dt_s) {
    const pulse_heat_config_t *cfg = &engine->cfg;
    switch (engine->current_state) {
    case PULSE_HEAT_STATE_IDLE:
        engine->current_cmd_a = 0.0f;
        engine->target_cmd_a = 0.0f;
        break;

    case PULSE_HEAT_STATE_PRECHECK:
        engine->current_cmd_a = 0.0f;
        engine->target_cmd_a = 0.0f;
        if (!telem->inverter_ready) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_INVERTER_NOT_READY;
            break;
        }
        if (est->t_surf_avg >= cfg->target_temp_c &&
            engine->core_temp_est_c >= cfg->target_temp_c) {
            engine->current_state = PULSE_HEAT_STATE_COMPLETED;
            break;
        }
        engine->current_state = PULSE_HEAT_STATE_FREQ_SWEEP;
        engine->current_cmd_a = 10.0f;
        engine->state_timer_s = 0.0f;
        break;

    case PULSE_HEAT_STATE_FREQ_SWEEP:
        engine->current_cmd_a = 10.0f;
        if (engine->state_timer_s < 1.0f) {
            float sweep_progress = engine->state_timer_s / 1.0f;
            engine->optimal_freq_hz = cfg->sweep_freq_start_hz +
                                     sweep_progress * (cfg->sweep_freq_end_hz - cfg->sweep_freq_start_hz);
        } else {
            engine->optimal_freq_hz = cfg->nominal_frequency_hz;
            engine->target_cmd_a = cfg->max_current_amp_a;
            engine->current_state = PULSE_HEAT_STATE_RAMP_UP;
            engine->state_timer_s = 0.0f;
        }
        break;

    case PULSE_HEAT_STATE_RAMP_UP: {
        float safe_target_a = cfg->max_current_amp_a;
        if (est->plating_margin_v < 0.10f) {
            float throttle_ratio = est->plating_margin_v / 0.10f;
            if (throttle_ratio < 0.2f) throttle_ratio = 0.2f;
            safe_target_a *= throttle_ratio;
        }
        engine->target_cmd_a = safe_target_a;

        float max_step = cfg->current_ramp_rate_a_per_s * dt_s;
        if (engine->current_cmd_a < engine->target_cmd_a) {
            engine->current_cmd_a += max_step;
            if (engine->current_cmd_a >= engine->target_cmd_a) {
                engine->current_cmd_a = engine->target_cmd_a;
                engine->current_state = PULSE_HEAT_STATE_ACTIVE_HEATING;
                engine->state_timer_s = 0.0f;
            }
        } else {
            engine->current_cmd_a = engine->target_cmd_a;
            engine->current_state = PULSE_HEAT_STATE_ACTIVE_HEATING;
            engine->state_timer_s = 0.0f;
        }
        break;
    }

    case PULSE_HEAT_STATE_ACTIVE_HEATING: {
        float safe_target_a = cfg->max_current_amp_a;
        if (est->plating_margin_v < 0.10f) {
            float throttle_ratio = est->plating_margin_v / 0.10f;
            if (throttle_ratio < 0.2f) throttle_ratio = 0.2f;
            safe_target_a *= throttle_ratio;
        }
        engine->target_cmd_a = safe_target_a;

        float max_step = cfg->current_ramp_rate_a_per_s * dt_s;
        if (fabsf(engine->current_cmd_a - engine->target_cmd_a) <= max_step) {
            engine->current_cmd_a = engine->target_cmd_a;
        } else if (engine->current_cmd_a < engine->target_cmd_a) {
            engine->current_cmd_a += max_step;
        } else {
            engine->current_cmd_a -= max_step;
        }

        if (engine->core_temp_est_c >= cfg->target_temp_c &&
            (est->t_surf_avg >= (cfg->target_temp_c - 4.0f) ||
             engine->core_temp_est_c >= (cfg->target_temp_c + 2.0f))) {
            engine->current_cmd_a = 0.0f;
            engine->current_state = PULSE_HEAT_STATE_COMPLETED;
            engine->state_timer_s = 0.0f;
            break;
        }

        if (est->temp_gradient_c >= cfg->max_thermal_gradient_c) {
            engine->current_cmd_a = 0.0f;
            engine->current_state = PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE;
            engine->state_timer_s = 0.0f;
            break;
        }

        if (engine->state_timer_s >= cfg->active_burst_duration_s) {
            engine->current_cmd_a = 0.0f;
            engine->current_state = PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE;
            engine->state_timer_s = 0.0f;
            break;
        }
        break;
    }

    case PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE:
        engine->current_cmd_a = 0.0f;
        if (engine->state_timer_s >= cfg->homogenize_duration_s) {
            if (engine->core_temp_est_c >= cfg->target_temp_c &&
                (est->t_surf_avg >= (cfg->target_temp_c - 4.0f) ||
                 engine->core_temp_est_c >= (cfg->target_temp_c + 2.0f))) {
                engine->current_state = PULSE_HEAT_STATE_COMPLETED;
                engine->state_timer_s = 0.0f;
            } else {
                engine->current_state = PULSE_HEAT_STATE_RAMP_UP;
                engine->state_timer_s = 0.0f;
            }
        }
        break;

    case PULSE_HEAT_STATE_COMPLETED:
        engine->current_cmd_a = 0.0f;
        engine->target_cmd_a = 0.0f;
        break;

    case PULSE_HEAT_STATE_FAULT:
    default:
        engine->current_cmd_a = 0.0f;
        engine->target_cmd_a = 0.0f;
        break;
    }
}

static void pulse_heat_emit(pulse_heat_engine_t *engine, pulse_heat_control_output_t *out,
                            const pulse_heat_estimate_t *est) {
    out->state = engine->current_state;
    out->fault = engine->active_fault;
    out->target_current_amp_a = engine->current_cmd_a;
    out->target_frequency_hz = engine->optimal_freq_hz;
    out->duty_ratio = 0.50f;
    out->inverter_enable = (engine->current_state == PULSE_HEAT_STATE_FREQ_SWEEP ||
                            engine->current_state == PULSE_HEAT_STATE_RAMP_UP ||
                            engine->current_state == PULSE_HEAT_STATE_ACTIVE_HEATING) &&
                           (engine->current_cmd_a > 0.0f);

    out->estimated_core_temp_c = engine->core_temp_est_c;
    out->estimated_surface_temp_c = engine->surf_temp_est_c;
    out->temp_gradient_c = est->temp_gradient_c;
    out->cell_temp_spread_c = est->temp_spread_c;
    out->estimated_heating_power_w = est->total_pack_heat_power_w;
    out->heating_rate_c_per_min = engine->calculated_rate_c_min;
    out->plating_safety_margin_v = est->plating_margin_v;
    out->elapsed_heating_time_s = engine->total_elapsed_s;
    out->total_energy_consumed_wh = engine->cumulative_energy_wh;
}

int pulse_heat_engine_step(pulse_heat_engine_t *engine,
                           const pulse_heat_cell_telemetry_t *telem,
                           float dt_s,
                           pulse_heat_control_output_t *out) {
    if (!engine || !engine->initialized || !telem || !out) {
        return -1;
    }
    if (dt_s <= 0.0f || !isfinite(dt_s)) {
        dt_s = 0.01f;
    }
    if (dt_s > 1.0f) {
        dt_s = 1.0f;
    }

    if (!pulse_heat_telemetry_valid(telem)) {
        pulse_heat_fill_fault(engine, out, PULSE_HEAT_FAULT_SENSOR_INVALID);
        return 0;
    }

    pulse_heat_estimate_t est = {0};
    pulse_heat_measure_cells(telem, &est);
    pulse_heat_update_thermal(engine, telem, dt_s, &est);
    pulse_heat_supervise(engine, telem, &est, dt_s);
    pulse_heat_advance(engine, telem, &est, dt_s);
    pulse_heat_emit(engine, out, &est);
    return 0;
}

const char *pulse_heat_state_to_str(pulse_heat_state_t state) {
    switch (state) {
    case PULSE_HEAT_STATE_IDLE: return "IDLE";
    case PULSE_HEAT_STATE_PRECHECK: return "PRECHECK";
    case PULSE_HEAT_STATE_FREQ_SWEEP: return "FREQ_SWEEP";
    case PULSE_HEAT_STATE_RAMP_UP: return "RAMP_UP";
    case PULSE_HEAT_STATE_ACTIVE_HEATING: return "ACTIVE_HEATING";
    case PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE: return "COOLDOWN_HOMOGENIZE";
    case PULSE_HEAT_STATE_COMPLETED: return "COMPLETED";
    case PULSE_HEAT_STATE_FAULT: return "FAULT";
    default: return "UNKNOWN";
    }
}

const char *pulse_heat_fault_to_str(pulse_heat_fault_t fault) {
    switch (fault) {
    case PULSE_HEAT_FAULT_NONE: return "NONE";
    case PULSE_HEAT_FAULT_CELL_UNDERVOLTAGE: return "CELL_UNDERVOLTAGE";
    case PULSE_HEAT_FAULT_CELL_OVERVOLTAGE: return "CELL_OVERVOLTAGE";
    case PULSE_HEAT_FAULT_OVERTEMP: return "OVERTEMP";
    case PULSE_HEAT_FAULT_HIGH_THERMAL_GRADIENT: return "HIGH_THERMAL_GRADIENT";
    case PULSE_HEAT_FAULT_SOC_OUT_OF_BOUNDS: return "SOC_OUT_OF_BOUNDS";
    case PULSE_HEAT_FAULT_LITHIUM_PLATING_RISK: return "LITHIUM_PLATING_RISK";
    case PULSE_HEAT_FAULT_INVERTER_NOT_READY: return "INVERTER_NOT_READY";
    case PULSE_HEAT_FAULT_TIMEOUT: return "TIMEOUT";
    case PULSE_HEAT_FAULT_SENSOR_INVALID: return "SENSOR_INVALID";
    default: return "UNKNOWN";
    }
}

int pulse_heat_engine_step_ports(pulse_heat_engine_t *engine,
                                 const pulse_heat_ports_t *ports,
                                 pulse_heat_control_output_t *out)
{
    if (!engine || !ports) {
        return -1;
    }

    float dt_s = 0.1f;
    if (ports->get_dt_s) {
        dt_s = ports->get_dt_s(ports->ctx);
    }

    pulse_heat_control_output_t local_out;
    pulse_heat_control_output_t *target_out = out ? out : &local_out;

    if (ports->read_telemetry) {
        pulse_heat_cell_telemetry_t telem;
        memset(&telem, 0, sizeof(telem));
        if (ports->read_telemetry(ports->ctx, &telem) == 0) {
            int rc = pulse_heat_engine_step(engine, &telem, dt_s, target_out);
            if (rc == 0 && ports->write_output) {
                ports->write_output(ports->ctx, target_out);
            }
            return rc;
        }
    }

    return -1;
}

const pulse_heat_config_t *pulse_heat_engine_get_config(const pulse_heat_engine_t *engine)
{
    return engine ? &engine->cfg : NULL;
}

pulse_heat_state_t pulse_heat_engine_get_state(const pulse_heat_engine_t *engine)
{
    return engine ? engine->current_state : PULSE_HEAT_STATE_IDLE;
}

pulse_heat_fault_t pulse_heat_engine_get_fault(const pulse_heat_engine_t *engine)
{
    return engine ? engine->active_fault : PULSE_HEAT_FAULT_NONE;
}

void pulse_heat_engine_set_test_state(pulse_heat_engine_t *engine,
                                      pulse_heat_state_t state,
                                      float core_t,
                                      float surf_t,
                                      float elapsed_s)
{
    if (!engine) return;
    engine->current_state = state;
    engine->core_temp_est_c = core_t;
    engine->surf_temp_est_c = surf_t;
    engine->temp_initialized = true;
    engine->total_elapsed_s = elapsed_s;
}
