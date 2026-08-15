/**
 * @file dcu_topology_lineloss.h
 * @brief Production DCU Substation Topology, 3-Phase Unbalance & Dynamic Line Loss Analytics Engine
 */

#ifndef DCU_TOPOLOGY_LINELOSS_H
#define DCU_TOPOLOGY_LINELOSS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DCU_MAX_TOPOLOGY_METERS 2048

typedef enum {
    PHASE_UNKNOWN = 0,
    PHASE_A       = 1,
    PHASE_B       = 2,
    PHASE_C       = 3,
    PHASE_THREE   = 4
} dcu_phase_t;

typedef enum {
    LINELOSS_HEALTH_NORMAL     = 0, // 0.0% ~ 4.0%
    LINELOSS_HEALTH_ATTENTION  = 1, // 4.0% ~ 8.0%
    LINELOSS_HEALTH_HIGH_LOSS  = 2, // > 8.0%
    LINELOSS_HEALTH_NEGATIVE   = 3  // < 0.0% (Theft or reverse flow)
} dcu_lineloss_health_t;

typedef struct {
    char meter_addr[16];
    uint32_t device_id;
    uint32_t branch_id;      // 所属分支箱 ID
    dcu_phase_t phase;       // 识别所属相位 (A/B/C)
    double energy_kwh;       // 抄读有功电量 (kWh)
    double voltage_v;        // 当前相电压 (V)
    double current_a;        // 当前相电流 (A)
    double power_factor;     // 功率因数
    int8_t snr_db;           // HPLC 载波信噪比
    uint8_t attenuation_db;  // 载波衰减
    bool is_online;
} dcu_meter_topology_node_t;

typedef struct {
    double total_input_energy_kwh;  // 台变总表输入有功电量
    double total_consumed_kwh;      // 挂接所有从表有功消耗电量总和
    double line_loss_kwh;           // 物理线损电量
    double line_loss_rate_pct;      // 实时台区线损率 (%)
    dcu_lineloss_health_t health;   // 线损健康评估状态

    // 三相不平衡度指标 (GB/T 15543)
    double phase_a_current_a;
    double phase_b_current_a;
    double phase_c_current_a;
    double current_unbalance_pct;   // 三相电流不平衡度 (%)

    double phase_a_voltage_v;
    double phase_b_voltage_v;
    double phase_c_voltage_v;
    double voltage_unbalance_pct;   // 三相电压不平衡度 (%)

    uint32_t online_meter_count;
    uint32_t offline_meter_count;
    uint64_t last_calc_timestamp_ms;
} dcu_substation_analytics_t;

typedef struct {
    dcu_meter_topology_node_t meters[DCU_MAX_TOPOLOGY_METERS];
    uint32_t meter_count;
    dcu_substation_analytics_t analytics;
} dcu_topology_engine_t;

/**
 * @brief 初始化台区拓扑与线损计算引擎
 */
void dcu_topology_init(dcu_topology_engine_t* engine);

/**
 * @brief 注册或更新从表拓扑节点
 */
int dcu_topology_update_meter(dcu_topology_engine_t* engine, const char* addr, uint32_t dev_id,
                             uint32_t branch_id, dcu_phase_t phase, double energy_kwh,
                             double u_val, double i_val, double pf, int8_t snr);

/**
 * @brief 执行台区线损分摊与三相不平衡度全量计算
 * @param master_input_kwh 台变总表有功电量
 * @param master_ia 台变 A 相电流
 * @param master_ib 台变 B 相电流
 * @param master_ic 台变 C 相电流
 * @param master_ua 台变 A 相电压
 * @param master_ub 台变 B 相电压
 * @param master_uc 台变 C 相电压
 */
int dcu_topology_compute_analytics(dcu_topology_engine_t* engine, double master_input_kwh,
                                  double master_ia, double master_ib, double master_ic,
                                  double master_ua, double master_ub, double master_uc,
                                  uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif // DCU_TOPOLOGY_LINELOSS_H
