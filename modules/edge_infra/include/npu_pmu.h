/**
 * @file npu_pmu.h
 * @brief Heterogeneous AI Accelerator Performance Monitoring Unit (PMU) & Hotspot Profiler (AEP-748 / Issue #278)
 *
 * Provides nanosecond-level operator execution profiling, hardware performance counters
 * (cycles, instructions, IPC, L1/L2 cache hit/miss ratio, MAC compute utilization, memory bandwidth),
 * Top-K bottleneck hotspot ranking, and 3-Sigma jitter anomaly detection.
 *
 * Designed with Caller-Allocated Storage (0 heap mallocs) and DataHub telemetry sync.
 */

#ifndef EDGE_NPU_PMU_H
#define EDGE_NPU_PMU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Constants & Types                                                          */
/* -------------------------------------------------------------------------- */

#define NPU_PMU_MAX_OP_NAME_LEN    48
#define NPU_PMU_RING_CAPACITY       64
#define NPU_PMU_MAX_TRACKED_OPS     32

/**
 * @brief AI Operator Classification
 */
typedef enum {
    NPU_OP_UNKNOWN         = 0,
    NPU_OP_CONV2D          = 1,
    NPU_OP_MATMUL_GEMM     = 2,
    NPU_OP_DEPTHWISE_CONV  = 3,
    NPU_OP_SOFTMAX         = 4,
    NPU_OP_LAYERNORM       = 5,
    NPU_OP_ACTIVATION_RELU = 6,
    NPU_OP_ELEMENTWISE     = 7,
    NPU_OP_POOLING         = 8,
    NPU_OP_CUSTOM          = 9
} npu_op_type_t;

/**
 * @brief Single Operator Profiling PMU Record
 */
typedef struct {
    uint32_t op_id;                                  /**< Operator sequence index */
    npu_op_type_t op_type;                           /**< Operator category */
    char op_name[NPU_PMU_MAX_OP_NAME_LEN];           /**< Descriptive layer name */
    uint64_t start_time_ns;                          /**< Start timestamp (CLOCK_MONOTONIC_RAW) */
    uint64_t end_time_ns;                            /**< End timestamp */
    uint64_t latency_ns;                             /**< Execution latency in ns */
    uint64_t cycles;                                 /**< Core clock cycles executed */
    uint64_t instructions;                           /**< Instruction / MAC count */
    float ipc;                                       /**< Instructions per cycle */
    uint64_t l1_hits;                                /**< L1 cache hits */
    uint64_t l1_misses;                              /**< L1 cache misses */
    float l1_miss_pct;                               /**< L1 cache miss percentage */
    uint64_t l2_hits;                                /**< L2 cache hits */
    uint64_t l2_misses;                              /**< L2 cache misses */
    float l2_miss_pct;                               /**< L2 cache miss percentage */
    float mac_util_pct;                              /**< MAC utilization % vs peak capacity */
    float memory_bw_gbps;                            /**< Effective memory bandwidth in GB/s */
    bool is_jitter_anomaly;                          /**< Flagged as 3-Sigma runtime jitter outlier */
} npu_pmu_record_t;

/**
 * @brief Hotspot Ranking Summary for a specific operator type or layer
 */
typedef struct {
    uint32_t op_id;
    npu_op_type_t op_type;
    char op_name[NPU_PMU_MAX_OP_NAME_LEN];
    uint64_t total_latency_ns;
    uint32_t invocation_count;
    float avg_latency_us;
    float time_share_pct;                            /**< Percentage of total inference time */
    float avg_mac_util_pct;
    float avg_l2_miss_pct;
} npu_pmu_hotspot_t;

/**
 * @brief Aggregated Hardware PMU Statistics
 */
typedef struct {
    uint64_t total_inferences;
    uint64_t total_ops_profiled;
    uint64_t total_cycles;
    uint64_t total_latency_ns;
    float avg_inference_latency_us;
    float avg_l1_miss_pct;
    float avg_l2_miss_pct;
    float avg_mac_util_pct;
    float overall_ipc;
    uint32_t jitter_anomaly_count;
} npu_pmu_stats_t;

/**
 * @brief PMU Configuration Descriptor
 */
typedef struct {
    uint32_t peak_macs_per_cycle;                    /**< Theoretical hardware MAC capacity per cycle (e.g. 512 for 1GHz=1TOPS) */
    float core_clock_ghz;                            /**< Operating clock frequency (e.g. 1.0 GHz) */
    float jitter_threshold_sigma;                    /**< Sigma outlier multiplier (default: 3.0) */
    bool enable_ring_buffer;                         /**< Enable circular trace history */
} npu_pmu_config_t;

/* -------------------------------------------------------------------------- */
/* Caller-Allocated Storage                                                   */
/* -------------------------------------------------------------------------- */

#define NPU_PMU_STORAGE_SIZE 16384

typedef struct {
    alignas(max_align_t) uint8_t raw[NPU_PMU_STORAGE_SIZE];
} npu_pmu_storage_t;

typedef struct npu_pmu npu_pmu_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief In-place initialize NPU PMU profiler instance
 */
npu_pmu_t *npu_pmu_create_in(npu_pmu_storage_t *storage, const npu_pmu_config_t *config);

/**
 * @brief Start profiling a single operator invocation
 *
 * @param pmu PMU profiler handle
 * @param op_id Unique operator index
 * @param op_type Operator classification
 * @param op_name Human-readable layer name
 * @param expected_macs Total MAC operations
 * @param memory_bytes Total bytes transferred (weights + activations)
 * @return Token/handle to pass to npu_pmu_op_end
 */
uint64_t npu_pmu_op_begin(npu_pmu_t *pmu, uint32_t op_id, npu_op_type_t op_type,
                          const char *op_name, uint64_t expected_macs, size_t memory_bytes);

/**
 * @brief Complete profiling for the active operator invocation
 *
 * @param pmu PMU profiler handle
 * @param token Token returned by npu_pmu_op_begin
 * @param l1_hits Measured L1 cache hits
 * @param l1_misses Measured L1 cache misses
 * @param l2_hits Measured L2 cache hits
 * @param l2_misses Measured L2 cache misses
 * @param out_record Optional output pointer to receive populated record
 * @return 0 on success, negative error code on failure
 */
int npu_pmu_op_end(npu_pmu_t *pmu, uint64_t token,
                   uint64_t l1_hits, uint64_t l1_misses,
                   uint64_t l2_hits, uint64_t l2_misses,
                   npu_pmu_record_t *out_record);

/**
 * @brief Retrieve aggregated PMU summary metrics
 */
int npu_pmu_get_stats(const npu_pmu_t *pmu, npu_pmu_stats_t *out_stats);

/**
 * @brief Get Top-K execution bottlenecks / hotspots
 *
 * @param pmu PMU profiler handle
 * @param out_hotspots Array of hotspot summaries
 * @param max_count Maximum items to return
 * @return Number of hotspots populated
 */
size_t npu_pmu_get_top_hotspots(const npu_pmu_t *pmu, npu_pmu_hotspot_t *out_hotspots, size_t max_count);

/**
 * @brief Synchronize aggregated PMU metrics with DataHub shadow bus
 */
int npu_pmu_sync_datahub(const npu_pmu_t *pmu, datahub_t *hub);

/**
 * @brief Reset PMU counters and trace history
 */
void npu_pmu_reset(npu_pmu_t *pmu);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_NPU_PMU_H */
