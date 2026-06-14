/**
 * @file dcu_dynamic_poller.c
 * @brief DCU 动态抄表管理器实现 (基于 SQLite 配置)
 */

#include "dcu_dynamic_poller.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"
#include "edge_request.h"

#define MAX_METERS 2048

typedef struct {
    uint32_t id;
    char protocol[16];
    char address[32];
    int interval_sec;
    int channel_id;
    uint64_t last_poll_ts;
} meter_info_t;

struct dcu_dynamic_poller_s {
    uv_loop_t* loop;
    bus_orchestrator_t* orch;
    sqlite_svc_t* db_svc;
    uv_timer_t timer;

    meter_info_t* meters;
    int meter_count;
};

static void _on_poll_timer(uv_timer_t* handle) {
    dcu_dynamic_poller_t* poller = (dcu_dynamic_poller_t*)handle->data;
    uint64_t now = uv_hrtime() / 1000000000;  // Second

    for (int i = 0; i < poller->meter_count; i++) {
        meter_info_t* m = &poller->meters[i];
        if (now - m->last_poll_ts >= (uint64_t)m->interval_sec) {
            // [Aetheris] 创建动态轮询请求
            // 这里根据 protocol 不同，构造不同的 edge_req_t
            // 简化演示：假设所有表都使用 DLT645
            LOG_DEBUG(LOG_CAT_BUS, "DCU: Dynamic Polling Meter %s (Addr: %s) on Channel %d",
                      m->protocol, m->address, m->channel_id);

            // TODO: 调用协议特定的请求构造函数
            // cmd_msg_t *cmd = ...
            // bus_orchestrator_submit_cmd(poller->orch, m->channel_id, cmd);

            m->last_poll_ts = now;
        }
    }
}

int dcu_dynamic_poller_reload(dcu_dynamic_poller_t* poller) {
    sqlite3* db = (sqlite3*)sqlite_svc_get_db_handle(poller->db_svc);
    if (!db) return -1;

    const char* sql =
        "SELECT id, protocol, address, interval_sec, channel_id FROM meters WHERE active = 1";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_BUS, "DCU: Failed to prepare meter query: %s", sqlite3_errmsg(db));
        return -1;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < MAX_METERS) {
        meter_info_t* m = &poller->meters[count++];
        m->id = (uint32_t)sqlite3_column_int(stmt, 0);
        strncpy(m->protocol, (const char*)sqlite3_column_text(stmt, 1), 15);
        strncpy(m->address, (const char*)sqlite3_column_text(stmt, 2), 31);
        m->interval_sec = sqlite3_column_int(stmt, 3);
        m->channel_id = sqlite3_column_int(stmt, 4);
        m->last_poll_ts = 0;
    }
    sqlite3_finalize(stmt);

    poller->meter_count = count;
    LOG_INFO(LOG_CAT_BUS, "DCU: Loaded %d active meters from database.", count);
    return 0;
}

dcu_dynamic_poller_t* dcu_dynamic_poller_create(uv_loop_t* loop, bus_orchestrator_t* orch,
                                                sqlite_svc_t* db) {
    dcu_dynamic_poller_t* poller = calloc(1, sizeof(dcu_dynamic_poller_t));
    poller->loop = loop;
    poller->orch = orch;
    poller->db_svc = db;
    poller->meters = calloc(MAX_METERS, sizeof(meter_info_t));

    uv_timer_init(loop, &poller->timer);
    poller->timer.data = poller;

    return poller;
}

int dcu_dynamic_poller_start(dcu_dynamic_poller_t* poller) {
    if (dcu_dynamic_poller_reload(poller) != 0) return -1;
    return uv_timer_start(&poller->timer, _on_poll_timer, 1000, 1000);
}

void dcu_dynamic_poller_destroy(dcu_dynamic_poller_t* poller) {
    if (!poller) return;
    uv_timer_stop(&poller->timer);
    if (poller->meters) free(poller->meters);
    free(poller);
}
