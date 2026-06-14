/**
 * @file dcu_inventory.c
 * @brief DCU 资产档案管理器 (接入 V3 统一持久化平面)
 */

#include "dcu_inventory.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus_orchestrator.h"
#include "edge_log.h"
#include "sqlite_service.h"

static void _on_dcu_cmd_cleanup(cmd_msg_t* cmd) {
    if (cmd) free(cmd);
}

void dcu_inventory_add_meter(service_t* svc, const char* name, int bus_id, uint32_t slave_id,
                             const char* protocol) {
    (void)bus_id;
    sqlite_svc_t* db = service_get_storage(svc);
    if (!db) return;

    // [补强] 使用统一的 inventory 表 (在 migration v1 中定义)
    // 字段对应：id (TEXT), type (TEXT), addr (INT), protocol (TEXT)
    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT OR REPLACE INTO inventory (id, type, addr, protocol, enabled) "
             "VALUES ('%s', 'METER', %u, '%s', 1);",
             name, slave_id, protocol);

    LOG_INFO(LOG_CAT_SCHED, "DCU: Registering Asset '%s' (Addr: %u, Proto: %s)", name, slave_id,
             protocol);
    sqlite3_exec(sqlite_svc_get_db_handle(db), sql, NULL, NULL, NULL);
}

static int _inventory_load_cb(void* user_data, int argc, char** argv, char** azColName) {
    (void)azColName;
    service_t* svc = (service_t*)user_data;
    arbiter_t* arb = service_get_arbiter(svc);

    if (argc < 4) return 0;

    // 根据统一表结构解析：0:id, 1:type, 2:addr, 3:protocol
    const char* name = argv[0];
    uint32_t slave_id = (uint32_t)atoll(argv[2]);
    const char* proto = argv[3];

    LOG_INFO(LOG_CAT_SCHED, "DCU: Loading Asset [%s] Proto=%s", name, proto);

    // 自动为发现的资产启动轮询 (S11 场景闭环)
    // 假设所有电表都挂在总线 0 上 (实际应从 extra 字段解析 bus_id)
    cmd_msg_t* cmd = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmd, slave_id, 1.0, ARBITER_PRIO_NORMAL);
    cmd->on_cleanup = _on_dcu_cmd_cleanup;

    arbiter_submit_cmd(arb, 0, cmd);
    return 0;
}

void dcu_inventory_load_and_run(service_t* svc) {
    sqlite_svc_t* db = service_get_storage(svc);
    if (!svc || !db) return;

    LOG_INFO(LOG_CAT_SCHED, "DCU: Synchronizing Inventory from persistence plane...");

    // 使用统一资产表进行检索
    const char* sql =
        "SELECT id, type, addr, protocol FROM inventory WHERE type='METER' AND enabled=1;";
    sqlite3_exec(sqlite_svc_get_db_handle(db), sql, _inventory_load_cb, svc, NULL);

    LOG_INFO(LOG_CAT_SCHED, "DCU: Persistence synchronization complete.");
}
