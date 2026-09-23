/**
 * @file iems_bms_cluster_engine.h
 * @brief Production Commercial BMS Multi-Rack Cluster Aggregation & Thermal Runaway Safety Engine
 */

#ifndef IEMS_BMS_CLUSTER_ENGINE_H
#define IEMS_BMS_CLUSTER_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define IEMS_MAX_BMS_RACKS 16

typedef enum {
    BMS_ALARM_NONE = 0,
    BMS_ALARM_LEVEL_1 = 1,  // 提示/轻度不平衡 (Cell Diff > 50mV)
    BMS_ALARM_LEVEL_2 = 2,  // 告警/降功率运行 (Temp > 45C or Diff > 100mV)
    BMS_ALARM_LEVEL_3 = 3   // 紧急跳闸保护 (Thermal runaway dT/dt > 1.0C/min or Temp > 55C)
} iems_bms_alarm_level_t;

typedef struct {
    uint8_t rack_id;
    char rack_sn[24];
    bool is_online;
    bool is_breaker_closed;

    double total_voltage_v;  // 簇端电压 (V, e.g. 768.0 V)
    double current_a;        // 簇电流 (A, 正放负充)
    double soc_pct;          // 荷电状态 (%)
    double soh_pct;          // 健康状态 (%)

    double cell_v_max_v;  // 单体最高电压 (V, e.g. 3.450 V)
    double cell_v_min_v;  // 单体最低电压 (V, e.g. 3.380 V)
    uint16_t cell_v_max_idx;
    uint16_t cell_v_min_idx;

    double cell_t_max_c;         // 单体最高温度 (℃, e.g. 32.5 ℃)
    double cell_t_min_c;         // 单体最低温度 (℃, e.g. 28.0 ℃)
    double temp_rate_c_per_min;  // 温度变化率 (℃/min)

    double pos_insulation_kohm;  // 正极对地绝缘电阻 (kΩ)
    double neg_insulation_kohm;  // 负极对地绝缘电阻 (kΩ)

    bool is_balancing_active;  // 均衡回路是否开启
    iems_bms_alarm_level_t alarm_level;
} iems_bms_rack_t;

typedef struct {
    iems_bms_rack_t racks[IEMS_MAX_BMS_RACKS];
    uint8_t rack_count;

    // 系统级聚合量 (System Aggregated Metrics)
    double sys_total_voltage_v;  // 系统平均簇电压
    double sys_total_current_a;  // 系统总电流 (各簇电流之和)
    double sys_total_power_kw;   // 系统实时充放电功率
    double sys_avg_soc_pct;      // 系统平均 SOC
    double sys_avg_soh_pct;      // 系统平均 SOH
    double sys_max_cell_v;       // 全系统单体极值电压
    double sys_min_cell_v;
    double sys_max_cell_t;          // 全系统单体最高温度
    double sys_max_cell_temp_diff;  // 全系统单体最大温差

    uint32_t active_rack_count;
    bool is_circulating_risk;  // 簇间环流风险 (并联压差 > 10V 闭锁合闸)
    bool is_emergency_trip;    // 紧急跳闸标志
} iems_bms_cluster_engine_t;

/**
 * @brief 初始化多簇 BMS 聚合与安全引擎
 */
void iems_bms_cluster_init(iems_bms_cluster_engine_t* engine);

/**
 * @brief 注册或更新单个电池簇物理采样数据
 */
int iems_bms_update_rack(iems_bms_cluster_engine_t* engine, uint8_t rack_id, const char* sn,
                         double u_v, double i_a, double soc, double soh, double v_max, double v_min,
                         double t_max, double t_min, double dt_rate, double r_iso);

/**
 * @brief 执行全堆簇级多源汇聚计算、环流抑制与热失控安全研判
 */
int iems_bms_evaluate_cluster_safety(iems_bms_cluster_engine_t* engine);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_BMS_CLUSTER_ENGINE_H
