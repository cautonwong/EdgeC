/**
 * @file dcu_vertical_tasks.h
 * @brief Production DCU Vertical Tasks Engine (1s Real-Time Sampling, 15min MTT Periodic Freeze,
 * Daily TOU & Monthly Billing)
 */

#ifndef DCU_VERTICAL_TASKS_H
#define DCU_VERTICAL_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define DCU_MAX_VERTICAL_METERS 2048
#define DCU_MAX_15MIN_CURVE_POINTS 96  // 96 points in 24 hours (15-min interval)

typedef struct {
    double energy_total_kwh;
    double energy_t1_peak_kwh;      // 尖峰费率 (Rate 1 - Peak)
    double energy_t2_shoulder_kwh;  // 高峰费率 (Rate 2 - Shoulder)
    double energy_t3_flat_kwh;      // 平段费率 (Rate 3 - Flat)
    double energy_t4_valley_kwh;    // 低谷费率 (Rate 4 - Valley)
    double reactive_q1_kvarh;
    double reactive_q2_kvarh;
    double reactive_q3_kvarh;
    double reactive_q4_kvarh;
    double max_demand_kw;       // 最大需量 (kW)
    uint64_t max_demand_ts_ms;  // 最大需量发生时间戳
    double max_voltage_v;       // 最高电压
    double min_voltage_v;       // 最低电压
} dcu_tou_energy_record_t;

typedef struct {
    uint64_t timestamp_ms;
    double avg_active_power_kw;      // 15分钟平均有功功率
    double avg_reactive_power_kvar;  // 15分钟平均无功功率
    double avg_power_factor;         // 15分钟平均功率因数
    double freeze_energy_kwh;        // 15分钟整点冻结电量
} dcu_15min_profile_point_t;

typedef struct {
    char meter_addr[16];
    uint32_t device_id;

    // 垂直任务 1: 秒级实时状态 (1s Real-time Ingestion)
    double last_voltage_v;
    double last_current_a;
    double last_active_power_kw;
    double last_frequency_hz;
    uint64_t last_sample_ts_ms;

    // 垂直任务 2: 15分钟负荷曲线 (15-min MTT Periodic Freeze)
    dcu_15min_profile_point_t load_curve[DCU_MAX_15MIN_CURVE_POINTS];
    uint32_t curve_point_count;

    // 垂直任务 3: 日冻结 (Daily Freeze at 00:00:00 with TOU Tariffs)
    dcu_tou_energy_record_t daily_freeze;
    uint64_t last_daily_freeze_ts_ms;

    // 垂直任务 4: 月冻结与账单 (Monthly Billing Snapshot)
    dcu_tou_energy_record_t monthly_freeze;
    uint64_t last_monthly_freeze_ts_ms;

    bool is_active;
} dcu_meter_vertical_state_t;

typedef struct {
    dcu_meter_vertical_state_t meters[DCU_MAX_VERTICAL_METERS];
    uint32_t meter_count;
    uint64_t last_wall_clock_check_ms;
} dcu_vertical_task_mgr_t;

/**
 * @brief 初始化 DCU 垂直任务调度引擎
 */
void dcu_vertical_mgr_init(dcu_vertical_task_mgr_t* mgr);

/**
 * @brief 注册或查找电表垂直数据上下文
 */
dcu_meter_vertical_state_t* dcu_vertical_get_or_create_meter(dcu_vertical_task_mgr_t* mgr,
                                                             const char* addr, uint32_t dev_id);

/**
 * @brief 垂直任务 1: 秒级实时数据采集与流引擎推送
 */
int dcu_vertical_1s_sample(dcu_vertical_task_mgr_t* mgr, uint32_t dev_id, double u_v, double i_a,
                           double p_kw, double freq, uint64_t now_ms);

/**
 * @brief 垂直任务 2: 15分钟 MTT 挂钟对齐负荷曲线生成 (Wall-Clock Aligned at XX:00, XX:15, XX:30,
 * XX:45)
 */
int dcu_vertical_15min_freeze(dcu_vertical_task_mgr_t* mgr, uint64_t freeze_ts_ms);

/**
 * @brief 垂直任务 3: 日冻结与尖峰平谷 (TOU) 阶梯费率归档 (Daily 00:00:00 Freeze)
 */
int dcu_vertical_daily_freeze(dcu_vertical_task_mgr_t* mgr, uint64_t day_start_ts_ms);

/**
 * @brief 垂直任务 4: 月度结算日账单冻结 (Monthly 1st 00:00:00 Freeze)
 */
int dcu_vertical_monthly_freeze(dcu_vertical_task_mgr_t* mgr, uint64_t month_start_ts_ms);

#ifdef __cplusplus
}
#endif

#endif  // DCU_VERTICAL_TASKS_H
