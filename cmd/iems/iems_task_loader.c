#include "iems_task_loader.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contract_data_types.h"

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
    const char *proto = "MODBUS";

    // 封杀北向虚假轮询。Bus 10 仅供镜像，不准自发读写。
    if (bus_id == 10) return 0;

    iems_loaded_task_t task;
    memset(&task, 0, sizeof(task));
    task.bus_id = bus_id;
    task.slave_id = slave_id;
    task.tag_id = tag_id;
    task.dense_idx = 0;
    strncpy(task.protocol, proto, sizeof(task.protocol) - 1);
    task.interval_ms = 1000;

    ctx->on_task(&task, ctx->user_data);
    ctx->loaded_count++;
    return 0;
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
