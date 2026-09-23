/**
 * @file iems_multi_pcs_dispatcher.h
 * @brief Production Commercial Multi-PCS Dynamic Load Sharing & Ramp-Rate Limiting Dispatcher
 */

#ifndef IEMS_MULTI_PCS_DISPATCHER_H
#define IEMS_MULTI_PCS_DISPATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define IEMS_MAX_PCS_UNITS 8

typedef struct {
    uint8_t pcs_id;
    char pcs_sn[24];
    bool is_online;
    bool is_fault;
    double rated_power_kw;    // 额定功率 (e.g. 250.0 kW)
    double current_power_kw;  // 实际运行功率 (kW)
    double target_power_kw;   // 调度下发功率 (kW)
    double temp_c;            // 内部 IGBT 温度 (℃)
    double derating_factor;   // 降额系数 (1.0 = 正常, <1.0 = 过温降额)
} iems_pcs_unit_t;

typedef struct {
    iems_pcs_unit_t units[IEMS_MAX_PCS_UNITS];
    uint8_t unit_count;
    double total_rated_capacity_kw;
    double ramp_rate_limit_kw_per_sec;  // 功率爬坡斜率限制 (如 50kW/s)
    double last_total_dispatched_kw;
} iems_multi_pcs_dispatcher_t;

/**
 * @brief 初始化多 PCS 并联调度器
 */
void iems_pcs_dispatcher_init(iems_multi_pcs_dispatcher_t* disp, double ramp_limit_kw_s);

/**
 * @brief 注册或更新单台 PCS 物理状态
 */
int iems_pcs_dispatcher_register_unit(iems_multi_pcs_dispatcher_t* disp, uint8_t pcs_id,
                                      const char* sn, double rated_kw);

/**
 * @brief 更新单台 PCS 运行反馈 (温度、故障、实际出力)
 */
int iems_pcs_dispatcher_update_status(iems_multi_pcs_dispatcher_t* disp, uint8_t pcs_id,
                                      bool is_online, bool is_fault, double cur_p, double temp_c);

/**
 * @brief 执行总功率需求动态均分与爬坡限制算法
 * @param total_demand_kw 全站总目标功率需求 (正放负充, kW)
 * @param dt_s 距离上一次调度的时步 (秒, e.g. 0.1s)
 */
int iems_pcs_dispatcher_dispatch(iems_multi_pcs_dispatcher_t* disp, double total_demand_kw,
                                 double dt_s);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_MULTI_PCS_DISPATCHER_H
