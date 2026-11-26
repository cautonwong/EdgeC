#include "dcu_scheme_store.h"

#include <sqlite3.h>
#include <string.h>

#include "dcu_driver_adapter.h"
#include "dcu_scheme_store_internal.h"
#include "edge_log.h"
#include "edge_slave_plane.h"

/* 回滚快照：SQL 先落，RAM 后改，任一步失败都恢复到调用前。 */
static dcu_scheme_t s_snapshot[MAX_SCHEMES];
static uint16_t s_snapshot_count;
static dcu_storage_t *s_storage;
static dcu_event_scheme_op_t s_event_op;
static dcu_transparent_op_t s_transparent_op;
static dcu_transparent_result_t s_result_buf;

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

    if (dcu_scheme_store_begin(db) != 0) return -1;
    int rc = 0;
    if (count > 0) {
        if (dcu_scheme_store_exec(db,
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
        dcu_scheme_store_exec(
            db, "INSERT OR REPLACE INTO dcu_scheme_meta (k, v) VALUES ('seeded', 1);") != 0) {
        rc = -1;
    }
    if (rc != 0 || dcu_scheme_store_commit(db) != 0) {
        dcu_scheme_store_rollback(db);
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
        "CREATE TABLE IF NOT EXISTS dcu_scheme_meta (k TEXT PRIMARY KEY, v INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS dcu_event_scheme ("
        " scheme_id INTEGER PRIMARY KEY, capture_type INTEGER NOT NULL,"
        " report_flag INTEGER NOT NULL, storage_depth INTEGER NOT NULL,"
        " ms_choice INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS dcu_event_road ("
        " scheme_id INTEGER NOT NULL, pos INTEGER NOT NULL, oad INTEGER NOT NULL,"
        " PRIMARY KEY (scheme_id, pos));"
        "CREATE TABLE IF NOT EXISTS dcu_event_road_rel ("
        " scheme_id INTEGER NOT NULL, pos INTEGER NOT NULL, rel_pos INTEGER NOT NULL,"
        " oad INTEGER NOT NULL, PRIMARY KEY (scheme_id, pos, rel_pos));"
        "CREATE TABLE IF NOT EXISTS dcu_transparent_scheme ("
        " scheme_id INTEGER PRIMARY KEY, storage_depth INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS dcu_transparent_content ("
        " scheme_id INTEGER NOT NULL, pos INTEGER NOT NULL, seq INTEGER NOT NULL,"
        " address BLOB NOT NULL, pre_script INTEGER NOT NULL, post_script INTEGER NOT NULL,"
        " report_and_wait INTEGER NOT NULL, wait_timeout INTEGER NOT NULL,"
        " compare_mode INTEGER NOT NULL, feature_byte INTEGER NOT NULL,"
        " cut_start INTEGER NOT NULL, cut_len INTEGER NOT NULL,"
        " PRIMARY KEY (scheme_id, pos));"
        "CREATE TABLE IF NOT EXISTS dcu_transparent_frame ("
        " scheme_id INTEGER NOT NULL, pos INTEGER NOT NULL, frame_pos INTEGER NOT NULL,"
        " seq INTEGER NOT NULL, data BLOB NOT NULL,"
        " PRIMARY KEY (scheme_id, pos, frame_pos));"
        "CREATE TABLE IF NOT EXISTS dcu_transparent_result ("
        " result_id INTEGER PRIMARY KEY, scheme_id INTEGER NOT NULL,"
        " exec_time INTEGER NOT NULL, address BLOB NOT NULL);"
        "CREATE TABLE IF NOT EXISTS dcu_transparent_result_item ("
        " result_id INTEGER NOT NULL, frame_pos INTEGER NOT NULL, seq INTEGER NOT NULL,"
        " resp_time INTEGER NOT NULL, data BLOB NOT NULL,"
        " PRIMARY KEY (result_id, frame_pos));"
        "CREATE TABLE IF NOT EXISTS dcu_report_scheme ("
        " scheme_id INTEGER PRIMARY KEY, timeout_unit INTEGER NOT NULL,"
        " timeout_value INTEGER NOT NULL, max_times INTEGER NOT NULL,"
        " content_type INTEGER NOT NULL, content_oad INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS dcu_report_channel ("
        " scheme_id INTEGER NOT NULL, pos INTEGER NOT NULL, oad INTEGER NOT NULL,"
        " PRIMARY KEY (scheme_id, pos));";
    if (dcu_scheme_store_exec(storage->db, schema) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "DCU scheme store: 建表失败");
        return -1;
    }
    s_storage = storage;
    dcu_scheme_store_report_bind(storage);
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
                if (!dcu_poller_get_scheme(poller, op->ids[i])) return EDGE_SLAVE_SRC_NOT_FOUND;
                if (dcu_poller_scheme_referenced(poller, op->ids[i])) {
                    LOG_WARN(LOG_CAT_NB, "DCU 6014: 方案 %u 被活动任务引用，拒绝删除",
                             (unsigned)op->ids[i]);
                    return EDGE_SLAVE_SRC_DENIED;
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
            rc = dcu_scheme_store_exec(
                storage->db, "DELETE FROM dcu_scheme_csd; DELETE FROM dcu_scheme_config;");
            break;
        case 130:
            rc = _sql_delete_csd(storage->db, op->units[0].scheme_id);
            if (rc == 0)
                rc = _sql_insert_csd(storage->db, op->units[0].scheme_id, tags[0], tag_count[0]);
            break;
    }
    if (rc != 0) {
        dcu_scheme_store_rollback(storage->db);
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
        dcu_scheme_store_rollback(storage->db);
        memcpy(poller->schemes, s_snapshot, sizeof(poller->schemes));
        poller->scheme_count = s_snapshot_count;
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

dcu_event_scheme_op_t *dcu_scheme_store_event_op(void) {
    return &s_event_op;
}

static int _ev_exists(sqlite3 *db, uint8_t scheme_id) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM dcu_event_scheme WHERE scheme_id = ?;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    int rc = sqlite3_step(stmt) == SQLITE_ROW ? 1 : 0;
    sqlite3_finalize(stmt);
    return rc;
}

static int _ev_validate(const dcu_event_scheme_cfg_t *u) {
    if (u->scheme_id == 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6016: 方案编号 0 是保留值（OAD 元素 0 = 最小现有编号）");
        return EDGE_SLAVE_SRC_DENIED;
    }
    if (u->capture_type > DCU_EVENT_CAPTURE_NOTIFY_LIST) {
        LOG_WARN(LOG_CAT_NB, "DCU 6016: 方案 %u 的采集类型 %u 不是标准定义的 0/1/2",
                 (unsigned)u->scheme_id, (unsigned)u->capture_type);
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (u->ms_choice != 1) {
        LOG_WARN(LOG_CAT_NB, "DCU 6016: 方案 %u 的电能表集合 choice %u 未实现",
                 (unsigned)u->scheme_id, (unsigned)u->ms_choice);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->road_count > DCU_MAX_EVENT_ROADS) return EDGE_SLAVE_SRC_TEMPORARY;
    if (u->capture_type == DCU_EVENT_CAPTURE_NOTIFY_ALL && u->road_count != 0)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    for (uint8_t i = 0; i < u->road_count; i++) {
        if (u->roads[i].oad == 0 || u->roads[i].related_count > DCU_ROAD_RELATED_MAX) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
    }
    return 0;
}

static int _ev_delete(sqlite3 *db, uint8_t scheme_id) {
    static const char *const sql[] = {
        "DELETE FROM dcu_event_road_rel WHERE scheme_id = ?;",
        "DELETE FROM dcu_event_road WHERE scheme_id = ?;",
        "DELETE FROM dcu_event_scheme WHERE scheme_id = ?;",
    };
    for (size_t i = 0; i < sizeof(sql) / sizeof(sql[0]); i++) {
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(db, sql[i], -1, &stmt, NULL) != SQLITE_OK) return -1;
        sqlite3_bind_int(stmt, 1, scheme_id);
        int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
        sqlite3_finalize(stmt);
        if (rc != 0) return -1;
    }
    return 0;
}

static int _ev_insert_road(sqlite3 *db, uint8_t scheme_id, uint8_t pos,
                           const dcu_event_road_t *road) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "INSERT INTO dcu_event_road (scheme_id, pos, oad) VALUES (?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    sqlite3_bind_int(stmt, 2, pos);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)road->oad);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    if (rc != 0) return -1;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_event_road_rel (scheme_id, pos, rel_pos, oad) "
                           "VALUES (?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    for (uint8_t r = 0; r < road->related_count; r++) {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, scheme_id);
        sqlite3_bind_int(stmt, 2, pos);
        sqlite3_bind_int(stmt, 3, r);
        sqlite3_bind_int64(stmt, 4, (sqlite3_int64)road->related[r]);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return -1;
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}

static int _ev_replace(sqlite3 *db, const dcu_event_scheme_cfg_t *u) {
    if (_ev_delete(db, u->scheme_id) != 0) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_event_scheme "
                           "(scheme_id, capture_type, report_flag, storage_depth, ms_choice) "
                           "VALUES (?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, u->scheme_id);
    sqlite3_bind_int(stmt, 2, u->capture_type);
    sqlite3_bind_int(stmt, 3, u->report_flag ? 1 : 0);
    sqlite3_bind_int(stmt, 4, u->storage_depth);
    sqlite3_bind_int(stmt, 5, u->ms_choice);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    if (rc != 0) return -1;
    for (uint8_t i = 0; i < u->road_count; i++) {
        if (_ev_insert_road(db, u->scheme_id, i, &u->roads[i]) != 0) return -1;
    }
    return 0;
}

int dcu_scheme_store_event_apply(const dcu_event_scheme_op_t *op) {
    if (!op || !s_storage || !s_storage->db) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3 *db = s_storage->db;
    int rc = 0;
    switch (op->method) {
        case 127:
            if (op->unit_count == 0 || op->unit_count > DCU_MAX_EVENT_SCHEMES) {
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            }
            for (uint8_t i = 0; i < op->unit_count; i++) {
                rc = _ev_validate(&op->units[i]);
                if (rc != 0) return rc;
                for (uint8_t j = 0; j < i; j++) {
                    if (op->units[j].scheme_id == op->units[i].scheme_id)
                        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
                }
            }
            break;
        case 128:
            if (op->id_count == 0 || op->id_count > DCU_MAX_EVENT_SCHEMES)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (uint8_t i = 0; i < op->id_count; i++) {
                if (_ev_exists(db, op->ids[i]) != 1) return EDGE_SLAVE_SRC_NOT_FOUND;
            }
            break;
        case 129:
            break;
        case 130:
            if (_ev_exists(db, op->scheme_id) != 1) return EDGE_SLAVE_SRC_NOT_FOUND;
            break;
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
    if (dcu_scheme_store_begin(db) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    switch (op->method) {
        case 127:
            for (uint8_t i = 0; i < op->unit_count && rc == 0; i++)
                rc = _ev_replace(db, &op->units[i]);
            break;
        case 128:
            for (uint8_t i = 0; i < op->id_count && rc == 0; i++)
                rc = _ev_delete(db, op->ids[i]);
            break;
        case 129:
            rc = dcu_scheme_store_exec(db,
                                       "DELETE FROM dcu_event_road_rel; DELETE FROM dcu_event_road;"
                                       "DELETE FROM dcu_event_scheme;");
            break;
        case 130: {
            sqlite3_stmt *stmt = NULL;
            if (sqlite3_prepare_v2(db, "UPDATE dcu_event_scheme SET report_flag = ? "
                                       "WHERE scheme_id = ?;",
                                   -1, &stmt, NULL) != SQLITE_OK) {
                rc = -1;
                break;
            }
            sqlite3_bind_int(stmt, 1, op->report_flag ? 1 : 0);
            sqlite3_bind_int(stmt, 2, op->scheme_id);
            rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
            sqlite3_finalize(stmt);
            break;
        }
    }
    if (rc != 0 || dcu_scheme_store_commit(db) != 0) {
        dcu_scheme_store_rollback(db);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

static int _ev_load(sqlite3 *db, uint8_t scheme_id, dcu_event_scheme_cfg_t *out) {
    memset(out, 0, sizeof(*out));
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT capture_type, report_flag, storage_depth, ms_choice "
                           "FROM dcu_event_scheme WHERE scheme_id = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    out->scheme_id = scheme_id;
    out->capture_type = (dcu_event_capture_t)sqlite3_column_int(stmt, 0);
    out->report_flag = sqlite3_column_int(stmt, 1) != 0;
    out->storage_depth = (uint16_t)sqlite3_column_int(stmt, 2);
    out->ms_choice = (uint8_t)sqlite3_column_int(stmt, 3);
    sqlite3_finalize(stmt);
    if (sqlite3_prepare_v2(db, "SELECT pos, oad FROM dcu_event_road WHERE scheme_id = ? "
                               "ORDER BY pos;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    uint8_t road_pos = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (road_pos >= DCU_MAX_EVENT_ROADS) {
            sqlite3_finalize(stmt);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        dcu_event_road_t *road = &out->roads[road_pos];
        road->oad = (uint32_t)sqlite3_column_int64(stmt, 1);
        sqlite3_stmt *rel = NULL;
        if (sqlite3_prepare_v2(db,
                               "SELECT rel_pos, oad FROM dcu_event_road_rel WHERE scheme_id = ? "
                               "AND pos = ? ORDER BY rel_pos;",
                               -1, &rel, NULL) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        sqlite3_bind_int(rel, 1, scheme_id);
        sqlite3_bind_int(rel, 2, (int)road_pos);
        uint8_t rel_pos = 0;
        while (sqlite3_step(rel) == SQLITE_ROW) {
            if (rel_pos >= DCU_ROAD_RELATED_MAX) {
                sqlite3_finalize(rel);
                sqlite3_finalize(stmt);
                return EDGE_SLAVE_SRC_TEMPORARY;
            }
            road->related[rel_pos++] = (uint32_t)sqlite3_column_int64(rel, 1);
        }
        sqlite3_finalize(rel);
        road->related_count = rel_pos;
        road_pos++;
    }
    sqlite3_finalize(stmt);
    out->road_count = road_pos;
    return 0;
}

int dcu_scheme_store_event_count(void) {
    if (!s_storage || !s_storage->db) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_storage->db, "SELECT COUNT(*) FROM dcu_event_scheme;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) rc = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_scheme_store_event_at(uint16_t pos, dcu_event_scheme_cfg_t *out) {
    if (!s_storage || !s_storage->db || !out) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_storage->db,
                           "SELECT scheme_id FROM dcu_event_scheme ORDER BY scheme_id "
                           "LIMIT 1 OFFSET ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, pos);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    uint8_t id = (uint8_t)sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return _ev_load(s_storage->db, id, out);
}

int dcu_scheme_store_event_get(uint8_t scheme_id, dcu_event_scheme_cfg_t *out) {
    if (!s_storage || !s_storage->db || !out) return EDGE_SLAVE_SRC_TEMPORARY;
    return _ev_load(s_storage->db, scheme_id, out);
}

dcu_transparent_op_t *dcu_scheme_store_transparent_op(void) {
    return &s_transparent_op;
}

static int _tp_validate_ctrl(const dcu_transparent_ctrl_t *c) {
    if (c->report_and_wait) {
        LOG_WARN(LOG_CAT_NB, "DCU 6018: 等待后续报文（多帧累积）未实现");
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (c->compare_mode != 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6018: 结果比对标识 %u 未实现", (unsigned)c->compare_mode);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

static int _tp_validate_content(const dcu_transparent_content_t *c) {
    if (c->address.len < 2 || c->address.len > DCU_TSA_MAX)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (c->frame_count > DCU_MAX_TRANSPARENT_FRAMES) return EDGE_SLAVE_SRC_TEMPORARY;
    for (uint8_t i = 0; i < c->frame_count; i++) {
        if (c->frames[i].len > DCU_MAX_TRANSPARENT_FRAME_LEN)
            return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return _tp_validate_ctrl(&c->ctrl);
}

static int _tp_validate_scheme(const dcu_transparent_scheme_t *s) {
    if (s->scheme_id == 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 6018: 方案编号 0 是保留值（OAD 元素 0 = 最小现有编号）");
        return EDGE_SLAVE_SRC_DENIED;
    }
    if (s->content_count > DCU_MAX_TRANSPARENT_CONTENTS) return EDGE_SLAVE_SRC_TEMPORARY;
    for (uint8_t i = 0; i < s->content_count; i++) {
        int rc = _tp_validate_content(&s->contents[i]);
        if (rc != 0) return rc;
        for (uint8_t j = 0; j < i; j++) {
            const dcu_tsa_t *a = &s->contents[j].address;
            const dcu_tsa_t *b = &s->contents[i].address;
            if (a->len == b->len && memcmp(a->bytes, b->bytes, a->len) == 0)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
    }
    return 0;
}

static int _tp_exists(sqlite3 *db, uint8_t scheme_id) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM dcu_transparent_scheme WHERE scheme_id = ?;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    int rc = sqlite3_step(stmt) == SQLITE_ROW ? 1 : 0;
    sqlite3_finalize(stmt);
    return rc;
}

static int _tp_delete_contents(sqlite3 *db, uint8_t scheme_id) {
    static const char *const sql[] = {
        "DELETE FROM dcu_transparent_frame WHERE scheme_id = ?;",
        "DELETE FROM dcu_transparent_content WHERE scheme_id = ?;",
    };
    for (size_t i = 0; i < sizeof(sql) / sizeof(sql[0]); i++) {
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(db, sql[i], -1, &stmt, NULL) != SQLITE_OK) return -1;
        sqlite3_bind_int(stmt, 1, scheme_id);
        int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
        sqlite3_finalize(stmt);
        if (rc != 0) return -1;
    }
    return 0;
}

static int _tp_insert_frames(sqlite3 *db, uint8_t scheme_id, uint8_t pos,
                             const dcu_transparent_content_t *c) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_transparent_frame "
                           "(scheme_id, pos, frame_pos, seq, data) VALUES (?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    for (uint8_t i = 0; i < c->frame_count; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, scheme_id);
        sqlite3_bind_int(stmt, 2, pos);
        sqlite3_bind_int(stmt, 3, i);
        sqlite3_bind_int(stmt, 4, c->frames[i].seq);
        sqlite3_bind_blob(stmt, 5, c->frames[i].data, c->frames[i].len, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return -1;
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}

static int _tp_insert_content(sqlite3 *db, uint8_t scheme_id, uint8_t pos,
                              const dcu_transparent_content_t *c) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_transparent_content "
                           "(scheme_id, pos, seq, address, pre_script, post_script, "
                           " report_and_wait, wait_timeout, compare_mode, feature_byte, "
                           " cut_start, cut_len) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    sqlite3_bind_int(stmt, 2, pos);
    sqlite3_bind_int(stmt, 3, c->seq);
    sqlite3_bind_blob(stmt, 4, c->address.bytes, c->address.len, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, c->pre_script);
    sqlite3_bind_int(stmt, 6, c->post_script);
    sqlite3_bind_int(stmt, 7, c->ctrl.report_and_wait ? 1 : 0);
    sqlite3_bind_int(stmt, 8, c->ctrl.wait_timeout_s);
    sqlite3_bind_int(stmt, 9, c->ctrl.compare_mode);
    sqlite3_bind_int(stmt, 10, c->ctrl.feature_byte);
    sqlite3_bind_int(stmt, 11, c->ctrl.cut_start);
    sqlite3_bind_int(stmt, 12, c->ctrl.cut_len);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    if (rc != 0) return -1;
    return _tp_insert_frames(db, scheme_id, pos, c);
}

static int _tp_replace_scheme(sqlite3 *db, const dcu_transparent_scheme_t *s) {
    if (_tp_delete_contents(db, s->scheme_id) != 0) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT OR REPLACE INTO dcu_transparent_scheme "
                           "(scheme_id, storage_depth) VALUES (?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, s->scheme_id);
    sqlite3_bind_int(stmt, 2, s->storage_depth);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    if (rc != 0) return -1;
    for (uint8_t i = 0; i < s->content_count; i++) {
        if (_tp_insert_content(db, s->scheme_id, i, &s->contents[i]) != 0) return -1;
    }
    return 0;
}

static int _tp_upsert_content(sqlite3 *db, uint8_t scheme_id,
                              const dcu_transparent_content_t *content) {
    sqlite3_stmt *stmt = NULL;
    int pos = -1;
    uint16_t seq = 0;
    if (sqlite3_prepare_v2(db,
                           "SELECT pos, seq FROM dcu_transparent_content "
                           "WHERE scheme_id = ? AND address = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    sqlite3_bind_blob(stmt, 2, content->address.bytes, content->address.len, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        pos = sqlite3_column_int(stmt, 0);
        seq = (uint16_t)sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    if (pos >= 0) {
        if (sqlite3_prepare_v2(db,
                               "DELETE FROM dcu_transparent_frame WHERE scheme_id = ? AND pos = ?;",
                               -1, &stmt, NULL) != SQLITE_OK) {
            return -1;
        }
        sqlite3_bind_int(stmt, 1, scheme_id);
        sqlite3_bind_int(stmt, 2, pos);
        int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
        sqlite3_finalize(stmt);
        if (rc != 0) return -1;
        dcu_transparent_content_t c = *content;
        c.seq = seq;
        if (_tp_insert_frames(db, scheme_id, (uint8_t)pos, &c) != 0) return -1;
        if (sqlite3_prepare_v2(db,
                               "UPDATE dcu_transparent_content SET "
                               "pre_script = ?, post_script = ?, report_and_wait = ?, "
                               "wait_timeout = ?, compare_mode = ?, feature_byte = ?, "
                               "cut_start = ?, cut_len = ? WHERE scheme_id = ? AND pos = ?;",
                               -1, &stmt, NULL) != SQLITE_OK) {
            return -1;
        }
        const dcu_transparent_ctrl_t *ctrl = &content->ctrl;
        sqlite3_bind_int(stmt, 1, content->pre_script);
        sqlite3_bind_int(stmt, 2, content->post_script);
        sqlite3_bind_int(stmt, 3, ctrl->report_and_wait ? 1 : 0);
        sqlite3_bind_int(stmt, 4, ctrl->wait_timeout_s);
        sqlite3_bind_int(stmt, 5, ctrl->compare_mode);
        sqlite3_bind_int(stmt, 6, ctrl->feature_byte);
        sqlite3_bind_int(stmt, 7, ctrl->cut_start);
        sqlite3_bind_int(stmt, 8, ctrl->cut_len);
        sqlite3_bind_int(stmt, 9, scheme_id);
        sqlite3_bind_int(stmt, 10, pos);
        rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
        sqlite3_finalize(stmt);
        return rc;
    }
    int count = -1;
    int max_pos = -1;
    if (sqlite3_prepare_v2(db,
                           "SELECT COUNT(*), COALESCE(MAX(pos), -1) "
                           "FROM dcu_transparent_content WHERE scheme_id = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
        max_pos = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    if (count < 0 || count >= DCU_MAX_TRANSPARENT_CONTENTS) return EDGE_SLAVE_SRC_TEMPORARY;
    if (sqlite3_prepare_v2(db,
                           "SELECT COALESCE(MAX(seq), 0) + 1 "
                           "FROM dcu_transparent_content WHERE scheme_id = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    if (sqlite3_step(stmt) == SQLITE_ROW) seq = (uint16_t)sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    dcu_transparent_content_t c = *content;
    c.seq = seq;
    return _tp_insert_content(db, scheme_id, (uint8_t)(max_pos + 1), &c);
}

int dcu_scheme_store_transparent_apply(const dcu_transparent_op_t *op) {
    if (!op || !s_storage || !s_storage->db) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3 *db = s_storage->db;
    int rc = 0;
    switch (op->method) {
        case 127:
            rc = _tp_validate_scheme(&op->scheme);
            if (rc != 0) return rc;
            break;
        case 128:
            if (_tp_exists(db, op->scheme_id) != 1) return EDGE_SLAVE_SRC_NOT_FOUND;
            rc = _tp_validate_content(&op->content);
            if (rc != 0) return rc;
            break;
        case 129:
            if (_tp_exists(db, op->scheme_id) != 1) return EDGE_SLAVE_SRC_NOT_FOUND;
            if (op->addr_count == 0 || op->addr_count > DCU_MAX_TRANSPARENT_CONTENTS)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (uint8_t i = 0; i < op->addr_count; i++) {
                sqlite3_stmt *stmt = NULL;
                if (sqlite3_prepare_v2(db,
                                       "SELECT 1 FROM dcu_transparent_content WHERE scheme_id = ? "
                                       "AND address = ?;",
                                       -1, &stmt, NULL) != SQLITE_OK) {
                    return EDGE_SLAVE_SRC_TEMPORARY;
                }
                sqlite3_bind_int(stmt, 1, op->scheme_id);
                sqlite3_bind_blob(stmt, 2, op->addresses[i].bytes, op->addresses[i].len,
                                  SQLITE_TRANSIENT);
                int found = sqlite3_step(stmt) == SQLITE_ROW ? 1 : 0;
                sqlite3_finalize(stmt);
                if (found != 1) return EDGE_SLAVE_SRC_NOT_FOUND;
            }
            break;
        case 130:
            if (op->id_count == 0 || op->id_count > DCU_MAX_TRANSPARENT_SCHEMES)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (uint8_t i = 0; i < op->id_count; i++) {
                if (_tp_exists(db, op->ids[i]) != 1) return EDGE_SLAVE_SRC_NOT_FOUND;
            }
            break;
        case 131:
            break;
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
    if (dcu_scheme_store_begin(db) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    switch (op->method) {
        case 127:
            rc = _tp_replace_scheme(db, &op->scheme);
            break;
        case 128:
            rc = _tp_upsert_content(db, op->scheme_id, &op->content);
            break;
        case 129:
            for (uint8_t i = 0; i < op->addr_count && rc == 0; i++) {
                sqlite3_stmt *stmt = NULL;
                if (sqlite3_prepare_v2(db,
                                       "DELETE FROM dcu_transparent_frame WHERE scheme_id = ? "
                                       "AND pos IN (SELECT pos FROM dcu_transparent_content "
                                       "WHERE scheme_id = ? AND address = ?);",
                                       -1, &stmt, NULL) != SQLITE_OK) {
                    rc = -1;
                    break;
                }
                sqlite3_bind_int(stmt, 1, op->scheme_id);
                sqlite3_bind_int(stmt, 2, op->scheme_id);
                sqlite3_bind_blob(stmt, 3, op->addresses[i].bytes, op->addresses[i].len,
                                  SQLITE_TRANSIENT);
                rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
                sqlite3_finalize(stmt);
                if (rc != 0) break;
                if (sqlite3_prepare_v2(db,
                                       "DELETE FROM dcu_transparent_content WHERE scheme_id = ? "
                                       "AND address = ?;",
                                       -1, &stmt, NULL) != SQLITE_OK) {
                    rc = -1;
                    break;
                }
                sqlite3_bind_int(stmt, 1, op->scheme_id);
                sqlite3_bind_blob(stmt, 2, op->addresses[i].bytes, op->addresses[i].len,
                                  SQLITE_TRANSIENT);
                rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
                sqlite3_finalize(stmt);
            }
            break;
        case 130:
            for (uint8_t i = 0; i < op->id_count && rc == 0; i++) {
                if (_tp_delete_contents(db, op->ids[i]) != 0) {
                    rc = -1;
                    break;
                }
                sqlite3_stmt *stmt = NULL;
                if (sqlite3_prepare_v2(db,
                                       "DELETE FROM dcu_transparent_scheme WHERE scheme_id = ?;",
                                       -1, &stmt, NULL) != SQLITE_OK) {
                    rc = -1;
                    break;
                }
                sqlite3_bind_int(stmt, 1, op->ids[i]);
                rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
                sqlite3_finalize(stmt);
            }
            break;
        case 131:
            rc = dcu_scheme_store_exec(db, "DELETE FROM dcu_transparent_frame;"
                           "DELETE FROM dcu_transparent_content;"
                           "DELETE FROM dcu_transparent_scheme;");
            break;
    }
    if (rc != 0 || dcu_scheme_store_commit(db) != 0) {
        dcu_scheme_store_rollback(db);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

static int _tp_load_content(sqlite3 *db, uint8_t scheme_id, int pos,
                            dcu_transparent_content_t *out) {
    memset(out, 0, sizeof(*out));
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT seq, address, pre_script, post_script, report_and_wait, "
                           "wait_timeout, compare_mode, feature_byte, cut_start, cut_len "
                           "FROM dcu_transparent_content WHERE scheme_id = ? AND pos = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    sqlite3_bind_int(stmt, 2, pos);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    out->seq = (uint16_t)sqlite3_column_int(stmt, 0);
    const void *addr = sqlite3_column_blob(stmt, 1);
    int addr_len = sqlite3_column_bytes(stmt, 1);
    if (addr_len < 2 || addr_len > DCU_TSA_MAX) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    out->address.len = (uint8_t)addr_len;
    memcpy(out->address.bytes, addr, (size_t)addr_len);
    out->pre_script = (uint16_t)sqlite3_column_int(stmt, 2);
    out->post_script = (uint16_t)sqlite3_column_int(stmt, 3);
    out->ctrl.report_and_wait = sqlite3_column_int(stmt, 4) != 0;
    out->ctrl.wait_timeout_s = (uint16_t)sqlite3_column_int(stmt, 5);
    out->ctrl.compare_mode = (uint8_t)sqlite3_column_int(stmt, 6);
    out->ctrl.feature_byte = (uint8_t)sqlite3_column_int(stmt, 7);
    out->ctrl.cut_start = (uint16_t)sqlite3_column_int(stmt, 8);
    out->ctrl.cut_len = (uint16_t)sqlite3_column_int(stmt, 9);
    sqlite3_finalize(stmt);
    if (sqlite3_prepare_v2(db,
                           "SELECT seq, data FROM dcu_transparent_frame "
                           "WHERE scheme_id = ? AND pos = ? ORDER BY frame_pos;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    sqlite3_bind_int(stmt, 2, pos);
    uint8_t frame_pos = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (frame_pos >= DCU_MAX_TRANSPARENT_FRAMES) {
            sqlite3_finalize(stmt);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        dcu_transparent_frame_t *f = &out->frames[frame_pos];
        f->seq = (uint8_t)sqlite3_column_int(stmt, 0);
        const void *data = sqlite3_column_blob(stmt, 1);
        int data_len = sqlite3_column_bytes(stmt, 1);
        if (data_len < 0 || data_len > DCU_MAX_TRANSPARENT_FRAME_LEN) {
            sqlite3_finalize(stmt);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        f->len = (uint16_t)data_len;
        if (data_len > 0) memcpy(f->data, data, (size_t)data_len);
        frame_pos++;
    }
    sqlite3_finalize(stmt);
    out->frame_count = frame_pos;
    return 0;
}

static int _tp_load(sqlite3 *db, uint8_t scheme_id, dcu_transparent_scheme_t *out) {
    memset(out, 0, sizeof(*out));
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT storage_depth FROM dcu_transparent_scheme "
                               "WHERE scheme_id = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    out->scheme_id = scheme_id;
    out->storage_depth = (uint16_t)sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (sqlite3_prepare_v2(db,
                           "SELECT pos FROM dcu_transparent_content WHERE scheme_id = ? "
                           "ORDER BY pos;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    uint8_t content_pos = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (content_pos >= DCU_MAX_TRANSPARENT_CONTENTS) {
            sqlite3_finalize(stmt);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        int pos = sqlite3_column_int(stmt, 0);
        int rc = _tp_load_content(db, scheme_id, pos, &out->contents[content_pos]);
        if (rc != 0) {
            sqlite3_finalize(stmt);
            return rc;
        }
        content_pos++;
    }
    sqlite3_finalize(stmt);
    out->content_count = content_pos;
    return 0;
}

int dcu_scheme_store_transparent_count(void) {
    if (!s_storage || !s_storage->db) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_storage->db, "SELECT COUNT(*) FROM dcu_transparent_scheme;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) rc = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_scheme_store_transparent_at(uint16_t pos, dcu_transparent_scheme_t *out) {
    if (!s_storage || !s_storage->db || !out) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_storage->db,
                           "SELECT scheme_id FROM dcu_transparent_scheme ORDER BY scheme_id "
                           "LIMIT 1 OFFSET ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, pos);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    uint8_t id = (uint8_t)sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return _tp_load(s_storage->db, id, out);
}

int dcu_scheme_store_transparent_get(uint8_t scheme_id, dcu_transparent_scheme_t *out) {
    if (!s_storage || !s_storage->db || !out) return EDGE_SLAVE_SRC_TEMPORARY;
    return _tp_load(s_storage->db, scheme_id, out);
}

dcu_transparent_result_t *dcu_scheme_store_result_buf(void) {
    return &s_result_buf;
}

int dcu_scheme_store_result_append(const dcu_transparent_result_t *r) {
    if (!r || !s_storage || !s_storage->db) return EDGE_SLAVE_SRC_TEMPORARY;
    if (r->scheme_id == 0) return EDGE_SLAVE_SRC_DENIED;
    if (r->address.len < 2 || r->address.len > DCU_TSA_MAX)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (r->item_count > DCU_MAX_RESULT_ITEMS) return EDGE_SLAVE_SRC_TEMPORARY;
    for (uint8_t i = 0; i < r->item_count; i++) {
        if (r->items[i].len > DCU_MAX_TRANSPARENT_FRAME_LEN)
            return EDGE_SLAVE_SRC_TEMPORARY;
    }
    /* 601B 的元素索引是一字节，结果数超过 255 后无法再寻址。 */
    int stored = dcu_scheme_store_result_count();
    if (stored < 0) return EDGE_SLAVE_SRC_TEMPORARY;
    if (stored >= 255) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3 *db = s_storage->db;
    if (dcu_scheme_store_begin(db) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_transparent_result "
                           "(scheme_id, exec_time, address) VALUES (?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        dcu_scheme_store_rollback(db);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, r->scheme_id);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)r->exec_time);
    sqlite3_bind_blob(stmt, 3, r->address.bytes, r->address.len, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    sqlite3_int64 result_id = rc == 0 ? sqlite3_last_insert_rowid(db) : 0;
    for (uint8_t i = 0; i < r->item_count && rc == 0; i++) {
        if (sqlite3_prepare_v2(db,
                               "INSERT INTO dcu_transparent_result_item "
                               "(result_id, frame_pos, seq, resp_time, data) "
                               "VALUES (?, ?, ?, ?, ?);",
                               -1, &stmt, NULL) != SQLITE_OK) {
            rc = -1;
            break;
        }
        sqlite3_bind_int64(stmt, 1, result_id);
        sqlite3_bind_int(stmt, 2, i);
        sqlite3_bind_int(stmt, 3, r->items[i].seq);
        sqlite3_bind_int64(stmt, 4, (sqlite3_int64)r->items[i].resp_time);
        sqlite3_bind_blob(stmt, 5, r->items[i].data, r->items[i].len, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) rc = -1;
        sqlite3_finalize(stmt);
    }
    if (rc != 0 || dcu_scheme_store_commit(db) != 0) {
        dcu_scheme_store_rollback(db);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

static int _res_load(sqlite3 *db, sqlite3_int64 result_id, dcu_transparent_result_t *out) {
    memset(out, 0, sizeof(*out));
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT scheme_id, exec_time, address FROM dcu_transparent_result "
                           "WHERE result_id = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int64(stmt, 1, result_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    out->result_id = (uint32_t)result_id;
    out->scheme_id = (uint8_t)sqlite3_column_int(stmt, 0);
    out->exec_time = (uint64_t)sqlite3_column_int64(stmt, 1);
    const void *addr = sqlite3_column_blob(stmt, 2);
    int addr_len = sqlite3_column_bytes(stmt, 2);
    if (addr_len < 2 || addr_len > DCU_TSA_MAX) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    out->address.len = (uint8_t)addr_len;
    memcpy(out->address.bytes, addr, (size_t)addr_len);
    sqlite3_finalize(stmt);
    if (sqlite3_prepare_v2(db,
                           "SELECT seq, resp_time, data FROM dcu_transparent_result_item "
                           "WHERE result_id = ? ORDER BY frame_pos;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int64(stmt, 1, result_id);
    uint8_t item_pos = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (item_pos >= DCU_MAX_RESULT_ITEMS) {
            sqlite3_finalize(stmt);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        dcu_transparent_result_item_t *item = &out->items[item_pos];
        item->seq = (uint8_t)sqlite3_column_int(stmt, 0);
        item->resp_time = (uint64_t)sqlite3_column_int64(stmt, 1);
        const void *data = sqlite3_column_blob(stmt, 2);
        int data_len = sqlite3_column_bytes(stmt, 2);
        if (data_len < 0 || data_len > DCU_MAX_TRANSPARENT_FRAME_LEN) {
            sqlite3_finalize(stmt);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        item->len = (uint16_t)data_len;
        if (data_len > 0) memcpy(item->data, data, (size_t)data_len);
        item_pos++;
    }
    sqlite3_finalize(stmt);
    out->item_count = item_pos;
    return 0;
}

int dcu_scheme_store_result_count(void) {
    if (!s_storage || !s_storage->db) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_storage->db, "SELECT COUNT(*) FROM dcu_transparent_result;", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) rc = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_scheme_store_result_at(uint16_t pos, dcu_transparent_result_t *out) {
    if (!s_storage || !s_storage->db || !out) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(s_storage->db,
                           "SELECT result_id FROM dcu_transparent_result ORDER BY result_id "
                           "LIMIT 1 OFFSET ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, pos);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    sqlite3_int64 result_id = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return _res_load(s_storage->db, result_id, out);
}

int dcu_scheme_store_result_get(uint16_t ordinal, dcu_transparent_result_t *out) {
    return dcu_scheme_store_result_at(ordinal == 0 ? 0 : (uint16_t)(ordinal - 1), out);
}

