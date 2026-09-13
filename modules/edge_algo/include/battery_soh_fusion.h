#ifndef BATTERY_SOH_FUSION_H
#define BATTERY_SOH_FUSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_SOH_ICA_WINDOW_SIZE    64
#define BATTERY_SOH_DEFAULT_EOL_THRESH 0.80  /**< 默认退役门限 80% SOH */

/**
 * @brief SOH 评估健康分级
 */
typedef enum {
    BATTERY_SOH_GRADE_EXCELLENT = 0, /**< 极佳 (SOH >= 95%) */
    BATTERY_SOH_GRADE_GOOD      = 1, /**< 良好 (90% <= SOH < 95%) */
    BATTERY_SOH_GRADE_AGED      = 2, /**< 轻度老化 (85% <= SOH < 90%) */
    BATTERY_SOH_GRADE_DEGRADED  = 3, /**< 显著衰退 (80% <= SOH < 85%) */
    BATTERY_SOH_GRADE_EOL       = 4  /**< 达到或超过退役门限 (SOH < 80%) */
} battery_soh_grade_t;

/**
 * @brief 半经验机理老化模型配置参数
 */
typedef struct {
    double nominal_capacity_ah;  /**< 标称额定容量 (Ah), 默认 280.0 */
    double fresh_r0_mohm;        /**< 新电芯 25°C 标称欧姆内阻 (mΩ), 默认 0.50 */
    double fresh_rct_mohm;       /**< 新电芯 25°C 标称电荷转移内阻 (mΩ), 默认 0.80 */
    double eol_soh_ratio;        /**< 寿命终止 EOL SOH 比例 (0.70 ~ 0.85), 默认 0.80 */
    
    /* 日历老化参数 (SEI 抛物线生长 Arrhenius 参数) */
    double a_cal;                /**< 日历老化指前因子 */
    double ea_cal;               /**< 日历老化活化能 (J/mol) */
    double beta_soc;             /**< SOC 应力影响因子 */
    
    /* 循环老化参数 */
    double b_cyc;                /**< 循环老化指前因子 */
    double ea_cyc;               /**< 循环老化活化能 (J/mol) */
    double alpha_crate;          /**< 充放电倍率幂次指数 */
    double beta_dod;             /**< 放电深度 DOD 幂次指数 */
    
    /* 库仑积分有效窗口门限 */
    double min_coulomb_delta_soc;/**< 触发库仑积分辨识的最小 SOC 跨度, 默认 0.25 (25%) */
} battery_soh_config_t;

/**
 * @brief ICA (增量容量 dQ/dV) 特征提取数据
 */
typedef struct {
    double voltage_history[BATTERY_SOH_ICA_WINDOW_SIZE];
    double charge_history_ah[BATTERY_SOH_ICA_WINDOW_SIZE];
    uint16_t sample_count;
    double peak1_voltage_v;      /**< 相变主峰 1 电压位置 (V) */
    double peak1_height;         /**< 相变主峰 1 高度 (Ah/V) */
    double peak2_voltage_v;      /**< 相变主峰 2 电压位置 (V) */
    double peak2_height;         /**< 相变主峰 2 高度 (Ah/V) */
    double peak_area;            /**< 峰下积分有效容量面积 (Ah) */
    bool   features_valid;       /**< ICA 特征是否已成功提取 */
} battery_soh_ica_state_t;

/**
 * @brief 动态库仑积分与区间容量估算状态
 */
typedef struct {
    double start_soc;            /**< 当前充放电过程起始 SOC (0.0 ~ 1.0) */
    double accumulated_ah;       /**< 当前积分区间累计电荷吞吐量 (Ah) */
    double start_timestamp_s;    /**< 起始时间戳 (s) */
    bool   in_progress;          /**< 是否处于有效连续充放电积分区间 */
    double last_identified_cap_ah;/**< 上次库仑积分辨识出的实际容量 (Ah) */
    double last_coulomb_soh;     /**< 上次库仑积分对应的 SOH 估计值 (0.0 ~ 1.0) */
    bool   last_coulomb_valid;   /**< 库仑积分结果是否有效 */
} battery_soh_coulomb_state_t;

/**
 * @brief SOH 与剩余寿命 RUL 综合评估报告
 */
typedef struct {
    double soh_capacity;         /**< 容量维度综合融合 SOH (0.0 ~ 1.0) */
    double soh_resistance;       /**< 内阻维度 SOH (0.0 ~ 1.0) */
    double actual_capacity_ah;   /**< 当前估算实际可用容量 (Ah) */
    
    /* 各分量估算值 */
    double soh_mechanistic;      /**< 机理模型推导 SOH (0.0 ~ 1.0) */
    double soh_coulomb;          /**< 库仑积分辨识 SOH (0.0 ~ 1.0) */
    double soh_ica;              /**< ICA 峰值特征 SOH (0.0 ~ 1.0) */
    double soh_nn;               /**< 神经网络衰减预测 SOH (0.0 ~ 1.0) */
    
    /* 寿命预测 RUL 指标 */
    double rul_remaining_cycles; /**< 预计剩余等效全充放循环次数 (Full Equivalent Cycles) */
    double rul_remaining_days;   /**< 预计距退役门限剩余天数 (Days to EOL) */
    double degradation_rate_pct_100cyc; /**< 衰减速率 (% 每 100 次等效循环) */
    
    /* 统计累计指标 */
    double total_equivalent_cycles; /**< 累计等效满充满放循环次数 */
    double total_throughput_kwh;    /**< 累计吞吐总电量 (kWh) */
    double total_calendar_days;     /**< 累计日历服役天数 */
    
    battery_soh_grade_t health_grade;/**< 当前健康等级 */
    double confidence_score;        /**< 评估置信度得分 (0.0 ~ 1.0) */
    bool   valid;
} battery_soh_result_t;

/**
 * @brief SOH & RUL 融合估计器结构体
 */
typedef struct {
    battery_soh_config_t config;
    battery_soh_ica_state_t ica;
    battery_soh_coulomb_state_t coulomb;
    
    /* 运行统计累计变量 */
    double total_ah_charge;
    double total_ah_discharge;
    double total_energy_kwh;
    double initial_timestamp_s;
    double last_timestamp_s;
    double avg_operating_temp_c;
    double avg_dod;
    uint64_t total_samples_fed;
    
    /* 贝叶斯融合状态变量与协方差 */
    double state_soh_cap;
    double state_soh_res;
    double cov_soh_cap;
    
    battery_soh_result_t result;
} battery_soh_fusion_estimator_t;

/**
 * @brief 初始化 SOH & RUL 融合估计器
 * @param est 估计器句柄
 * @param cfg 算法配置参数 (若为 NULL 则使用工业级默认参数)
 * @return 0 成功, 负数 错误码
 */
int battery_soh_init(battery_soh_fusion_estimator_t *est, const battery_soh_config_t *cfg);

/**
 * @brief 实时喂入单个采样周期的物理运行数据
 * @param est 估计器句柄
 * @param current_a 采集物理电流 (A, 正放负充)
 * @param voltage_v 采集端电压 (V)
 * @param temp_c 采集电芯温度 (°C)
 * @param soc 当前 SOC 估计值 (0.0 ~ 1.0)
 * @param timestamp_s 绝对时间戳 (s)
 * @return 0 成功, 负数 错误码
 */
int battery_soh_feed_sample(battery_soh_fusion_estimator_t *est,
                            double current_a,
                            double voltage_v,
                            double temp_c,
                            double soc,
                            double timestamp_s);

/**
 * @brief 喂入由 EIS 或卡尔曼滤波在线辨识得到的最新内阻值
 * @param est 估计器句柄
 * @param r0_mohm 欧姆内阻 R0 (mΩ)
 * @param rct_mohm 电荷转移内阻 Rct (mΩ)
 * @param temp_c 测量时电芯温度 (°C)
 * @return 0 成功, 负数 错误码
 */
int battery_soh_feed_impedance(battery_soh_fusion_estimator_t *est,
                               double r0_mohm,
                               double rct_mohm,
                               double temp_c);

/**
 * @brief 执行多源 SOH 融合解算与 RUL 寿命退役趋势预测
 * @param est 估计器句柄
 * @param cur_timestamp_s 当前绝对时间戳 (s)
 * @return 0 成功, 负数 错误码
 */
int battery_soh_process_fusion(battery_soh_fusion_estimator_t *est,
                               double cur_timestamp_s);

/**
 * @brief 获取最新 SOH & RUL 评估诊断报告
 * @param est 估计器句柄
 * @return 诊断报告指针, 若无效返回 NULL
 */
const battery_soh_result_t *battery_soh_get_result(const battery_soh_fusion_estimator_t *est);

/**
 * @brief 重置估计器内部运行状态
 * @param est 估计器句柄
 */
void battery_soh_reset(battery_soh_fusion_estimator_t *est);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_SOH_FUSION_H */
