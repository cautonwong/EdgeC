#include "battery_gas_early_warning.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

int battery_gas_ew_init(battery_gas_ew_detector_t *ew, const battery_gas_ew_config_t *cfg)
{
    if (!ew) {
        return -1;
    }

    memset(ew, 0, sizeof(*ew));

    if (cfg) {
        ew->config = *cfg;
    } else {
        ew->config.voc_trace_ppb = 300.0;
        ew->config.h2_trace_ppm = 15.0;
        ew->config.co_trace_ppm = 10.0;
        
        ew->config.co_vent_ppm = 50.0;
        ew->config.h2_vent_ppm = 80.0;
        ew->config.pressure_pulse_pa_s = 30.0;
        ew->config.temp_rate_vent_cs = 0.25;
        
        ew->config.smoke_runaway_thresh = 0.15;
        ew->config.co_runaway_ppm = 250.0;
        ew->config.temp_runaway_c = 75.0;
        
        ew->config.max_valid_temp_c = 150.0;
        ew->config.min_valid_pressure_pa = 60000.0;
        ew->config.max_valid_pressure_pa = 150000.0;
    }

    /* 校验参数有效性 */
    if (ew->config.voc_trace_ppb <= 0.0) ew->config.voc_trace_ppb = 300.0;
    if (ew->config.h2_trace_ppm <= 0.0) ew->config.h2_trace_ppm = 15.0;
    if (ew->config.co_trace_ppm <= 0.0) ew->config.co_trace_ppm = 10.0;
    if (ew->config.co_vent_ppm <= 0.0) ew->config.co_vent_ppm = 50.0;
    if (ew->config.h2_vent_ppm <= 0.0) ew->config.h2_vent_ppm = 80.0;
    if (ew->config.pressure_pulse_pa_s <= 0.0) ew->config.pressure_pulse_pa_s = 30.0;
    if (ew->config.temp_rate_vent_cs <= 0.0) ew->config.temp_rate_vent_cs = 0.25;
    if (ew->config.smoke_runaway_thresh <= 0.0) ew->config.smoke_runaway_thresh = 0.15;
    if (ew->config.co_runaway_ppm <= 0.0) ew->config.co_runaway_ppm = 250.0;
    if (ew->config.temp_runaway_c <= 0.0) ew->config.temp_runaway_c = 75.0;
    if (ew->config.max_valid_temp_c <= 50.0) ew->config.max_valid_temp_c = 150.0;
    if (ew->config.min_valid_pressure_pa <= 10000.0) ew->config.min_valid_pressure_pa = 60000.0;
    if (ew->config.max_valid_pressure_pa <= ew->config.min_valid_pressure_pa) ew->config.max_valid_pressure_pa = 150000.0;

    battery_gas_ew_reset(ew);
    return 0;
}

int battery_gas_ew_push_sample(battery_gas_ew_detector_t *ew, const battery_gas_sensor_sample_t *sample)
{
    if (!ew || !sample) {
        return -1;
    }

    uint16_t idx = ew->sample_count;
    if (idx < BATTERY_GAS_HISTORY_LEN) {
        ew->history[idx] = *sample;
        ew->sample_count++;
    } else {
        memmove(&ew->history[0], &ew->history[1], sizeof(battery_gas_sensor_sample_t) * (BATTERY_GAS_HISTORY_LEN - 1));
        ew->history[BATTERY_GAS_HISTORY_LEN - 1] = *sample;
    }

    ew->total_samples_processed++;
    ew->last_timestamp_s = sample->timestamp_s;
    return 0;
}

static double clamp01(double v)
{
    if (v > 1.0) return 1.0;
    if (v < 0.0) return 0.0;
    return v;
}

int battery_gas_ew_process_step(battery_gas_ew_detector_t *ew, double cur_timestamp_s)
{
    if (!ew || ew->sample_count == 0) {
        return -1;
    }

    uint16_t curr_idx = ew->sample_count - 1;
    const battery_gas_sensor_sample_t *curr = &ew->history[curr_idx];

    /* 1. 传感器物理故障/断线/量程越界检查 */
    if (curr->pressure_pa < ew->config.min_valid_pressure_pa ||
        curr->pressure_pa > ew->config.max_valid_pressure_pa ||
        curr->max_cell_temp_c < -40.0 ||
        curr->max_cell_temp_c > ew->config.max_valid_temp_c ||
        curr->co_ppm < 0.0 || curr->h2_ppm < 0.0 || curr->voc_ppb < 0.0 ||
        curr->smoke_density < 0.0 || curr->smoke_density > 1.0) {
        
        memset(&ew->report, 0, sizeof(ew->report));
        ew->report.stage = BATTERY_GAS_STAGE_SENSOR_FAULT;
        ew->report.estimated_seconds_to_runaway = -1.0;
        ew->report.valid = true;
        return 0;
    }

    /* 2. 自适应环境背景基线追踪 */
    if (ew->total_samples_processed == 1) {
        ew->baseline_co = curr->co_ppm;
        ew->baseline_h2 = curr->h2_ppm;
        ew->baseline_voc = curr->voc_ppb;
        ew->baseline_pressure = curr->pressure_pa;
    } else if (curr->co_ppm < 10.0 && curr->h2_ppm < 15.0 && curr->voc_ppb < 200.0) {
        double alpha_b = 0.01;
        ew->baseline_co = (1.0 - alpha_b) * ew->baseline_co + alpha_b * curr->co_ppm;
        ew->baseline_h2 = (1.0 - alpha_b) * ew->baseline_h2 + alpha_b * curr->h2_ppm;
        ew->baseline_voc = (1.0 - alpha_b) * ew->baseline_voc + alpha_b * curr->voc_ppb;
        ew->baseline_pressure = (1.0 - alpha_b) * ew->baseline_pressure + alpha_b * curr->pressure_pa;
    }

    /* 3. 一阶时域变化率微分计算 */
    double dco_dt = 0.0;
    double dh2_dt = 0.0;
    double dvoc_dt = 0.0;
    double dpress_dt = 0.0;
    double dtemp_dt = 0.0;

    if (ew->sample_count >= 2) {
        const battery_gas_sensor_sample_t *prev = &ew->history[curr_idx - 1];
        double dt = curr->timestamp_s - prev->timestamp_s;
        if (dt > 0.001) {
            dco_dt = (curr->co_ppm - prev->co_ppm) / dt;
            dh2_dt = (curr->h2_ppm - prev->h2_ppm) / dt;
            dvoc_dt = (curr->voc_ppb - prev->voc_ppb) / dt;
            dpress_dt = (curr->pressure_pa - prev->pressure_pa) / dt;
            dtemp_dt = (curr->max_cell_temp_c - prev->max_cell_temp_c) / dt;
        }
    }

    /* 4. 贝叶斯证据因子提取 */
    double delta_co = curr->co_ppm - ew->baseline_co;
    if (delta_co < 0.0) delta_co = 0.0;
    double delta_h2 = curr->h2_ppm - ew->baseline_h2;
    if (delta_h2 < 0.0) delta_h2 = 0.0;
    double delta_voc = curr->voc_ppb - ew->baseline_voc;
    if (delta_voc < 0.0) delta_voc = 0.0;

    double e_voc_trace = clamp01(delta_voc / ew->config.voc_trace_ppb);
    double e_h2_trace = clamp01(delta_h2 / ew->config.h2_trace_ppm);
    double e_co_vent = clamp01(delta_co / ew->config.co_vent_ppm);
    double e_h2_vent = clamp01(delta_h2 / ew->config.h2_vent_ppm);
    double e_press_pulse = clamp01(dpress_dt / ew->config.pressure_pulse_pa_s);
    double e_temp_rate = clamp01(dtemp_dt / ew->config.temp_rate_vent_cs);
    double e_smoke = clamp01(curr->smoke_density / ew->config.smoke_runaway_thresh);
    double e_temp_high = clamp01((curr->max_cell_temp_c - 55.0) / (ew->config.temp_runaway_c - 55.0));

    /* 5. 贝叶斯网络多级后验概率融合 */
    double prob_stage1 = 0.50 * e_voc_trace + 0.35 * e_h2_trace + 0.15 * e_temp_high;
    double prob_stage2 = 0.35 * e_co_vent + 0.30 * e_h2_vent + 0.20 * e_press_pulse + 0.15 * e_temp_rate;
    double prob_stage3 = 0.40 * e_smoke + 0.30 * clamp01(curr->co_ppm / ew->config.co_runaway_ppm) +
                         0.30 * clamp01(curr->max_cell_temp_c / ew->config.temp_runaway_c);

    prob_stage1 = clamp01(prob_stage1);
    prob_stage2 = clamp01(prob_stage2);
    prob_stage3 = clamp01(prob_stage3);

    /* 6. 多级演化状态研判与联动输出决策 */
    battery_gas_stage_t stage = BATTERY_GAS_STAGE_NORMAL;
    double t_remain_s = 99999.0;
    bool trip_contactor = false;
    bool start_fan = false;
    bool trigger_exting = false;

    if (prob_stage3 >= 0.50 || (curr->smoke_density >= ew->config.smoke_runaway_thresh && curr->max_cell_temp_c >= 70.0)) {
        stage = BATTERY_GAS_STAGE_STAGE3_RUNAWAY_FIRE;
        t_remain_s = 30.0;
        trip_contactor = true;
        start_fan = true;
        trigger_exting = true; /* 触发灭火介质喷放 */
    } else if (prob_stage2 >= 0.45 || (curr->co_ppm >= ew->config.co_vent_ppm && curr->h2_ppm >= ew->config.h2_vent_ppm)) {
        stage = BATTERY_GAS_STAGE_STAGE2_VENT_CONFIRM;
        t_remain_s = 360.0; /* 6 分钟前置告警 */
        trip_contactor = true;
        start_fan = true;
        trigger_exting = false;
    } else if (prob_stage1 >= 0.40 || (curr->voc_ppb >= ew->config.voc_trace_ppb || curr->h2_ppm >= ew->config.h2_trace_ppm)) {
        stage = BATTERY_GAS_STAGE_STAGE1_PREVENT_TRACE;
        t_remain_s = 1200.0; /* 20 分钟潜伏期前置预警 */
        trip_contactor = false;
        start_fan = true; /* 开启排风稀释气体 */
        trigger_exting = false;
    } else {
        stage = BATTERY_GAS_STAGE_NORMAL;
        t_remain_s = 99999.0;
        trip_contactor = false;
        start_fan = false;
        trigger_exting = false;
    }

    /* 7. 生成诊断报告 */
    ew->report.stage = stage;
    ew->report.prob_prevent_latent = prob_stage1;
    ew->report.prob_vent_confirmed = prob_stage2;
    ew->report.prob_runaway_fire = prob_stage3;
    ew->report.estimated_seconds_to_runaway = t_remain_s;
    
    ew->report.dco_dt_ppm_s = dco_dt;
    ew->report.dh2_dt_ppm_s = dh2_dt;
    ew->report.dvoc_dt_ppb_s = dvoc_dt;
    ew->report.dpressure_dt_pa_s = dpress_dt;
    ew->report.dtemp_dt_cs = dtemp_dt;
    
    ew->report.trip_contactor_req = trip_contactor;
    ew->report.start_exhaust_fan_req = start_fan;
    ew->report.trigger_extinguisher_req = trigger_exting;
    ew->report.valid = true;

    return 0;
}

const battery_gas_ew_report_t *battery_gas_ew_get_report(const battery_gas_ew_detector_t *ew)
{
    if (!ew || !ew->report.valid) {
        return NULL;
    }
    return &ew->report;
}

void battery_gas_ew_reset(battery_gas_ew_detector_t *ew)
{
    if (!ew) {
        return;
    }

    memset(ew->history, 0, sizeof(ew->history));
    memset(&ew->report, 0, sizeof(ew->report));
    ew->sample_count = 0;
    ew->baseline_co = 1.0;
    ew->baseline_h2 = 2.0;
    ew->baseline_voc = 50.0;
    ew->baseline_pressure = 101325.0;
    ew->total_samples_processed = 0;
    ew->last_timestamp_s = 0.0;
}
