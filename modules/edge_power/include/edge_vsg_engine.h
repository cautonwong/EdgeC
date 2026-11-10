/**
 * @file edge_vsg_engine.h
 * @brief Virtual Synchronous Generator (VSG) & Fast Frequency Response (FFR) Engine (AEP-607)
 *
 * Implements:
 * 1. 2nd-order virtual rotor swing equation: J * d(omega)/dt = P_ref/omega_0 - P_e/omega - D * (omega - omega_0).
 * 2. Virtual stator impedance decoupling & voltage droop (P-f, Q-V).
 * 3. Sub-20ms Fast Frequency Response (FFR) for microgrid and weak-grid (SCR < 1.5) support.
 * 4. Zero dynamic allocation (Caller-Allocated storage).
 */

#ifndef EDGE_VSG_ENGINE_H
#define EDGE_VSG_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VSG_OMEGA_0 (2.0f * 3.141592653589793f * 50.0f) /* 50Hz = 314.159 rad/s */

/**
 * @brief VSG 算法配置参数
 */
typedef struct {
    float j_inertia;          /* 虚拟转动惯量 J (kg*m^2) */
    float d_damping;          /* 虚拟阻尼系数 D (N*m*s/rad) */
    float droop_p_f;          /* 有功-频率下垂系数 Dp (kW/Hz) */
    float droop_q_v;          /* 无功-电压下垂系数 Dq (kVar/V) */
    float virtual_r;          /* 虚拟定子电阻 Rv (Ohm) */
    float virtual_x;          /* 虚拟定子电抗 Xv (Ohm) */
    float nominal_voltage_v;  /* 额定线电压有效值 (V, 默认 380V) */
    float nominal_freq_hz;    /* 额定频率 (Hz, 默认 50.0Hz) */
    float max_power_kw;       /* 变流器有功功率上限 (kW) */
    float max_current_a;      /* 变流器相电流上限 (A) */
    bool enable_ffr;          /* 是否使能快速调频 FFR */
} edge_vsg_config_t;

/**
 * @brief VSG 运行时动态物理状态
 */
typedef struct {
    float grid_freq_hz;        /* 电网实测频率 (Hz) */
    float grid_volt_rms;       /* PCC 电网相电压有效值 (V) */
    float omega_virtual;       /* 虚拟转子角速度 (rad/s) */
    float theta_virtual;       /* 虚拟内电势相位角 (rad) */
    float p_active_kw;         /* 输出有功功率 (kW) */
    float q_reactive_kvar;     /* 输出无功功率 (kVar) */
    float ffr_active_power_kw; /* FFR 快速有功补偿 (kW) */
    float id_ref;              /* d 轴电流指令 (A) */
    float iq_ref;              /* q 轴电流指令 (A) */
    uint64_t step_count;
} edge_vsg_state_t;

/**
 * @brief VSG 引擎主结构 (Caller-Allocated)
 */
typedef struct {
    edge_vsg_config_t config;
    edge_vsg_state_t state;
    bool is_initialized;
} edge_vsg_engine_t;

/**
 * @brief 初始化 VSG / FFR 协同控制引擎
 */
int edge_vsg_engine_init(edge_vsg_engine_t *engine, const edge_vsg_config_t *config);

/**
 * @brief 执行单步 VSG 虚拟转子动力学积分与阻抗解耦计算 (500us 周期调用)
 * @param dt_s 积分步长 (秒, 如 0.0005s)
 * @param p_ref_kw 有功功率设定值
 * @param q_ref_kvar 无功功率设定值
 * @param grid_freq_hz 实测电网频率
 * @param grid_volt_rms 实测 PCC 相电压有效值
 * @return 0 on success, <0 on error
 */
int edge_vsg_engine_step(edge_vsg_engine_t *engine, float dt_s, float p_ref_kw,
                         float q_ref_kvar, float grid_freq_hz, float grid_volt_rms);

/**
 * @brief 获取当前 VSG 物理状态快照
 */
int edge_vsg_engine_get_state(const edge_vsg_engine_t *engine, edge_vsg_state_t *out_state);

/**
 * @brief 重置 VSG 虚拟角速度与相位
 */
void edge_vsg_engine_reset(edge_vsg_engine_t *engine, float initial_freq_hz, float initial_voltage_v);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_VSG_ENGINE_H */
