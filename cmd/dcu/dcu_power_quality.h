/**
 * @file dcu_power_quality.h
 * @brief DCU Power Quality Analytics & Three-Phase Unbalance Phase-Switching Optimization Engine (AEP-441)
 */

#ifndef DCU_POWER_QUALITY_H
#define DCU_POWER_QUALITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DCU_MAX_HARMONICS 32
#define DCU_MAX_SWITCHABLE_BRANCHES 16

typedef enum {
    PQ_PHASE_A = 0,
    PQ_PHASE_B = 1,
    PQ_PHASE_C = 2,
    PQ_PHASE_UNKNOWN = 3
} pq_phase_t;

typedef enum {
    PQ_EVENT_NONE         = 0,
    PQ_EVENT_VOLTAGE_SAG  = 1, // 电压暂降 (< 0.9 Un)
    PQ_EVENT_VOLTAGE_SWELL= 2, // 电压暂升 (> 1.1 Un)
    PQ_EVENT_INTERRUPTION = 3  // 短时中断 (< 0.1 Un)
} pq_transient_event_type_t;

typedef struct {
    uint32_t meter_id;
    pq_phase_t current_phase;
    double current_amps; // 实时负荷电流 (A)
} dcu_switchable_branch_t;

typedef struct {
    uint32_t meter_id;
    pq_phase_t from_phase;
    pq_phase_t to_phase;
    double migrated_current_amps;
} dcu_phase_switch_action_t;

typedef struct {
    dcu_phase_switch_action_t actions[DCU_MAX_SWITCHABLE_BRANCHES];
    uint8_t action_count;
    double initial_unbalance_pct;
    double projected_unbalance_pct;
    bool is_action_recommended;
} dcu_phase_switch_plan_t;

typedef struct {
    double fundamental_val;                  // 基波有效值 (V 或 A)
    double harmonic_vals[DCU_MAX_HARMONICS]; // 2~31 次谐波分量 (0/1 对应保留与基波)
    double harmonic_pct[DCU_MAX_HARMONICS];  // 2~31 次谐波含有率 (%)
    double thd_total_pct;                   // 总谐波畸变率 THD (%)
    double thd_odd_pct;                     // 奇次谐波含有率 (%)
    double thd_even_pct;                    // 偶次谐波含有率 (%)
} dcu_harmonic_analysis_t;

typedef struct {
    pq_transient_event_type_t event_type;
    uint64_t start_timestamp_ms;
    uint64_t duration_ms;
    double nominal_voltage; // 标称电压 (如 220.0V)
    double extreme_voltage; // 极值电压 (暂降最低点或暂升最高点)
    bool is_active;
} dcu_transient_event_t;

typedef struct {
    double nominal_voltage;        // 标称电压 (如 220.0V)
    double unbalance_threshold_pct;// 不平衡越限阈值 (如 15.0%)
    double ia, ib, ic;             // 三相当前电流 (A)
    double va, vb, vc;             // 三相当前电压 (V)
    double current_unbalance_pct;  // 当前电流不平衡度 (%)
    double voltage_unbalance_pct;  // 当前电压不平衡度 (%)
    dcu_harmonic_analysis_t harmonics_va;
    dcu_transient_event_t transient_event;
} dcu_power_quality_engine_t;

/**
 * @brief 初始化电能质量与三相不平衡分析引擎
 */
void dcu_power_quality_init(dcu_power_quality_engine_t *engine, double nominal_voltage, double unbalance_threshold_pct);

/**
 * @brief 计算三相电流与电压不平衡度 (符合 GB/T 15543 标准)
 */
double dcu_calc_three_phase_unbalance(double a, double b, double c);

/**
 * @brief 求解三相不平衡最优负荷换相迁移组合
 * @param engine 电能质量引擎
 * @param branches 可换相单相支路列表
 * @param branch_count 支路数量
 * @param out_plan 求解产出的换相迁移决策计划
 */
int dcu_solve_phase_switch_plan(dcu_power_quality_engine_t *engine,
                                const dcu_switchable_branch_t *branches,
                                uint8_t branch_count,
                                dcu_phase_switch_plan_t *out_plan);

/**
 * @brief 2~31 次高精度谐波分量与 THD 畸变率分析
 * @param fundamental 基波有效值
 * @param harmonic_components 2~31 次谐波分量有效值数组 (长度 >= 32, 下标 2..31)
 * @param out_analysis 分析结果输出
 */
int dcu_analyze_harmonics_thd(double fundamental,
                              const double *harmonic_components,
                              dcu_harmonic_analysis_t *out_analysis);

/**
 * @brief 电压暂态事件 (暂降/暂升/短时中断) 捕获与状态机判定
 * @param engine 电能质量引擎
 * @param current_voltage 实时瞬时电压有效值 (V)
 * @param timestamp_ms 实时毫秒时间戳
 * @param out_event 产出的暂态事件通知
 */
int dcu_detect_voltage_transient(dcu_power_quality_engine_t *engine,
                                 double current_voltage,
                                 uint64_t timestamp_ms,
                                 dcu_transient_event_t *out_event);

#ifdef __cplusplus
}
#endif

#endif // DCU_POWER_QUALITY_H
