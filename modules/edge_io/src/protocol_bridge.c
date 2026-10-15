/**
 * @file protocol_bridge.c
 * @brief 自动协议桥接器 (北向控制下发路由)
 */

#include "protocol_bridge.h"
#include <uv.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "edge_log.h"
#include "edge_request.h"

#define MAX_BRIDGE_MAPS 32

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
    cmd_msg_t cmd_slots[MAX_BRIDGE_MAPS];
};

_Static_assert(sizeof(struct protocol_bridge_s) <= sizeof(protocol_bridge_storage_t),
               "protocol_bridge_storage_t is too small for struct protocol_bridge_s");

static protocol_bridge_storage_t g_default_bridge_storage;

static void _bridge_cmd_cleanup(struct cmd_msg *cmd) {
    (void)cmd;
}

static void _on_data_async(uv_async_t *handle) {
    protocol_bridge_t *bridge = (protocol_bridge_t *)handle->data;
    pipe_msg_t msg;

    while (datahub_read_ring(bridge->hub, &bridge->last_seq, &msg) == 0) {
        // [Nexus] 自动控制路由 (Northbound -> Southbound)
        // 生产环境应过滤 SourceID，防止自循环
        if (msg.attr.source_id == 0xFE) continue; 

        for (int i = 0; i < bridge->map_count; i++) {
            if (bridge->maps[i].tag_id == msg.tag_id) {
                LOG_INFO(LOG_CAT_NB, "Bridge: Routing Command Tag %u -> Bus %d Dev %u", 
                         msg.tag_id, bridge->maps[i].target_bus_id, bridge->maps[i].target_dev_id);
                
                cmd_msg_t *cmd = &bridge->cmd_slots[i];
                memset(cmd, 0, sizeof(*cmd));
                cmd->signature = 0xFEFE; // Mark as routed from bridge
                cmd->on_cleanup = _bridge_cmd_cleanup;
                strncpy(cmd->req_id, "BRIDGE_DOWN", sizeof(cmd->req_id) - 1);
                
                bus_orchestrator_submit_cmd(bridge->orch, bridge->maps[i].target_bus_id, cmd);
                break;
            }
        }
    }
}

protocol_bridge_t *protocol_bridge_create_in(protocol_bridge_storage_t *storage, uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch) {
    if (!storage || !loop || !hub || !orch) return NULL;
    memset(storage, 0, sizeof(*storage));
    protocol_bridge_t *bridge = (protocol_bridge_t *)storage;
    bridge->loop = loop;
    bridge->hub = hub;
    bridge->orch = orch;
    
    uv_async_init(loop, &bridge->notify, _on_data_async);
    bridge->notify.data = bridge;
    
    return bridge;
}

protocol_bridge_t *protocol_bridge_create(uv_loop_t *loop, datahub_t *hub, bus_orchestrator_t *orch) {
    return protocol_bridge_create_in(&g_default_bridge_storage, loop, hub, orch);
}

int protocol_bridge_start(protocol_bridge_t *bridge) {
    if (!bridge || bridge->map_count >= MAX_BRIDGE_MAPS) return -1;
    // 注入演示映射：Tag 1001 (北向控制) -> Bus 0, Dev 1 (南向设备)
    bridge->maps[bridge->map_count++] = (bridge_map_t){.tag_id = 1001, .target_bus_id = 0, .target_dev_id = 1};
    
    return datahub_add_listener(bridge->hub, &bridge->notify);
}

static void _bridge_close_cb(uv_handle_t *handle) {
    (void)handle;
}

void protocol_bridge_destroy(protocol_bridge_t *bridge) {
    if (!bridge) return;
    datahub_remove_listener(bridge->hub, &bridge->notify);
    uv_close((uv_handle_t *)&bridge->notify, _bridge_close_cb);
}
