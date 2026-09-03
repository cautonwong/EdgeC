/**
 * @file iems_microgrid_coordinator.h
 * @brief Production Commercial Microgrid EMS Coordinator: Zero Export, Demand Shaving & TOU Arbitrage
 */

#ifndef IEMS_MICROGRID_COORDINATOR_H
#define IEMS_MICROGRID_COORDINATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define IEMS_MAX_SCHEDULE_SLOTS 24

typedef enum {
    TOU_ACTION_STANDBY   = 0,
    TOU_ACTION_CHARGE    = 1, // 谷段/平段充电
    TOU_ACTION_DISCHARGE = 2  // 峰段/尖段放电
} iems_tou_action_t;

typedef struct {
    uint8_t start_hour;
    uint8_t start_minute;
    uint8_t end_hour;
    uint8_t end_minute;
    iems_tou_action_t action;
    double target_power_kw; // 计划充放电功率 (kW)
} iems_tou_schedule_slot_t;

typedef struct {
    iems_tou_schedule_slot_t slots[IEMS_MAX_SCHEDULE_SLOTS];
    uint8_t slot_count;
    bool is_schedule_enabled;
} iems_tou_planner_t;

typedef struct {
    // 物理实时量 (kW)
    double p_grid_kw;       // 关口有功功率 (正为从电网受电, 负为向电网倒送)
    double p_pv_kw;         // 光伏实时总有功功率 (kW >= 0)
    double p_battery_kw;    // 储能实时总功率 (正放电, 负充电)
    double p_load_kw;       // 厂区负载总功率 (kW >= 0)
    double p_ev_kw;         // 充电桩负荷功率 (kW >= 0)

    // 控制约束
    double transformer_capacity_kva; // 变压器额定容量 (kVA, 如 630kVA)
    double demand_limit_kw;          // 需量控制红线 (kW, 如 500kW)
    double anti_reverse_deadband_kw; // 防逆流安全死区 (kW, 如 5kW)

    // PID 快速防逆流闭环参数 (<100ms)
    double pid_kp;
    double pid_ki;
    double pid_kd;
    double pid_integral;
    double pid_prev_error;

    // 需量预测与滑窗统计 (15分钟滚动负荷均值)
    double demand_window_sum;
    uint32_t demand_window_samples;
    double demand_15min_avg_kw;

    // 输出调度指令
    double target_pcs_power_kw;      // 建议下发 PCS 功率 (正放负充, kW)
    double target_pv_limit_kw;       // 建议下发光伏出力限额 (kW)
    bool is_anti_reverse_active;     // 是否触发防逆流调节
    bool is_demand_shaving_active;   // 是否触发需量削峰
} iems_microgrid_coordinator_t;

/**
 * @brief 初始化微电网多能协同协调器
 */
void iems_coordinator_init(iems_microgrid_coordinator_t* coord, double transformer_kva, double demand_limit_kw);

/**
 * @brief 配置标准 2充2放 分时电价 (TOU) 峰谷套利计划排程
 */
void iems_coordinator_setup_default_tou(iems_tou_planner_t* planner);

/**
 * @brief 毫秒级微电网功率平衡与防逆流/需量控制闭环计算 (100ms 周期执行)
 * @param p_grid 关口功率 (kW)
 * @param p_pv 光伏功率 (kW)
 * @param p_load 负荷功率 (kW)
 * @param p_ev 充电桩功率 (kW)
 * @param battery_soc 电池当前 SOC (0.0 ~ 100.0)
 * @param current_hour 当前挂钟小时 (0~23)
 * @param current_minute 当前挂钟分钟 (0~59)
 * @param planner TOU 调度计划
 */
int iems_coordinator_evaluate_cycle(iems_microgrid_coordinator_t* coord,
                                    double p_grid, double p_pv, double p_load, double p_ev,
                                    double battery_soc, uint8_t current_hour, uint8_t current_minute,
                                    const iems_tou_planner_t* planner);

/**
 * @brief 重置防逆流 PID 积分与历史状态
 */
void iems_coordinator_reset_pid(iems_microgrid_coordinator_t* coord);

#ifdef __cplusplus
}
#endif

#endif // IEMS_MICROGRID_COORDINATOR_H
