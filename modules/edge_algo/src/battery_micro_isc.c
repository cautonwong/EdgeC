#include "battery_micro_isc.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_NOMINAL_CAPACITY_AH     280.0f
#define DEFAULT_SAMPLE_INTERVAL_S       1.0f
#define DEFAULT_WARN_VOLTAGE_DIFF_V     0.015f  /* 15 mV */
#define DEFAULT_ALARM_VOLTAGE_DIFF_V    0.035f  /* 35 mV */
#define DEFAULT_WARN_R_ISC_OHM          1000.0f /* 1000 Ω */
#define DEFAULT_ALARM_R_ISC_OHM         200.0f  /* 200 Ω */
#define DEFAULT_CRITICAL_R_ISC_OHM      50.0f   /* 50 Ω */
#define DEFAULT_LI_PLATING_D2V_THRESH   0.0002f /* V/s^2 */
#define DEFAULT_REST_CURRENT_THRESH_A   0.2f    /* 0.2 A */
#define HEALTHY_R_ISC_BASE_OHM          1000000.0f /* 1 MΩ */

static int compare_floats(const void *a, const void *b) {
    float fa = *(const float *)a;
    float fb = *(const float *)b;
    bool fa_fin = isfinite(fa);
    bool fb_fin = isfinite(fb);
    if (!fa_fin && !fb_fin) return 0;
    if (!fa_fin) return 1;  /* non-finite sinks to bottom */
    if (!fb_fin) return -1;
    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

int battery_micro_isc_init(pack_micro_isc_detector_t *det,
                           const battery_micro_isc_config_t *cfg,
                           uint16_t cell_count) {
    if (!det || cell_count == 0 || cell_count > BATTERY_MAX_CELLS_PER_PACK) {
        return -1;
    }

    memset(det, 0, sizeof(*det));
    det->cell_count = cell_count;

    if (cfg) {
        det->config = *cfg;
    } else {
        det->config.nominal_capacity_ah = DEFAULT_NOMINAL_CAPACITY_AH;
        det->config.sample_interval_s = DEFAULT_SAMPLE_INTERVAL_S;
        det->config.warn_voltage_diff_v = DEFAULT_WARN_VOLTAGE_DIFF_V;
        det->config.alarm_voltage_diff_v = DEFAULT_ALARM_VOLTAGE_DIFF_V;
        det->config.warn_r_isc_ohm = DEFAULT_WARN_R_ISC_OHM;
        det->config.alarm_r_isc_ohm = DEFAULT_ALARM_R_ISC_OHM;
        det->config.critical_r_isc_ohm = DEFAULT_CRITICAL_R_ISC_OHM;
        det->config.li_plating_d2v_threshold = DEFAULT_LI_PLATING_D2V_THRESH;
        det->config.rest_current_threshold_a = DEFAULT_REST_CURRENT_THRESH_A;
    }

    for (uint16_t i = 0; i < cell_count; i++) {
        det->cells[i].cell_id = i;
        det->cells[i].r_isc_ohm = HEALTHY_R_ISC_BASE_OHM;
        det->cells[i].self_discharge_rate_pct_day = 0.05f; /* 典型健康自放电 ~0.05%/天 */
        det->cells[i].estimated_days_to_failure = 30.0f;
        det->cells[i].alarm_level = BATTERY_ALARM_NORMAL;
    }

    return 0;
}

int battery_micro_isc_feed_cell(pack_micro_isc_detector_t *det,
                                uint16_t cell_id,
                                float voltage,
                                float current,
                                float temp,
                                float soc,
                                double timestamp_s) {
    if (!det || cell_id >= det->cell_count) {
        return -1;
    }

    if (!isfinite(voltage)) voltage = 3.30f;
    if (!isfinite(current)) current = 0.0f;
    if (!isfinite(temp)) temp = 25.0f;
    if (!isfinite(soc)) soc = 0.50f;
    if (!isfinite(timestamp_s)) timestamp_s = 0.0;

    cell_isc_state_t *c = &det->cells[cell_id];
    c->voltage = voltage;
    c->current = current;
    c->temperature = temp;
    c->soc = soc;

    bool is_resting = (fabsf(current) <= det->config.rest_current_threshold_a);

    if (is_resting) {
        if (c->relaxation.window_count == 0) {
            c->relaxation.rest_start_voltage = voltage;
            c->relaxation.rest_start_soc = soc;
            c->relaxation.rest_start_time_s = timestamp_s;
            c->relaxation.total_rest_time_s = 0.0;
            c->relaxation.d2v_dt2_max_residual = 0.0f;
            c->relaxation.li_plating_detected = false;
            c->relaxation.li_plating_severity = 0.0f;
        }

        c->relaxation.total_rest_time_s = timestamp_s - c->relaxation.rest_start_time_s;

        if (c->relaxation.window_count < BATTERY_RELAXATION_WINDOW_SIZE) {
            uint16_t idx = c->relaxation.window_count;
            c->relaxation.voltage_window[idx] = voltage;
            c->relaxation.time_window[idx] = (float)c->relaxation.total_rest_time_s;
            c->relaxation.window_count++;
        } else {
            /* 滑动窗口移位 */
            memmove(&c->relaxation.voltage_window[0],
                    &c->relaxation.voltage_window[1],
                    (BATTERY_RELAXATION_WINDOW_SIZE - 1) * sizeof(float));
            memmove(&c->relaxation.time_window[0],
                    &c->relaxation.time_window[1],
                    (BATTERY_RELAXATION_WINDOW_SIZE - 1) * sizeof(float));
            c->relaxation.voltage_window[BATTERY_RELAXATION_WINDOW_SIZE - 1] = voltage;
            c->relaxation.time_window[BATTERY_RELAXATION_WINDOW_SIZE - 1] = (float)c->relaxation.total_rest_time_s;
        }
    } else {
        /* 充放电态，重置松弛历史 */
        c->relaxation.window_count = 0;
        c->relaxation.total_rest_time_s = 0.0;
    }

    return 0;
}

int battery_micro_isc_detect_lithium_plating(cell_isc_state_t *cell,
                                             float d2v_threshold,
                                             float *out_curvature_residual,
                                             bool *out_detected,
                                             float *out_severity) {
    if (!cell) {
        return -1;
    }

    cell_relaxation_data_t *r = &cell->relaxation;
    if (r->window_count < 5) {
        if (out_curvature_residual) *out_curvature_residual = 0.0f;
        if (out_detected) *out_detected = false;
        if (out_severity) *out_severity = 0.0f;
        return 0;
    }

    float max_curvature_anomaly = 0.0f;

    /* 5 点 Savitzky-Golay 二阶微分核: [-2, 1, 2, 1, -2] 或 [2, -1, -2, -1, 2]/7 */
    for (uint16_t i = 2; i < r->window_count - 2; i++) {
        float dt = (r->time_window[i + 1] - r->time_window[i - 1]) * 0.5f;
        if (dt <= 1e-4f || !isfinite(dt)) {
            dt = 1.0f;
        }

        float v_m2 = r->voltage_window[i - 2];
        float v_m1 = r->voltage_window[i - 1];
        float v_0  = r->voltage_window[i];
        float v_p1 = r->voltage_window[i + 1];
        float v_p2 = r->voltage_window[i + 2];

        /* 计算归一化二阶差分/曲率 (Savitzky-Golay 5点平滑二阶导数) */
        float d2v = (2.0f * v_m2 - v_m1 - 2.0f * v_0 - v_p1 + 2.0f * v_p2) / 7.0f;

        /* 正常松弛曲线单调凹陷 d2v > 0；析锂剥离相变引起平台 inflection 导致曲率发生异常突变或反弯 (d2v < 0 或显著波动) */
        float anomaly = fabsf(d2v);
        if (d2v < 0.0f) {
            /* 出现反弯平台，严重度加权 */
            anomaly = fabsf(d2v) * 2.0f;
        }

        if (anomaly > max_curvature_anomaly) {
            max_curvature_anomaly = anomaly;
        }
    }

    bool detected = (max_curvature_anomaly >= d2v_threshold);
    float severity = 0.0f;
    if (detected) {
        severity = (max_curvature_anomaly - d2v_threshold) / (d2v_threshold * 4.0f);
        if (severity > 1.0f) severity = 1.0f;
        if (severity < 0.05f) severity = 0.05f;
    }

    r->d2v_dt2_max_residual = max_curvature_anomaly;
    r->li_plating_detected = detected;
    r->li_plating_severity = severity;

    if (out_curvature_residual) *out_curvature_residual = max_curvature_anomaly;
    if (out_detected) *out_detected = detected;
    if (out_severity) *out_severity = severity;

    return 0;
}

int battery_micro_isc_identify_r_isc(cell_isc_state_t *cell,
                                     float baseline_avg_voltage,
                                     float baseline_delta_soc,
                                     float nominal_capacity_ah,
                                     float rest_time_s,
                                     float *out_r_isc,
                                     float *out_discharge_rate_pct_day) {
    if (!cell || !isfinite(nominal_capacity_ah) || nominal_capacity_ah <= 0.0f) {
        return -1;
    }

    if (!isfinite(rest_time_s) || rest_time_s < 5.0f ||
        !isfinite(baseline_avg_voltage) || !isfinite(baseline_delta_soc)) {
        if (out_r_isc) *out_r_isc = cell->r_isc_ohm;
        if (out_discharge_rate_pct_day) *out_discharge_rate_pct_day = cell->self_discharge_rate_pct_day;
        return 0;
    }

    float delta_soc = cell->relaxation.rest_start_soc - cell->soc;
    if (!isfinite(delta_soc) || delta_soc < 0.0f) {
        delta_soc = 0.0f;
    }

    /* 扣除基准自放电，得到微短路引起的额外自放电 */
    float net_excess_soc = delta_soc - baseline_delta_soc;
    float r_isc = HEALTHY_R_ISC_BASE_OHM;
    float self_discharge_pct_day = (delta_soc * 100.0f) / (rest_time_s / 86400.0f);

    if (net_excess_soc > 1e-6f) {
        /* 额外自放电容量 (Ah) */
        float excess_capacity_ah = net_excess_soc * nominal_capacity_ah;
        /* 等效短路漏电流 (A) = (Ah * 3600) / s */
        float i_leakage_a = (excess_capacity_ah * 3600.0f) / rest_time_s;
        if (i_leakage_a > 1e-5f) {
            float v_eval = (cell->voltage > 1.0f && isfinite(cell->voltage)) ? cell->voltage : baseline_avg_voltage;
            r_isc = v_eval / i_leakage_a;
        }
    }

    cell->r_isc_ohm = r_isc;
    cell->self_discharge_rate_pct_day = self_discharge_pct_day;

    if (out_r_isc) *out_r_isc = r_isc;
    if (out_discharge_rate_pct_day) *out_discharge_rate_pct_day = self_discharge_pct_day;

    return 0;
}

int battery_micro_isc_process_step(pack_micro_isc_detector_t *det,
                                   double cur_timestamp_s) {
    if (!det || det->cell_count == 0) {
        return -1;
    }

    float v_sum = 0.0f;
    float v_arr[BATTERY_MAX_CELLS_PER_PACK];
    float delta_soc_arr[BATTERY_MAX_CELLS_PER_PACK];
    float i_sum = 0.0f;
    uint16_t valid_cells = 0;

    for (uint16_t i = 0; i < det->cell_count; i++) {
        float v = det->cells[i].voltage;
        float curr_i = det->cells[i].current;
        float s = det->cells[i].soc;

        if (isfinite(v)) {
            v_sum += v;
            v_arr[valid_cells] = v;
        } else {
            v_arr[valid_cells] = 3.30f;
        }

        if (isfinite(curr_i)) {
            i_sum += curr_i;
        }

        float d_soc = det->cells[i].relaxation.rest_start_soc - s;
        if (isfinite(d_soc) && d_soc > 0.0f) {
            delta_soc_arr[valid_cells] = d_soc;
        } else {
            delta_soc_arr[valid_cells] = 0.0f;
        }
        valid_cells++;
    }

    det->pack_avg_voltage = (valid_cells > 0) ? (v_sum / (float)valid_cells) : 3.30f;
    det->pack_current = (valid_cells > 0) ? (i_sum / (float)valid_cells) : 0.0f;

    /* 计算电压中位数与基准 SOC 衰减 */
    qsort(v_arr, valid_cells, sizeof(float), compare_floats);
    det->pack_median_voltage = (valid_cells > 0) ? v_arr[valid_cells / 2] : 3.30f;

    qsort(delta_soc_arr, valid_cells, sizeof(float), compare_floats);
    float baseline_delta_soc = (valid_cells > 0) ? delta_soc_arr[valid_cells / 2] : 0.0f;

    bool pack_in_rest = (fabsf(det->pack_current) <= det->config.rest_current_threshold_a);
    if (pack_in_rest) {
        if (!det->is_in_rest) {
            det->is_in_rest = true;
            det->rest_start_timestamp = cur_timestamp_s;
            det->accumulated_rest_s = 0.0;
        } else {
            det->accumulated_rest_s = cur_timestamp_s - det->rest_start_timestamp;
        }
    } else {
        det->is_in_rest = false;
        det->accumulated_rest_s = 0.0;
    }

    /* 逐电芯诊断与早期预警评估 */
    for (uint16_t i = 0; i < det->cell_count; i++) {
        cell_isc_state_t *c = &det->cells[i];

        /* 计算相对于中位数的低压偏离残差 (mV) */
        c->voltage_residual_mv = (det->pack_median_voltage - c->voltage) * 1000.0f;

        if (c->relaxation.window_count >= 5) {
            battery_micro_isc_detect_lithium_plating(c,
                                                     det->config.li_plating_d2v_threshold,
                                                     NULL, NULL, NULL);
        }

        if (c->relaxation.total_rest_time_s >= 5.0) {
            battery_micro_isc_identify_r_isc(c,
                                             det->pack_avg_voltage,
                                             baseline_delta_soc,
                                             det->config.nominal_capacity_ah,
                                             (float)c->relaxation.total_rest_time_s,
                                             NULL, NULL);
        }

        /* 1~7 天早期预警模型判定 */
        battery_isc_alarm_level_t level = BATTERY_ALARM_NORMAL;
        float days_to_fail = 30.0f;

        if (c->voltage_residual_mv >= det->config.alarm_voltage_diff_v * 1000.0f ||
            c->r_isc_ohm <= det->config.critical_r_isc_ohm) {
            /* 立即危险告警: 压差 > 35mV 或 R_isc < 50Ω */
            level = BATTERY_ALARM_CRITICAL_IMMEDIATE;
            days_to_fail = 0.5f;
        } else if (c->r_isc_ohm <= det->config.alarm_r_isc_ohm ||
                   c->voltage_residual_mv >= det->config.warn_voltage_diff_v * 1000.0f) {
            /* 1天紧急告警: R_isc <= 200Ω 或 压差 >= 15mV */
            level = BATTERY_ALARM_URGENT_1D;
            float ratio = (c->r_isc_ohm - det->config.critical_r_isc_ohm) /
                          (det->config.alarm_r_isc_ohm - det->config.critical_r_isc_ohm + 1e-3f);
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            days_to_fail = 1.0f + ratio * 0.9f;
        } else if (c->r_isc_ohm <= det->config.warn_r_isc_ohm ||
                   (c->relaxation.li_plating_detected && c->relaxation.li_plating_severity >= 0.5f)) {
            /* 3天早期预警: R_isc <= 1000Ω 或 显著析锂 */
            level = BATTERY_ALARM_EARLY_WARNING_3D;
            float ratio = (c->r_isc_ohm - det->config.alarm_r_isc_ohm) /
                          (det->config.warn_r_isc_ohm - det->config.alarm_r_isc_ohm + 1e-3f);
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            days_to_fail = 2.0f + ratio * 2.0f;
        } else if (c->r_isc_ohm <= det->config.warn_r_isc_ohm * 2.0f ||
                   c->relaxation.li_plating_detected) {
            /* 7天早期预警: 轻度析锂剥离相变或 R_isc 1000~2000Ω */
            level = BATTERY_ALARM_EARLY_WARNING_7D;
            float ratio = (c->r_isc_ohm - det->config.warn_r_isc_ohm) /
                          (det->config.warn_r_isc_ohm + 1e-3f);
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            days_to_fail = 4.0f + ratio * 3.0f;
        } else {
            level = BATTERY_ALARM_NORMAL;
            days_to_fail = 30.0f;
        }

        c->alarm_level = level;
        c->estimated_days_to_failure = days_to_fail;
    }

    det->total_samples_processed++;
    return 0;
}

int battery_micro_isc_get_cell_report(const pack_micro_isc_detector_t *det,
                                      uint16_t cell_id,
                                      cell_isc_state_t *out_state) {
    if (!det || !out_state || cell_id >= det->cell_count) {
        return -1;
    }
    *out_state = det->cells[cell_id];
    return 0;
}

int battery_micro_isc_get_highest_alarm(const pack_micro_isc_detector_t *det,
                                        battery_isc_alarm_level_t *out_max_alarm,
                                        uint16_t *out_worst_cell_id) {
    if (!det || det->cell_count == 0) {
        return -1;
    }

    battery_isc_alarm_level_t max_alarm = BATTERY_ALARM_NORMAL;
    uint16_t worst_cell = 0;
    float min_days = 999.0f;

    for (uint16_t i = 0; i < det->cell_count; i++) {
        if (det->cells[i].alarm_level > max_alarm) {
            max_alarm = det->cells[i].alarm_level;
            worst_cell = i;
            min_days = det->cells[i].estimated_days_to_failure;
        } else if (det->cells[i].alarm_level == max_alarm && max_alarm != BATTERY_ALARM_NORMAL) {
            if (det->cells[i].estimated_days_to_failure < min_days) {
                worst_cell = i;
                min_days = det->cells[i].estimated_days_to_failure;
            }
        }
    }

    if (out_max_alarm) *out_max_alarm = max_alarm;
    if (out_worst_cell_id) *out_worst_cell_id = worst_cell;

    return 0;
}

void battery_micro_isc_reset(pack_micro_isc_detector_t *det) {
    if (!det) return;
    uint16_t count = det->cell_count;
    battery_micro_isc_config_t cfg = det->config;
    battery_micro_isc_init(det, &cfg, count);
}
