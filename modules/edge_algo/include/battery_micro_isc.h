#ifndef BATTERY_MICRO_ISC_H
#define BATTERY_MICRO_ISC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_MAX_CELLS_PER_PACK    64
#define BATTERY_RELAXATION_WINDOW_SIZE 64

/**
 * @brief 储能电芯早期预警级别
 */
typedef enum {
    BATTERY_ALARM_NORMAL              = 0,  /**< 正常健康状态 */
    BATTERY_ALARM_EARLY_WARNING_7D    = 1,  /**< 7天早期预警 (轻度析锂或微弱自放电异常, R_isc 1k~2kΩ) */
    BATTERY_ALARM_EARLY_WARNING_3D    = 2,  /**< 3天早期预警 (显著析锂或中度微短路, R_isc 200~1kΩ) */
    BATTERY_ALARM_URGENT_1D           = 3,  /**< 1天紧急告警 (严重微短路加速, R_isc 50~200Ω) */
    BATTERY_ALARM_CRITICAL_IMMEDIATE  = 4   /**< 立即危险告警 (突发硬短路风险, R_isc < 50Ω 或 ΔV > 35mV) */
} battery_isc_alarm_level_t;

/**
 * @brief 微短路与析锂辨识算法配置参数
 */
typedef struct {
    float nominal_capacity_ah;           /**< 电芯标称容量 (Ah), 默认 280.0 */
    float sample_interval_s;             /**< 采样间隔 (s), 默认 1.0 */
    float warn_voltage_diff_v;           /**< 早期预警电压偏差阈值 (V), 默认 0.015 (15mV) */
    float alarm_voltage_diff_v;          /**< 严重短路电压偏差阈值 (V), 默认 0.035 (35mV) */
    float warn_r_isc_ohm;                /**< 微短路内阻预警阈值 (Ω), 默认 1000.0 */
    float alarm_r_isc_ohm;               /**< 微短路内阻告警阈值 (Ω), 默认 200.0 */
    float critical_r_isc_ohm;            /**< 紧急硬短路内阻阈值 (Ω), 默认 50.0 */
    float li_plating_d2v_threshold;      /**< 松弛电压二阶导数曲率残差析锂阈值 (V/s^2), 默认 0.0002 */
    float rest_current_threshold_a;      /**< 静置松弛判定电流阈值 (A), 绝对值小于此值视为静置, 默认 0.2 */
} battery_micro_isc_config_t;

/**
 * @brief 单电芯松弛电压滑动窗口与曲率特征
 */
typedef struct {
    float voltage_window[BATTERY_RELAXATION_WINDOW_SIZE];  /**< 松弛电压历史窗口 */
    float time_window[BATTERY_RELAXATION_WINDOW_SIZE];     /**< 松弛时间历史 (s) */
    uint16_t window_count;                                 /**< 窗口当前样本数 */
    float rest_start_voltage;                              /**< 进入静置瞬间初始电压 (V) */
    float rest_start_soc;                                  /**< 进入静置瞬间初始 SOC (0.0~1.0) */
    double rest_start_time_s;                              /**< 静置起始绝对时间戳 */
    double total_rest_time_s;                              /**< 累计静置时长 (s) */
    float d2v_dt2_max_residual;                            /**< 松弛期二阶导数曲率残差峰值 */
    bool li_plating_detected;                              /**< 是否检出析锂剥离平台 */
    float li_plating_severity;                             /**< 析锂严重度 (0.0 ~ 1.0) */
} cell_relaxation_data_t;

/**
 * @brief 单电芯状态与辨识诊断报告
 */
typedef struct {
    uint16_t cell_id;                                      /**< 电芯逻辑编号 (0 ~ N-1) */
    float voltage;                                         /**< 当前采集端电压 (V) */
    float current;                                         /**< 当前电芯电流 (A, 正放负充) */
    float temperature;                                     /**< 当前电芯温度 (°C) */
    float soc;                                             /**< 当前电芯 SOC (0.0 ~ 1.0) */
    float r_isc_ohm;                                       /**< 辨识得到的微短路内阻 (Ω) */
    float self_discharge_rate_pct_day;                     /**< 日自放电率 (%/day) */
    float voltage_residual_mv;                             /**< 相对于 Pack 统计基准的压差残差 (mV) */
    float estimated_days_to_failure;                       /**< 预估发生严重短路/热失控前置天数 (1.0 ~ 7.0+) */
    battery_isc_alarm_level_t alarm_level;                 /**< 当前诊断预警级别 */
    cell_relaxation_data_t relaxation;                     /**< 松弛曲率分析数据 */
} cell_isc_state_t;

/**
 * @brief 多电芯 Pack 级微短路与析锂辨识检测引擎结构体
 */
typedef struct {
    battery_micro_isc_config_t config;                     /**< 算法参数配置 */
    uint16_t cell_count;                                   /**< 电芯总数量 (<= BATTERY_MAX_CELLS_PER_PACK) */
    cell_isc_state_t cells[BATTERY_MAX_CELLS_PER_PACK];   /**< 电芯状态列表 */
    float pack_avg_voltage;                                /**< 组端电芯均值电压 (V) */
    float pack_median_voltage;                             /**< 组端电芯中位数电压 (V) */
    float pack_current;                                    /**< 电池簇总电流 (A) */
    bool is_in_rest;                                       /**< 当前电池簇是否处于静置松弛态 */
    double rest_start_timestamp;                           /**< 当前静置开始时间戳 */
    double accumulated_rest_s;                             /**< 连续静置时长 (s) */
    uint64_t total_samples_processed;                      /**< 累计处理样本计数 */
} pack_micro_isc_detector_t;

/**
 * @brief 初始化微短路与析锂早期预警检测器
 * @param det 检测器结构体指针
 * @param cfg 配置参数指针 (若为 NULL 则使用默认工业级参数)
 * @param cell_count 电芯数量 (1 ~ BATTERY_MAX_CELLS_PER_PACK)
 * @return 0 成功, 负数 错误码
 */
int battery_micro_isc_init(pack_micro_isc_detector_t *det,
                           const battery_micro_isc_config_t *cfg,
                           uint16_t cell_count);

/**
 * @brief 喂入单个电芯最新物理采集数据
 * @param det 检测器指针
 * @param cell_id 电芯编号
 * @param voltage 物理端电压 (V)
 * @param current 物理电流 (A, 正放负充)
 * @param temp 物理温度 (°C)
 * @param soc 估计 SOC (0.0 ~ 1.0)
 * @param timestamp_s 绝对时间戳 (s)
 * @return 0 成功, 负数 错误码
 */
int battery_micro_isc_feed_cell(pack_micro_isc_detector_t *det,
                                uint16_t cell_id,
                                float voltage,
                                float current,
                                float temp,
                                float soc,
                                double timestamp_s);

/**
 * @brief 执行 Pack 级批量辨识、析锂曲率残差提取与 1~7 天早期预警诊断计算
 * @param det 检测器指针
 * @param cur_timestamp_s 当前时间戳 (s)
 * @return 0 成功, 负数 错误码
 */
int battery_micro_isc_process_step(pack_micro_isc_detector_t *det,
                                   double cur_timestamp_s);

/**
 * @brief 针对单电芯提取松弛电压二阶导数曲率残差并检出析锂相变
 * @param cell 单电芯状态结构体
 * @param d2v_threshold 二阶导数阈值
 * @param out_curvature_residual 输出二阶导数残差峰值
 * @param out_detected 输出是否检出析锂剥离特征
 * @param out_severity 输出析锂严重度 (0.0 ~ 1.0)
 * @return 0 成功, 负数 错误码
 */
int battery_micro_isc_detect_lithium_plating(cell_isc_state_t *cell,
                                             float d2v_threshold,
                                             float *out_curvature_residual,
                                             bool *out_detected,
                                             float *out_severity);

/**
 * @brief 基于多电芯基准自放电分离计算单电芯微短路等效电阻 R_isc
 * @param cell 单电芯状态
 * @param baseline_avg_voltage Pack 均值基准电压 (V)
 * @param baseline_delta_soc Pack 均值基准 SOC 衰减量
 * @param nominal_capacity_ah 标称容量 (Ah)
 * @param rest_time_s 静置时长 (s)
 * @param out_r_isc 输出辨识得到的微短路电阻 (Ω)
 * @param out_discharge_rate_pct_day 输出日自放电率 (%/day)
 * @return 0 成功, 负数 错误码
 */
int battery_micro_isc_identify_r_isc(cell_isc_state_t *cell,
                                     float baseline_avg_voltage,
                                     float baseline_delta_soc,
                                     float nominal_capacity_ah,
                                     float rest_time_s,
                                     float *out_r_isc,
                                     float *out_discharge_rate_pct_day);

/**
 * @brief 获取指定电芯的完整诊断状态
 */
int battery_micro_isc_get_cell_report(const pack_micro_isc_detector_t *det,
                                      uint16_t cell_id,
                                      cell_isc_state_t *out_state);

/**
 * @brief 获取 Pack 组内最高告警级别与最危险电芯编号
 */
int battery_micro_isc_get_highest_alarm(const pack_micro_isc_detector_t *det,
                                        battery_isc_alarm_level_t *out_max_alarm,
                                        uint16_t *out_worst_cell_id);

/**
 * @brief 重置检测器内部状态
 */
void battery_micro_isc_reset(pack_micro_isc_detector_t *det);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_MICRO_ISC_H */
