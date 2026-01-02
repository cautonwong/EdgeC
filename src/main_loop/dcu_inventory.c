/**
 * @file dcu_inventory.c
 * @brief DCU 档案管理器物理实现 (SQLite 持久化版)
 */

#include "dcu_inventory.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus_orchestrator.h"
#include "edge_log.h"
#include "sqlite_service.h"

static void _on_dcu_cmd_cleanup(cmd_msg_t *cmd) {
    if (cmd) free(cmd);
}

void dcu_inventory_add_meter(service_t *svc, const char *name, int bus_id, uint32_t slave_id,
                             const char *proto) {
    sqlite_svc_t *db = service_get_storage(svc);
    if (!db) return;

    // 确证表结构
    const char *schema =
        "CREATE TABLE IF NOT EXISTS meters (name TEXT, bus_id INT, slave_id INT PRIMARY KEY, "
        "protocol TEXT);";
    sqlite3_exec(sqlite_svc_get_db_handle(db), schema, NULL, NULL, NULL);

    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT OR REPLACE INTO meters (name, bus_id, slave_id, protocol) VALUES ('%s', %d, "
             "%u, '%s');",
             name, bus_id, slave_id, proto);

    // 同步写入确证档案安全
    sqlite3_exec(sqlite_svc_get_db_handle(db), sql, NULL, NULL, NULL);
}

static int _meter_load_cb(void *user_data, int argc, char **argv, char **azColName) {
    (void)azColName;
    service_t *svc = (service_t *)user_data;
    arbiter_t *arb = service_get_arbiter(svc);

    if (argc < 4) return 0;

    int bus_id = atoi(argv[1]);
    uint32_t slave_id = (uint32_t)atoll(argv[2]);

    // 为电表分发周期任务
    cmd_msg_t *cmd = calloc(1, sizeof(cmd_msg_t));
    cmd_msg_init_write(cmd, slave_id, 1.0, ARBITER_PRIO_NORMAL);
    cmd->on_cleanup = _on_dcu_cmd_cleanup;

    arbiter_submit_cmd(arb, bus_id, cmd);
    return 0;
}

void dcu_inventory_load_and_run(service_t *svc) {
    sqlite_svc_t *db = service_get_storage(svc);
    if (!svc || !db) return;

    LOG_INFO(LOG_CAT_SCHED, "DCU: Loading inventory from SQLite...");

    // 1. 确证表结构
    const char *schema =
        "CREATE TABLE IF NOT EXISTS meters (name TEXT, bus_id INT, slave_id INT PRIMARY KEY, "
        "protocol TEXT);";
    sqlite3_exec(sqlite_svc_get_db_handle(db), schema, NULL, NULL, NULL);

    // 2. 加载并运行
    sqlite3_exec(sqlite_svc_get_db_handle(db), "SELECT * FROM meters;", _meter_load_cb, svc, NULL);

    LOG_INFO(LOG_CAT_SCHED, "DCU: Inventory loaded and tasks dispatched.");
}