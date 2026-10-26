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
    if (cmd->user_data) {
        /* 释放由总线提交时分配的引擎指令代理，以防止内存泄漏 */
        free(cmd->user_data);
        cmd->user_data = NULL;
    }
}

int edge_api_read_tag(int bus_id, const char *protocol, uint32_t slave_id, uint32_t physical_tag_id,
                      legacy_on_complete_cb cb, void *user_data) {
    if (!g_orch) {
        static uint64_t last_no_orch = 0;
        if (uv_now(uv_default_loop()) - last_no_orch > 1000) {
            printf("[API-ERROR] reason=no-orchestrator bus=%d proto=%s\n", bus_id,
                   protocol ? protocol : "?");
            last_no_orch = uv_now(uv_default_loop());
        }
        return -1;
    }

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
                                               (void *)inject->lookup_by_key, inject->meta_list,
                                               inject->meta_list_len, cb, user_data);
    if (!req) {
        static uint64_t last_no_req = 0;
        if (uv_now(uv_default_loop()) - last_no_req > 1000) {
            printf("[API-ERROR] reason=req-create-failed bus=%d proto=%s slave=%u tag=%08x "
                   "drv_mgr=%p\n",
                   bus_id, protocol ? protocol : "?", slave_id, physical_tag_id,
                   (void *)g_drv_mgr);
            last_no_req = uv_now(uv_default_loop());
        }
        return -1;
    }

    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->priority = ARBITER_PRIO_NORMAL;
    cmd->signature = 0x55AA;
    cmd->timeout_ms = 2000; // 💡 [Nexus Fix] 修复 0 毫秒立刻导致超时取消的问题
    cmd->on_cleanup = _on_api_cmd_cleanup;
    strncpy(cmd->req_id, "LEGACY_READ", sizeof(cmd->req_id) - 1);

    int rc = bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
    if (rc != 0) {
        static uint64_t last_submit_err = 0;
        if (uv_now(uv_default_loop()) - last_submit_err > 1000) {
            printf("[API-ERROR] reason=submit-failed bus=%d proto=%s tag=%08x rc=%d\n", bus_id,
                   protocol ? protocol : "?", physical_tag_id, rc);
            last_submit_err = uv_now(uv_default_loop());
        }
    }
    return rc;
}

int edge_api_write_tag(int bus_id, const char *protocol, uint32_t slave_id,
                       uint32_t physical_tag_id, double value, legacy_on_complete_cb cb, void *user_data) {
    if (!g_orch) return -1;

    const driver_contract_injection_t *inject = edge_model_find(protocol);
    if (!inject) {
        printf("[API-ERROR] No contract for '%s'. Check g_model_registry!\n", protocol);
        return -1;
    }

    edge_req_t *req = req_legacy_driver_create(g_drv_mgr, protocol, slave_id, CMD_ACTION_WRITE,
                                               physical_tag_id, value, (void *)inject->lookup_by_tag,
                                               (void *)inject->lookup_by_key, inject->meta_list,
                                               inject->meta_list_len, cb, user_data);
    if (!req) return -1;

    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->priority = ARBITER_PRIO_HIGH;
    cmd->signature = 0x55AA;
    cmd->timeout_ms = 2000; // 💡 [Nexus Fix] 修复 0 毫秒立刻导致超时取消的问题
    cmd->on_cleanup = _on_api_cmd_cleanup;
    strncpy(cmd->req_id, "LEGACY_WRITE", sizeof(cmd->req_id) - 1);

    return bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
}

int edge_api_raw_io(int bus_id, net_buf_t *tx_buf, raw_io_on_complete_cb cb) {
    if (!g_orch) {
        if (tx_buf) net_buf_unref(tx_buf);
        return -1;
    }

    edge_req_t *req = req_raw_io_create(tx_buf, cb);
    if (tx_buf) net_buf_unref(tx_buf);  // req_raw_io_create internally refs it
    if (!req) return -1;

    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd->req = req;
    cmd->action = CMD_ACTION_RAW;
    cmd->priority = ARBITER_PRIO_HIGH;  // Raw IO usually needs fast path
    cmd->timeout_ms = ARBITER_DEFAULT_TIMEOUT_MS;
    cmd->signature = 0x55AA;
    cmd->on_cleanup = _on_api_cmd_cleanup;
    strncpy(cmd->req_id, "RAW_IO", sizeof(cmd->req_id) - 1);

    return bus_orchestrator_submit_cmd(g_orch, bus_id, cmd);
}
