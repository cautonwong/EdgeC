/**
 * @file edge_algo_stream_adapter.h
 * @brief 算法层-DataHub 流式防腐适配器 (Anti-Corruption Stream Adapter)
 *
 * 职责：在流引擎平面 (edge_stream) 中作为适配桥梁，将纯数学算法层 (edge_algo)
 * 的抽象输出端口 (edge_algo_sink_t) 桥接发布至 DataHub 影子寄存器。
 * 保持算法层纯粹性与单片机/裸机移植性，彻底杜绝算法层反向依赖 DataHub。
 */

#ifndef EDGE_ALGO_DATAHUB_ADAPTER_H
#define EDGE_ALGO_DATAHUB_ADAPTER_H

#include "stream_bus_port.h"
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
edge_algo_sink_t edge_algo_stream_sink_create(const stream_bus_port_t *bus);

/* 适配器辅助同步函数 */
int ai_awq_sync_stream(const ai_awq_metrics_t *metrics, const stream_bus_port_t *bus);
int ai_dyn_pool_sync_stream(const ai_dyn_pool_t *pool, const stream_bus_port_t *bus);
int ai_fast_gemm_sync_stream(const ai_fast_gemm_t *gemm, const stream_bus_port_t *bus);
int ai_graph_pruner_sync_stream(const ai_graph_pruner_t *pruner, const stream_bus_port_t *bus);
int ai_hetero_pipeline_sync_stream(const ai_hetero_pipeline_t *pipe, const stream_bus_port_t *bus);
int ai_pinn_evaluator_sync_stream(const ai_pinn_result_t *result, const stream_bus_port_t *bus);
int ai_quant_sync_stream(const ai_quant_params_t *params, const stream_bus_port_t *bus);
int ai_slm_kv_cache_sync_stream(const ai_slm_kv_cache_t *cache, const stream_bus_port_t *bus);
int ai_temporal_fault_net_sync_stream(const ai_fault_result_t *result, const stream_bus_port_t *bus);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALGO_DATAHUB_ADAPTER_H */
