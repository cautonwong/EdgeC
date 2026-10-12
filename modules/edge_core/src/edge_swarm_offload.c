/**
 * @file edge_swarm_offload.c
 * @brief Multi-Edge Swarm AI Cooperative Scheduling & Stream Task Offloading Engine Implementation
 */

#include "edge_swarm_offload.h"
#include <string.h>

int edge_swarm_engine_init(edge_swarm_engine_t *engine, const char *self_uuid) {
    if (!engine || !self_uuid) return -1;
    memset(engine, 0, sizeof(edge_swarm_engine_t));

    strncpy(engine->self_uuid, self_uuid, sizeof(engine->self_uuid) - 1);

    /* Register self as node 0 */
    edge_swarm_node_status_t self_node = {
        .cpu_load_percent = 10,
        .npu_available_tops = 4,
        .mem_free_mb = 1024,
        .avg_rtt_us = 0,
        .last_heartbeat_ts_ms = 0,
        .is_online = true,
        .is_self = true
    };
    strncpy(self_node.node_uuid, self_uuid, sizeof(self_node.node_uuid) - 1);
    memcpy(&engine->nodes[0], &self_node, sizeof(edge_swarm_node_status_t));
    engine->node_count = 1;
    engine->telemetry.active_node_count = 1;
    return 0;
}

int edge_swarm_update_node(edge_swarm_engine_t *engine, const edge_swarm_node_status_t *status) {
    if (!engine || !status) return -1;

    for (uint32_t i = 0; i < engine->node_count; i++) {
        if (strncmp(engine->nodes[i].node_uuid, status->node_uuid, SWARM_NODE_UUID_LEN) == 0) {
            memcpy(&engine->nodes[i], status, sizeof(edge_swarm_node_status_t));
            return 0;
        }
    }

    if (engine->node_count >= SWARM_MAX_NODES) return -2;

    uint32_t idx = engine->node_count++;
    memcpy(&engine->nodes[idx], status, sizeof(edge_swarm_node_status_t));
    engine->telemetry.active_node_count = engine->node_count;
    return 0;
}

void edge_swarm_set_network_partition(edge_swarm_engine_t *engine, bool is_partitioned) {
    if (engine) {
        engine->is_isolated_partition = is_partitioned;
        if (is_partitioned) {
            engine->telemetry.partition_fallbacks++;
        }
    }
}

int edge_swarm_schedule_task(edge_swarm_engine_t *engine, const edge_swarm_task_t *task,
                             uint64_t current_time_ms, swarm_offload_result_t *out_result) {
    if (!engine || !task || !out_result) return -1;

    engine->telemetry.total_tasks_scheduled++;

    /* 1. Safety critical tasks must run locally */
    if (task->is_safety_critical) {
        out_result->decision = SWARM_ROUTE_LOCAL;
        strncpy(out_result->target_node_uuid, engine->self_uuid, SWARM_NODE_UUID_LEN);
        out_result->estimated_latency_us = 500;
        engine->telemetry.tasks_executed_locally++;
        return 0;
    }

    /* 2. If under network partition, fall back to local rule degradation */
    if (engine->is_isolated_partition) {
        out_result->decision = SWARM_ROUTE_DEGRADED;
        strncpy(out_result->target_node_uuid, engine->self_uuid, SWARM_NODE_UUID_LEN);
        out_result->estimated_latency_us = 200;
        engine->telemetry.tasks_degraded_count++;
        return 0;
    }

    /* 3. Check local node capacity */
    edge_swarm_node_status_t *self = &engine->nodes[0];
    if (self->cpu_load_percent < 75 && self->npu_available_tops >= task->required_tops) {
        out_result->decision = SWARM_ROUTE_LOCAL;
        strncpy(out_result->target_node_uuid, engine->self_uuid, SWARM_NODE_UUID_LEN);
        out_result->estimated_latency_us = 1000;
        engine->telemetry.tasks_executed_locally++;
        return 0;
    }

    /* 4. Local overloaded: search for best neighbor node */
    int best_node_idx = -1;
    uint32_t lowest_cost = 0xFFFFFFFF;

    for (uint32_t i = 1; i < engine->node_count; i++) {
        edge_swarm_node_status_t *peer = &engine->nodes[i];
        if (!peer->is_online) continue;

        /* Check heartbeat freshness (< 5000ms) */
        if (current_time_ms > 0 && peer->last_heartbeat_ts_ms > 0 &&
            (current_time_ms - peer->last_heartbeat_ts_ms) > 5000) {
            continue;
        }

        if (peer->npu_available_tops < task->required_tops) continue;

        /* Cost metric = CPU_load * 10 + RTT_us */
        uint32_t cost = peer->cpu_load_percent * 10 + peer->avg_rtt_us;
        if (cost < lowest_cost && peer->avg_rtt_us * 2 < task->max_allowed_latency_us) {
            lowest_cost = cost;
            best_node_idx = (int)i;
        }
    }

    if (best_node_idx >= 0) {
        out_result->decision = SWARM_ROUTE_OFFLOAD;
        strncpy(out_result->target_node_uuid, engine->nodes[best_node_idx].node_uuid, SWARM_NODE_UUID_LEN);
        out_result->estimated_latency_us = engine->nodes[best_node_idx].avg_rtt_us * 2 + 1000;
        engine->telemetry.tasks_offloaded_success++;
        return 0;
    }

    /* No peer available or RTT exceeds deadline -> degraded local execution */
    out_result->decision = SWARM_ROUTE_DEGRADED;
    strncpy(out_result->target_node_uuid, engine->self_uuid, SWARM_NODE_UUID_LEN);
    out_result->estimated_latency_us = 300;
    engine->telemetry.tasks_degraded_count++;
    return 0;
}

void edge_swarm_get_telemetry(const edge_swarm_engine_t *engine, swarm_telemetry_t *out_telem) {
    if (engine && out_telem) {
        memcpy(out_telem, &engine->telemetry, sizeof(swarm_telemetry_t));
    }
}
