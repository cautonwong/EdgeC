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

/* --- 协议设备地址表登记（取代 driver_contract_injection_t.meta_list） --- */
#define EDGE_API_META_MAX 8

typedef struct {
    char protocol[16];
    const void *meta;
    uint32_t count;
} edge_api_meta_entry_t;

static edge_api_meta_entry_t g_meta[EDGE_API_META_MAX];
static uint32_t g_meta_count;

void edge_api_set_protocol_meta(const char *protocol, const void *meta, uint32_t count) {
    if (!protocol) return;
    for (uint32_t i = 0; i < g_meta_count; i++) {
        if (strcmp(g_meta[i].protocol, protocol) == 0) {
            g_meta[i].meta = meta;
            g_meta[i].count = count;
            return;
        }
    }
    if (g_meta_count >= EDGE_API_META_MAX) return;
    strncpy(g_meta[g_meta_count].protocol, protocol, sizeof(g_meta[0].protocol) - 1);
    g_meta[g_meta_count].meta = meta;
    g_meta[g_meta_count].count = count;
    g_meta_count++;
}

static const edge_api_meta_entry_t *_meta_of(const char *protocol) {
    if (!protocol) return NULL;
    for (uint32_t i = 0; i < g_meta_count; i++) {
        if (strcmp(g_meta[i].protocol, protocol) == 0) return &g_meta[i];
    }
    return NULL;
}

int edge_api_read_tag_ex(int bus_id, const char *protocol, uint32_t slave_id, uint32_t native_id,
                         uint32_t result_tag_id, double scale, const void *point_desc,
                         size_t point_desc_len, legacy_on_complete_cb cb, void *user_data) {
    if (!g_orch) {
        static uint64_t last_no_orch = 0;
        if (uv_now(uv_default_loop()) - last_no_orch > 1000) {
            printf("[API-ERROR] reason=no-orchestrator bus=%d proto=%s\n", bus_id,
                   protocol ? protocol : "?");
            last_no_orch = uv_now(uv_default_loop());
        }
        return -1;
    }

    /* 设备地址表由组合根显式登记；没登记就传空，驱动按自己的默认值处理。 */
    const edge_api_meta_entry_t *meta = _meta_of(protocol);

    edge_req_t *req = req_legacy_driver_create(
        g_drv_mgr, protocol, slave_id, CMD_ACTION_READ, native_id, result_tag_id, scale, 0.0,
        meta ? meta->meta : NULL, meta ? meta->count : 0, point_desc, point_desc_len, cb,
        user_data);
    if (!req) {
        static uint64_t last_no_req = 0;
        if (uv_now(uv_default_loop()) - last_no_req > 1000) {
            printf("[API-ERROR] reason=req-create-failed bus=%d proto=%s slave=%u tag=%08x "
                   "drv_mgr=%p\n",
                   bus_id, protocol ? protocol : "?", slave_id, native_id, (void *)g_drv_mgr);
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
                   protocol ? protocol : "?", native_id, rc);
            last_submit_err = uv_now(uv_default_loop());
        }
    }
    return rc;
}

/* 兼容入口：老调用方只有一个标识，视作"原生标识 == 结果 tag"，量纲为 1。 */
int edge_api_read_tag(int bus_id, const char *protocol, uint32_t slave_id, uint32_t physical_tag_id,
                      legacy_on_complete_cb cb, void *user_data) {
    return edge_api_read_tag_ex(bus_id, protocol, slave_id, physical_tag_id, physical_tag_id, 1.0,
                                NULL, 0, cb, user_data);
}

int edge_api_write_tag(int bus_id, const char *protocol, uint32_t slave_id,
                       uint32_t physical_tag_id, double value, legacy_on_complete_cb cb, void *user_data) {
    if (!g_orch) return -1;

    const edge_api_meta_entry_t *meta = _meta_of(protocol);

    edge_req_t *req = req_legacy_driver_create(
        g_drv_mgr, protocol, slave_id, CMD_ACTION_WRITE, physical_tag_id, physical_tag_id, 1.0,
        value, meta ? meta->meta : NULL, meta ? meta->count : 0, NULL, 0, cb, user_data);
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
