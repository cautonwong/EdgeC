/**
 * @file dcu_load_control.h
 * @brief Production DCU Transformer Overload Protection & Demand Response Load Shaving Engine
 */

#ifndef DCU_LOAD_CONTROL_H
#define DCU_LOAD_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LOAD_STATE_NORMAL    = 0, // < 85% Rated Capacity
    LOAD_STATE_WARNING   = 1, // 85% ~ 95% (Level 1 Curtailment: EV Chargers -30%)
    LOAD_STATE_HIGH      = 2, // 95% ~ 110% (Level 2 Curtailment: PV inverters limit, Battery Discharge)
    LOAD_STATE_OVERLOAD  = 3  // > 110% (Level 3 Emergency: Trip non-essential flexible circuits)
} dcu_load_state_t;

typedef struct {
    double rated_capacity_kva;     // 台变额定容量 (kVA, e.g. 630 kVA)
    double current_apparent_kva;   // 当前视在功率 S = sqrt(3) * U * I (kVA)
    double active_power_kw;        // 当前有功功率 P (kW)
    double reactive_power_kvar;    // 当前无功功率 Q (kVAR)
    double load_ratio_pct;         // 负载率 (%)
    dcu_load_state_t state;        // 负载控制状态
    uint32_t overload_duration_s;  // 持续超载时间 (s)
    bool is_curtailment_active;    // 柔性负荷削减是否处于触发状态
    double target_reduction_kw;    // 建议削减负荷目标 (kW)
} dcu_load_controller_t;

/**
 * @brief 初始化台区负荷控制引擎
 * @param rated_kva 台区变压器额定容量 (kVA)
 */
void dcu_load_control_init(dcu_load_controller_t* ctrl, double rated_kva);

/**
 * @brief 评估台变负荷状态并生成柔性削峰调度决策
 * @param u_line 线电压平均值 (V, 如 400V)
 * @param i_line 线电流平均值 (A)
 * @param power_factor 功率因数
 * @param delta_time_s 距上次评估时间步长 (s)
 */
dcu_load_state_t dcu_load_control_evaluate(dcu_load_controller_t* ctrl, double u_line,
                                           double i_line, double power_factor,
                                           uint32_t delta_time_s);

#ifdef __cplusplus
}
#endif

#endif // DCU_LOAD_CONTROL_H
