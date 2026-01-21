#include "edge_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bus_orchestrator_t *g_orch = NULL;
static driver_manager_t *g_drv_mgr = NULL;

void edge_api_init(bus_orchestrator_t *orch, driver_manager_t *drv_mgr) {
    g_orch = orch;
    g_drv_mgr = drv_mgr;
}

static void _on_api_cmd_cleanup(cmd_msg_t *cmd) {
    if (cmd->req && cmd->req->ops->teardown) {
        cmd->req->ops->teardown(cmd->req);
    }
    // 释放 cmd 自身内存
    free(cmd);
}

int edge_api_read_tag(int bus_id, const char *protocol, uint32_t slave_id, uint32_t physical_tag_id,
                      void *lookup_by_tag, void *lookup_by_key, legacy_on_complete_cb cb) {
    if (!g_orch || !g_drv_mgr) return -1;

    edge_req_t *req = req_legacy_driver_create(g_drv_mgr, protocol, slave_id, physical_tag_id,
                                               lookup_by_tag, lookup_by_key, cb);
    if (!req) return -1;

    // [整改] 改回堆内存分配，由 Arbiter 驱动完成后的清理
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->priority = ARBITER_PRIO_NORMAL;
    cmd->signature = 0x55AA;
    cmd->on_cleanup = _on_api_cmd_cleanup;
    snprintf(cmd->req_id, sizeof(cmd->req_id), "P_%u", physical_tag_id);

    printf(" 📥 [API] Submitting %s to Bus %d\n", cmd->req_id, bus_id);
    fflush(stdout);

    return bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
}

int edge_api_raw_io(int bus_id, net_buf_t *tx_buf, raw_io_on_complete_cb cb) {
    if (!g_orch) return -1;

    edge_req_t *req = req_raw_io_create(tx_buf, cb);
    if (!req) return -1;

    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->priority = ARBITER_PRIO_NORMAL;
    cmd->signature = 0x55AA;
    cmd->on_cleanup = _on_api_cmd_cleanup;
    strncpy(cmd->req_id, "RAW", sizeof(cmd->req_id));

    return bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
}