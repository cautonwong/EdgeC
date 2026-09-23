/**
 * @file dcu_horizontal_tasks.h
 * @brief Production DCU Horizontal Tasks Engine (Spatial Branch Line Loss, 3-Phase Dynamic
 * Balancing & Volt-VAR Control)
 */

#ifndef DCU_HORIZONTAL_TASKS_H
#define DCU_HORIZONTAL_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define DCU_MAX_BRANCHES 16

typedef struct {
    uint32_t branch_id;
    char branch_name[32];
    double branch_input_energy_kwh;  // 分支进线总电量
    double branch_consumed_kwh;      // 分支下挂所有从表电量总和
    double branch_loss_rate_pct;     // 分支线损率 (%)
    uint32_t meter_count;
    bool is_abnormal;  // 是否判定为高线损/疑似窃电分支
} dcu_branch_spatial_loss_t;

typedef struct {
    uint32_t target_meter_dev_id;
    char target_meter_addr[16];
    uint8_t from_phase;               // 当前所在相位 (1=A, 2=B, 3=C)
    uint8_t to_phase;                 // 建议切换目标相位 (1=A, 2=B, 3=C)
    double expected_shift_current_a;  // 预期转移负荷电流 (A)
    uint64_t decision_timestamp_ms;
} dcu_phase_switch_command_t;

typedef struct {
    double feeder_head_voltage_v;   // 线路首端电压
    double feeder_tail_voltage_v;   // 线路末端最低电压
    double target_voltage_v;        // 目标基准电压 (e.g. 220.0V)
    double total_pv_active_kw;      // 台区分布式光伏总有功 (kW)
    double required_q_inject_kvar;  // 建议下发无功功率补偿总量 (kVAR)
    bool is_vvc_active;             // 是否触发无功电压联动控制
} dcu_volt_var_state_t;

typedef struct {
    dcu_branch_spatial_loss_t branches[DCU_MAX_BRANCHES];
    uint32_t branch_count;
    dcu_phase_switch_command_t last_commutation;
    dcu_volt_var_state_t vvc_state;
} dcu_horizontal_engine_t;

/**
 * @brief 初始化 DCU 水平任务协同计算引擎
 */
void dcu_horizontal_init(dcu_horizontal_engine_t* engine);

/**
 * @brief 水平任务 1: 16路低压分支箱空间线损横向分摊与防窃电排查
 */
int dcu_horizontal_compute_spatial_loss(dcu_horizontal_engine_t* engine, uint32_t branch_id,
                                        const char* branch_name, double input_kwh,
                                        double consumed_kwh, uint32_t meters);

/**
 * @brief 水平任务 2: 三相负荷动态横向均衡分析与换相开关 (PSU) 调度决策
 * @param ia A 相总负荷电流 (A)
 * @param ib B 相总负荷电流 (A)
 * @param ic C 相总负荷电流 (A)
 * @param heaviest_meter_id 重负荷电表设备 ID
 * @param heaviest_meter_addr 重负荷电表通信地址
 * @param meter_phase 当前所在相
 * @param meter_current 电表负荷电流
 */
int dcu_horizontal_evaluate_phase_balancing(dcu_horizontal_engine_t* engine, double ia, double ib,
                                            double ic, uint32_t heaviest_meter_id,
                                            const char* heaviest_meter_addr, uint8_t meter_phase,
                                            double meter_current, uint64_t now_ms);

/**
 * @brief 水平任务 3: 分布式光伏与储能台区横向协同无功电压控制 (Volt-VAR Control)
 * @param head_u 首端电压 (V)
 * @param tail_u 末端电压 (V)
 * @param total_pv_kw 全台区光伏总出力 (kW)
 */
int dcu_horizontal_compute_volt_var(dcu_horizontal_engine_t* engine, double head_u, double tail_u,
                                    double total_pv_kw);

#ifdef __cplusplus
}
#endif

#endif  // DCU_HORIZONTAL_TASKS_H
