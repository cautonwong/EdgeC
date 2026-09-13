/**
 * @file battery_pulse_heating.h
 * @brief 极端低温环境电池脉冲自加热与热电耦合控制状态机 (AEP-738)
 * @details 基于高频交流正弦/双向脉冲电流激励与双节点核心-外壳电热耦合动力学模型，
 *          实现 -30℃ 至 0℃ 极端低温工况下的无损快速内部自加热与防析锂过电位安全闭环控制。
 */

#ifndef BATTERY_PULSE_HEATING_H
#define BATTERY_PULSE_HEATING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_HEAT_MAX_CELLS 64
#define PULSE_HEAT_MAX_TEMPS 32

/**
 * @brief 脉冲自加热控制状态机枚举
 */
typedef enum {
    PULSE_HEAT_STATE_IDLE = 0,               /**< 空闲/待机 (温度正常或未启动) */
    PULSE_HEAT_STATE_PRECHECK,               /**< 自检与准入评估 (电压/SOC/绝缘/硬件就绪) */
    PULSE_HEAT_STATE_FREQ_SWEEP,             /**< 宽频微扰阻抗扫频 (辨识纯欧姆热最优化频率) */
    PULSE_HEAT_STATE_RAMP_UP,                /**< 激励电流软起动斜坡上升 */
    PULSE_HEAT_STATE_ACTIVE_HEATING,         /**< 稳态闭环高频脉冲自加热 */
    PULSE_HEAT_STATE_COOLDOWN_HOMOGENIZE,    /**< 间歇静置热扩散均温期 */
    PULSE_HEAT_STATE_COMPLETED,              /**< 自加热完成 (达到目标温升且梯次均匀) */
    PULSE_HEAT_STATE_FAULT                   /**< 故障保护切断 (超压/欠压/过温/超梯度/析锂告警) */
} pulse_heat_state_t;

/**
 * @brief 脉冲自加热故障保护码
 */
typedef enum {
    PULSE_HEAT_FAULT_NONE = 0,               /**< 无故障 */
    PULSE_HEAT_FAULT_CELL_UNDERVOLTAGE,      /**< 单体电压低于动态下限截止 */
    PULSE_HEAT_FAULT_CELL_OVERVOLTAGE,       /**< 单体电压突破动态上限保护 */
    PULSE_HEAT_FAULT_OVERTEMP,               /**< 电池局部温度超限 */
    PULSE_HEAT_FAULT_HIGH_THERMAL_GRADIENT,  /**< 核心-表面或电芯间温差超限 */
    PULSE_HEAT_FAULT_SOC_OUT_OF_BOUNDS,      /**< SOC 超出允许加热安全区间 (20%~85%) */
    PULSE_HEAT_FAULT_LITHIUM_PLATING_RISK,   /**< 负极过电位逼近零点 (析锂高危阻断) */
    PULSE_HEAT_FAULT_INVERTER_NOT_READY,     /**< 逆变功率桥臂硬件未就绪或过流 */
    PULSE_HEAT_FAULT_TIMEOUT                 /**< 加热总耗时超过最大限制超时 */
} pulse_heat_fault_t;

/**
 * @brief 自加热控制系统参数配置
 */
typedef struct {
    float target_temp_c;                     /**< 目标加热终止温度 (℃, e.g. 10.0) */
    float min_start_temp_c;                  /**< 允许启动自加热的温度门限 (℃, e.g. 0.0) */
    float max_temp_limit_c;                  /**< 紧急过温切断阈值 (℃, e.g. 45.0) */
    float max_thermal_gradient_c;            /**< 最大允许核心-表面温差 (℃, e.g. 5.0) */
    float max_cell_temp_spread_c;            /**< 最大允许电芯间温差极差 (℃, e.g. 4.0) */
    float min_soc;                           /**< 最小允许 SOC (0.0~1.0, e.g. 0.20) */
    float max_soc;                           /**< 最大允许 SOC (0.0~1.0, e.g. 0.85) */
    float max_current_amp_a;                 /**< 最大交流脉冲幅值 (A, e.g. 150.0) */
    float nominal_frequency_hz;              /**< 标称最优加热频率 (Hz, e.g. 500.0) */
    float sweep_freq_start_hz;               /**< 扫频起始频率 (Hz, e.g. 100.0) */
    float sweep_freq_end_hz;                 /**< 扫频截止频率 (Hz, e.g. 1000.0) */
    float current_ramp_rate_a_per_s;         /**< 电流软起动变化率 (A/s, e.g. 25.0) */
    float active_burst_duration_s;           /**< 连续加热脉冲窗口周期 (s, e.g. 20.0) */
    float homogenize_duration_s;             /**< 间歇均温静置周期 (s, e.g. 4.0) */
    float timeout_limit_s;                   /**< 加热总流程硬超时限制 (s, e.g. 600.0) */
    float cell_v_min_nominal;                /**< 标称截止下限 (V, e.g. 2.2) */
    float cell_v_max_nominal;                /**< 标称截止上限 (V, e.g. 3.75) */
    float cell_heat_capacity_j_per_k;        /**< 电芯等效热容 (J/K, e.g. 450.0) */
    float core_surface_r_th;                 /**< 核心至表面热阻 (K/W, e.g. 0.8) */
    float surface_ambient_r_th;              /**< 表面至环境热阻 (K/W, e.g. 3.5) */
    float baseline_resistance_ohm;           /**< 25℃ 标称交流欧姆内阻 (Ω, e.g. 0.001) */
    float arrhenius_activation_energy_j;     /**< Arrhenius 活化能 Ea (J/mol, e.g. 30000.0) */
} pulse_heat_config_t;

/**
 * @brief 实时电芯遥测采样结构
 */
typedef struct {
    float cell_voltages[PULSE_HEAT_MAX_CELLS]; /**< 电芯采样端电压 (V) */
    uint16_t cell_count;                       /**< 电芯总数 (1..64) */
    float cell_temps[PULSE_HEAT_MAX_TEMPS];    /**< 表面温度采样点 (℃) */
    uint16_t temp_count;                       /**< 温度传感器总数 (1..32) */
    float ambient_temp_c;                      /**< 环境温度 (℃) */
    float pack_soc;                            /**< Pack 估算 SOC (0.0~1.0) */
    float actual_current_rms_a;                /**< 逆变器实际交流电流反馈 RMS (A) */
    float actual_frequency_hz;                 /**< 实际激励频率反馈 (Hz) */
    bool inverter_ready;                       /**< 逆变功率桥臂就绪信号 */
} pulse_heat_cell_telemetry_t;

/**
 * @brief 脉冲自加热实时控制输出与电热耦合观测结构
 */
typedef struct {
    pulse_heat_state_t state;                  /**< 当前状态机状态 */
    pulse_heat_fault_t fault;                  /**< 故障码 */
    float target_current_amp_a;                /**< 目标交流激励电流幅值 (A) */
    float target_frequency_hz;                 /**< 目标激励频率 (Hz) */
    float duty_ratio;                          /**< 双向脉冲占空比 (标称 0.50 对称零电荷偏移) */
    bool inverter_enable;                      /**< 逆变功率桥使能/断开指令 */
    float estimated_core_temp_c;               /**< 双节点模型估算电芯内部核心温度 (℃) */
    float estimated_surface_temp_c;            /**< 双节点模型估算表面温度 (℃) */
    float temp_gradient_c;                     /**< 核心与表面动态温差 (℃) */
    float cell_temp_spread_c;                  /**< 电芯间最高最低表面温差 (℃) */
    float estimated_heating_power_w;           /**< 实时内部焦耳发热功率 (W) */
    float heating_rate_c_per_min;              /**< 实时升温速率 (℃/min) */
    float plating_safety_margin_v;             /**< 负极过电位防析锂安全裕度 (V) */
    float elapsed_heating_time_s;              /**< 累计加热总耗时 (s) */
    float total_energy_consumed_wh;            /**< 累计自加热能耗 (Wh) */
} pulse_heat_control_output_t;

/**
 * @brief 脉冲自加热算法引擎主上下文
 */
typedef struct {
    pulse_heat_config_t cfg;                   /**< 配置参数 */
    pulse_heat_state_t current_state;          /**< 状态机运行态 */
    pulse_heat_fault_t active_fault;           /**< 当前告警故障 */
    float current_cmd_a;                       /**< 当前平滑电流指令 */
    float target_cmd_a;                        /**< 目标稳态电流 */
    float optimal_freq_hz;                     /**< 辨识得到的最优频率 */
    float core_temp_est_c;                     /**< 核心温度状态变量 */
    float surf_temp_est_c;                     /**< 表面温度状态变量 */
    float state_timer_s;                       /**< 当前子状态停留计时器 */
    float total_elapsed_s;                     /**< 总流程计时器 */
    float cumulative_energy_wh;                /**< 累计消耗能量 */
    float last_dt_s;                           /**< 步长记忆 */
    float prev_temp_for_rate_c;                /**< 速率差分参考温度 */
    float rate_timer_s;                        /**< 速率计算差分窗口计时 */
    float calculated_rate_c_min;               /**< 滤波后升温速率 */
    bool initialized;                          /**< 初始化标志 */
} pulse_heat_engine_t;

/**
 * @brief 初始化脉冲自加热引擎
 * @param engine 算法引擎句柄
 * @param cfg 配置参数 (传 NULL 则使用工业默认配置)
 * @return 0 成功, 负数 错误码
 */
int pulse_heat_engine_init(pulse_heat_engine_t *engine, const pulse_heat_config_t *cfg);

/**
 * @brief 重置算法引擎为 IDLE 待机态
 * @param engine 算法引擎句柄
 * @return 0 成功, 负数 错误码
 */
int pulse_heat_engine_reset(pulse_heat_engine_t *engine);

/**
 * @brief 手动触发启动脉冲自加热流程
 * @param engine 算法引擎句柄
 * @return 0 成功启动, 负数 失败 (如未初始化或处于故障态)
 */
int pulse_heat_engine_start(pulse_heat_engine_t *engine);

/**
 * @brief 手动停止脉冲自加热流程并安全退出
 * @param engine 算法引擎句柄
 * @return 0 成功, 负数 错误码
 */
int pulse_heat_engine_stop(pulse_heat_engine_t *engine);

/**
 * @brief 核心控制周期步进计算 (建议 10ms ~ 100ms 周期调用)
 * @param engine 算法引擎句柄
 * @param telem 实时电芯遥测数据
 * @param dt_s 距离上一周期的物理时间增量 (秒)
 * @param out 控制输出结构指针
 * @return 0 成功, 负数 错误码
 */
int pulse_heat_engine_step(pulse_heat_engine_t *engine,
                           const pulse_heat_cell_telemetry_t *telem,
                           float dt_s,
                           pulse_heat_control_output_t *out);

/**
 * @brief 查询当前状态机状态字符串
 * @param state 状态枚举
 * @return 状态字符串名称
 */
const char *pulse_heat_state_to_str(pulse_heat_state_t state);

/**
 * @brief 查询故障码字符串
 * @param fault 故障枚举
 * @return 故障描述字符串
 */
const char *pulse_heat_fault_to_str(pulse_heat_fault_t fault);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_PULSE_HEATING_H */
