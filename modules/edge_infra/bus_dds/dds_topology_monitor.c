/**
 * @file dds_topology_monitor.c
 * @brief Cyclone DDS Distributed Topology Discovery, Liveliness & Self-Healing Monitor Implementation
 */

#include "dds_topology_monitor.h"
#include "datahub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dds_topology_monitor_init(dds_topology_monitor_t *mon, datahub_t *hub, uint64_t lease_ms) {
    if (!mon) return -1;
    memset(mon, 0, sizeof(dds_topology_monitor_t));
    mon->hub = hub;
    mon->liveliness_lease_ms = (lease_ms > 0) ? lease_ms : DDS_DEFAULT_LIVELINESS_LEASE_MS;
    return 0;
}

int dds_topology_monitor_register_node(dds_topology_monitor_t *mon,
                                       uint32_t node_id,
                                       const char *name,
                                       const char *partition,
                                       uint32_t tag_start,
                                       uint32_t tag_end) {
    if (!mon) return -1;

    // 检查节点是否已存在
    for (uint32_t i = 0; i < mon->node_count; i++) {
        if (mon->nodes[i].node_id == node_id) {
            if (name) strncpy(mon->nodes[i].node_name, name, sizeof(mon->nodes[i].node_name) - 1);
            if (partition) strncpy(mon->nodes[i].partition, partition, sizeof(mon->nodes[i].partition) - 1);
            mon->nodes[i].tag_domain_start = tag_start;
            mon->nodes[i].tag_domain_end = tag_end;
            return 0;
        }
    }

    if (mon->node_count >= DDS_TOPOLOGY_MAX_NODES) return -1;

    dds_node_info_t *node = &mon->nodes[mon->node_count++];
    memset(node, 0, sizeof(dds_node_info_t));
    node->node_id = node_id;
    if (name) strncpy(node->node_name, name, sizeof(node->node_name) - 1);
    if (partition) strncpy(node->partition, partition, sizeof(node->partition) - 1);
    node->tag_domain_start = tag_start;
    node->tag_domain_end = tag_end;
    node->is_online = true;
    mon->online_nodes_count++;

    if (mon->on_node_event) {
        mon->on_node_event(node_id, true, mon->user_data);
    }

    return 0;
}

int dds_topology_monitor_heartbeat(dds_topology_monitor_t *mon, uint32_t node_id, uint64_t now_ms) {
    if (!mon) return -1;

    for (uint32_t i = 0; i < mon->node_count; i++) {
        if (mon->nodes[i].node_id == node_id) {
            mon->nodes[i].last_heartbeat_ms = now_ms;
            mon->nodes[i].heartbeat_count++;
            if (!mon->nodes[i].is_online) {
                mon->nodes[i].is_online = true;
                mon->online_nodes_count++;
                if (mon->on_node_event) {
                    mon->on_node_event(node_id, true, mon->user_data);
                }
            }
            return 0;
        }
    }

    return -1; // 节点未注册
}

int dds_topology_monitor_check_liveliness(dds_topology_monitor_t *mon, uint64_t now_ms) {
    if (!mon) return -1;

    for (uint32_t i = 0; i < mon->node_count; i++) {
        dds_node_info_t *node = &mon->nodes[i];
        if (node->is_online) {
            if (now_ms > node->last_heartbeat_ms && (now_ms - node->last_heartbeat_ms) > mon->liveliness_lease_ms) {
                // 节点失活 (Liveliness Lost)
                node->is_online = false;
                if (mon->online_nodes_count > 0) mon->online_nodes_count--;
                mon->total_liveliness_lost_events++;

                // 自愈治理: 将对应关联点位打上 QUALITY_INVALID 标志
                if (mon->hub && node->tag_domain_start > 0 && node->tag_domain_end >= node->tag_domain_start) {
                    for (uint32_t tag = node->tag_domain_start; tag <= node->tag_domain_end; tag++) {
                        pipe_msg_t invalid_msg = {
                            .tag_id = tag,
                            .ts = now_ms * 1000,
                            .type = PIPEMSG_TYPE_SCALAR,
                            .quality = 0x02, // QUALITY_INVALID
                            .payload.val.f = 0.0
                        };
                        datahub_write_shadow(mon->hub, &invalid_msg);
                    }
                }

                if (mon->on_node_event) {
                    mon->on_node_event(node->node_id, false, mon->user_data);
                }
            }
        }
    }

    return 0;
}

bool dds_topology_monitor_is_node_online(const dds_topology_monitor_t *mon, uint32_t node_id) {
    if (!mon) return false;
    for (uint32_t i = 0; i < mon->node_count; i++) {
        if (mon->nodes[i].node_id == node_id) {
            return mon->nodes[i].is_online;
        }
    }
    return false;
}

void dds_topology_monitor_set_event_cb(dds_topology_monitor_t *mon, dds_node_event_fn cb, void *user_data) {
    if (!mon) return;
    mon->on_node_event = cb;
    mon->user_data = user_data;
}

bool dds_topology_monitor_match_partition(const char *pub_partition, const char *sub_partition) {
    if (!pub_partition || !sub_partition) return true;
    if (pub_partition[0] == '\0' || sub_partition[0] == '\0') return true;
    if (strcmp(pub_partition, "*") == 0 || strcmp(sub_partition, "*") == 0) return true;
    return (strcmp(pub_partition, sub_partition) == 0);
}

void dds_topology_monitor_destroy(dds_topology_monitor_t *mon) {
    if (!mon) return;
    memset(mon, 0, sizeof(dds_topology_monitor_t));
}
