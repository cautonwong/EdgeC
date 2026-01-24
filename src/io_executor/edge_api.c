#include "edge_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver_manager.h"

static bus_orchestrator_t *g_orch = NULL;
static driver_manager_t *g_drv_mgr = NULL;

void edge_api_init(bus_orchestrator_t *orch, driver_manager_t *drv_mgr) {
    g_orch = orch;
    g_drv_mgr = drv_mgr;
}

static void _on_api_cmd_cleanup(cmd_msg_t *cmd) {
    if (cmd->req && cmd->req->ops->teardown) cmd->req->ops->teardown(cmd->req);
    free(cmd);
}

int edge_api_read_tag(int bus_id, const char *protocol, uint32_t slave_id, uint32_t physical_tag_id,
                      legacy_on_complete_cb cb) {
    if (!g_orch) return -1;

    const driver_contract_injection_t *inject = edge_model_find(protocol);
    if (!inject) {
        // 每秒最多打一次，防止刷屏
        static uint64_t last_err = 0;
        if (uv_now(uv_default_loop()) - last_err > 1000) {
            printf("[API-ERROR] No contract for '%s'. Check g_model_registry!\n", protocol);
            last_err = uv_now(uv_default_loop());
        }
        return -1;
    }

    edge_req_t *req = req_legacy_driver_create(g_drv_mgr, protocol, slave_id, CMD_ACTION_READ,
                                               physical_tag_id, 0.0, (void *)inject->lookup_by_tag,
                                               (void *)inject->lookup_by_key, cb);
    if (!req) return -1;

    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->priority = ARBITER_PRIO_NORMAL;
    cmd->signature = 0x55AA;
    cmd->on_cleanup = _on_api_cmd_cleanup;
    snprintf(cmd->req_id, sizeof(cmd->req_id), "TAG_%u", physical_tag_id);

    return bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
}

int edge_api_write_tag(int bus_id, const char *protocol, uint32_t slave_id,
                       uint32_t physical_tag_id, double value, legacy_on_complete_cb cb) {
    if (!g_orch) return -1;

    const driver_contract_injection_t *inject = edge_model_find(protocol);
    if (!inject) {
        return -1;
    }

    edge_req_t *req = req_legacy_driver_create(
        g_drv_mgr, protocol, slave_id, CMD_ACTION_WRITE, physical_tag_id, value,
        (void *)inject->lookup_by_tag, (void *)inject->lookup_by_key, cb);
    if (!req) return -1;

    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->signature = 0x55AA;
    cmd->on_cleanup = _on_api_cmd_cleanup;
    snprintf(cmd->req_id, sizeof(cmd->req_id), "WRITE_%u", physical_tag_id);

    return bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
}