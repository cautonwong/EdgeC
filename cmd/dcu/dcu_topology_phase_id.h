/**
 * @file dcu_topology_phase_id.h
 * @brief HPLC Dual-Mode Carrier Topology & Phase Identification Solver (AEP-461)
 */

#ifndef DCU_TOPOLOGY_PHASE_ID_H
#define DCU_TOPOLOGY_PHASE_ID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dcu_topology_lineloss.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRID_NOMINAL_PERIOD_US 20000.0  // 50Hz grid period = 20,000 us
#define PHASE_A_REF_US 0.0
#define PHASE_B_REF_US (GRID_NOMINAL_PERIOD_US / 3.0)        // 6666.67 us
#define PHASE_C_REF_US (2.0 * GRID_NOMINAL_PERIOD_US / 3.0)  // 13333.33 us

typedef struct {
    char meter_addr[16];
    uint32_t device_id;
    uint32_t branch_box_id;   // 1级分支箱 ID
    uint32_t meter_box_id;    // 2级表箱 ID
    double zero_crossing_us;  // HPLC 测得过零点时延 (us)
    dcu_phase_t identified_phase;
    float confidence;  // 置信度 (0.0 ~ 1.0)
} dcu_hplc_meter_sample_t;

typedef struct {
    uint32_t total_meters;
    uint32_t phase_a_count;
    uint32_t phase_b_count;
    uint32_t phase_c_count;
    uint32_t unknown_count;
    float avg_confidence;
} dcu_phase_id_summary_t;

/**
 * @brief 单表 HPLC 过零点相位辨识算法
 * @param zero_crossing_offset_us 相对台变基准的时差 (us)
 * @param out_confidence 输出置信度
 * @return 辨识出的相别 (PHASE_A, PHASE_B, PHASE_C)
 */
dcu_phase_t dcu_hplc_identify_single_phase(double zero_crossing_offset_us, float *out_confidence);

/**
 * @brief 批量台区表计相位与拓扑树辨识求解
 * @param samples 表计采样数组
 * @param count 表计数量
 * @param out_summary 辨识汇总统计
 * @return 0 成功, 非 0 失败
 */
int dcu_hplc_solve_topology_and_phases(dcu_hplc_meter_sample_t *samples, uint32_t count,
                                       dcu_phase_id_summary_t *out_summary);

#ifdef __cplusplus
}
#endif

#endif  // DCU_TOPOLOGY_PHASE_ID_H
