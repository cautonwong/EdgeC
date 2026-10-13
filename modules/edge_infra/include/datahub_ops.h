/**
 * @file datahub_ops.h
 * @brief EdgeOS DataHub Subsystem Abstract Interface (Port / VTable)
 * @copyright (c) 2026 EdgeOS Project. All rights reserved.
 * 
 * [AEP-201 / AEP-203] Clean Architecture / Anti-Corruption Layer:
 * Defines pure consumer interfaces for all DataHub distributed / SHM bus operations.
 * Concrete backends (SHM, CycloneDDS, MQTT, Zenoh) implement datahub_ops_t.
 */

#ifndef EDGE_INFRA_DATAHUB_OPS_H
#define EDGE_INFRA_DATAHUB_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct uv_loop_s uv_loop_t;

#include "contract_data_types.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct datahub_s datahub_t;

/**
 * @brief DataHub 后端协议类型
 */
typedef enum {
    DATAHUB_BACKEND_SHM    = 0,
    DATAHUB_BACKEND_DDS    = 1,
    DATAHUB_BACKEND_MQTT   = 2,
    DATAHUB_BACKEND_HYBRID = 3
} datahub_backend_type_t;

/**
 * @brief DataHub 后端初始化配置
 */
typedef struct {
    datahub_backend_type_t backend_type;
    uint32_t dds_domain_id;             /**< Default 0 */
    char dds_network_interface[64];     /**< e.g. "eth0" or "lo" */
    char shm_name[128];                 /**< e.g. "edgeos_shadow" */
    uint32_t shadow_capacity;
    uint32_t ring_slots;
    char mqtt_broker_url[128];          /**< e.g. "mqtt://localhost:1883" */
    char mqtt_topic_prefix[64];
    void *extra_opts;
} datahub_backend_config_t;

typedef datahub_backend_config_t datahub_config_t;

/**
 * @brief 后端性能统计指标
 */
typedef struct {
    uint64_t total_publishes;
    uint64_t total_shadow_writes;
    uint64_t total_shadow_reads;
    uint64_t network_packets_tx;
    uint64_t network_packets_rx;
    uint64_t drops;
} datahub_backend_stats_t;

typedef datahub_backend_stats_t datahub_stats_t;

/**
 * @brief 数据到达通知回调
 */
typedef void (*datahub_on_data_cb)(datahub_t *hub, void *user_data);

/**
 * @brief DataHub 协议/总线操作虚表 (DataHub Operations VTable)
 */
typedef struct datahub_ops_s {
    /**
     * @brief 初始化总线后端
     */
    int (*init)(datahub_t *hub, const datahub_backend_config_t *config);

    /**
     * @brief 向总线发布遥测消息
     */
    int (*publish)(datahub_t *hub, const PipeMsg *msg);

    /**
     * @brief 写入影子内存/本地缓存
     */
    int (*write_shadow)(datahub_t *hub, const PipeMsg *msg);

    /**
     * @brief 从影子内存读取最新快照
     */
    int (*read_shadow)(datahub_t *hub, uint32_t tag_id, PipeMsg *out_msg);

    /**
     * @brief 订阅总线全局数据流
     */
    int (*subscribe)(datahub_t *hub, datahub_on_data_cb cb, void *user_data);

    /**
     * @brief 按测点 ID 查询最新快照
     */
    int (*query_snapshot)(datahub_t *hub, uint32_t tag_id, PipeMsg *out_msg);

    /**
     * @brief 获取后端运行统计
     */
    int (*get_stats)(datahub_t *hub, datahub_backend_stats_t *stats);

    /**
     * @brief 注册特定测点事件监听器 (notes 3)
     */
    int (*register_listener)(datahub_t *hub, uint32_t tag_id, datahub_on_data_cb cb, void *user_data);

    /**
     * @brief 注销特定测点事件监听器 (notes 3)
     */
    int (*unregister_listener)(datahub_t *hub, uint32_t tag_id, datahub_on_data_cb cb);

    /**
     * @brief 测点 TagID 到影子矩阵索引查找 (notes 3)
     */
    int (*shadow_index_lookup)(datahub_t *hub, uint32_t tag_id, uint32_t *out_index);

    /**
     * @brief 获取底层共享内存矩阵基地址 (notes 3 - 仅限快车道 0 拷贝访问)
     */
    void *(*get_shadow_matrix_ptr)(datahub_t *hub);

    /**
     * @brief 销毁总线后端资源
     */
    void (*destroy)(datahub_t *hub);
} datahub_ops_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_INFRA_DATAHUB_OPS_H */
