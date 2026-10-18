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

#ifdef __cplusplus
}
#endif

#endif  // IEMS_NODE_H
