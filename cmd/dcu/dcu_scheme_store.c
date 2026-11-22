#include "dcu_scheme_store.h"

#include <sqlite3.h>
#include <string.h>

#include "dcu_driver_adapter.h"
#include "edge_log.h"
#include "edge_slave_plane.h"

/* 回滚快照：SQL 先落，RAM 后改，任一步失败都恢复到调用前。 */
static dcu_scheme_t s_snapshot[MAX_SCHEMES];
static uint16_t s_snapshot_count;

static int _exec(sqlite3 *db, const char *sql) {
    return sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

static void _rollback(sqlite3 *db) {
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
}

static void _take_snapshot(const dcu_poller_t *poller) {
    memcpy(s_snapshot, poller->schemes, sizeof(poller->schemes));
    s_snapshot_count = poller->scheme_count;
}

static int _seed_default(sqlite3 *db, const uint32_t *tags, uint16_t count) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT v FROM dcu_scheme_meta WHERE k='seeded';", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    int step = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (step == SQLITE_ROW) return 0; /* 已经播过种；Clear 之后也不重播 */
    if (step != SQLITE_DONE) return -1;
    if (count > MAX_SCHEME_OADS) count = MAX_SCHEME_OADS;

    if (_exec(db, "BEGIN IMMEDIATE TRANSACTION;") != 0) return -1;
    int rc = 0;
    if (count > 0) {
        if (_exec(db,
                  "INSERT OR IGNORE INTO dcu_scheme_config "
                  "(scheme_id, depth, capture_type, ms_kind, time_label) "
                  "VALUES (1, 0, 0, 1, 0);") != 0) {
            rc = -1;
        }
        for (uint16_t i = 0; i < count && rc == 0; i++) {
            sqlite3_stmt *ins = NULL;
            if (sqlite3_prepare_v2(db,
                                   "INSERT OR IGNORE INTO dcu_scheme_csd (scheme_id, pos, tag) "
                                   "VALUES (1, ?, ?);",
                                   -1, &ins, NULL) != SQLITE_OK) {
                rc = -1;
                break;
            }
            sqlite3_bind_int(ins, 1, i);
            sqlite3_bind_int(ins, 2, (int)tags[i]);
            if (sqlite3_step(ins) != SQLITE_DONE) rc = -1;
            sqlite3_finalize(ins);
        }
    }
    if (rc == 0 &&
        _exec(db, "INSERT OR REPLACE INTO dcu_scheme_meta (k, v) VALUES ('seeded', 1);") != 0) {
        rc = -1;
    }
    if (rc != 0 || _exec(db, "COMMIT;") != 0) {
        _rollback(db);
        return -1;
    }
    return 0;
}

int dcu_scheme_store_init(dcu_storage_t *storage, const uint32_t *default_tags,
                          uint16_t default_count) {
    if (!storage || !storage->db || (default_count > 0 && !default_tags)) return -1;
    const char *schema =
        "CREATE TABLE IF NOT EXISTS dcu_scheme_config ("
        " scheme_id INTEGER PRIMARY KEY, depth INTEGER NOT NULL,"
        " capture_type INTEGER NOT NULL, ms_kind INTEGER NOT NULL,"
        " time_label INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS dcu_scheme_csd ("
        " scheme_id INTEGER NOT NULL, pos INTEGER NOT NULL, tag INTEGER NOT NULL,"
        " PRIMARY KEY (scheme_id, pos));"
        "CREATE TABLE IF NOT EXISTS dcu_scheme_meta (k TEXT PRIMARY KEY, v INTEGER NOT NULL);";
    if (_exec(storage->db, schema) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "DCU scheme store: 建表失败");
        return -1;
    }
    return _seed_default(storage->db, default_tags, default_count);
}

static int _load_csd(sqlite3 *db, uint32_t scheme_id, dcu_scheme_t *s) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT tag FROM dcu_scheme_csd WHERE scheme_id = ? ORDER BY pos;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, (int)scheme_id);
    uint16_t count = 0;
    int rc = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        uint32_t tag = (uint32_t)sqlite3_column_int64(stmt, 0);
        if (count >= MAX_SCHEME_OADS || dcu_driver_adapter_oad_to_proto("DLT698", tag) == 0) {
            rc = -1;
            break;
        }
        s->data.normal.csd_list[count++] = tag;
    }
    sqlite3_finalize(stmt);
    if (rc != 0 || count == 0) return -1;
    s->data.normal.csd_count = count;
    return 0;
}

int dcu_scheme_store_load(dcu_storage_t *storage, dcu_poller_t *poller) {
    if (!storage || !storage->db || !poller) return -1;
    dcu_poller_clear_schemes(poller);

    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT scheme_id, depth, capture_type, ms_kind, time_label FROM dcu_scheme_config "
        "ORDER BY scheme_id;";
    if (sqlite3_prepare_v2(storage->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        dcu_scheme_t s;
        memset(&s, 0, sizeof(s));
        uint32_t id = (uint32_t)sqlite3_column_int64(stmt, 0);
        uint16_t depth = (uint16_t)sqlite3_column_int(stmt, 1);
        uint8_t capture = (uint8_t)sqlite3_column_int(stmt, 2);
        uint8_t ms_kind = (uint8_t)sqlite3_column_int(stmt, 3);
        uint8_t label = (uint8_t)sqlite3_column_int(stmt, 4);
        if (id == 0 || id > UINT8_MAX || depth != 0 || capture != 0 || ms_kind != 1 || label != 0) {
            LOG_WARN(LOG_CAT_CORE, "DCU scheme store: 跳过无法执行的方案 %u（不删库）",
                     (unsigned)id);
            continue;
        }
        s.scheme_id = id;
        s.type = SCHEME_TYPE_NORMAL;
        s.data.normal.capture_mode = capture;
        s.data.normal.ms_type = ms_kind;
        s.data.normal.storage_depth = depth;
        s.data.normal.storage_time = label;
        if (_load_csd(storage->db, id, &s) != 0 || dcu_poller_upsert_scheme(poller, &s) != 0) {
            LOG_WARN(LOG_CAT_CORE, "DCU scheme store: 跳过无法装载的方案 %u（不删库）",
                     (unsigned)id);
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}

static int _validate_csd(const dcu_scheme_cfg_t *u, uint32_t *tags, uint8_t *out_count) {
    if (u->has_road) {
        LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 的 CSD 含 ROAD（记录型列）未实现",
                 (unsigned)u->scheme_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->csd_count == 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 的 CSD 为空（标准语义为全选）未实现",
                 (unsigned)u->scheme_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->csd_count > MAX_SCHEME_OADS) return EDGE_SLAVE_SRC_TEMPORARY;
    for (uint8_t i = 0; i < u->csd_count; i++) {
        uint32_t tag = dcu_driver_adapter_proto_to_oad("DLT698", u->csd_oads[i]);
        if (tag == 0) {
            LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 的 CSD OAD 0x%08X 不是可采集点位",
                     (unsigned)u->scheme_id, (unsigned)u->csd_oads[i]);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        tags[i] = tag;
    }
    *out_count = u->csd_count;
    return 0;
}

static int _validate_unit(const dcu_scheme_cfg_t *u, uint32_t *tags, uint8_t *out_count) {
    if (u->scheme_id == 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案编号 0 是保留值（OAD 元素 0 = 最小现有编号）");
        return EDGE_SLAVE_SRC_DENIED;
    }
    if (u->storage_depth != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 的存储深度 %u 未实现", (unsigned)u->scheme_id,
                 (unsigned)u->storage_depth);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->capture_type != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 的采集类型 %u 未实现", (unsigned)u->scheme_id,
                 (unsigned)u->capture_type);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->storage_time != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 的存储时标 %u 未实现", (unsigned)u->scheme_id,
                 (unsigned)u->storage_time);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->ms_choice != 1) {
        LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 的电能表集合 choice %u 未实现",
                 (unsigned)u->scheme_id, (unsigned)u->ms_choice);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return _validate_csd(u, tags, out_count);
}

static bool _same_csd(const dcu_scheme_t *s, const uint32_t *tags, uint8_t count) {
    if (!s || s->data.normal.csd_count != count) return false;
    return memcmp(s->data.normal.csd_list, tags, sizeof(uint32_t) * count) == 0;
}

static int _sql_replace_config(sqlite3 *db, const dcu_scheme_cfg_t *u) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT OR REPLACE INTO dcu_scheme_config "
                           "(scheme_id, depth, capture_type, ms_kind, time_label) "
                           "VALUES (?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, u->scheme_id);
    sqlite3_bind_int(stmt, 2, u->storage_depth);
    sqlite3_bind_int(stmt, 3, u->capture_type);
    sqlite3_bind_int(stmt, 4, u->ms_choice);
    sqlite3_bind_int(stmt, 5, u->storage_time);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _sql_delete_csd(sqlite3 *db, uint32_t scheme_id) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM dcu_scheme_csd WHERE scheme_id = ?;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, (int)scheme_id);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _sql_insert_csd(sqlite3 *db, uint32_t scheme_id, const uint32_t *tags, uint8_t count) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "INSERT INTO dcu_scheme_csd (scheme_id, pos, tag) VALUES (?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    int rc = 0;
    for (uint8_t i = 0; i < count && rc == 0; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, (int)scheme_id);
        sqlite3_bind_int(stmt, 2, i);
        sqlite3_bind_int(stmt, 3, (int)tags[i]);
        if (sqlite3_step(stmt) != SQLITE_DONE) rc = -1;
    }
    sqlite3_finalize(stmt);
    return rc;
}

static int _sql_replace_scheme(sqlite3 *db, const dcu_scheme_cfg_t *u, const uint32_t *tags,
                               uint8_t count) {
    if (_sql_replace_config(db, u) != 0) return -1;
    if (_sql_delete_csd(db, u->scheme_id) != 0) return -1;
    return _sql_insert_csd(db, u->scheme_id, tags, count);
}

static int _sql_delete_scheme(sqlite3 *db, uint32_t scheme_id) {
    if (_sql_delete_csd(db, scheme_id) != 0) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM dcu_scheme_config WHERE scheme_id = ?;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, (int)scheme_id);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _ram_apply_scheme(dcu_poller_t *poller, const dcu_scheme_cfg_t *u, const uint32_t *tags,
                             uint8_t count) {
    dcu_scheme_t s;
    memset(&s, 0, sizeof(s));
    s.scheme_id = u->scheme_id;
    s.type = SCHEME_TYPE_NORMAL;
    s.data.normal.capture_mode = u->capture_type;
    s.data.normal.ms_type = u->ms_choice;
    s.data.normal.storage_depth = u->storage_depth;
    s.data.normal.storage_time = u->storage_time;
    s.data.normal.csd_count = count;
    memcpy(s.data.normal.csd_list, tags, sizeof(uint32_t) * count);
    return dcu_poller_upsert_scheme(poller, &s);
}

int dcu_scheme_store_apply(dcu_storage_t *storage, dcu_poller_t *poller,
                           const dcu_scheme_op_t *op) {
    if (!storage || !storage->db || !poller || !op) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint32_t tags[MAX_SCHEMES][MAX_SCHEME_OADS] = {{0}};
    uint8_t tag_count[MAX_SCHEMES] = {0};
    int rc = 0;

    switch (op->method) {
        case 127:
            if (op->unit_count == 0 || op->unit_count > MAX_SCHEMES)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (uint8_t i = 0; i < op->unit_count; i++) {
                rc = _validate_unit(&op->units[i], tags[i], &tag_count[i]);
                if (rc != 0) return rc;
                if (dcu_poller_scheme_referenced(poller, op->units[i].scheme_id) &&
                    !_same_csd(dcu_poller_get_scheme(poller, op->units[i].scheme_id), tags[i],
                               tag_count[i])) {
                    LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 被活动任务引用，拒绝改记录列选择",
                             (unsigned)op->units[i].scheme_id);
                    return EDGE_SLAVE_SRC_DENIED;
                }
            }
            {
                int live = poller->scheme_count;
                for (uint8_t i = 0; i < op->unit_count; i++) {
                    if (!dcu_poller_get_scheme(poller, op->units[i].scheme_id)) live++;
                }
                if (live > MAX_SCHEMES) {
                    LOG_WARN(LOG_CAT_NB, "DCU 6014: Add 超过 %d 个方案上限", MAX_SCHEMES);
                    return EDGE_SLAVE_SRC_TEMPORARY;
                }
            }
            break;
        case 128:
            if (op->id_count == 0 || op->id_count > MAX_SCHEMES)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (uint8_t i = 0; i < op->id_count; i++) {
                if (!dcu_poller_get_scheme(poller, op->ids[i]) ||
                    dcu_poller_scheme_referenced(poller, op->ids[i])) {
                    return EDGE_SLAVE_SRC_NOT_FOUND;
                }
            }
            break;
        case 129:
            for (int i = 0; i < poller->scheme_count; i++) {
                if (dcu_poller_scheme_referenced(poller, poller->schemes[i].scheme_id)) {
                    LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 被活动任务引用，拒绝 Clear",
                             (unsigned)poller->schemes[i].scheme_id);
                    return EDGE_SLAVE_SRC_DENIED;
                }
            }
            break;
        case 130: {
            if (op->unit_count != 1) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            const dcu_scheme_cfg_t *u = &op->units[0];
            if (!dcu_poller_get_scheme(poller, u->scheme_id)) return EDGE_SLAVE_SRC_NOT_FOUND;
            rc = _validate_csd(u, tags[0], &tag_count[0]);
            if (rc != 0) return rc;
            if (dcu_poller_scheme_referenced(poller, u->scheme_id)) {
                LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 被活动任务引用，拒绝 Set_CSD",
                         (unsigned)u->scheme_id);
                return EDGE_SLAVE_SRC_DENIED;
            }
            break;
        }
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }

    _take_snapshot(poller);
    if (dcu_storage_begin_trans(storage) != 0) return EDGE_SLAVE_SRC_TEMPORARY;

    switch (op->method) {
        case 127:
            for (uint8_t i = 0; i < op->unit_count && rc == 0; i++) {
                rc = _sql_replace_scheme(storage->db, &op->units[i], tags[i], tag_count[i]);
            }
            break;
        case 128:
            for (uint8_t i = 0; i < op->id_count && rc == 0; i++) {
                rc = _sql_delete_scheme(storage->db, op->ids[i]);
            }
            break;
        case 129:
            rc = _exec(storage->db, "DELETE FROM dcu_scheme_csd; DELETE FROM dcu_scheme_config;");
            break;
        case 130:
            rc = _sql_delete_csd(storage->db, op->units[0].scheme_id);
            if (rc == 0)
                rc = _sql_insert_csd(storage->db, op->units[0].scheme_id, tags[0], tag_count[0]);
            break;
    }
    if (rc != 0) {
        _rollback(storage->db);
        memcpy(poller->schemes, s_snapshot, sizeof(poller->schemes));
        poller->scheme_count = s_snapshot_count;
        return EDGE_SLAVE_SRC_TEMPORARY;
    }

    switch (op->method) {
        case 127:
            for (uint8_t i = 0; i < op->unit_count && rc == 0; i++) {
                rc = _ram_apply_scheme(poller, &op->units[i], tags[i], tag_count[i]);
            }
            break;
        case 128:
            for (uint8_t i = 0; i < op->id_count; i++) dcu_poller_remove_scheme(poller, op->ids[i]);
            break;
        case 129:
            dcu_poller_clear_schemes(poller);
            break;
        case 130: {
            dcu_scheme_t *s = dcu_poller_get_scheme(poller, op->units[0].scheme_id);
            if (s) {
                s->data.normal.csd_count = tag_count[0];
                memcpy(s->data.normal.csd_list, tags[0], sizeof(uint32_t) * tag_count[0]);
            } else {
                rc = -1;
            }
            break;
        }
    }
    if (rc != 0 || dcu_storage_commit_trans(storage) != 0) {
        _rollback(storage->db);
        memcpy(poller->schemes, s_snapshot, sizeof(poller->schemes));
        poller->scheme_count = s_snapshot_count;
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}
