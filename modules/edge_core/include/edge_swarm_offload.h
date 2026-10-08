/**
 * @file edge_swarm_offload.h
 * @brief Multi-Edge Swarm AI Cooperative Scheduling & Stream Task Offloading Engine (AEP-606)
 *
 * Implements:
 * 1. Distributed computing topology discovery & dynamic load status registry (DDS/TSN).
 * 2. Deterministic task offloading decision engine (<5ms latency bound).
 * 3. Network partition local fallback state machine (<10ms recovery).
 * 4. Zero dynamic memory allocation (Caller-Allocated storage).
 */

#ifndef EDGE_SWARM_OFFLOAD_H
#define EDGE_SWARM_OFFLOAD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SWARM_MAX_NODES         8
#define SWARM_NODE_UUID_LEN     36
#define SWARM_OFFLOAD_TIMEOUT_US 5000 /* 5ms deadline */

/**
 * @brief 边缘集群节点算力与网络负载状态
 */
typedef struct {
    char node_uuid[SWARM_NODE_UUID_LEN];
    uint32_t cpu_load_percent;   /* CPU 负载百分比 (0~100) */
    uint32_t npu_available_tops; /* 可用 NPU 算力 (TOPS) */
    uint32_t mem_free_mb;        /* 空闲内存 (MB) */
    uint32_t avg_rtt_us;         /* 节点网络往返时延 (微秒) */
    uint64_t last_heartbeat_ts_ms;
    bool is_online;
    bool is_self;
} edge_swarm_node_status_t;

/**
 * @brief 流式 AI 计算任务描述
 */
typedef struct {
    uint32_t task_id;
    uint32_t model_id;
    uint32_t required_tops;
    uint32_t input_size_bytes;
    uint32_t max_allowed_latency_us;
    uint8_t priority;
    bool is_safety_critical;     /* 安全关键任务强制本地执行 */
} edge_swarm_task_t;

/**
 * @brief 任务卸载决策结果
 */
typedef enum {
    SWARM_ROUTE_LOCAL = 0,       /* 本地执行 */
    SWARM_ROUTE_OFFLOAD = 1,     /* 卸载至最优邻居节点 */
    SWARM_ROUTE_DEGRADED = 2     /* 集群过载/分区断网，执行本地规则降级 */
} swarm_route_decision_t;

typedef struct {
    swarm_route_decision_t decision;
    char target_node_uuid[SWARM_NODE_UUID_LEN];
    uint32_t estimated_latency_us;
} swarm_offload_result_t;

/**
 * @brief Swarm 遥测指标
 */
typedef struct {
    uint64_t total_tasks_scheduled;
    uint64_t tasks_executed_locally;
    uint64_t tasks_offloaded_success;
    uint64_t tasks_degraded_count;
    uint64_t partition_fallbacks;
    uint32_t active_node_count;
} swarm_telemetry_t;

/**
 * @brief Swarm 引擎主结构 (Caller-Allocated)
 */
typedef struct {
    char self_uuid[SWARM_NODE_UUID_LEN];
    edge_swarm_node_status_t nodes[SWARM_MAX_NODES];
    uint32_t node_count;
    bool is_isolated_partition;
    swarm_telemetry_t telemetry;
} edge_swarm_engine_t;

/**
 * @brief 初始化 Swarm 协同调度引擎
 */
int edge_swarm_engine_init(edge_swarm_engine_t *engine, const char *self_uuid);

/**
 * @brief 注册或更新集群节点拓扑状态 (由 DDS SPDP/Heartbeat 驱动)
 */
int edge_swarm_update_node(edge_swarm_engine_t *engine, const edge_swarm_node_status_t *status);

/**
 * @brief 任务卸载路由决策求解器 (<5ms 极速求解)
 */
int edge_swarm_schedule_task(edge_swarm_engine_t *engine, const edge_swarm_task_t *task,
                             uint64_t current_time_ms, swarm_offload_result_t *out_result);

/**
 * @brief 注入或解除网络分区故障
 */
void edge_swarm_set_network_partition(edge_swarm_engine_t *engine, bool is_partitioned);

/**
 * @brief 获取 Swarm 引擎遥测数据
 */
void edge_swarm_get_telemetry(const edge_swarm_engine_t *engine, swarm_telemetry_t *out_telem);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_SWARM_OFFLOAD_H */
