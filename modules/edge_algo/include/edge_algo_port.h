/**
 * @file edge_algo_port.h
 * @brief 边缘算法层消费端输出端口 (Consumer-Defined Algo Sink)
 * 算法作为 Compute Plane，仅依赖自身消费端口输出指标与张量，彻底与 Foundation/Storage (DataHub) 隔离
 */

#ifndef EDGE_ALGO_PORT_H
#define EDGE_ALGO_PORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct edge_algo_sink_s edge_algo_sink_t;

/**
 * @brief 算法消费端数据接收器回调规范
 */
typedef int (*edge_algo_sink_scalar_fn)(void *user_ctx, uint32_t tag_id, double val, uint64_t ts_ns);
typedef int (*edge_algo_sink_tensor_fn)(void *user_ctx, uint32_t tag_id, const double *data, size_t count, uint64_t ts_ns);

struct edge_algo_sink_s {
    void *user_ctx;
    edge_algo_sink_scalar_fn publish_scalar;
    edge_algo_sink_tensor_fn publish_tensor;
};

static inline int edge_algo_sink_emit_scalar(const edge_algo_sink_t *sink, uint32_t tag_id, double val, uint64_t ts_ns) {
    if (sink && sink->publish_scalar) {
        return sink->publish_scalar(sink->user_ctx, tag_id, val, ts_ns);
    }
    return -1;
}

static inline int edge_algo_sink_emit_tensor(const edge_algo_sink_t *sink, uint32_t tag_id, const double *data, size_t count, uint64_t ts_ns) {
    if (sink && sink->publish_tensor) {
        return sink->publish_tensor(sink->user_ctx, tag_id, data, count, ts_ns);
    }
    return -1;
}

#ifdef __cplusplus
}
#endif

#endif /* EDGE_ALGO_PORT_H */
