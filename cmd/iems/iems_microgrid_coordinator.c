/**
 * @file iems_microgrid_coordinator.c
 * @brief Production Commercial Microgrid EMS Coordinator Implementation
 */

#include "iems_microgrid_coordinator.h"

#include <math.h>
#include <string.h>

void iems_coordinator_init(iems_microgrid_coordinator_t* coord, double transformer_kva,
                           double demand_limit_kw) {
    if (!coord) return;
    memset(coord, 0, sizeof(iems_microgrid_coordinator_t));
    coord->transformer_capacity_kva = (transformer_kva > 0.0) ? transformer_kva : 630.0;
    coord->demand_limit_kw = (demand_limit_kw > 0.0) ? demand_limit_kw : 500.0;
    coord->anti_reverse_deadband_kw = 5.0;  // 5kW 安全防倒送裕度

    // 默认 PID 参数 (针对 100ms 快速闭环)
    coord->pid_kp = 1.0;
    coord->pid_ki = 0.2;
    coord->pid_kd = 0.05;
}

void iems_coordinator_reset_pid(iems_microgrid_coordinator_t* coord) {
    if (!coord) return;
    coord->pid_integral = 0.0;
    coord->pid_prev_error = 0.0;
}

void iems_coordinator_setup_default_tou(iems_tou_planner_t* planner) {
    if (!planner) return;
    memset(planner, 0, sizeof(iems_tou_planner_t));
    planner->is_schedule_enabled = true;
    planner->slot_count = 4;

    // Slot 1: 00:00 - 08:00 (Valley 谷段充电)
    planner->slots[0] = (iems_tou_schedule_slot_t){.start_hour = 0,
                                                   .start_minute = 0,
                                                   .end_hour = 8,
                                                   .end_minute = 0,
                                                   .action = TOU_ACTION_CHARGE,
                                                   .target_power_kw = -100.0};
    // Slot 2: 08:00 - 11:30 (Peak 峰段放电 1)
    planner->slots[1] = (iems_tou_schedule_slot_t){.start_hour = 8,
                                                   .start_minute = 0,
                                                   .end_hour = 11,
                                                   .end_minute = 30,
                                                   .action = TOU_ACTION_DISCHARGE,
                                                   .target_power_kw = 100.0};
    // Slot 3: 11:30 - 14:00 (Flat 平段充电 2 / 吸收午间光伏)
    planner->slots[2] = (iems_tou_schedule_slot_t){.start_hour = 11,
                                                   .start_minute = 30,
                                                   .end_hour = 14,
                                                   .end_minute = 0,
                                                   .action = TOU_ACTION_CHARGE,
                                                   .target_power_kw = -80.0};
    // Slot 4: 17:00 - 21:00 (Peak 尖段放电 2)
    planner->slots[3] = (iems_tou_schedule_slot_t){.start_hour = 17,
                                                   .start_minute = 0,
                                                   .end_hour = 21,
                                                   .end_minute = 0,
                                                   .action = TOU_ACTION_DISCHARGE,
                                                   .target_power_kw = 120.0};
}

int iems_coordinator_evaluate_cycle(iems_microgrid_coordinator_t* coord, double p_grid, double p_pv,
                                    double p_load, double p_ev, double battery_soc,
                                    uint8_t current_hour, uint8_t current_minute,
                                    const iems_tou_planner_t* planner) {
    if (!coord) return -1;

    coord->p_grid_kw = p_grid;
    coord->p_pv_kw = p_pv;
    coord->p_load_kw = p_load;
    coord->p_ev_kw = p_ev;

    double total_consumption = p_load + p_ev;
    double net_load = total_consumption - p_pv;

    // 滑动窗口需量更新
    coord->demand_window_sum += (net_load > 0.0) ? net_load : 0.0;
    coord->demand_window_samples++;
    if (coord->demand_window_samples > 9000) {  // 超过 15 分钟 (9000 * 100ms) 时重置窗口保持平滑
        coord->demand_window_sum = coord->demand_window_sum / 2.0;
        coord->demand_window_samples = coord->demand_window_samples / 2;
    }
    coord->demand_15min_avg_kw = coord->demand_window_sum / (double)coord->demand_window_samples;

    // 1. 基础 TOU 计划功率决策
    double scheduled_power_kw = 0.0;
    if (planner && planner->is_schedule_enabled) {
        uint16_t now_min = (uint16_t)(current_hour * 60 + current_minute);
        for (uint8_t i = 0; i < planner->slot_count; i++) {
            const iems_tou_schedule_slot_t* s = &planner->slots[i];
            uint16_t start_min = (uint16_t)(s->start_hour * 60 + s->start_minute);
            uint16_t end_min = (uint16_t)(s->end_hour * 60 + s->end_minute);
            if (now_min >= start_min && now_min < end_min) {
                if (s->action == TOU_ACTION_CHARGE && battery_soc < 95.0) {
                    scheduled_power_kw = s->target_power_kw;  // Negative (charging)
                } else if (s->action == TOU_ACTION_DISCHARGE && battery_soc > 10.0) {
                    scheduled_power_kw = s->target_power_kw;  // Positive (discharging)
                }
                break;
            }
        }
    }

    coord->target_pcs_power_kw = scheduled_power_kw;
    coord->target_pv_limit_kw = p_pv;  // Default full generation
    coord->is_anti_reverse_active = false;
    coord->is_demand_shaving_active = false;

    // 2. 需量控制闭环 (Demand Peak Shaving):
    // 若瞬时或滚动需量超过需量红线 (demand_limit_kw), 储能强制加大放电进行削峰
    double peak_load =
        (net_load > coord->demand_15min_avg_kw) ? net_load : coord->demand_15min_avg_kw;
    if (peak_load > coord->demand_limit_kw && battery_soc > 10.0) {
        double required_discharge_kw = peak_load - coord->demand_limit_kw;
        coord->is_demand_shaving_active = true;
        if (required_discharge_kw > coord->target_pcs_power_kw) {
            coord->target_pcs_power_kw = required_discharge_kw;
        }
    }

    // 3. 动态防逆流闭环 (Anti-Reverse Power Flow / Zero Export with Fast PID):
    // 关口功率倒送 (p_grid < safety_deadband, 出现向电网反送电风险)
    if (p_grid < coord->anti_reverse_deadband_kw) {
        coord->is_anti_reverse_active = true;
        double error = coord->anti_reverse_deadband_kw - p_grid;

        // PID 调节量计算 (dt = 0.1s = 100ms)
        double dt = 0.1;
        coord->pid_integral += error * dt;
        if (coord->pid_integral > 500.0) coord->pid_integral = 500.0;
        if (coord->pid_integral < -500.0) coord->pid_integral = -500.0;

        double derivative = (error - coord->pid_prev_error) / dt;
        double pid_out = coord->pid_kp * error + coord->pid_ki * coord->pid_integral +
                         coord->pid_kd * derivative;
        coord->pid_prev_error = error;

        if (pid_out < error) pid_out = error;

        // 优先利用储能加大充电功率吸收余电
        if (battery_soc < 95.0) {
            coord->target_pcs_power_kw -= pid_out;  // 进一步加大充电功率 (负值加大)
        } else {
            // 储能已满，光伏逆变器快速降额限出力
            double surplus_kw = coord->anti_reverse_deadband_kw - p_grid;
            coord->target_pv_limit_kw = (p_pv > surplus_kw) ? (p_pv - surplus_kw) : 0.0;
        }
    } else {
        // 无倒送风险时缓慢复位 PID 误差
        coord->pid_prev_error = 0.0;
        if (coord->pid_integral > 0.0) {
            coord->pid_integral *= 0.95;
        }
    }

    return 0;
}
