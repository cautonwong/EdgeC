#ifndef BATTERY_GAS_EARLY_WARNING_H
#define BATTERY_GAS_EARLY_WARNING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_GAS_HISTORY_LEN 16

/**
 * @brief 电池热失控多级预警状态枚举
 */
typedef enum {
    BATTERY_GAS_STAGE_NORMAL              = 0, /**< 正常安全背景状态 */
    BATTERY_GAS_STAGE_STAGE1_PREVENT_TRACE= 1, /**< 喷阀前微量潜伏预警 (VOC/H2 微量初渗, 10~30min) */
    BATTERY_GAS_STAGE_STAGE2_VENT_CONFIRM = 2, /**< 安全阀喷阀确认 (CO/H2 突增 + 气压脉冲, 3~10min) */
    BATTERY_GAS_STAGE_STAGE3_RUNAWAY_FIRE = 3, /**< 热失控起火临界 (高浓度烟雾 + 极速温升, <60s) */
    BATTERY_GAS_STAGE_SENSOR_FAULT        = 4  /**< 传感器断线/数据越界故障 */
} battery_gas_stage_t;

/**
 * @brief 传感器输入采样结构体
 */
typedef struct {
    double co_ppm;               /**< 一氧化碳浓度 (ppm, 0.0 ~ 5000.0) */
    double h2_ppm;               /**< 氢气浓度 (ppm, 0.0 ~ 5000.0) */
    double voc_ppb;              /**< 挥发性有机物 VOC (ppb, 0.0 ~ 50000.0) */
    double pressure_pa;          /**< 舱体/Pack 大气压强 (Pa, 80000 ~ 120000) */
    double smoke_density;        /**< 烟雾光电衰减率 / 相对密度 (0.0 ~ 1.0) */
    double max_cell_temp_c;      /**< 最高电芯温度 (°C) */
    double min_cell_volt_v;      /**< 最低电芯电压 (V) */
    double timestamp_s;          /**< 绝对采样时间戳 (s) */
} battery_gas_sensor_sample_t;

/**
 * @brief 早期预警模型配置参数
 */
typedef struct {
    /* 阶段 1 (喷阀前微量渗漏) 门限 */
    double voc_trace_ppb;        /**< VOC 潜伏预警门限 (ppb), 默认 300.0 */
    double h2_trace_ppm;         /**< H2 潜伏预警门限 (ppm), 默认 15.0 */
    double co_trace_ppm;         /**< CO 潜伏预警门限 (ppm), 默认 10.0 */
    
    /* 阶段 2 (安全阀破裂喷阀) 门限 */
    double co_vent_ppm;          /**< CO 喷阀突增门限 (ppm), 默认 50.0 */
    double h2_vent_ppm;          /**< H2 喷阀突增门限 (ppm), 默认 80.0 */
    double pressure_pulse_pa_s;  /**< 气压脉冲突变上升速率 (Pa/s), 默认 30.0 */
    double temp_rate_vent_cs;    /**< 温升速率门限 (°C/s), 默认 0.25 */
    
    /* 阶段 3 (热失控临界起火) 门限 */
    double smoke_runaway_thresh; /**< 烟雾起火判定门限 (0.0 ~ 1.0), 默认 0.15 */
    double co_runaway_ppm;       /**< CO 剧烈失控门限 (ppm), 默认 250.0 */
    double temp_runaway_c;       /**< 电芯危险高温门限 (°C), 默认 75.0 */
    
    /* 传感器有效物理量程 */
    double max_valid_temp_c;     /**< 温度上限 (°C), 默认 150.0 */
    double min_valid_pressure_pa;/**< 气压下限 (Pa), 默认 60000.0 */
    double max_valid_pressure_pa;/**< 气压上限 (Pa), 默认 150000.0 */
} battery_gas_ew_config_t;

/**
 * @brief 多源融合预警诊断与联动动作报告
 */
typedef struct {
    battery_gas_stage_t stage;   /**< 当前综合研判预警等级 */
    double prob_prevent_latent;  /**< 喷阀前潜伏期后验概率 P(Stage1|E) (0.0 ~ 1.0) */
    double prob_vent_confirmed;  /**< 喷阀已发生后验概率 P(Stage2|E) (0.0 ~ 1.0) */
    double prob_runaway_fire;    /**< 热失控起火后验概率 P(Stage3|E) (0.0 ~ 1.0) */
    double estimated_seconds_to_runaway; /**< 预估距不可逆剧烈起火剩余前置时间 (s) */
    
    /* 物理微分特征 */
    double dco_dt_ppm_s;         /**< CO 变化率 (ppm/s) */
    double dh2_dt_ppm_s;         /**< H2 变化率 (ppm/s) */
    double dvoc_dt_ppb_s;        /**< VOC 变化率 (ppb/s) */
    double dpressure_dt_pa_s;    /**< 气压变化率 (Pa/s) */
    double dtemp_dt_cs;          /**< 温升变化率 (°C/s) */
    
    /* 应急消防与保护联动指令 */
    bool trip_contactor_req;     /**< 主回路高压直流接触器分断跳闸请求 */
    bool start_exhaust_fan_req;  /**< 启动防爆排风机排放可燃气体 */
    bool trigger_extinguisher_req;/**< 触发全氟己酮 / 气溶胶灭火介质释放 */
    
    bool valid;
} battery_gas_ew_report_t;

/**
 * @brief 气体预警检测器内部状态结构体
 */
typedef struct {
    battery_gas_ew_config_t config;
    battery_gas_sensor_sample_t history[BATTERY_GAS_HISTORY_LEN];
    uint16_t sample_count;
    
    /* 自适应基线追踪 */
    double baseline_co;
    double baseline_h2;
    double baseline_voc;
    double baseline_pressure;
    
    battery_gas_ew_report_t report;
    uint64_t total_samples_processed;
    double last_timestamp_s;
} battery_gas_ew_detector_t;

/**
 * @brief 初始化热失控喷阀前气体多源预警检测器
 * @param ew 检测器句柄
 * @param cfg 配置参数 (若为 NULL 则使用工业级默认参数)
 * @return 0 成功, 负数 错误码
 */
int battery_gas_ew_init(battery_gas_ew_detector_t *ew, const battery_gas_ew_config_t *cfg);

/**
 * @brief 喂入一组最新采样的多传感器特征数据
 * @param ew 检测器句柄
 * @param sample 采样数据
 * @return 0 成功, 负数 错误码
 */
int battery_gas_ew_push_sample(battery_gas_ew_detector_t *ew, const battery_gas_sensor_sample_t *sample);

/**
 * @brief 执行贝叶斯信念网络多源概率融合推理与火灾联动逻辑
 * @param ew 检测器句柄
 * @param cur_timestamp_s 当前绝对时间戳 (s)
 * @return 0 成功, 负数 错误码
 */
int battery_gas_ew_process_step(battery_gas_ew_detector_t *ew, double cur_timestamp_s);

/**
 * @brief 获取最新评估报告与联动动作指令
 * @param ew 检测器句柄
 * @return 报告指针, 若无效返回 NULL
 */
const battery_gas_ew_report_t *battery_gas_ew_get_report(const battery_gas_ew_detector_t *ew);

/**
 * @brief 重置检测器内部状态与基线
 * @param ew 检测器句柄
 */
void battery_gas_ew_reset(battery_gas_ew_detector_t *ew);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_GAS_EARLY_WARNING_H */
