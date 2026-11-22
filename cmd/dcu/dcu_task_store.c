#include "dcu_task_store.h"

#include <sqlite3.h>
#include <string.h>

#include "edge_slave_plane.h"
#include "edge_log.h"

/* 库里的配置子集就是 dcu_task_cfg_t；这是它的 RAM 镜像，用于回滚与重启装载。 */
static dcu_task_cfg_t s_cfg[MAX_TASKS];
static uint8_t s_cfg_count;

/* 回滚快照：SQL 先落，RAM 后改，任一步失败都恢复到调用前。 */
static dcu_task_unit_t s_snapshot[MAX_TASKS];
static uint16_t s_snapshot_count;
static dcu_task_cfg_t s_cfg_snapshot[MAX_TASKS];
static uint8_t s_cfg_snapshot_count;

static int _exec(sqlite3 *db, const char *sql) {
    return sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

static void _rollback(sqlite3 *db) {
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
}

static bool _time_nonzero(const uint8_t t[7]) {
    for (int i = 0; i < 7; i++) {
        if (t[i] != 0) return true;
    }
    return false;
}

static int _seed_default(sqlite3 *db) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT v FROM dcu_task_meta WHERE k='seeded';", -1, &stmt, NULL) !=
        SQLITE_OK) {
        return -1;
    }
    int step = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (step == SQLITE_ROW) return 0; /* 已经播过种；Clear 之后也不重播 */
    if (step != SQLITE_DONE) return -1;

    if (_exec(db, "BEGIN IMMEDIATE TRANSACTION;") != 0) return -1;
    if (_exec(db,
              "INSERT OR IGNORE INTO dcu_task_config (task_id, interval_unit, interval_value, "
              "scheme_type, scheme_id, status, period_type) VALUES (1, 0, 5, 1, 1, 1, 0);") != 0 ||
        _exec(db, "INSERT OR REPLACE INTO dcu_task_meta (k, v) VALUES ('seeded', 1);") != 0) {
        _rollback(db);
        return -1;
    }
    if (_exec(db, "COMMIT;") != 0) {
        _rollback(db);
        return -1;
    }
    return 0;
}

int dcu_task_store_init(dcu_storage_t *storage) {
    if (!storage || !storage->db) return -1;
    const char *schema =
        "CREATE TABLE IF NOT EXISTS dcu_task_config ("
        " task_id INTEGER PRIMARY KEY, interval_unit INTEGER NOT NULL,"
        " interval_value INTEGER NOT NULL, scheme_type INTEGER NOT NULL,"
        " scheme_id INTEGER NOT NULL, status INTEGER NOT NULL,"
        " period_type INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS dcu_task_meta (k TEXT PRIMARY KEY, v INTEGER NOT NULL);";
    if (_exec(storage->db, schema) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "DCU task store: 建表失败");
        return -1;
    }
    return _seed_default(storage->db);
}

static void _mirror_from_poller(const dcu_poller_t *poller) {
    s_cfg_count = 0;
    for (int i = 0; i < poller->task_count && s_cfg_count < MAX_TASKS; i++) {
        const dcu_task_unit_t *t = &poller->tasks[i];
        if (t->status == DCU_TASK_STATUS_FREE) continue;
        dcu_task_cfg_t *c = &s_cfg[s_cfg_count++];
        c->task_id = (uint8_t)t->task_id;
        c->interval_unit = t->interval_unit;
        c->interval_value = t->interval_value;
        c->scheme_type = t->scheme_type;
        c->scheme_id = (uint8_t)t->scheme_id;
        c->status = t->status;
        c->period_type = t->period_type;
    }
}

static void _restore_snapshot(dcu_poller_t *poller) {
    memcpy(poller->tasks, s_snapshot, sizeof(poller->tasks));
    poller->task_count = s_snapshot_count;
    memcpy(s_cfg, s_cfg_snapshot, sizeof(s_cfg));
    s_cfg_count = s_cfg_snapshot_count;
}

static void _take_snapshot(const dcu_poller_t *poller) {
    memcpy(s_snapshot, poller->tasks, sizeof(poller->tasks));
    s_snapshot_count = poller->task_count;
    memcpy(s_cfg_snapshot, s_cfg, sizeof(s_cfg));
    s_cfg_snapshot_count = s_cfg_count;
}

static int _load_row(dcu_task_unit_t *t, const dcu_task_cfg_t *c) {
    uint32_t interval_s = 0;
    if (c->task_id == 0 || c->interval_unit > 3 || c->scheme_type != 1 ||
        (c->status != DCU_TASK_STATUS_NORMAL && c->status != DCU_TASK_STATUS_DISABLED) ||
        c->period_type > 3) {
        return -1;
    }
    if (dcu_task_interval_seconds(c->interval_unit, c->interval_value, &interval_s) != 0) {
        return -1;
    }
    memset(t, 0, sizeof(*t));
    t->task_id = c->task_id;
    t->interval_s = interval_s;
    t->interval_unit = c->interval_unit;
    t->interval_value = c->interval_value;
    t->has_interval = c->interval_value != 0;
    t->status = c->status;
    t->period_type = c->period_type;
    t->scheme_type = c->scheme_type;
    t->scheme_id = c->scheme_id;
    return 0;
}

int dcu_task_store_load(dcu_storage_t *storage, dcu_poller_t *poller) {
    if (!storage || !storage->db || !poller) return -1;

    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT task_id, interval_unit, interval_value, scheme_type, scheme_id, status, "
        "period_type FROM dcu_task_config ORDER BY task_id;";
    if (sqlite3_prepare_v2(storage->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    s_cfg_count = 0;
    int rc = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        dcu_task_cfg_t c = {
            .task_id = (uint8_t)sqlite3_column_int(stmt, 0),
            .interval_unit = (uint8_t)sqlite3_column_int(stmt, 1),
            .interval_value = (uint16_t)sqlite3_column_int(stmt, 2),
            .scheme_type = (uint8_t)sqlite3_column_int(stmt, 3),
            .scheme_id = (uint8_t)sqlite3_column_int(stmt, 4),
            .status = (uint8_t)sqlite3_column_int(stmt, 5),
            .period_type = (uint8_t)sqlite3_column_int(stmt, 6),
        };
        dcu_task_unit_t unit;
        if (_load_row(&unit, &c) != 0 || dcu_poller_upsert_task(poller, &unit) != 0) {
            LOG_WARN(LOG_CAT_CORE, "DCU task store: 跳过无法装载的任务 %u（不删库）",
                     (unsigned)c.task_id);
            continue;
        }
        if (s_cfg_count < MAX_TASKS) s_cfg[s_cfg_count++] = c;
    }
    sqlite3_finalize(stmt);
    return rc;
}

static int _validate_unit(dcu_poller_t *poller, const dcu_task_config_t *u,
                          uint32_t *out_interval_s) {
    if (u->task_id == 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 ID 0 是保留值（OAD 元素 0 = 最小现有 ID）");
        return EDGE_SLAVE_SRC_DENIED;
    }

    uint32_t interval_s = 0;
    if (u->interval_unit > 3) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的 TI 单位 %u（月/年）无固定秒数",
                 (unsigned)u->task_id, (unsigned)u->interval_unit);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (dcu_task_interval_seconds(u->interval_unit, u->interval_value, &interval_s) != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的执行周期换算成秒溢出", (unsigned)u->task_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->scheme_type != 1) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的方案类型 %u 无执行引擎", (unsigned)u->task_id,
                 (unsigned)u->scheme_type);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (_time_nonzero(u->start_time)) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的开始时间（时间窗口）未实现",
                 (unsigned)u->task_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (_time_nonzero(u->end_time)) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的结束时间（时间窗口）未实现",
                 (unsigned)u->task_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->delay_unit != 0 || u->delay_value != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的执行延时未实现", (unsigned)u->task_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->priority != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的执行优先级未实现", (unsigned)u->task_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->pre_script != 0 || u->post_script != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的前/后脚本未实现", (unsigned)u->task_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->period_count != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 的运行时段表未实现", (unsigned)u->task_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (!dcu_poller_get_scheme(poller, u->scheme_id)) {
        LOG_WARN(LOG_CAT_NB, "DCU 6012: 任务 %u 引用不存在的方案 %u", (unsigned)u->task_id,
                 (unsigned)u->scheme_id);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    *out_interval_s = interval_s;
    return 0;
}

static int _add_capacity_ok(const dcu_poller_t *poller, const dcu_task_op_t *op) {
    int live = 0;
    for (int i = 0; i < poller->task_count; i++) {
        if (poller->tasks[i].status != DCU_TASK_STATUS_FREE) live++;
    }
    int added = 0;
    for (int i = 0; i < op->unit_count; i++) {
        if (dcu_poller_task_find(poller, op->units[i].task_id)) continue;
        bool dup = false;
        for (int j = 0; j < i; j++) {
            if (op->units[j].task_id == op->units[i].task_id) dup = true;
        }
        if (!dup) added++;
    }
    return live + added <= MAX_TASKS;
}

static int _sql_add(sqlite3 *db, const dcu_task_config_t *u) {
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT OR REPLACE INTO dcu_task_config (task_id, interval_unit, interval_value, "
        "scheme_type, scheme_id, status, period_type) VALUES (?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(stmt, 1, u->task_id);
    sqlite3_bind_int(stmt, 2, u->interval_unit);
    sqlite3_bind_int(stmt, 3, u->interval_value);
    sqlite3_bind_int(stmt, 4, u->scheme_type);
    sqlite3_bind_int(stmt, 5, u->scheme_id);
    sqlite3_bind_int(stmt, 6, u->status);
    sqlite3_bind_int(stmt, 7, u->period_type);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _sql_delete_id(sqlite3 *db, uint8_t task_id) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM dcu_task_config WHERE task_id = ?;", -1, &stmt, NULL) !=
        SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, task_id);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _sql_update_status(sqlite3 *db, uint8_t task_id, uint8_t status) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "UPDATE dcu_task_config SET status = ? WHERE task_id = ?;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_int(stmt, 2, task_id);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _ram_add(dcu_poller_t *poller, const dcu_task_config_t *u, uint32_t interval_s) {
    dcu_task_unit_t task;
    memset(&task, 0, sizeof(task));
    task.task_id = u->task_id;
    task.interval_s = interval_s;
    task.interval_unit = u->interval_unit;
    task.interval_value = u->interval_value;
    task.has_interval = u->interval_value != 0;
    task.status = u->status;
    task.period_type = u->period_type;
    task.scheme_type = u->scheme_type;
    task.scheme_id = u->scheme_id;
    return dcu_poller_upsert_task(poller, &task) == 0 ? 0 : -1;
}

int dcu_task_store_apply(dcu_storage_t *storage, dcu_poller_t *poller, const dcu_task_op_t *op) {
    if (!storage || !storage->db || !poller || !op) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint32_t interval_s[MAX_TASKS] = {0};
    switch (op->method) {
        case 127:
            if (op->unit_count == 0 || op->unit_count > MAX_TASKS)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (!_add_capacity_ok(poller, op)) {
                LOG_WARN(LOG_CAT_NB, "DCU 6012: Add 超过 %d 条任务上限", MAX_TASKS);
                return EDGE_SLAVE_SRC_TEMPORARY;
            }
            for (int i = 0; i < op->unit_count; i++) {
                int rc = _validate_unit(poller, &op->units[i], &interval_s[i]);
                if (rc != 0) return rc;
            }
            break;
        case 128:
            if (op->id_count == 0 || op->id_count > MAX_TASKS)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (int i = 0; i < op->id_count; i++) {
                if (!dcu_poller_task_find(poller, op->ids[i])) return EDGE_SLAVE_SRC_NOT_FOUND;
            }
            break;
        case 129:
            break;
        case 130: {
            if (op->unit_count != 1) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            const dcu_task_config_t *u = &op->units[0];
            if (!dcu_poller_task_find(poller, u->task_id)) return EDGE_SLAVE_SRC_NOT_FOUND;
            if (u->status != DCU_TASK_STATUS_NORMAL && u->status != DCU_TASK_STATUS_DISABLED)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            break;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }

    _take_snapshot(poller);
    if (dcu_storage_begin_trans(storage) != 0) return EDGE_SLAVE_SRC_TEMPORARY;

    int rc = 0;
    switch (op->method) {
        case 127:
            for (int i = 0; i < op->unit_count && rc == 0; i++) {
                rc = _sql_add(storage->db, &op->units[i]);
            }
            break;
        case 128:
            for (int i = 0; i < op->id_count && rc == 0; i++) {
                rc = _sql_delete_id(storage->db, op->ids[i]);
            }
            break;
        case 129:
            rc = _exec(storage->db, "DELETE FROM dcu_task_config;");
            break;
        case 130:
            rc = _sql_update_status(storage->db, op->units[0].task_id, op->units[0].status);
            break;
    }
    if (rc != 0) {
        _rollback(storage->db);
        _restore_snapshot(poller);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }

    switch (op->method) {
        case 127:
            for (int i = 0; i < op->unit_count && rc == 0; i++) {
                rc = _ram_add(poller, &op->units[i], interval_s[i]);
            }
            break;
        case 128:
            for (int i = 0; i < op->id_count; i++) dcu_poller_remove_task(poller, op->ids[i]);
            break;
        case 129:
            dcu_poller_clear_tasks(poller);
            break;
        case 130:
            rc = dcu_poller_set_task_status(poller, op->units[0].task_id,
                                            op->units[0].status) == 0
                     ? 0
                     : -1;
            break;
    }
    if (rc != 0 || dcu_storage_commit_trans(storage) != 0) {
        _rollback(storage->db);
        _restore_snapshot(poller);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }

    _mirror_from_poller(poller);
    return 0;
}
