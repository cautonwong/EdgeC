/**
 * @file smartbox_thermal_aging.c
 * @brief SmartBox Transformer Winding Hotspot & Thermal Aging Life Loss Implementation
 */

#include "smartbox_thermal_aging.h"
#include <string.h>
#include <math.h>

void smartbox_thermal_init(smartbox_thermal_model_t *model, double rated_capacity_kva) {
    if (!model) return;
    memset(model, 0, sizeof(smartbox_thermal_model_t));
    model->rated_capacity_kva = (rated_capacity_kva > 0.0) ? rated_capacity_kva : 630.0;
    model->rated_hotspot_rise_k = 30.0; // 额定最热点对顶层油温升 30K
    model->winding_exponent_m = 0.8;    // 绕组指数
    model->rated_top_oil_rise_k = 50.0; // 额定顶层油温升 50K
    model->remaining_useful_life_hours = TRANSFORMER_NORMAL_LIFE_HOURS;
}

int smartbox_thermal_evaluate_cycle(smartbox_thermal_model_t *model,
                                    double ambient_temp,
                                    double top_oil_temp,
                                    double load_kva,
                                    double delta_time_sec) {
    if (!model || delta_time_sec <= 0.0) return -1;

    model->ambient_temp_c = ambient_temp;
    model->top_oil_temp_c = top_oil_temp;
    model->load_power_kva = load_kva;

    double k = (model->rated_capacity_kva > 0.0) ? (load_kva / model->rated_capacity_kva) : 0.0;
    model->load_factor_k = k;

    // 1. 最热点温升计算 (IEEE C57.91)
    // ΔθH = ΔθH,R * K^(2m)
    double delta_theta_h = model->rated_hotspot_rise_k * pow(k, 2.0 * model->winding_exponent_m);
    double theta_h = top_oil_temp + delta_theta_h;
    if (theta_h < ambient_temp) theta_h = ambient_temp;
    model->hotspot_temp_c = theta_h;

    // 2. 相对老化率 FAA 计算 (基准参考温度 110℃ 对应 1.0)
    // FAA = exp(15000/383.15 - 15000/(θH + 273.15))
    double abs_temp_k = theta_h + 273.15;
    double exp_val = (15000.0 / 383.15) - (15000.0 / abs_temp_k);
    double faa = exp(exp_val);
    model->relative_aging_rate_faa = faa;

    // 3. 寿命损耗与剩余寿命推算
    double dt_hours = delta_time_sec / 3600.0;
    model->total_operating_hours += dt_hours;
    double equivalent_aging_hours = faa * dt_hours;

    double incremental_loss_pct = (equivalent_aging_hours / TRANSFORMER_NORMAL_LIFE_HOURS) * 100.0;
    model->cumulative_loss_of_life_pct += incremental_loss_pct;

    double rul = TRANSFORMER_NORMAL_LIFE_HOURS * (1.0 - (model->cumulative_loss_of_life_pct / 100.0));
    if (rul < 0.0) rul = 0.0;
    model->remaining_useful_life_hours = rul;

    // 4. 热过载越限报警 (最热点 > 140℃ 或 顶油 > 105℃)
    model->is_thermal_alarm = (theta_h > 140.0 || top_oil_temp > 105.0);

    return 0;
}
