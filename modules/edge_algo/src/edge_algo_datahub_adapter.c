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

static int _datahub_sink_publish_tensor(void *user_ctx, uint32_t tag_id, const double *data, size_t count, uint64_t ts_ns) {
    if (!user_ctx || !data || count == 0) return -1;
    datahub_t *hub = (datahub_t *)user_ctx;
    pipe_msg_t msg;
    pipe_msg_init_batch(&msg, tag_id, data, count, ts_ns);
    return datahub_write_shadow(hub, &msg);
}

edge_algo_sink_t edge_algo_datahub_sink_create(datahub_t *hub) {
    edge_algo_sink_t sink;
    sink.user_ctx = hub;
    sink.publish_scalar = _datahub_sink_publish_scalar;
    sink.publish_tensor = _datahub_sink_publish_tensor;
    return sink;
}

int ai_awq_sync_datahub(const ai_awq_metrics_t *metrics, datahub_t *hub) {
    if (!metrics || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_awq_emit_metrics(metrics, &sink);
}

int ai_dyn_pool_sync_datahub(const ai_dyn_pool_t *pool, datahub_t *hub) {
    if (!pool || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_dyn_pool_emit_metrics(pool, &sink);
}

int ai_fast_gemm_sync_datahub(const ai_fast_gemm_t *gemm, datahub_t *hub) {
    if (!gemm || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_fast_gemm_emit_metrics(gemm, &sink);
}

int ai_graph_pruner_sync_datahub(const ai_graph_pruner_t *pruner, datahub_t *hub) {
    if (!pruner || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_graph_pruner_emit_metrics(pruner, &sink);
}

int ai_hetero_pipeline_sync_datahub(const ai_hetero_pipeline_t *pipe, datahub_t *hub) {
    if (!pipe || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_hetero_pipeline_emit_metrics(pipe, &sink);
}

int ai_pinn_evaluator_sync_datahub(const ai_pinn_result_t *result, datahub_t *hub) {
    if (!result || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_pinn_evaluator_emit_results(result, &sink);
}

int ai_quant_sync_datahub(const ai_quant_params_t *params, datahub_t *hub) {
    if (!params || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_quant_emit_params(params, &sink);
}

int ai_slm_kv_cache_sync_datahub(const ai_slm_kv_cache_t *cache, datahub_t *hub) {
    if (!cache || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_slm_kv_cache_emit_metrics(cache, &sink);
}

int ai_temporal_fault_net_sync_datahub(const ai_fault_result_t *result, datahub_t *hub) {
    if (!result || !hub) return -1;
    edge_algo_sink_t sink = edge_algo_datahub_sink_create(hub);
    return ai_temporal_fault_net_emit_results(result, &sink);
}
