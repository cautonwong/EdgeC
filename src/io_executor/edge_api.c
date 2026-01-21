#include "edge_api.h"
#include <stdlib.h>

static bus_orchestrator_t *g_orch = NULL;
static driver_manager_t *g_drv_mgr = NULL;

void edge_api_init(bus_orchestrator_t *orch, driver_manager_t *drv_mgr) {
    g_orch = orch;
    g_drv_mgr = drv_mgr;
}

int edge_api_read_tag(int bus_id, const char *protocol, uint32_t physical_tag_id, legacy_on_complete_cb cb) {
    if (!g_orch || !g_drv_mgr) return -1;

    // 1. Create the specific request object
    edge_req_t *req = req_legacy_driver_create(g_drv_mgr, protocol, physical_tag_id, cb);
    if (!req) return -1;

    // 2. Create the command message and attach the request
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->priority = ARBITER_PRIO_NORMAL;

    // 3. Submit to the orchestrator
    return bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
}

int edge_api_raw_io(int bus_id, net_buf_t *tx_buf, raw_io_on_complete_cb cb) {
    if (!g_orch) return -1;

    // 1. Create the specific request object
    edge_req_t *req = req_raw_io_create(tx_buf, cb);
    if (!req) {
        net_buf_unref(tx_buf); // Don't leak the buffer on failure
        return -1;
    }
    
    // 2. Create the command message and attach the request
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->priority = ARBITER_PRIO_NORMAL;

    // 3. Submit to the orchestrator
    return bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
}
