/**
 * @file edge_tensor_pipeline.h
 * @brief Multi-Modal Time-Series Tensor Pipeline & Declarative DAG Engine (AEP-603)
 *
 * Implements:
 * 1. YAML contract schema extension for high-frequency multidimensional tensors (Vibration/Waveform/Acoustic).
 * 2. SIMD 64-byte aligned zero-copy tensor slicing.
 * 3. Declarative DAG execution pipeline with static circular-dependency verification.
 * 4. Zero dynamic allocation (Caller-Allocated storage).
 */

#ifndef EDGE_TENSOR_PIPELINE_H
#define EDGE_TENSOR_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TENSOR_PIPE_MAX_STAGES    8
#define TENSOR_PIPE_MAX_DIMS      3
#define TENSOR_PIPE_NAME_LEN      32
#define TENSOR_SIMD_ALIGNMENT     64

typedef enum {
    TENSOR_STAGE_DSP_WINDOW_HANNING = 1,
    TENSOR_STAGE_FFT_SPECTRUM       = 2,
    TENSOR_STAGE_TINYML_INFERENCE   = 3,
    TENSOR_STAGE_THRESHOLD_ALARM    = 4
} tensor_stage_type_t;

/**
 * @brief 单个算子 DAG 节点
 */
typedef struct {
    uint32_t stage_id;
    tensor_stage_type_t stage_type;
    uint32_t depends_on_stage_id; /* 依赖的前驱 Stage ID (0 表示 Root) */
    uint32_t wcet_limit_us;
} tensor_dag_stage_t;

/**
 * @brief 多模态张量域元数据定义
 */
typedef struct {
    char domain_name[TENSOR_PIPE_NAME_LEN];
    uint32_t domain_id;
    uint32_t sample_rate_hz;
    uint32_t window_size;
    uint32_t stride;
    uint32_t shape[TENSOR_PIPE_MAX_DIMS]; /* e.g. [1, 1024, 3] */
    float quant_scale;
    int32_t quant_zero_point;
    uint32_t stage_count;
    tensor_dag_stage_t stages[TENSOR_PIPE_MAX_STAGES];
} tensor_domain_def_t;

/**
 * @brief 64 字节对齐的张量切片缓冲区
 */
typedef struct __attribute__((aligned(TENSOR_SIMD_ALIGNMENT))) {
    uint8_t buffer[2048];
    size_t length;
    uint64_t timestamp_ns;
} tensor_aligned_slice_t;

/**
 * @brief 张量流水线运行时引擎 (Caller-Allocated)
 */
typedef struct {
    tensor_domain_def_t def;
    tensor_aligned_slice_t slice;
    uint64_t total_frames_processed;
    uint64_t total_alarms_triggered;
    bool is_valid_dag;
} edge_tensor_pipeline_t;

/**
 * @brief 初始化多模态张量流水线并验证 DAG 拓扑 (检测环形死锁)
 * @return 0 on success, -2 on circular dependency detected
 */
int edge_tensor_pipeline_init(edge_tensor_pipeline_t *pipe, const tensor_domain_def_t *def);

/**
 * @brief 喂入原始波形帧并在 SIMD 64 字节对齐缓冲区上执行流水线
 */
int edge_tensor_pipeline_process_frame(edge_tensor_pipeline_t *pipe, const void *raw_samples,
                                       size_t sample_bytes, uint64_t timestamp_ns, bool *out_alarm);

/**
 * @brief 检查 DAG 是否存在环形依赖 (A -> B -> A)
 */
bool edge_tensor_pipeline_has_circular_dependency(const tensor_domain_def_t *def);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_TENSOR_PIPELINE_H */
