/**
 * @file smartbox_thermal_aging.h
 * @brief SmartBox Transformer Winding Hotspot & Thermal Aging Life Loss Engine (IEEE C57.91 / IEC
 * 60076-7) (AEP-442)
 */

#ifndef SMARTBOX_THERMAL_AGING_H
#define SMARTBOX_THERMAL_AGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TRANSFORMER_NORMAL_LIFE_HOURS 180000.0  // 标称设计寿命 (约 20.5 年)

typedef struct {
    double rated_capacity_kva;    // 变压器额定容量 (kVA, 如 630kVA)
    double rated_hotspot_rise_k;  // 额定绕组最热点对油温升 (K, 典型值 25~35K)
    double winding_exponent_m;    // 绕组指数 m (典型值 0.8)
    double rated_top_oil_rise_k;  // 额定顶层油温升 (K, 典型值 45~55K)

    // 实时状态
    double ambient_temp_c;  // 环境温度 (℃)
    double top_oil_temp_c;  // 顶层油温 (℃)
    double load_power_kva;  // 实时视在功率 (kVA)
    double load_factor_k;   // 负载率 K (0.0 ~ 2.5)

    // 输出计算量
    double hotspot_temp_c;               // 绕组最热点温度 θH (℃)
    double relative_aging_rate_faa;      // 相对老化率 FAA (基准 110℃ 对应 1.0)
    double total_operating_hours;        // 累计运行时长 (小时)
    double cumulative_loss_of_life_pct;  // 累计寿命损耗 LOL (%)
    double remaining_useful_life_hours;  // 预估剩余寿命 (小时)
    bool is_thermal_alarm;               // 绕组热过载报警标志 (> 140℃)
} smartbox_thermal_model_t;

/**
 * @brief 初始化变压器热寿命评估模型
 */
void smartbox_thermal_init(smartbox_thermal_model_t *model, double rated_capacity_kva);

/**
 * @brief 周期性评估变压器绕组温升与热老化损耗 (IEEE C57.91)
 * @param model 热模型
 * @param ambient_temp 环境温度 (℃)
 * @param top_oil_temp 顶层油温 (℃)
 * @param load_kva 实时负载 (kVA)
 * @param delta_time_sec 评估周期步长 (秒, 如 1.0s 或 60.0s)
 */
int smartbox_thermal_evaluate_cycle(smartbox_thermal_model_t *model, double ambient_temp,
                                    double top_oil_temp, double load_kva, double delta_time_sec);

#ifdef __cplusplus
}
#endif

#endif  // SMARTBOX_THERMAL_AGING_H
