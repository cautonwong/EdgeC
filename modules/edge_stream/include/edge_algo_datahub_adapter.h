/**
 * @file edge_algo_datahub_adapter.h
 * @brief 算法层-DataHub 流式防腐适配器 (Anti-Corruption Stream Adapter)
 *
 * 职责：在流引擎平面 (edge_stream) 中作为适配桥梁，将纯数学算法层 (edge_algo)
 * 的抽象输出端口 (edge_algo_sink_t) 桥接发布至 DataHub 影子寄存器。
 * 保持算法层纯粹性与单片机/裸机移植性，彻底杜绝算法层反向依赖 DataHub。
 */

#ifndef EDGE_ALGO_DATAHUB_ADAPTER_H
#define EDGE_ALGO_DATAHUB_ADAPTER_H

#include "datahub.h"
#include "edge_algo_port.h"

#include "ai_awq_gptq_gemm.h"
#include "ai_dynamic_tensor_pool.h"
#include "ai_fast_gemm.h"
#include "ai_graph_pruner.h"
#include "ai_hetero_pipeline.h"
#include "ai_pinn_evaluator.h"
#include "ai_quant_calibrator.h"
#include "ai_slm_kv_cache.h"
#include "ai_temporal_fault_net.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建面向 DataHub 影子寄存器的算法输出接收器
 */
edge_algo_sink_t edge_algo_datahub_sink_create(datahub_t *hub);

/* 适配器辅助同步函数 */
int ai_awq_sync_datahub(const ai_awq_metrics_t *metrics, datahub_t *hub);
int ai_dyn_pool_sync_datahub(const ai_dyn_pool_t *pool, datahub_t *hub);
int ai_fast_gemm_sync_datahub(const ai_fast_gemm_t *gemm, datahub_t *hub);
int ai_graph_pruner_sync_datahub(const ai_graph_pruner_t *pruner, datahub_t *hub);
int ai_hetero_pipeline_sync_datahub(const ai_hetero_pipeline_t *pipe, datahub_t *hub);
int ai_pinn_evaluator_sync_datahub(const ai_pinn_result_t *result, datahub_t *hub);
int ai_quant_sync_datahub(const ai_quant_params_t *params, datahub_t *hub);
int ai_slm_kv_cache_sync_datahub(const ai_slm_kv_cache_t *cache, datahub_t *hub);
int ai_temporal_fault_net_sync_datahub(const ai_fault_result_t *result, datahub_t *hub);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALGO_DATAHUB_ADAPTER_H */
