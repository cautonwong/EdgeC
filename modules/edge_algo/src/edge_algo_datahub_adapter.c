/**
 * @file edge_algo_datahub_adapter.c
 * @brief 算法层-DataHub 防腐适配器 (Anti-Corruption Adapter)
 * 职责：作为外部适配层，将算法层的抽象输出端口 (edge_algo_sink_t) 桥接到具体的 DataHub
 * 保持核心算法实现完全纯净，不依赖 DataHub 任何内部结构
 */

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

static int _datahub_sink_publish_scalar(void *user_ctx, uint32_t tag_id, double val, uint64_t ts_ns) {
    if (!user_ctx) return -1;
    datahub_t *hub = (datahub_t *)user_ctx;
    pipe_msg_t msg;
    pipe_msg_init_scalar(&msg, tag_id, val, ts_ns);
    return datahub_write_shadow(hub, &msg);
}

edge_algo_sink_t edge_algo_datahub_sink_create(datahub_t *hub) {
    edge_algo_sink_t sink = { .user_ctx = hub, .publish_scalar = _datahub_sink_publish_scalar };
    return sink;
}

#define ALGO_SYNC_ADAPTER(fn_name, arg_type, emit_fn) \
int fn_name(const arg_type *obj, datahub_t *hub) { \
    if (!obj || !hub) return -1; \
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub); \
    return emit_fn(obj, &sink); \
}

ALGO_SYNC_ADAPTER(ai_awq_sync_datahub, ai_awq_metrics_t, ai_awq_emit_metrics)
ALGO_SYNC_ADAPTER(ai_dyn_pool_sync_datahub, ai_dyn_pool_t, ai_dyn_pool_emit_metrics)
ALGO_SYNC_ADAPTER(ai_fast_gemm_sync_datahub, ai_fast_gemm_t, ai_fast_gemm_emit_metrics)
ALGO_SYNC_ADAPTER(ai_graph_pruner_sync_datahub, ai_graph_pruner_t, ai_graph_pruner_emit_metrics)
ALGO_SYNC_ADAPTER(ai_hetero_pipeline_sync_datahub, ai_hetero_pipeline_t, ai_hetero_pipeline_emit_metrics)
ALGO_SYNC_ADAPTER(ai_pinn_evaluator_sync_datahub, ai_pinn_result_t, ai_pinn_evaluator_emit_results)
ALGO_SYNC_ADAPTER(ai_quant_sync_datahub, ai_quant_params_t, ai_quant_emit_params)
ALGO_SYNC_ADAPTER(ai_slm_kv_cache_sync_datahub, ai_slm_kv_cache_t, ai_slm_kv_cache_emit_metrics)
ALGO_SYNC_ADAPTER(ai_temporal_fault_net_sync_datahub, ai_fault_result_t, ai_temporal_fault_net_emit_results)
