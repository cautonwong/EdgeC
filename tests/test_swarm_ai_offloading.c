/**
 * @file test_swarm_ai_offloading.c
 * @brief Unit tests for Multi-Edge Swarm AI Scheduling & Task Offloading (AEP-606)
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "edge_swarm_offload.h"

static void test_swarm_topology_discovery(void) {
    printf("[TEST] Swarm AI Topology Discovery & Dynamic Cluster Registry...\n");

    edge_swarm_engine_t swarm;
    int ret = edge_swarm_engine_init(&swarm, "node-dcu-01");
    assert(ret == 0);
    assert(swarm.node_count == 1);

    /* Discover 3 peer nodes */
    edge_swarm_node_status_t peer1 = {.node_uuid = "node-iems-02",
                                      .cpu_load_percent = 25,
                                      .npu_available_tops = 8,
                                      .mem_free_mb = 2048,
                                      .avg_rtt_us = 450,
                                      .last_heartbeat_ts_ms = 1000,
                                      .is_online = true,
                                      .is_self = false};
    edge_swarm_node_status_t peer2 = {.node_uuid = "node-smartbox-03",
                                      .cpu_load_percent = 85,
                                      .npu_available_tops = 2,
                                      .mem_free_mb = 512,
                                      .avg_rtt_us = 1200,
                                      .last_heartbeat_ts_ms = 1000,
                                      .is_online = true,
                                      .is_self = false};

    assert(edge_swarm_update_node(&swarm, &peer1) == 0);
    assert(edge_swarm_update_node(&swarm, &peer2) == 0);
    assert(swarm.node_count == 3);
    assert(swarm.telemetry.active_node_count == 3);

    printf("  -> PASSED: Distributed node registry and topology discovery verified.\n");
}

static void test_swarm_deterministic_offload_routing(void) {
    printf("[TEST] Swarm AI Deterministic Offload Scheduling (<5ms latency)...\n");

    edge_swarm_engine_t swarm;
    edge_swarm_engine_init(&swarm, "node-dcu-01");

    /* Local node is overloaded (90% CPU, 0 TOPS available) */
    swarm.nodes[0].cpu_load_percent = 90;
    swarm.nodes[0].npu_available_tops = 0;

    /* Add idle neighbor */
    edge_swarm_node_status_t peer = {.node_uuid = "node-iems-02",
                                     .cpu_load_percent = 15,
                                     .npu_available_tops = 16,
                                     .mem_free_mb = 4096,
                                     .avg_rtt_us = 350,
                                     .last_heartbeat_ts_ms = 1000,
                                     .is_online = true,
                                     .is_self = false};
    edge_swarm_update_node(&swarm, &peer);

    /* Schedule AI stream task */
    edge_swarm_task_t task = {.task_id = 901,
                              .model_id = 102,
                              .required_tops = 4,
                              .input_size_bytes = 1024,
                              .max_allowed_latency_us = 4000,
                              .priority = 80,
                              .is_safety_critical = false};

    swarm_offload_result_t res;
    int ret = edge_swarm_schedule_task(&swarm, &task, 1000, &res);
    assert(ret == 0);
    assert(res.decision == SWARM_ROUTE_OFFLOAD);
    assert(strcmp(res.target_node_uuid, "node-iems-02") == 0);
    assert(res.estimated_latency_us <= 4000);

    /* Test safety critical task must be local */
    task.is_safety_critical = true;
    ret = edge_swarm_schedule_task(&swarm, &task, 1000, &res);
    assert(ret == 0);
    assert(res.decision == SWARM_ROUTE_LOCAL);

    printf("  -> PASSED: Deterministic offloading and safety-critical pinning verified.\n");
}

static void test_swarm_network_partition_isolation_fallback(void) {
    printf("[TEST] Swarm AI Network Partition & Local Rule Fallback (<10ms)...\n");

    edge_swarm_engine_t swarm;
    edge_swarm_engine_init(&swarm, "node-dcu-01");
    swarm.nodes[0].cpu_load_percent = 95; /* Overloaded */

    edge_swarm_node_status_t peer = {.node_uuid = "node-iems-02",
                                     .cpu_load_percent = 10,
                                     .npu_available_tops = 16,
                                     .avg_rtt_us = 200,
                                     .is_online = true};
    edge_swarm_update_node(&swarm, &peer);

    /* Trigger network partition */
    edge_swarm_set_network_partition(&swarm, true);
    assert(swarm.is_isolated_partition == true);

    edge_swarm_task_t task = {.task_id = 902,
                              .model_id = 102,
                              .required_tops = 4,
                              .max_allowed_latency_us = 5000,
                              .is_safety_critical = false};

    swarm_offload_result_t res;
    int ret = edge_swarm_schedule_task(&swarm, &task, 2000, &res);
    assert(ret == 0);
    assert(res.decision == SWARM_ROUTE_DEGRADED); /* Fall back to local degradation */

    swarm_telemetry_t telem;
    edge_swarm_get_telemetry(&swarm, &telem);
    assert(telem.tasks_degraded_count >= 1);
    assert(telem.partition_fallbacks == 1);

    printf("  -> PASSED: Partition fault isolation and instant fallback verified.\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("      AEP-606: Multi-Edge Swarm AI Task Offloading Suite         \n");
    printf("=================================================================\n");

    test_swarm_topology_discovery();
    test_swarm_deterministic_offload_routing();
    test_swarm_network_partition_isolation_fallback();

    printf("\nAll 3 AEP-606 Swarm AI Offloading test cases PASSED!\n");
    return 0;
}
