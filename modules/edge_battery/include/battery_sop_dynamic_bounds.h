/**
 * @file battery_sop_dynamic_bounds.h
 * @brief 电池峰值充放电功率 (SOP) 多约束物理边界动态预测求解器 (AEP-740 / Issue #270)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 *
 * 核心功能与物理机理：
 * 1. 电压硬边界约束求解 (端电压极化动态递推，防过充析锂与过放欠压)
 * 2. 荷电状态 (SOC) 边界硬约束预测 (动态剩余充放电电量积分边界)
 * 3. 电流硬件限值与热电耦合降额边界约束 (低温析锂防过充、高温热失控防过热)
 * 4. 多时间尺度 (1s/10s/30s/60s) 峰值瞬态与稳态功率协同解算
 * 5. 瓶颈限制原因 (Limiting Factor) 状态机诊断与模组/Pack 串并联尺度推演
 */

#ifndef EDGE_ALGO_BATTERY_SOP_DYNAMIC_BOUNDS_H
#define EDGE_ALGO_BATTERY_SOP_DYNAMIC_BOUNDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BATTERY_SOP_MAX_HORIZONS 8

/**
 * @brief 电池电化学体系枚举
 */
typedef enum {
    BATTERY_CHEM_LFP = 0,    /**< 磷酸铁锂 (Lithium Iron Phosphate, 3.2V标称) */
    BATTERY_CHEM_NMC,        /**< 三元锂 (Nickel Manganese Cobalt, 3.7V标称) */
    BATTERY_CHEM_SODIUM_ION  /**< 钠离子电池 (Sodium-ion, 3.0V标称) */
} battery_chem_type_t;

/**
 * @brief 功率限制主导瓶颈原因枚举
 */
typedef enum {
    SOP_LIMIT_NONE = 0,             /**< 无限制 (处于额定功率区间) */
    SOP_LIMIT_VOLTAGE_UPPER,        /**< 充电受限于上限截止电压 (V_max 过充保护) */
    SOP_LIMIT_VOLTAGE_LOWER,        /**< 放电受限于下限截止电压 (V_min 过放保护) */
    SOP_LIMIT_SOC_UPPER,            /**< 充电受限于 SOC 上限 (SOC_max 充满截断) */
    SOP_LIMIT_SOC_LOWER,            /**< 放电受限于 SOC 下限 (SOC_min 放空截断) */
    SOP_LIMIT_CURRENT_HARDWARE,     /**< 受限于电芯/继电器额定最大硬件电流 */
    SOP_LIMIT_THERMAL_COLD,         /**< 低温析锂抑制与脉冲加热降额限制 */
    SOP_LIMIT_THERMAL_HOT           /**< 高温热失控风险抑制降额限制 */
} battery_sop_limit_factor_t;

/**
 * @brief 电芯物理模型参数与边界约束规格
 */
typedef struct {
    battery_chem_type_t chem_type;  /**< 电化学体系 */
    double capacity_ah;             /**< 额定电芯容量 (Ah) */
    double v_min;                   /**< 下限截止电压 (V, e.g. LFP 2.50V, NMC 2.80V) */
    double v_max;                   /**< 上限截止电压 (V, e.g. LFP 3.65V, NMC 4.20V) */
    double soc_min;                 /**< 安全可用 SOC 下限 (0.0 ~ 1.0, typ. 0.05) */
    double soc_max;                 /**< 安全可用 SOC 上限 (0.0 ~ 1.0, typ. 0.95) */
    double max_charge_current_a;    /**< 允许最大瞬态充电电流硬件上限 (A) */
    double max_discharge_current_a; /**< 允许最大瞬态放电电流硬件上限 (A) */
    
    /* 等效二阶/一阶 RC 阻抗模型参数 (25°C 基准) */
    double r0_ref_ohm;              /**< 欧姆内阻 R0 (Ohm, typ. 0.0005 ~ 0.005) */
    double rp_ref_ohm;              /**< 极化内阻 Rp (Ohm, typ. 0.001 ~ 0.008) */
    double cp_farad;                /**< 极化电容 Cp (F, typ. 1000 ~ 5000) */
    double temp_coeff_r0;           /**< 欧姆内阻阿伦尼乌斯温度敏感系数 (1/K) */
    double temp_ref_c;              /**< 参考温度 (°C, typ. 25.0) */
    
    /* 充放电库伦效率 */
    double coulombic_eff_charge;    /**< 充电库伦效率 (typ. 0.985 ~ 0.998) */
    double coulombic_eff_discharge; /**< 放电库伦效率 (typ. 0.990 ~ 1.000) */
    
    /* 热电耦合温度降额阈值 */
    double temp_cold_derate_start_c; /**< 低温充电开始降额温度 (°C, typ. 15.0) */
    double temp_cold_stop_charge_c;  /**< 低温禁止充电温度 (°C, typ. 0.0) */
    double temp_hot_derate_start_c;  /**< 高温开始降额温度 (°C, typ. 45.0) */
    double temp_hot_cutoff_c;        /**< 高温切断温度 (°C, typ. 55.0) */
} battery_sop_cell_spec_t;

/**
 * @brief 实时电芯动态状态观测输入
 */
typedef struct {
    double soc;                     /**< 当前荷电状态 SOC (0.0 ~ 1.0) */
    double up_polarization_v;       /**< 当前极化过电位 Up (V, 充电时为正/放电时为负) */
    double cell_temp_c;             /**< 当前电芯表面温度 (°C) */
    double terminal_voltage_meas_v; /**< 当前实测端电压 (V) */
    double current_meas_a;          /**< 当前实测电流 (A, >0 充电, <0 放电) */
} battery_sop_cell_state_t;

/**
 * @brief 单预测时间尺度下的 SOP 求解结果
 */
typedef struct {
    double horizon_s;                           /**< 预测时间窗口 (秒, e.g. 1.0, 10.0, 30.0) */
    double max_charge_current_a;                /**< 允许最大峰值充电电流 (A, >= 0) */
    double max_discharge_current_a;             /**< 允许最大峰值放电电流 (A, >= 0) */
    double max_charge_power_w;                  /**< 允许最大峰值充电功率 (W, >= 0) */
    double max_discharge_power_w;               /**< 允许最大峰值放电功率 (W, >= 0) */
    double pred_terminal_v_charge;              /**< 满峰值充电截止预测端电压 (V) */
    double pred_terminal_v_discharge;           /**< 满峰值放电截止预测端电压 (V) */
    double pred_soc_charge;                     /**< 预测期末充电截止 SOC */
    double pred_soc_discharge;                  /**< 预测期末放电截止 SOC */
    battery_sop_limit_factor_t charge_limit_reason;    /**< 充电功率限制主导原因 */
    battery_sop_limit_factor_t discharge_limit_reason; /**< 放电功率限制主导原因 */
} battery_sop_result_t;

/**
 * @brief 多时间尺度 SOP 综合矩阵结果
 */
typedef struct {
    size_t num_horizons;                                  /**< 求解的时间尺度数量 */
    battery_sop_result_t horizons[BATTERY_SOP_MAX_HORIZONS]; /**< 各尺度求解结果 */
} battery_sop_multi_horizon_result_t;

/**
 * @brief 初始化典型化学体系电芯默认物理参数
 * 
 * @param spec 输出的电芯参数结构体
 * @param chem 电化学体系 (LFP / NMC / 钠电)
 * @param capacity_ah 电芯容量 (Ah)
 * @return true 成功, false 参数无效
 */
bool battery_sop_spec_init_default(battery_sop_cell_spec_t *spec,
                                  battery_chem_type_t chem,
                                  double capacity_ah);

/**
 * @brief 求解单一预测时间窗口下的电芯 SOP 峰值功率与约束边界
 * 
 * 严格按照 GB/T 34131 与 IEEE 1547 标准：
 * 1. 联立端电压极化动态方程与 OCV-SOC 导数关系，计算电压极值约束下的临界电流；
 * 2. 积分计算 SOC 极值（防过充 SOC_max / 防过放 SOC_min）约束下的持续电流；
 * 3. 施加低温析锂抑制与高温过热阶梯降额曲线；
 * 4. 施加硬件瞬态最大允许电流限值；
 * 5. 取多约束最小包络 (Min-Envelope)，判定主导限制因子并计算端电压与极限功率。
 * 
 * @param spec 电芯模型物理参数规格
 * @param state 当前电芯实时观测状态
 * @param horizon_s 预测时间窗口 (秒, 必须 > 0.0)
 * @param out_result 输出的 SOP 求解结果
 * @return true 成功求解, false 输入非法或计算发散
 */
bool battery_sop_solve_single_horizon(const battery_sop_cell_spec_t *spec,
                                     const battery_sop_cell_state_t *state,
                                     double horizon_s,
                                     battery_sop_result_t *out_result);

/**
 * @brief 求解多时间尺度 (如 1s, 10s, 30s, 60s) SOP 矩阵
 * 
 * @param spec 电芯模型物理参数规格
 * @param state 当前电芯实时观测状态
 * @param horizons_s 预测时间尺度数组 (秒)
 * @param num_horizons 时间尺度个数 (1 <= num <= BATTERY_SOP_MAX_HORIZONS)
 * @param out_multi_result 输出的多尺度结果集
 * @return true 全部尺度成功求解, false 失败
 */
bool battery_sop_solve_multi_horizons(const battery_sop_cell_spec_t *spec,
                                     const battery_sop_cell_state_t *state,
                                     const double *horizons_s,
                                     size_t num_horizons,
                                     battery_sop_multi_horizon_result_t *out_multi_result);

/**
 * @brief 将单电芯 SOP 结果缩放至电池簇 / Pack 级 (S串 P并 拓扑)
 * 
 * @param cell_sop 单电芯 SOP 求解结果
 * @param series_count 串联电芯数 (S)
 * @param parallel_count 并联支路数 (P)
 * @param out_pack_sop 输出的 Pack 级 SOP 结果
 * @return true 成功, false 拓扑参数无效
 */
bool battery_sop_scale_to_pack(const battery_sop_result_t *cell_sop,
                               uint32_t series_count,
                               uint32_t parallel_count,
                               battery_sop_result_t *out_pack_sop);

/**
 * @brief 获取限制因子的可读字符串描述
 * 
 * @param factor 限制因子枚举
 * @return 描述字符串
 */
const char *battery_sop_limit_factor_str(battery_sop_limit_factor_t factor);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALGO_BATTERY_SOP_DYNAMIC_BOUNDS_H */
