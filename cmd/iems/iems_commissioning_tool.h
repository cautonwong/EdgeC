/**
 * @file iems_commissioning_tool.h
 * @brief Production Commercial Energy Storage Site Commissioning, CT/PT Calibration & Pre-Flight Self-Check Engine
 */

#ifndef IEMS_COMMISSIONING_TOOL_H
#define IEMS_COMMISSIONING_TOOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    // 关口电表互感器变比配置
    uint32_t ct_primary;    // 电流互感器一次侧 (e.g. 1000 A)
    uint32_t ct_secondary;  // 电流互感器二次侧 (e.g. 5 A)
    uint32_t pt_primary;    // 电压互感器一次侧 (e.g. 10000 V)
    uint32_t pt_secondary;  // 电压互感器二次侧 (e.g. 100 V)
    double calculated_ct_ratio;
    double calculated_pt_ratio;
    double total_power_multiplier;
} iems_meter_transformer_cfg_t;

typedef struct {
    // 现场一键自检结果 (Pre-Flight Check Matrix)
    bool is_grid_voltage_ok;      // 0.9 ~ 1.1 Un (342V ~ 440V)
    bool is_grid_frequency_ok;    // 49.5Hz ~ 50.5Hz
    bool is_bms_insulation_ok;    // R_iso >= 500 kOhm
    bool is_bms_voltage_matched;  // 各簇压差 <= 10V
    bool is_pcs_standby_ready;    // PCS 无故障且处于待机
    bool is_fire_loop_normal;     // 消防回路无告警且正常
    bool is_comm_bus_healthy;     // RS485/CAN 总线丢包率 < 1%

    uint8_t total_score;          // 电站健康度评分 (0 ~ 100)
    bool is_go_for_grid_tie;      // 是否允许并网闭合断路器 (Go / No-Go)
    char failure_reason[128];     // 阻断原因描述
} iems_commissioning_report_t;

/**
 * @brief 配置并计算关口电表 CT/PT 变比参数
 */
void iems_commissioning_set_transformer(iems_meter_transformer_cfg_t* cfg,
                                        uint32_t ct_pri, uint32_t ct_sec,
                                        uint32_t pt_pri, uint32_t pt_sec);

/**
 * @brief 将电表裸寄存器数值通过 CT/PT 换算为真实物理电能/功率
 */
double iems_commissioning_scale_power(const iems_meter_transformer_cfg_t* cfg, double raw_power_w);

/**
 * @brief 执行电站现场开局一键自检预检逻辑 (Pre-Flight Health Audit)
 */
int iems_commissioning_run_preflight_check(iems_commissioning_report_t* report,
                                          double u_grid_v, double f_grid_hz,
                                          double bms_r_iso_kohm, double bms_max_rack_v_diff,
                                          bool pcs_fault, bool fire_alarm, double bus_drop_rate_pct);

#ifdef __cplusplus
}
#endif

#endif // IEMS_COMMISSIONING_TOOL_H
