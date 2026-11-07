/**
 * @file dcu_pipeline_adapter.h
 * @brief DCU 事件与冻结管线装配（在已创建的流引擎上挂载）
 */

#ifndef DCU_PIPELINE_ADAPTER_H
#define DCU_PIPELINE_ADAPTER_H

#include "dcu_inventory.h"
#include "dcu_node.h"
#include "dcu_storage.h"
#include "datahub.h"
#include "stream_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 挂载 EVENT_PIPELINE 与 FREEZE_PIPELINE。
 */
void dcu_pipelines_attach(stream_engine_t *se, datahub_t *hub, dcu_inventory_t *inv,
                          dcu_node_t *node, dcu_storage_t *storage);

#ifdef __cplusplus
}
#endif

#endif  // DCU_PIPELINE_ADAPTER_H
