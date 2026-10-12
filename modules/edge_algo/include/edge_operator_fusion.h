/**
 * @file edge_operator_fusion.h
 * @brief Industrial Edge Quantized Operator Fusion & Lock-Free Model Hot-Swapping Engine (AEP-604)
 *
 * Implements:
 * 1. SIMD/Cache-friendly fused operator pipeline: GEMM + Bias + Activation (ReLU/GELU) + INT8 Quantize.
 * 2. RCU-style lock-free double-buffered model hot-swapping (0 read lock contention, 0 loss).
 * 3. Model file cryptographic integrity & hash verification.
 * 4. Zero dynamic allocation (Caller-Allocated storage).
 */

#ifndef EDGE_OPERATOR_FUSION_H
#define EDGE_OPERATOR_FUSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AI_MAX_FUSED_TENSORS 16
#define AI_MAX_WEIGHT_SIZE   4096

typedef enum {
    AI_ACT_NONE = 0,
    AI_ACT_RELU = 1,
    AI_ACT_LEAKY_RELU = 2,
    AI_ACT_GELU = 3
} ai_activation_type_t;

/**
 * @brief 融合算子参数配置
 */
typedef struct {
    uint32_t m;                   /* 矩阵 M 维 (行数) */
    uint32_t k;                   /* 矩阵 K 维 (内积维) */
    uint32_t n;                   /* 矩阵 N 维 (列数) */
    ai_activation_type_t act;     /* 融合激活函数类型 */
    float leaky_alpha;            /* LeakyReLU 斜率系数 */
    float output_scale;           /* INT8 量化比例系数 (scale) */
    int32_t output_zero_point;    /* INT8 量化零点 (zero-point) */
} ai_fused_op_config_t;

/**
 * @brief 边缘 AI 模型槽位
 */
typedef struct edge_ai_model_s {
    uint32_t model_id;
    uint32_t version;
    uint8_t weights[AI_MAX_WEIGHT_SIZE];
    size_t weight_size;
    uint32_t checksum;
    atomic_uint_fast32_t ref_count;
    bool is_valid;
} edge_ai_model_t;

/**
 * @brief 无锁双缓冲模型管理器 (Caller-Allocated)
 */
typedef struct {
    edge_ai_model_t model_slots[2]; /* 双缓冲槽位 */
    _Atomic(uint32_t) active_idx;    /* 当前活跃槽位索引 (0 或 1) */
    uint64_t total_inferences;
    uint64_t total_hotswaps;
    uint64_t rejected_tampered_swaps;
} edge_model_manager_t;

/**
 * @brief 初始化无锁双缓冲模型管理器
 */
int edge_model_mgr_init(edge_model_manager_t *mgr, uint32_t initial_model_id,
                        const void *initial_weights, size_t weight_size);

/**
 * @brief 安全获取当前活跃模型引用 (绝对无锁，原子自增 ref_count)
 */
edge_ai_model_t *edge_model_acquire_active(edge_model_manager_t *mgr);

/**
 * @brief 释放模型引用
 */
void edge_model_release(edge_ai_model_t *model);

/**
 * @brief 无锁热插拔更新模型权重 (原子 release 交换，校验完整性)
 */
int edge_model_hotswap(edge_model_manager_t *mgr, uint32_t new_version,
                       const void *new_weights, size_t weight_size,
                       uint32_t expected_checksum);

/**
 * @brief 执行单次融合算子计算: (A * B + Bias) -> Act -> Quantize_INT8
 * @note 全程在 CPU 寄存器与 L1 Cache 完成，杜绝中间内存写回
 */
int edge_fused_gemm_act_quant(const int8_t *matrix_a, const int8_t *matrix_b,
                              const int32_t *bias, const ai_fused_op_config_t *cfg,
                              int8_t *out_quant_matrix);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_OPERATOR_FUSION_H */
