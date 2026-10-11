/**
 * @file battery_thermal_pinn.h
 * @brief Megawatt ESS Battery Thermal Runaway Early Warning & PINN Digital Twin Engine (AEP-608)
 *
 * Implements:
 * 1. Physics-Informed Neural Network (PINN) + Fractional-Order ECM thermal generation solver.
 * 2. Lithium deposition micro-internal short circuit (ISC) resistance (R_isc) identification.
 * 3. 1~7 day early warning risk scoring with 4-level safety protection arbitration (Notice/Alarm/Trip).
 * 4. Zero dynamic allocation (Caller-Allocated storage).
 */

#ifndef BATTERY_THERMAL_PINN_H
#define BATTERY_THERMAL_PINN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PINN_MAX_CELLS         64
#define PINN_WINDOW_SIZE       16

typedef enum {
    PINN_WARN_NORMAL = 0,
    PINN_WARN_NOTICE = 1,
    PINN_WARN_ALARM  = 2,
    PINN_WARN_TRIP   = 3
} pinn_warning_level_t;

/**
 * @brief 电芯热失控与电化学风险状态
 */
typedef struct {
    uint32_t cell_id;
    float r0_mohm;                /* 实时辨识欧姆内阻 (mΩ) */
    float r_isc_ohm;              /* 估算微内短路等效电阻 (Ω, 正常 > 1000Ω) */
    float thermal_generation_w;   /* 异常产热功率 Q_abnormal (W) */
    float predicted_temp_1h;      /* 1小时后预测电芯温度 (°C) */
    float runaway_risk_score;     /* 热失控综合风险评分 (0.00 ~ 1.00) */
    pinn_warning_level_t warning_level;
    uint64_t update_timestamp_ns;
} cell_thermal_risk_t;

/**
 * @brief 电芯时序采样样本
 */
typedef struct {
    float voltage_v;
    float current_a;
    float temp_c;
} cell_sample_t;

/**
 * @brief PINN 物理参数配置
 */
typedef struct {
    float nominal_capacity_ah;    /* 额定容量 (Ah, 如 280Ah) */
    float c_th_joule_per_k;       /* 电芯热容 C_th (J/K) */
    float r_th_k_per_w;           /* 等效热阻 R_th (K/W) */
    float temp_ambient_c;         /* 环境温度 (°C) */
    float r_isc_alarm_ohm;        /* 微短路报警阈值 (Ω, 默认 50.0Ω) */
    float r_isc_trip_ohm;         /* 微短路跳闸阈值 (Ω, 默认 5.0Ω) */
    float q_abnormal_trip_w;      /* 异常产热跳闸阈值 (W, 默认 15.0W) */
} pinn_config_t;

/**
 * @brief PINN 引擎遥测统计指标
 */
typedef struct {
    uint64_t total_inference_cycles;
    uint32_t active_cells_monitored;
    uint32_t cells_in_notice;
    uint32_t cells_in_alarm;
    uint32_t cells_in_trip;
    float max_risk_score;
    float max_q_abnormal_w;
} pinn_telemetry_t;

/**
 * @brief 单电芯通道上下文
 */
typedef struct {
    uint32_t cell_id;
    cell_sample_t window[PINN_WINDOW_SIZE];
    uint32_t sample_count;
    cell_thermal_risk_t current_risk;
    bool is_initialized;
} pinn_cell_channel_t;

/**
 * @brief PINN 引擎主结构 (Caller-Allocated)
 */
typedef struct {
    pinn_config_t config;
    pinn_cell_channel_t channels[PINN_MAX_CELLS];
    uint32_t channel_count;
    pinn_telemetry_t telemetry;
} battery_thermal_pinn_t;

/**
 * @brief 初始化 PINN 储能电芯热失控数字孪生推理引擎
 */
int battery_thermal_pinn_init(battery_thermal_pinn_t *engine, const pinn_config_t *config);

/**
 * @brief 注册监控电芯通道
 */
int battery_thermal_pinn_register_cell(battery_thermal_pinn_t *engine, uint32_t cell_id);

/**
 * @brief 输入单电芯实时物理采样并执行单步 PINN 推理与电化学残差辨识
 */
int battery_thermal_pinn_feed_sample(battery_thermal_pinn_t *engine, uint32_t cell_id,
                                     float voltage_v, float current_a, float temp_c,
                                     uint64_t timestamp_ns, cell_thermal_risk_t *out_risk);

/**
 * @brief 获取特定电芯的最新风险评分
 */
int battery_thermal_pinn_get_cell_risk(const battery_thermal_pinn_t *engine, uint32_t cell_id,
                                       cell_thermal_risk_t *out_risk);

/**
 * @brief 获取 PINN 引擎整体遥测状态
 */
void battery_thermal_pinn_get_telemetry(const battery_thermal_pinn_t *engine, pinn_telemetry_t *out_telem);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_THERMAL_PINN_H */
