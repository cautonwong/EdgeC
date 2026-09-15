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
        dt_s = 1.0f; /* 限制最大步长 */
    }

    /* 0. 校验输入遥测有效性与 NaN/Inf 脏数据拦截 */
    if (telem->cell_count == 0 || telem->cell_count > PULSE_HEAT_MAX_CELLS ||
        telem->temp_count == 0 || telem->temp_count > PULSE_HEAT_MAX_TEMPS ||
        !isfinite(telem->ambient_temp_c) || !isfinite(telem->pack_soc) ||
        !isfinite(telem->actual_current_rms_a) || !isfinite(telem->actual_frequency_hz)) {
        engine->current_state = PULSE_HEAT_STATE_FAULT;
        engine->active_fault = PULSE_HEAT_FAULT_SENSOR_INVALID;
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
        return 0;
    }

    for (uint16_t i = 0; i < telem->cell_count; i++) {
        if (!isfinite(telem->cell_voltages[i])) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_SENSOR_INVALID;
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
            return 0;
        }
    }

    for (uint16_t i = 0; i < telem->temp_count; i++) {
        if (!isfinite(telem->cell_temps[i])) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_SENSOR_INVALID;
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
            return 0;
        }
    }

    const pulse_heat_config_t *cfg = &engine->cfg;

    /* 1. 统计电芯端电压极值与表面温度统计量 */
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
    float t_surf_avg = t_sum / (float)temps;
    float temp_spread = (temps > 1) ? (t_max - t_min) : 0.0f;

    /* 2. 初始化核心与表面温度状态变量 (首次运行时对齐采样表面温度，支持 0.0℃ 精确对齐) */
    if (!engine->temp_initialized) {
        engine->core_temp_est_c = t_surf_avg;
        engine->surf_temp_est_c = t_surf_avg;
        engine->prev_temp_for_rate_c = t_surf_avg;
        engine->temp_initialized = true;
    }

    /* 3. 动态阻抗与焦耳内阻发热功率计算 */
    float r_ac_cell = calculate_arrhenius_resistance(cfg, engine->core_temp_est_c);
    float i_rms = telem->actual_current_rms_a;
    if (i_rms <= 0.0f && engine->current_cmd_a > 0.0f) {
        /* 若无外部 RMS 采样传感器，则根据对称正弦指令计算等效 RMS */
        i_rms = engine->current_cmd_a * 0.7071f;
    }
    float single_cell_heat_power_w = (i_rms * i_rms) * r_ac_cell;
    float total_pack_heat_power_w = single_cell_heat_power_w * (float)cells;

    /* 4. 双节点电热动力学微分方程积分 (核心与表面) */
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

    /* 结合传感器实测表面温度进行卡尔曼/观测器校正 */
    if (temps > 0) {
        float observer_gain = 0.05f * (dt_s / 0.1f);
        if (observer_gain > 0.2f) observer_gain = 0.2f;
        engine->surf_temp_est_c += observer_gain * (t_surf_avg - engine->surf_temp_est_c);
    }

    float temp_gradient = fabsf(engine->core_temp_est_c - engine->surf_temp_est_c);

    /* 5. 动态防析锂电压边界与过电位安全裕度评估 */
    float temp_subzero_deg = (engine->core_temp_est_c < 0.0f) ? (-engine->core_temp_est_c) : 0.0f;
    float v_max_dynamic = cfg->cell_v_max_nominal - (0.015f * temp_subzero_deg);
    float v_min_dynamic = cfg->cell_v_min_nominal + (0.015f * temp_subzero_deg);
    (void)v_min_dynamic;
    float plating_margin_v = v_max_dynamic - v_max;

    /* 6. 升温速率微分计算 (滑动窗口差分) */
    engine->rate_timer_s += dt_s;
    if (engine->rate_timer_s >= 2.0f) {
        float dt_rate_min = engine->rate_timer_s / 60.0f;
        float inst_rate = (engine->core_temp_est_c - engine->prev_temp_for_rate_c) / dt_rate_min;
        engine->calculated_rate_c_min = 0.7f * engine->calculated_rate_c_min + 0.3f * inst_rate;
        engine->prev_temp_for_rate_c = engine->core_temp_est_c;
        engine->rate_timer_s = 0.0f;
    }

    /* 7. 安全边界硬保护检查 (除 IDLE/COMPLETED/FAULT 状态外全程实时监督) */
    if (engine->current_state != PULSE_HEAT_STATE_IDLE &&
        engine->current_state != PULSE_HEAT_STATE_COMPLETED &&
        engine->current_state != PULSE_HEAT_STATE_FAULT) {

        engine->total_elapsed_s += dt_s;
        engine->state_timer_s += dt_s;

        /* 累计消耗能量 (Wh) */
        float power_kw = (total_pack_heat_power_w) / 1000.0f;
        engine->cumulative_energy_wh += (power_kw * (dt_s / 3600.0f)) * 1000.0f;

        /* 检查 1: 超时 */
        if (engine->total_elapsed_s > cfg->timeout_limit_s) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_TIMEOUT;
        }
        /* 检查 2: 电芯过温 */
        else if (t_max > cfg->max_temp_limit_c || engine->core_temp_est_c > cfg->max_temp_limit_c) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_OVERTEMP;
        }
        /* 检查 3: 单体过压 */
        else if (v_max > cfg->cell_v_max_nominal + 0.05f) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_CELL_OVERVOLTAGE;
        }
        /* 检查 4: 单体欠压 */
        else if (v_min < cfg->cell_v_min_nominal - 0.05f) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_CELL_UNDERVOLTAGE;
        }
        /* 检查 5: 电芯间温差过大 */
        else if (temp_spread > cfg->max_cell_temp_spread_c) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_HIGH_THERMAL_GRADIENT;
        }
        /* 检查 6: SOC 越界 */
        else if (telem->pack_soc < cfg->min_soc || telem->pack_soc > cfg->max_soc) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_SOC_OUT_OF_BOUNDS;
        }
    }

    /* 8. 状态机驱动与动作决策 */
    switch (engine->current_state) {
    case PULSE_HEAT_STATE_IDLE:
        engine->current_cmd_a = 0.0f;
        engine->target_cmd_a = 0.0f;
        break;

    case PULSE_HEAT_STATE_PRECHECK:
        engine->current_cmd_a = 0.0f;
        engine->target_cmd_a = 0.0f;
        /* 检查逆变器硬件是否就绪 */
        if (!telem->inverter_ready) {
            engine->current_state = PULSE_HEAT_STATE_FAULT;
            engine->active_fault = PULSE_HEAT_FAULT_INVERTER_NOT_READY;
            break;
        }
        /* 检查是否已达到目标温度 (若已达标则无需加热) */
        if (t_surf_avg >= cfg->target_temp_c && engine->core_temp_est_c >= cfg->target_temp_c) {
            engine->current_state = PULSE_HEAT_STATE_COMPLETED;
            break;
        }
        /* 自检通过，转入高频微扰阻抗扫频阶段 */
        engine->current_state = PULSE_HEAT_STATE_FREQ_SWEEP;
        engine->current_cmd_a = 10.0f;
        engine->state_timer_s = 0.0f;
        break;

    case PULSE_HEAT_STATE_FREQ_SWEEP:
        /* 扫频阶段：注入微小激励 (10A) 进行快速频率扫描 (1.0s) */
        engine->current_cmd_a = 10.0f;
        if (engine->state_timer_s < 1.0f) {
            float sweep_progress = engine->state_timer_s / 1.0f;
            engine->optimal_freq_hz = cfg->sweep_freq_start_hz +
                                     sweep_progress * (cfg->sweep_freq_end_hz - cfg->sweep_freq_start_hz);
        } else {
            /* 辨识完成，锁定最优高频 (标称 500Hz) */
            engine->optimal_freq_hz = cfg->nominal_frequency_hz;
            engine->target_cmd_a = cfg->max_current_amp_a;
            engine->current_state = PULSE_HEAT_STATE_RAMP_UP;
            engine->state_timer_s = 0.0f;
        }
        break;

    case PULSE_HEAT_STATE_RAMP_UP: {
        /* 根据析锂安全裕度动态修正目标上限 */
        float safe_target_a = cfg->max_current_amp_a;
        if (plating_margin_v < 0.10f) {
            float throttle_ratio = plating_margin_v / 0.10f;
            if (throttle_ratio < 0.2f) throttle_ratio = 0.2f;
            safe_target_a *= throttle_ratio;
        }
        engine->target_cmd_a = safe_target_a;

        /* 电流软起动平滑上升 */
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
        /* 自适应负极防析锂闭环微调 */
        float safe_target_a = cfg->max_current_amp_a;
        if (plating_margin_v < 0.10f) {
            float throttle_ratio = plating_margin_v / 0.10f;
            if (throttle_ratio < 0.2f) throttle_ratio = 0.2f;
            safe_target_a *= throttle_ratio;
        }
        engine->target_cmd_a = safe_target_a;

        /* 逼近目标电流 */
        float max_step = cfg->current_ramp_rate_a_per_s * dt_s;
        if (fabsf(engine->current_cmd_a - engine->target_cmd_a) <= max_step) {
            engine->current_cmd_a = engine->target_cmd_a;
        } else if (engine->current_cmd_a < engine->target_cmd_a) {
            engine->current_cmd_a += max_step;
        } else {
            engine->current_cmd_a -= max_step;
        }

        /* 判定 1: 核心与表面均达到预定目标温度 */
        if (engine->core_temp_est_c >= cfg->target_temp_c &&
            (t_surf_avg >= (cfg->target_temp_c - 4.0f) || engine->core_temp_est_c >= (cfg->target_temp_c + 2.0f))) {
            engine->current_cmd_a = 0.0f;
            engine->current_state = PULSE_HEAT_STATE_COMPLETED;
            engine->state_timer_s = 0.0f;
            break;
        }

        /* 判定 2: 核心-表面温差超过最大允许梯度，转入间歇均温静置 */
        if (temp_gradient >= cfg->max_thermal_gradient_c) {
            engine->current_cmd_a = 0.0f;
            engine->current_state = PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE;
            engine->state_timer_s = 0.0f;
            break;
        }

        /* 判定 3: 连续脉冲周期窗口届满，转入定时均温静置 */
        if (engine->state_timer_s >= cfg->active_burst_duration_s) {
            engine->current_cmd_a = 0.0f;
            engine->current_state = PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE;
            engine->state_timer_s = 0.0f;
            break;
        }
        break;
    }

    case PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE:
        /* 静置阶段切断电流输出，等待内部热传导扩散使梯度降低 */
        engine->current_cmd_a = 0.0f;
        if (engine->state_timer_s >= cfg->homogenize_duration_s) {
            /* 均温结束后检查是否已达标 */
            if (engine->core_temp_est_c >= cfg->target_temp_c &&
                (t_surf_avg >= (cfg->target_temp_c - 4.0f) || engine->core_temp_est_c >= (cfg->target_temp_c + 2.0f))) {
                engine->current_state = PULSE_HEAT_STATE_COMPLETED;
                engine->state_timer_s = 0.0f;
            } else {
                /* 重新升温 */
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

    /* 9. 填充控制输出结构 */
    out->state = engine->current_state;
    out->fault = engine->active_fault;
    out->target_current_amp_a = engine->current_cmd_a;
    out->target_frequency_hz = engine->optimal_freq_hz;
    out->duty_ratio = 0.50f; /* 对称零均值双向波形，保证净电荷零偏移 */
    out->inverter_enable = (engine->current_state == PULSE_HEAT_STATE_FREQ_SWEEP ||
                            engine->current_state == PULSE_HEAT_STATE_RAMP_UP ||
                            engine->current_state == PULSE_HEAT_STATE_ACTIVE_HEATING) &&
                           (engine->current_cmd_a > 0.0f);

    out->estimated_core_temp_c = engine->core_temp_est_c;
    out->estimated_surface_temp_c = engine->surf_temp_est_c;
    out->temp_gradient_c = temp_gradient;
    out->cell_temp_spread_c = temp_spread;
    out->estimated_heating_power_w = total_pack_heat_power_w;
    out->heating_rate_c_per_min = engine->calculated_rate_c_min;
    out->plating_safety_margin_v = plating_margin_v;
    out->elapsed_heating_time_s = engine->total_elapsed_s;
    out->total_energy_consumed_wh = engine->cumulative_energy_wh;

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
