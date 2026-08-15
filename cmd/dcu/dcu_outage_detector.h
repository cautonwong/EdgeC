/**
 * @file dcu_outage_detector.h
 * @brief Production DCU Low-Voltage Outage Detection & Last-Gasp Fault Location Engine
 */

#ifndef DCU_OUTAGE_DETECTOR_H
#define DCU_OUTAGE_DETECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OUTAGE_STATUS_NORMAL         = 0, // 正常供电 (All 3 Phases OK)
    OUTAGE_STATUS_SINGLE_PHASE   = 1, // 单相失压 (e.g. Phase A down)
    OUTAGE_STATUS_TWO_PHASE      = 2, // 两相失压
    OUTAGE_STATUS_FULL_BLACKOUT  = 3  // 全停电 (Last-Gasp triggered on Supercap)
} dcu_outage_status_t;

typedef struct {
    dcu_outage_status_t status;
    uint64_t outage_start_timestamp_ms;
    uint64_t last_restoration_timestamp_ms;
    uint32_t total_outage_events;
    uint32_t supercap_voltage_mv;      // 超级电容电压 (mV, e.g. 5200 mV)
    bool is_last_gasp_sent;            // 最后一公报是否已封包发出
    char affected_branch_mask[32];     // 受影响分支掩码
} dcu_outage_detector_t;

/**
 * @brief 初始化停电检测与最后一公报研判引擎
 */
void dcu_outage_detector_init(dcu_outage_detector_t* detector);

/**
 * @brief 评估三相供电与超级电容状态，研判停电故障
 * @param ua A 相电压 (V)
 * @param ub B 相电压 (V)
 * @param uc C 相电压 (V)
 * @param nominal_v 额定电压 (V, e.g. 220.0V)
 * @param supercap_mv 超级电容电压 (mV)
 * @param now_ms 当前时间戳 (ms)
 */
dcu_outage_status_t dcu_outage_detector_evaluate(dcu_outage_detector_t* detector,
                                                 double ua, double ub, double uc,
                                                 double nominal_v, uint32_t supercap_mv,
                                                 uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif // DCU_OUTAGE_DETECTOR_H
