/**
 * @file dds_topology_monitor.h
 * @brief Cyclone DDS Distributed Topology Discovery, Liveliness & Self-Healing Monitor (AEP-452)
 */

#ifndef DDS_TOPOLOGY_MONITOR_H
#define DDS_TOPOLOGY_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "datahub.h"

#define DDS_TOPOLOGY_MAX_NODES 32
#define DDS_DEFAULT_LIVELINESS_LEASE_MS 2000ULL

typedef void (*dds_node_event_fn)(uint32_t node_id, bool is_online, void *user_data);

typedef struct {
    uint32_t node_id;
    char node_name[64];
    char partition[64];
    bool is_online;
    uint64_t last_heartbeat_ms;
    uint32_t tag_domain_start;
    uint32_t tag_domain_end;
    uint32_t heartbeat_count;
} dds_node_info_t;

typedef struct {
    dds_node_info_t nodes[DDS_TOPOLOGY_MAX_NODES];
    uint32_t node_count;
    uint64_t liveliness_lease_ms;
    datahub_t *hub;
    dds_node_event_fn on_node_event;
    void *user_data;
    uint32_t online_nodes_count;
    uint32_t total_liveliness_lost_events;
} dds_topology_monitor_t;

/**
 * @brief 初始化 DDS 拓扑健康与失活监控器
 */
int dds_topology_monitor_init(dds_topology_monitor_t *mon, datahub_t *hub, uint64_t lease_ms);

/**
 * @brief 注册或发现新分布式节点 (带 Partition 隔离空间与关联点位域)
 */
int dds_topology_monitor_register_node(dds_topology_monitor_t *mon,
                                       uint32_t node_id,
                                       const char *name,
                                       const char *partition,
                                       uint32_t tag_start,
                                       uint32_t tag_end);

/**
 * @brief 节点心跳刷新 (Liveliness Heartbeat)
 */
int dds_topology_monitor_heartbeat(dds_topology_monitor_t *mon, uint32_t node_id, uint64_t now_ms);

/**
 * @brief 周期性检测节点失活租约并执行自愈 (若失联则将对应 DataHub 关联点位打上 QUALITY_INVALID)
 */
int dds_topology_monitor_check_liveliness(dds_topology_monitor_t *mon, uint64_t now_ms);

/**
 * @brief 查询节点在线状态
 */
bool dds_topology_monitor_is_node_online(const dds_topology_monitor_t *mon, uint32_t node_id);

/**
 * @brief 设置节点上下线事件通知回调
 */
void dds_topology_monitor_set_event_cb(dds_topology_monitor_t *mon, dds_node_event_fn cb, void *user_data);

/**
 * @brief 校验两节点间 Partition QoS 是否匹配 (支持通配符 "*" 与精确匹配)
 */
bool dds_topology_monitor_match_partition(const char *pub_partition, const char *sub_partition);

/**
 * @brief 销毁拓扑监控器
 */
void dds_topology_monitor_destroy(dds_topology_monitor_t *mon);

#ifdef __cplusplus
}
#endif

#endif // DDS_TOPOLOGY_MONITOR_H
