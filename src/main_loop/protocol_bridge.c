/**
 * @file protocol_bridge.c
 * @brief 自动协议桥接器 (北向控制下发路由)
 */

#include "protocol_bridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "edge_log.h"
#include "edge_request.h"

#define MAX_BRIDGE_MAPS 128

typedef struct {
    uint32_t tag_id;
    int target_bus_id;
    uint32_t target_dev_id;
} bridge_map_t;

struct protocol_bridge_s {
    uv_loop_t *loop;
    datahub_t *hub;
    bus_orchestrator_t *orch;
    uv_async_t notify;
    uint64_t last_seq;

    bridge_map_t maps[MAX_BRIDGE_MAPS];
    int map_count;
};

static void _on_data_async(uv_async_t *handle) {
    protocol_bridge_t *bridge = (protocol_bridge_t *)handle->data;
    pipe_msg_t msg;

    while (shm_ring_read(&bridge->hub->ring, &bridge->last_seq, &msg) == 0) {
        // [Nexus] 自动控制路由 (Northbound -> Southbound)
        // 生产环境应过滤 SourceID，防止自循环
        if (msg.attr.source_id == 0xFE) continue; 

        for (int i = 0; i < bridge->map_count; i++) {
            if (bridge->maps[i].tag_id == msg.tag_id) {
                LOG_INFO(LOG_CAT_NB, "Bridge: Routing Command Tag %u -> Bus %d Dev %u", 
                         msg.tag_id, bridge->maps[i].target_bus_id, bridge->maps[i].target_dev_id);
                
                cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
                cmd->signature = 0xFEFE; // Mark as routed from bridge
                strncpy(cmd->req_id, "BRIDGE_DOWN", 31);
                
                if (bus_orchestrator_submit_cmd(bridge->orch, bridge->maps[i].target_bus_id, cmd) != 0) {
                    free(cmd);
                }
                break;
            }
        }
    }
}

protocol_bridge_t *protocol_bridge_create(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch) {
    protocol_bridge_t *bridge = calloc(1, sizeof(protocol_bridge_t));
    bridge->loop = loop;
    bridge->hub = hub;
    bridge->orch = orch;
    
    uv_async_init(loop, &bridge->notify, _on_data_async);
    bridge->notify.data = bridge;
    
    return bridge;
}

int protocol_bridge_start(protocol_bridge_t *bridge) {
    // 注入演示映射：Tag 1001 (北向控制) -> Bus 0, Dev 1 (南向设备)
    bridge->maps[bridge->map_count++] = (bridge_map_t){.tag_id = 1001, .target_bus_id = 0, .target_dev_id = 1};
    
    return datahub_add_listener(bridge->hub, &bridge->notify);
}

void protocol_bridge_destroy(protocol_bridge_t *bridge) {
    if (!bridge) return;
    datahub_remove_listener(bridge->hub, &bridge->notify);
    uv_close((uv_handle_t *)&bridge->notify, (uv_close_cb)free);
}
