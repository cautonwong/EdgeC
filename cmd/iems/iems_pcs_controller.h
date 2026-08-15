/**
 * @file iems_pcs_controller.h
 * @brief Production Power Conversion System (PCS) Controller: PQ, VF Islanding & VSG Virtual Synchronous Generator
 */

#ifndef IEMS_PCS_CONTROLLER_H
#define IEMS_PCS_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    PCS_MODE_STANDBY   = 0, // 待机模式
    PCS_MODE_GRID_PQ   = 1, // 并网 PQ 恒功率解耦控制模式
    PCS_MODE_ISLAND_VF = 2, // 离网 VF 孤岛黑启动自主构网模式
    PCS_MODE_VSG       = 3, // 虚拟同步机 (VSG) 惯量与下垂阻尼模式
    PCS_MODE_FAULT     = 4  // 故障停机保护
} iems_pcs_mode_t;

typedef struct {
    // VSG 虚拟惯量与阻尼参数
    double vsg_inertia_j;       // 转动惯量 J (kg*m^2, e.g. 5.0)
    double vsg_damping_dp;      // 有功-频率下垂系数 Dp (kW/Hz, e.g. 20.0)
    double vsg_droop_dq;        // 无功-电压下垂系数 Dq (kVAR/V, e.g. 2.5)

    // 额定电气参数
    double rated_voltage_v;     // 额定线电压 (400.0 V)
    double rated_frequency_hz;  // 额定系统频率 (50.00 Hz)
    double max_charge_kw;       // 最大允许充电功率 (e.g. 250.0 kW)
    double max_discharge_kw;    // 最大允许放电功率 (e.g. 250.0 kW)
    double max_reactive_kvar;   // 最大无功调节范围 (e.g. 150.0 kVAR)

    // 实时状态
    iems_pcs_mode_t mode;
    double current_active_kw;
    double current_reactive_kvar;
    double measured_voltage_v;
    double measured_freq_hz;
    double vsg_freq_cmd_hz;
    double vsg_voltage_cmd_v;

    bool is_grid_connected;
    bool is_black_start_ready;
} iems_pcs_controller_t;

/**
 * @brief 初始化 PCS 变流器多模式控制器
 */
void iems_pcs_init(iems_pcs_controller_t* pcs, double max_power_kw);

/**
 * @brief 切换 PCS 运行模式
 */
int iems_pcs_set_mode(iems_pcs_controller_t* pcs, iems_pcs_mode_t target_mode);

/**
 * @brief PQ 恒功率并网调度控制循环 (PQ Grid-Tied Loop)
 */
int iems_pcs_exec_pq_control(iems_pcs_controller_t* pcs, double target_p_kw, double target_q_kvar);

/**
 * @brief VSG 虚拟同步机动态惯量与有功/无功下垂闭环 (VSG Dynamic Inertia & Droop Loop)
 * @param v_meas 实时测量网端线电压 (V)
 * @param f_meas 实时测量电网频率 (Hz)
 * @param df_dt 频率变化率 RoCoF (Hz/s)
 * @param p_setpoint 有功设定基准 (kW)
 * @param q_setpoint 无功设定基准 (kVAR)
 */
int iems_pcs_exec_vsg_cycle(iems_pcs_controller_t* pcs, double v_meas, double f_meas,
                            double df_dt, double p_setpoint, double q_setpoint);

/**
 * @brief VF 孤岛黑启动与零起升压自主构网 (VF Black Start Ramp-Up)
 */
int iems_pcs_exec_black_start(iems_pcs_controller_t* pcs, double target_v, double target_f);

#ifdef __cplusplus
}
#endif

#endif // IEMS_PCS_CONTROLLER_H
