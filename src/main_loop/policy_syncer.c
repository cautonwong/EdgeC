#include "policy_syncer.h"

#include <sqlite3.h>
#include <stdio.h>

#include "edge_log.h"

int policy_syncer_run(sqlite_svc_t *db_svc, bus_orchestrator_t *orch) {
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(db_svc);
    if (!db || !orch) return -1;

    const char *sql =
        "SELECT interval_ms, slave_id, start_addr, reg_count FROM collection_policies;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_CORE, "Failed to prepare policy query: %s", sqlite3_errmsg(db));
        return -1;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int interval = sqlite3_column_int(stmt, 0);
        edge_poll_group_t group = {0};
        group.slave_id = (uint32_t)sqlite3_column_int(stmt, 1);
        group.start_addr = (uint16_t)sqlite3_column_int(stmt, 2);
        group.count = (uint16_t)sqlite3_column_int(stmt, 3);
        group.func_code = 0x03;  // 默认读 Hold Reg

        bus_orchestrator_add_poll(orch, interval, &group);
        count++;
    }

    sqlite3_finalize(stmt);
    LOG_INFO(LOG_CAT_CORE, "Policy Syncer: Loaded %d polling policies from database.", count);
    return 0;
}
