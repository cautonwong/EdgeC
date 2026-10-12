/**
 * @file edge_wasm_nn.h
 * @brief Wasm-NN & Shared Memory Zero-Copy Heterogeneous AI Inference Sandbox (AEP-601)
 *
 * Implements:
 * 1. Zero-copy DataHub shared memory TensorLayoutView mapping (0 heap malloc).
 * 2. Standard Wasm-NN Host ABI execution pipeline.
 * 3. Hard real-time Watchdog & WCET Throttling (default 50ms deadline).
 * 4. Fault containment (segfault/overflow isolation & safe fallback).
 */

#ifndef EDGE_WASM_NN_H
#define EDGE_WASM_NN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WASM_NN_MAX_DIMS          4
#define WASM_NN_MAX_MODELS        4
#define WASM_NN_DEFAULT_WCET_US   50000 /* 50ms maximum execution budget */

typedef enum {
    TENSOR_DTYPE_FLOAT32 = 0,
    TENSOR_DTYPE_INT8    = 1,
    TENSOR_DTYPE_UINT8   = 2,
    TENSOR_DTYPE_INT16   = 3,
    TENSOR_DTYPE_FLOAT16 = 4,
} edge_tensor_dtype_t;

/**
 * @brief 零拷贝物理张量视图 (直接映射 DataHub SHM RingBuffer)
 */
typedef struct {
    uint32_t dimensions[WASM_NN_MAX_DIMS];
    uint8_t dim_count;
    edge_tensor_dtype_t dtype;
    const void *raw_data_ptr; /* 直接指向共享内存，无私有分配 */
    size_t data_bytes;
    uint64_t source_mtt_ts;
} edge_tensor_view_t;

/**
 * @brief Wasm-NN 沙箱遥测指标
 */
typedef struct {
    uint64_t total_inferences;
    uint64_t successful_inferences;
    uint64_t watchdog_timeouts;
    uint64_t faults_intercepted;
    uint64_t last_execution_time_us;
    uint64_t max_execution_time_us;
} wasm_nn_telemetry_t;

/**
 * @brief Wasm-NN 执行沙箱实例 (Caller-Allocated)
 */
typedef struct {
    uint32_t graph_id;
    uint32_t wcet_budget_us;
    bool is_active;
    bool sandbox_fault_triggered;
    wasm_nn_telemetry_t telemetry;
} edge_wasm_nn_sandbox_t;

/**
 * @brief 初始化 Wasm-NN 沙箱
 */
int edge_wasm_nn_sandbox_init(edge_wasm_nn_sandbox_t *sandbox, uint32_t graph_id, uint32_t wcet_budget_us);

/**
 * @brief 构建并校验零拷贝张量视图 (从共享内存物理地址建立)
 */
int edge_wasm_nn_bind_tensor_view(edge_tensor_view_t *view, const void *shm_addr, size_t size,
                                  edge_tensor_dtype_t dtype, const uint32_t *dims, uint8_t dim_count);

/**
 * @brief 在安全沙箱内执行异构张量推理
 * @param sandbox 沙箱实例
 * @param input 输入张量
 * @param out_buffer 输出内存缓冲区
 * @param out_buffer_cap 输出缓冲区容量
 * @param sim_exec_time_us 仿真执行耗时 (用于验证 Watchdog 熔断)
 * @param out_bytes_written 实际写入字节数
 * @return 0 on success, <0 on fault/timeout
 */
int edge_wasm_nn_compute(edge_wasm_nn_sandbox_t *sandbox, const edge_tensor_view_t *input,
                         void *out_buffer, size_t out_buffer_cap,
                         uint32_t sim_exec_time_us, size_t *out_bytes_written);

/**
 * @brief 获取 Wasm-NN 遥测统计
 */
void edge_wasm_nn_get_telemetry(const edge_wasm_nn_sandbox_t *sandbox, wasm_nn_telemetry_t *out_telem);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_WASM_NN_H */
