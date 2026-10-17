/**
 * @file edge_algo_datahub_adapter.c
 * @brief 算法层-DataHub 流式防腐适配器 (Anti-Corruption Stream Adapter)
 *
 * 职责：在流引擎平面 (edge_stream) 中作为适配桥梁，将纯数学算法层 (edge_algo)
 * 的抽象输出端口 (edge_algo_sink_t) 桥接发布至 DataHub 影子寄存器。
 * 保持算法层纯粹性与单片机/裸机移植性，彻底杜绝算法层反向依赖 DataHub。
 */

#include "edge_algo_datahub_adapter.h"

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
