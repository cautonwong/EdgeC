/**
 * @file dcu_soe_event_engine.h
 * @brief Production SGCC/CSG DL/T 698.45 3xxx Event Alarms & SOE (Sequence of Events) Engine
 */

#ifndef DCU_SOE_EVENT_ENGINE_H
#define DCU_SOE_EVENT_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DCU_MAX_SOE_EVENTS 512

typedef enum {
    OAD_EVT_OVERVOLTAGE = 0x3000,       // 3000: 过压事件 (Overvoltage)
    OAD_EVT_UNDERVOLTAGE = 0x3001,      // 3001: 欠压事件 (Undervoltage)
    OAD_EVT_OVERLOAD = 0x3002,          // 3002: 过负荷/过流 (Overload)
    OAD_EVT_UNBALANCE = 0x3005,         // 3005: 三相不平衡越限 (3-Phase Unbalance)
    OAD_EVT_ZERO_SEQ_CURRENT = 0x3006,  // 3006: 零序电流超限 (Zero-Sequence Current Exceeded)
    OAD_EVT_PHASE_LOSS = 0x3008,        // 3008: 缺相/断相事件 (Phase Loss)
    OAD_EVT_OUTAGE_LASTGASP = 0x3011,  // 3011: 停电事件/最后一公报 (Power Outage Last-Gasp)
    OAD_EVT_VOLTAGE_SAG = 0x3012,      // 3012: 电压暂降 (Voltage Sag)
    OAD_EVT_TEMP_EXCEEDED = 0x3015,    // 3015: 终端/端子超温告警 (Terminal Overtemperature)
    OAD_EVT_THEFT_TAMPER = 0x3017,  // 3017: 计量箱开盖/疑似窃电 (Enclosure Tampering / Anti-theft)
    OAD_EVT_HARMONIC_EXCEED = 0x3020  // 3020: 谐波畸变率超标 (Harmonic THD Exceeded)
} dcu_oad_event_code_t;

typedef enum { EVT_STATE_TRIGGERED = 1, EVT_STATE_RESTORED = 2 } dcu_event_state_t;

typedef struct {
    uint32_t event_id;
    uint16_t oad_event_code;  // DL/T 698.45 OAD (e.g. 0x3011)
    char meter_addr[16];
    uint32_t device_id;
    dcu_event_state_t state;    // 触发 (1) 或 恢复 (2)
    uint64_t soe_timestamp_ms;  // 毫秒级 SOE 时标
    double trigger_value;       // 触发时物理瞬时值 (如 265.5V, 95.0℃)
    double restore_value;       // 恢复时物理瞬时值
    char description[64];
} dcu_soe_event_record_t;

typedef struct {
    dcu_soe_event_record_t records[DCU_MAX_SOE_EVENTS];
    uint32_t head;
    uint32_t total_events_logged;
    uint32_t active_alarm_count;
} dcu_soe_event_engine_t;

/**
 * @brief 初始化国网/南网 SOE 事件引擎
 */
void dcu_soe_init(dcu_soe_event_engine_t* engine);

/**
 * @brief 触发并记录一项毫秒级 SOE 事件
 */
int dcu_soe_record_event(dcu_soe_event_engine_t* engine, uint16_t oad_code, const char* addr,
                         uint32_t dev_id, dcu_event_state_t state, double val,
                         uint64_t timestamp_ms, const char* desc);

/**
 * @brief 获取最近未确认的活动告警总数
 */
uint32_t dcu_soe_get_active_alarms(const dcu_soe_event_engine_t* engine);

#ifdef __cplusplus
}
#endif

#endif  // DCU_SOE_EVENT_ENGINE_H
