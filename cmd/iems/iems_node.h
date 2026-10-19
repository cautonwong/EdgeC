/**
 * @file iems_node.h
 * @brief IEMS Commercial Energy Storage Aggregate Root (No-Contract Architecture)
 */

#ifndef IEMS_NODE_H
#define IEMS_NODE_H

#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

#include "datahub.h"
#include "edge_composition_root.h"
#include "iems_bms_cluster_engine.h"
#include "iems_fire_safety_interlock.h"
#include "iems_microgrid_coordinator.h"
#include "iems_multi_pcs_dispatcher.h"
#include "iems_pcs_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iems_node_s iems_node_t;

typedef struct {
    const char *product_name;          /**< 产品标识，默认 "IEMS" */
    const char *db_path;               /**< 数据库文件路径 */
    const char *shm_name;              /**< 共享内存段标识 */
    uint32_t datahub_capacity;         /**< 影子寄存器容量 */
    double max_pcs_power_kw;           /**< PCS 额定功率 (kW) */
    double transformer_capacity_kva;   /**< 变压器额定容量 (kVA) */
    double demand_limit_kw;            /**< 需量控制上限 (kW) */
} iems_node_config_t;

/**
 * @brief 创建并组装 IEMS 聚合根
 */
iems_node_t *iems_node_create(uv_loop_t *loop, const iems_node_config_t *cfg);

/**
 * @brief 销毁 IEMS 聚合根并释放所有资源
 */
void iems_node_destroy(iems_node_t *node);

/**
 * @brief 启动 IEMS 节点与自律业务引擎
 */
int iems_node_start(iems_node_t *node);

/**
 * @brief 停止 IEMS 节点
 */
int iems_node_stop(iems_node_t *node);

/**
 * @brief 单步推进微网协调与控制决策周期
 */
int iems_node_step(iems_node_t *node);

/* --- 子聚合获取接口 (Domain Getters) --- */
iems_bms_cluster_engine_t *iems_node_get_bms(iems_node_t *node);
iems_pcs_controller_t *iems_node_get_pcs(iems_node_t *node);
iems_microgrid_coordinator_t *iems_node_get_coordinator(iems_node_t *node);
iems_fire_safety_interlock_t *iems_node_get_fire_safety(iems_node_t *node);
iems_multi_pcs_dispatcher_t *iems_node_get_pcs_dispatcher(iems_node_t *node);
datahub_t *iems_node_get_datahub(iems_node_t *node);
edge_composition_container_t *iems_node_get_container(iems_node_t *node);

/* --- 核心业务领域方法 (Domain Operations) --- */

/**
 * @brief 调度充放电功率 (带消防联动闭锁、BMS热失控限功率与变流器多机分摊保护)
 *
 * @param node 聚合根句柄
 * @param target_kw 期望调度功率 (正放负充)
 * @param out_actual_kw 输出实际被约束裁决后的执行功率
 * @return 0 成功执行, <0 故障/闭锁拒绝
 */
int iems_node_dispatch_power(iems_node_t *node, double target_kw, double *out_actual_kw);

/**
 * @brief 紧急停机断开保护
 */
int iems_node_emergency_trip(iems_node_t *node, const char *reason);

/**
 * @brief 更新单簇 BMS 遥测数据并触发系统聚合与不变量校验
 */
int iems_node_update_bms_rack(iems_node_t *node, uint8_t rack_id, double voltage_v,
                              double current_a, double soc_pct, double max_temp_c);

/**
 * @brief IEMS 控制环路延迟预算与看门狗统计 (AEP-407)
 */
typedef struct {
    uint64_t last_cycle_us;       /**< 最近单步决策与下发总耗时 (微秒) */
    uint64_t max_cycle_us;        /**< 历史最大决策耗时 (微秒) */
    uint64_t watchdog_budget_us;  /**< 控制环路安全看门狗预算截止时间 (微秒，默认 10000us 即 10ms) */
    uint32_t watchdog_trips;      /**< 超出预算触发降级/看门狗保护的次数 */
    bool watchdog_degraded;       /**< 当前是否处于超时降级保护状态 */
} iems_loop_watchdog_stats_t;

/**
 * @brief 配置控制环路看门狗预算与安全降级阈值
 */
int iems_node_set_watchdog_budget(iems_node_t *node, uint64_t budget_us);

/**
 * @brief 获取控制环路延迟统计与看门狗状态
 */
int iems_node_get_watchdog_stats(iems_node_t *node, iems_loop_watchdog_stats_t *out_stats);

/**
 * @brief 重置看门狗降级状态，恢复正常调度
 */
int iems_node_reset_watchdog_degrade(iems_node_t *node);

#ifdef __cplusplus
}
#endif

#endif  // IEMS_NODE_H
