/**
 * @file edge_tsn_engine.h
 * @brief IEEE 802.1Qbv TAS & IEEE 802.1AS TSN Deterministic Transport Engine (AEP-609)
 *
 * Implements:
 * 1. IEEE 802.1Qbv Time-Aware Shaper (TAS) Gate Control List (GCL) cycle modeling.
 * 2. IEEE 802.1AS PTP nanosecond hardware clock synchronization and drift compensation.
 * 3. Linux SO_TXTIME deterministic packet scheduling & gate window calculation.
 * 4. Priority queue classification (PCP 0~7) with zero drop for critical traffic.
 */

#ifndef EDGE_TSN_ENGINE_H
#define EDGE_TSN_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TSN_MAX_GCL_ENTRIES   16
#define TSN_MAX_PRIORITIES    8
#define TSN_IFNAME_LEN        16

/**
 * @brief TSN GCL (Gate Control List) 门控条目
 */
typedef struct {
    uint32_t gate_mask;        /* 8 个优先级的门控开关掩码 (Bit 0~7, 1=Open, 0=Closed) */
    uint32_t time_interval_ns; /* 门控状态持续时间 (纳秒) */
} tsn_gcl_entry_t;

/**
 * @brief TSN 调度表配置
 */
typedef struct {
    char interface_name[TSN_IFNAME_LEN];
    uint64_t cycle_time_ns;    /* 调度总周期 (纳秒, 如 1000000 = 1ms) */
    uint64_t base_time_ns;     /* GCL 生效绝对基准时间 (纳秒) */
    uint32_t entry_count;      /* GCL 条目数 */
    tsn_gcl_entry_t entries[TSN_MAX_GCL_ENTRIES];
} tsn_schedule_config_t;

/**
 * @brief TSN PTP (IEEE 802.1AS) 时钟同步状态
 */
typedef struct {
    int64_t phc_offset_ns;     /* 当前 PHC 与主时钟的纳秒偏差 */
    int64_t last_offset_ns;
    double freq_drift_ppm;     /* 频率漂移 (PPM) */
    uint64_t last_sync_time_ns;
    bool is_synchronized;
    uint32_t sync_count;
} tsn_ptp_state_t;

/**
 * @brief TSN 优先级遥测统计
 */
typedef struct {
    uint64_t frames_sent;
    uint64_t frames_dropped;
    uint64_t max_jitter_ns;
    uint64_t total_jitter_ns;
    uint64_t window_violations;
} tsn_priority_stats_t;

/**
 * @brief TSN 引擎遥测数据
 */
typedef struct {
    uint64_t total_cycles_executed;
    uint64_t total_scheduled_frames;
    int64_t current_phc_offset_ns;
    tsn_priority_stats_t prio_stats[TSN_MAX_PRIORITIES];
} tsn_telemetry_t;

/**
 * @brief TSN 确定性传输引擎 (Caller-Allocated)
 */
typedef struct {
    tsn_schedule_config_t config;
    tsn_ptp_state_t ptp;
    tsn_telemetry_t telemetry;
    bool is_active;
} edge_tsn_engine_t;

/**
 * @brief 初始化 TSN 引擎
 */
int edge_tsn_engine_init(edge_tsn_engine_t *engine, const tsn_schedule_config_t *config);

/**
 * @brief 同步 IEEE 802.1AS PTP 硬件时钟 (PHC)
 */
int edge_tsn_sync_ptp(edge_tsn_engine_t *engine, uint64_t grandmaster_time_ns, uint64_t local_phc_time_ns);

/**
 * @brief 计算给定优先级在当前时间下的 SO_TXTIME 与门控开放状态
 * @param priority 优先级 0~7 (7 为最高关键度)
 * @param req_send_time_ns 期望发送时间
 * @param out_txtime_ns 计算得出的 SO_TXTIME 发送时隙时间戳
 * @param out_gate_open 当前时间门控是否开启
 * @return 0 on success, <0 on error
 */
int edge_tsn_calculate_txtime(edge_tsn_engine_t *engine, uint8_t priority,
                              uint64_t req_send_time_ns, uint64_t *out_txtime_ns,
                              bool *out_gate_open);

/**
 * @brief 记录发送完成遥测并统计抖动
 */
int edge_tsn_record_tx_telemetry(edge_tsn_engine_t *engine, uint8_t priority,
                                 uint64_t sched_txtime_ns, uint64_t actual_tx_ns);

/**
 * @brief 获取当前活动周期与门控状态
 */
int edge_tsn_get_gate_state(const edge_tsn_engine_t *engine, uint64_t current_time_ns,
                            uint32_t *out_gate_mask, uint32_t *out_entry_idx);

/**
 * @brief 获取 TSN 引擎遥测统计
 */
void edge_tsn_get_telemetry(const edge_tsn_engine_t *engine, tsn_telemetry_t *out_telem);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_TSN_ENGINE_H */
