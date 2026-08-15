/**
 * @file dcu_data_fusion_engine.h
 * @brief Production SGCC/CSG Substation Multi-Source Data Fusion & Topology Digital Twin Engine
 */

#ifndef DCU_DATA_FUSION_ENGINE_H
#define DCU_DATA_FUSION_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DCU_MAX_FUSION_METERS 2048

typedef struct {
    char meter_addr[16];
    uint32_t device_id;
    double instantaneous_active_kw;  // 瞬时有功
    double instantaneous_voltage_v;  // 瞬时电压
    double voltage_correlation_coeff;// 与台变总表电压相关系数 (0.0 ~ 1.0)
    int8_t hplc_snr_db;              // HPLC 信噪比 (dB)
    int8_t rf_rssi_dbm;              // 微功率无线信号强度 (dBm)
    uint8_t identified_phase;        // 融合辨识相位 (1=A, 2=B, 3=C)
    uint32_t identified_branch_id;   // 融合归属分支箱 ID (1..16)
    bool is_theft_suspect;           // 综合相关性与残差判定的疑似窃电标志
} dcu_meter_fusion_node_t;

typedef struct {
    dcu_meter_fusion_node_t meters[DCU_MAX_FUSION_METERS];
    uint32_t meter_count;
    double master_voltage_v;
    double master_power_kw;
    uint64_t last_fusion_timestamp_ms;
    uint32_t theft_suspect_count;
} dcu_data_fusion_engine_t;

/**
 * @brief 初始化台区多源数据融合引擎
 */
void dcu_fusion_init(dcu_data_fusion_engine_t* engine);

/**
 * @brief 注册或更新从表的多源物理信号 (HPLC + RF + 遥测)
 */
int dcu_fusion_update_meter_sources(dcu_data_fusion_engine_t* engine, const char* addr,
                                    uint32_t dev_id, double u_val, double p_val,
                                    int8_t snr, int8_t rssi);

/**
 * @brief 执行多源数据融合与拓扑数字孪生推理
 * @param master_u 台变母线基准电压
 * @param master_p 台变母线总有功功率
 */
int dcu_fusion_process_analytics(dcu_data_fusion_engine_t* engine, double master_u,
                                 double master_p, uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif // DCU_DATA_FUSION_ENGINE_H
