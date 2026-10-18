/**
 * @file smartbox_node.h
 * @brief SmartBox Edge Controller Aggregate Root (No-Contract Architecture)
 */

#ifndef SMARTBOX_NODE_H
#define SMARTBOX_NODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "datahub.h"
#include "edge_composition_root.h"
#include "smartbox_five_safety_interlock.h"
#include "smartbox_thermal_aging.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct smartbox_node_s smartbox_node_t;

typedef struct {
    const char *product_name;        /**< 产品标识，默认 "smart_box" */
    const char *shm_name;            /**< 共享内存段标识 */
    uint32_t datahub_capacity;       /**< 影子寄存器容量 */
    double transformer_rated_kva;    /**< 变压器标称容量 (kVA), 默认 630.0 */
} smartbox_node_config_t;

/**
 * @brief 创建并组装 SmartBox 聚合根
 */
smartbox_node_t *smartbox_node_create(uv_loop_t *loop, const smartbox_node_config_t *cfg);

/**
 * @brief 销毁 SmartBox 聚合根并释放所有资源
 */
void smartbox_node_destroy(smartbox_node_t *node);

/**
 * @brief 启动 SmartBox 节点
 */
int smartbox_node_start(smartbox_node_t *node);

/**
 * @brief 停止 SmartBox 节点
 */
int smartbox_node_stop(smartbox_node_t *node);

/* --- 子聚合获取接口 (Domain Getters) --- */
smartbox_thermal_model_t *smartbox_node_get_thermal_model(smartbox_node_t *node);
smartbox_switchgear_bay_t *smartbox_node_get_switchgear_bay(smartbox_node_t *node);
datahub_t *smartbox_node_get_datahub(smartbox_node_t *node);
edge_composition_container_t *smartbox_node_get_container(smartbox_node_t *node);

/* --- 核心业务领域方法 (Domain Operations) --- */

/**
 * @brief 变压器热寿命周期评估 (带高温与寿命衰减检测)
 */
int smartbox_node_evaluate_thermal(smartbox_node_t *node, double ambient_temp_c,
                                  double top_oil_temp_c, double load_kva, double dt_s);

/**
 * @brief 五防闭锁断路器操作校验与执行
 * @param node 聚合根句柄
 * @param close_cmd true=合闸, false=分闸
 * @param out_reason 若闭锁拒绝返回原因
 * @return 0 成功操作, <0 闭锁拦截拒绝
 */
int smartbox_node_operate_breaker(smartbox_node_t *node, bool close_cmd, const char **out_reason);

/**
 * @brief 五防闭锁接地刀闸操作校验与执行
 */
int smartbox_node_operate_earth_switch(smartbox_node_t *node, bool close_cmd, const char **out_reason);

#ifdef __cplusplus
}
#endif

#endif  // SMARTBOX_NODE_H
