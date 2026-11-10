/**
 * @file edge_uns_engine.h
 * @brief ISA-95 Unified Namespace (UNS) Engine & Sparkplug B / OPC UA Mirror (AEP-613)
 *
 * Implements:
 * 1. ISA-95 hierarchy path mapping (Enterprise/Site/Area/Line/Cell/Metric).
 * 2. Report-by-Exception (RBE) with configurable deadband and scaling.
 * 3. Zero-allocation Sparkplug B (NBIRTH, NDATA, NDEATH) payload generation.
 * 4. OPC UA node mapping & browse path resolution.
 */

#ifndef EDGE_UNS_ENGINE_H
#define EDGE_UNS_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sparkplug_b.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_UNS_MAX_NODES         64
#define EDGE_UNS_PATH_SEGMENT_LEN  32
#define EDGE_UNS_FULL_PATH_LEN     192

/**
 * @brief ISA-95 统一命名空间层级拓扑定义
 */
typedef struct {
    char enterprise[EDGE_UNS_PATH_SEGMENT_LEN];
    char site[EDGE_UNS_PATH_SEGMENT_LEN];
    char area[EDGE_UNS_PATH_SEGMENT_LEN];
    char line[EDGE_UNS_PATH_SEGMENT_LEN];
    char cell[EDGE_UNS_PATH_SEGMENT_LEN];
    char metric_name[EDGE_UNS_PATH_SEGMENT_LEN];
} edge_uns_isa95_path_t;

/**
 * @brief UNS 点位定义
 */
typedef struct {
    edge_uns_isa95_path_t path;
    uint32_t tag_id;              /* 映射的 DataHub Tag ID / 偏移量 */
    spb_datatype_t datatype;      /* 数据类型 */
    float scale;                  /* 工程量换算系数 (scale factor) */
    float deadband;               /* 变化传输 (RBE) 死区阈值 */
    char unit[16];                /* 物理单位 */
    uint64_t sparkplug_alias;     /* Sparkplug B Metric Alias ID */
    uint32_t opcua_node_id;       /* OPC UA 节点数字 ID */
} edge_uns_point_def_t;

/**
 * @brief UNS 运行时点位状态与 RBE 缓存
 */
typedef struct {
    edge_uns_point_def_t def;
    double last_raw_val;
    double last_published_val;
    uint64_t last_update_ts_ms;
    bool is_initialized;
    bool is_online;
} edge_uns_node_slot_t;

/**
 * @brief UNS 引擎遥测统计指标
 */
typedef struct {
    uint64_t total_evaluations;
    uint64_t rbe_events_triggered;
    uint64_t deadband_filtered_count;
    uint64_t sparkplug_nbirth_count;
    uint64_t sparkplug_ndata_count;
    uint64_t sparkplug_ndeath_count;
} edge_uns_telemetry_t;

/**
 * @brief UNS 引擎主结构 (Caller-Allocated)
 */
typedef struct {
    edge_uns_node_slot_t slots[EDGE_UNS_MAX_NODES];
    uint32_t node_count;
    char group_id[EDGE_UNS_PATH_SEGMENT_LEN];
    char edge_node_id[EDGE_UNS_PATH_SEGMENT_LEN];
    uint64_t birth_timestamp_ms;
    uint64_t seq_number;
    edge_uns_telemetry_t telemetry;
} edge_uns_engine_t;

/**
 * @brief 初始化 UNS 引擎
 */
int edge_uns_engine_init(edge_uns_engine_t *engine, const char *group_id, const char *edge_node_id);

/**
 * @brief 注册 ISA-95 点位映射
 */
int edge_uns_register_point(edge_uns_engine_t *engine, const edge_uns_point_def_t *point_def);

/**
 * @brief 格式化生成点位完整 ISA-95 路径
 */
int edge_uns_format_path(const edge_uns_isa95_path_t *path, char *out_buf, size_t max_len);

/**
 * @brief 按 ISA-95 路径查找点位索引
 */
int edge_uns_find_by_path(const edge_uns_engine_t *engine, const char *full_path);

/**
 * @brief 按 Tag ID 查找点位索引
 */
int edge_uns_find_by_tag(const edge_uns_engine_t *engine, uint32_t tag_id);

/**
 * @brief 点位值评估与 RBE (Report-by-Exception) 死区判定
 * @return 1 if RBE triggered (exceeded deadband), 0 if filtered, <0 on error
 */
int edge_uns_evaluate_point(edge_uns_engine_t *engine, uint32_t tag_id, double new_raw_val,
                            uint64_t timestamp_ms, bool *out_rbe_triggered, double *out_scaled_val);

/**
 * @brief 生成 Sparkplug B NBIRTH 报文载荷 (Protobuf Payload)
 */
int edge_uns_generate_sparkplug_nbirth(edge_uns_engine_t *engine, uint8_t *out_buf, size_t buf_cap, size_t *out_len);

/**
 * @brief 为单个或变化点位生成 Sparkplug B NDATA 报文载荷
 */
int edge_uns_generate_sparkplug_ndata_single(edge_uns_engine_t *engine, uint32_t tag_id,
                                             uint64_t timestamp_ms, uint8_t *out_buf,
                                             size_t buf_cap, size_t *out_len);

/**
 * @brief 生成 Sparkplug B NDEATH 报文载荷
 */
int edge_uns_generate_sparkplug_ndeath(edge_uns_engine_t *engine, uint8_t *out_buf, size_t buf_cap, size_t *out_len);

/**
 * @brief 获取 UNS 引擎遥测统计指标
 */
void edge_uns_get_telemetry(const edge_uns_engine_t *engine, edge_uns_telemetry_t *out_telem);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_UNS_ENGINE_H */
