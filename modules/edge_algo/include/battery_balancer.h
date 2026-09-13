#ifndef BATTERY_BALANCER_H
#define BATTERY_BALANCER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_BAL_MAX_CELLS           64
#define BATTERY_BAL_DEFAULT_START_MV    15.0  /**< 默认启动均衡压差门限 (mV) */
#define BATTERY_BAL_DEFAULT_STOP_MV     5.0   /**< 默认停止均衡压差门限 (mV) */
#define BATTERY_BAL_DEFAULT_MAX_CURR_A  2.5   /**< 默认主动均衡最大转移电流 (A) */
#define BATTERY_BAL_BLEED_RESISTOR_OHM  33.0  /**< 被动放电电阻阻值 (Ω) */

/**
 * @brief 均衡工作模式
 */
typedef enum {
    BATTERY_BAL_MODE_DISABLED            = 0, /**< 均衡功能关闭 */
    BATTERY_BAL_MODE_PASSIVE             = 1, /**< 被动阻性放电均衡 */
    BATTERY_BAL_MODE_ACTIVE_PAIRWISE     = 2, /**< 双向主动电荷转移均衡 (Cell-to-Cell) */
    BATTERY_BAL_MODE_HYBRID_OPTIMAL      = 3  /**< 混合最优能效动态规划均衡 */
} battery_bal_mode_t;

/**
 * @brief 单电芯执行动作状态
 */
typedef enum {
    BATTERY_BAL_ACTION_IDLE              = 0, /**< 旁路闲置 */
    BATTERY_BAL_ACTION_DISCHARGE_PASSIVE = 1, /**< 被动阻性放电 */
    BATTERY_BAL_ACTION_ACTIVE_SOURCE     = 2, /**< 主动均衡能量源 (放电转移) */
    BATTERY_BAL_ACTION_ACTIVE_TARGET     = 3  /**< 主动均衡能量宿 (接收充电) */
} battery_bal_action_t;

/**
 * @brief 均衡算法安全与控制配置
 */
typedef struct {
    battery_bal_mode_t mode;             /**< 均衡模式 */
    uint16_t cell_count;                 /**< 电芯总数量 (<= BATTERY_BAL_MAX_CELLS) */
    double nominal_capacity_ah;          /**< 标称容量 (Ah) */
    
    /* 门限与阈值 */
    double start_voltage_diff_mv;        /**< 启动均衡的电压差门限 (mV) */
    double stop_voltage_diff_mv;         /**< 达成一致停止均衡的门限 (mV) */
    double start_soc_diff_pct;           /**< 启动均衡的 SOC 偏差百分比 (0.5% ~ 5.0%) */
    double stop_soc_diff_pct;            /**< 停止均衡的 SOC 偏差百分比 */
    
    /* 电流与功率限制 */
    double max_active_current_a;         /**< 主动均衡最大电流幅值 (A) */
    double bleed_resistor_ohm;           /**< 被动放电电阻 (Ω) */
    uint8_t max_concurrent_channels;     /**< 最大允许同时激活通道数 */
    bool   adjacent_interlock;           /**< 是否开启相邻电芯硬件互锁 */
    
    /* 安全边界 */
    double temp_min_c;                   /**< 允许均衡的最低温度 (°C, 如 5.0) */
    double temp_max_c;                   /**< 允许均衡的最高温度 (°C, 如 45.0) */
    double cell_min_volt_v;              /**< 允许被动放电的最低电芯电压 (V, 如 2.85) */
    double cell_max_volt_v;              /**< 允许主动充电的最高电芯电压 (V, 如 3.60) */
} battery_bal_config_t;

/**
 * @brief 单电芯物理状态输入
 */
typedef struct {
    uint16_t cell_id;
    double voltage_v;                    /**< 电芯电压 (V) */
    double soc;                          /**< 电芯 SOC (0.0 ~ 1.0) */
    double temperature_c;                /**< 电芯温度 (°C) */
    double soh;                          /**< 电芯 SOH (0.0 ~ 1.0) */
    double r0_mohm;                      /**< 欧姆内阻 (mΩ) */
} battery_bal_cell_input_t;

/**
 * @brief 单电芯均衡指令输出
 */
typedef struct {
    uint16_t cell_id;
    battery_bal_action_t action;         /**< 执行动作 */
    double duty_cycle;                   /**< PWM 占空比 (0.0 ~ 1.0) */
    double target_current_a;             /**< 目标充放电电流 (A, 正为充电, 负为放电) */
    double power_dissipated_w;           /**< 损耗发热功率 (W) */
    bool   interlocked;                  /**< 是否因硬件/安全互锁被抑制 */
} battery_bal_channel_cmd_t;

/**
 * @brief 主动双向电荷转移配对
 */
typedef struct {
    uint16_t source_cell_id;             /**< 能量流出高电芯编号 */
    uint16_t target_cell_id;             /**< 能量流入低电芯编号 */
    double transfer_current_a;           /**< 转移电流 (A) */
    double transfer_power_w;             /**< 转移功率 (W) */
    double efficiency;                   /**< 转移能效 (0.80 ~ 0.95) */
} battery_bal_pair_t;

/**
 * @brief 均衡轨迹与能效优化诊断报告
 */
typedef struct {
    bool   is_balancing_active;          /**< 当前是否处于活跃均衡状态 */
    uint8_t active_channels_count;       /**< 当前激活通道数 */
    uint8_t active_pairs_count;          /**< 当前配对数量 */
    
    double max_voltage_v;                /**< 最高电芯电压 (V) */
    double min_voltage_v;                /**< 最低电芯电压 (V) */
    double voltage_diff_mv;              /**< 当前最大极差电压 (mV) */
    double avg_soc;                      /**< 平均 SOC */
    double max_soc_diff_pct;             /**< 当前最大 SOC 极差 (%) */
    
    double estimated_time_to_balance_s;  /**< 预估达到目标一致所需时长 (s) */
    double total_energy_transferred_wh;  /**< 本轮累计有效转移能量 (Wh) */
    double total_energy_dissipated_wh;   /**< 本轮累计焦耳热耗散能量 (Wh) */
    double energy_efficiency_score;      /**< 综合能效得分 (0.0 ~ 1.0) */
    
    battery_bal_channel_cmd_t commands[BATTERY_BAL_MAX_CELLS];
    battery_bal_pair_t pairs[BATTERY_BAL_MAX_CELLS / 2];
    bool   valid;
} battery_bal_plan_t;

/**
 * @brief 多电芯均衡控制器句柄
 */
typedef struct {
    battery_bal_config_t config;
    battery_bal_cell_input_t cells[BATTERY_BAL_MAX_CELLS];
    battery_bal_plan_t plan;
    
    /* 历史累计运行统计 */
    double cum_transferred_wh;
    double cum_dissipated_wh;
    uint64_t total_cycles_planned;
    double last_timestamp_s;
} battery_balancer_t;

/**
 * @brief 初始化多电芯能效优化均衡控制器
 * @param bal 控制器句柄
 * @param cfg 配置参数 (若为 NULL 则使用工业级默认参数)
 * @param cell_count 电芯数量 (2 ~ BATTERY_BAL_MAX_CELLS)
 * @return 0 成功, 负数 错误码
 */
int battery_balancer_init(battery_balancer_t *bal, const battery_bal_config_t *cfg, uint16_t cell_count);

/**
 * @brief 更新单电芯物理输入采样
 * @param bal 控制器句柄
 * @param cell_id 电芯编号 (0 ~ cell_count-1)
 * @param input 电芯物理参数
 * @return 0 成功, 负数 错误码
 */
int battery_balancer_update_cell(battery_balancer_t *bal, uint16_t cell_id, const battery_bal_cell_input_t *input);

/**
 * @brief 执行动态规划最优能效均衡规划
 * @param bal 控制器句柄
 * @param cur_timestamp_s 当前绝对时间戳 (s)
 * @return 0 成功, 负数 错误码
 */
int battery_balancer_plan_step(battery_balancer_t *bal, double cur_timestamp_s);

/**
 * @brief 获取最新生成的均衡执行计划与控制指令
 * @param bal 控制器句柄
 * @return 计划报告指针, 若无效返回 NULL
 */
const battery_bal_plan_t *battery_balancer_get_plan(const battery_balancer_t *bal);

/**
 * @brief 重置均衡控制器状态
 * @param bal 控制器句柄
 */
void battery_balancer_reset(battery_balancer_t *bal);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_BALANCER_H */
