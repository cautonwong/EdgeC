#include "iems_task_loader.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"
#include "edge_log.h"
#include "iems_driver_adapter.h"

typedef struct {
    iems_task_registered_cb on_task;
    void *user_data;
    int loaded_count;
} loader_ctx_t;

static int _load_tasks_sql_cb(void *user_data, int argc, char **argv, char **cols) {
    loader_ctx_t *ctx = (loader_ctx_t *)user_data;
    (void)cols;
    if (argc < 4 || !ctx || !ctx->on_task) return 0;

    int bus_id = argv[0] ? atoi(argv[0]) : 0;
    uint32_t slave_id = argv[1] ? (uint32_t)atoi(argv[1]) : 1;
    if (slave_id == 0) slave_id = 1;
    uint32_t tag_id = argv[2] ? (uint32_t)atoi(argv[2]) : 0;

    // 封杀北向虚假轮询。Bus 10 仅供镜像，不准自发读写。
    if (bus_id == 10) return 0;

    iems_loaded_task_t task;
    memset(&task, 0, sizeof(task));
    task.bus_id = bus_id;
    task.slave_id = slave_id;
    task.tag_id = tag_id;
    task.dense_idx = 0;
    task.interval_ms = 1000;

    if (iems_driver_protocol_canonical(argv[3], task.protocol) != 0) {
        LOG_WARN(LOG_CAT_CORE, "IEMS task skipped: bus=%d slave=%u tag=%u protocol='%s'", bus_id,
                 slave_id, tag_id, argv[3] ? argv[3] : "(null)");
        return 0;
    }

    ctx->on_task(&task, ctx->user_data);
    ctx->loaded_count++;
    return 0;
}

int iems_poll_task_route(sqlite_svc_t *db_svc, uint32_t tag_id, iems_device_route_t *route) {
    if (!db_svc || !route) return -1;
    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(db_svc);
    if (!db) return -1;

    const char *sql =
        "SELECT t.bus_id, i.addr, i.protocol FROM poll_tasks t INNER JOIN inventory i "
        "ON t.device_id = i.id WHERE t.tag_id = ? AND t.bus_id <> 10;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, (int)tag_id);

    int rc = -1;
    bool have_route = false;
    iems_device_route_t found = {0};
    int step;
    while ((step = sqlite3_step(stmt)) == SQLITE_ROW) {
        char canonical[16];
        const char *raw = (const char *)sqlite3_column_text(stmt, 2);
        if (iems_driver_protocol_canonical(raw, canonical) != 0) {
            LOG_WARN(LOG_CAT_CORE, "IEMS route: tag %u 的协议 '%s' 无法识别，忽略该映射", tag_id,
                     raw ? raw : "(null)");
            continue;
        }
        int bus_id = sqlite3_column_int(stmt, 0);
        uint32_t slave_id = (uint32_t)sqlite3_column_int(stmt, 1);
        if (slave_id == 0) slave_id = 1;

        if (!have_route) {
            found.bus_id = bus_id;
            found.slave_id = slave_id;
            snprintf(found.protocol, sizeof(found.protocol), "%s", canonical);
            have_route = true;
            rc = 0;
        } else if (found.bus_id != bus_id || found.slave_id != slave_id ||
                   strcmp(found.protocol, canonical) != 0) {
            LOG_WARN(LOG_CAT_CORE,
                     "IEMS route: tag %u 映射到多个设备 (bus %d/%d, slave %u/%u)，拒绝控制写",
                     tag_id, found.bus_id, bus_id, found.slave_id, slave_id);
            rc = -2;
            break;
        }
    }
    if (step != SQLITE_ROW && step != SQLITE_DONE) rc = -1;

    if (rc == 0) *route = found;
    sqlite3_finalize(stmt);
    return rc;
}

int iems_poll_tasks_load(sqlite_svc_t *db_svc, iems_task_registered_cb on_task, void *user_data) {
    if (!db_svc || !on_task) return -1;

    sqlite3 *db = (sqlite3 *)sqlite_svc_get_db_handle(db_svc);
    if (!db) return -1;

    loader_ctx_t ctx = {.on_task = on_task, .user_data = user_data, .loaded_count = 0};

    const char *sql =
        "SELECT t.bus_id, i.addr, t.tag_id, i.protocol FROM poll_tasks t INNER JOIN inventory "
        "i ON t.device_id = i.id;";
    char *err = NULL;
    if (sqlite3_exec(db, sql, _load_tasks_sql_cb, &ctx, &err) != SQLITE_OK) {
        if (err) sqlite3_free(err);
        return -1;
    }

    return ctx.loaded_count;
}
