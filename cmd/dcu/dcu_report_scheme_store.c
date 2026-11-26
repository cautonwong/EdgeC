#include <string.h>

#include "dcu_scheme_store.h"
#include "dcu_scheme_store_internal.h"
#include "edge_log.h"
#include "edge_slave_plane.h"

static dcu_report_op_t s_report_op;
static dcu_storage_t *s_storage;

void dcu_scheme_store_report_bind(dcu_storage_t *storage) {
    s_storage = storage;
}

dcu_report_op_t *dcu_scheme_store_report_op(void) {
    return &s_report_op;
}

static int _rp_exists(sqlite3 *db, uint8_t scheme_id) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM dcu_report_scheme WHERE scheme_id = ?;", -1, &stmt,
                           NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    int rc = sqlite3_step(stmt) == SQLITE_ROW ? 1 : 0;
    sqlite3_finalize(stmt);
    return rc;
}

static int _rp_validate(const dcu_report_scheme_cfg_t *u) {
    if (u->scheme_id == 0) {
        LOG_WARN(LOG_CAT_NB, "DCU 601C: 方案编号 0 是保留值（OAD 元素 0 = 最小现有编号）");
        return EDGE_SLAVE_SRC_DENIED;
    }
    if (u->channel_count == 0 || u->channel_count > DCU_MAX_REPORT_CHANNELS) {
        LOG_WARN(LOG_CAT_NB, "DCU 601C: 方案 %u 的上报通道个数 %u 无效", (unsigned)u->scheme_id,
                 (unsigned)u->channel_count);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    for (uint8_t i = 0; i < u->channel_count; i++) {
        if (u->channels[i] == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    }
    if (u->timeout_unit > 3) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (u->content_type == DCU_REPORT_CONTENT_RECORD) {
        LOG_WARN(LOG_CAT_NB, "DCU 601C: 方案 %u 的记录型上报（RCSD/RSD）未实现",
                 (unsigned)u->scheme_id);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (u->content_type != DCU_REPORT_CONTENT_OAD || u->content_oad == 0)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    return 0;
}

static int _rp_delete(sqlite3 *db, uint8_t scheme_id) {
    static const char *const sql[] = {
        "DELETE FROM dcu_report_channel WHERE scheme_id = ?;",
        "DELETE FROM dcu_report_scheme WHERE scheme_id = ?;",
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

static int _rp_replace(sqlite3 *db, const dcu_report_scheme_cfg_t *u) {
    if (_rp_delete(db, u->scheme_id) != 0) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_report_scheme "
                           "(scheme_id, timeout_unit, timeout_value, max_times, content_type, "
                           " content_oad) VALUES (?, ?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, u->scheme_id);
    sqlite3_bind_int(stmt, 2, u->timeout_unit);
    sqlite3_bind_int(stmt, 3, u->timeout_value);
    sqlite3_bind_int(stmt, 4, u->max_times);
    sqlite3_bind_int(stmt, 5, (int)u->content_type);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)u->content_oad);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    if (rc != 0) return -1;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_report_channel (scheme_id, pos, oad) VALUES (?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    for (uint8_t i = 0; i < u->channel_count; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, u->scheme_id);
        sqlite3_bind_int(stmt, 2, i);
        sqlite3_bind_int64(stmt, 3, (sqlite3_int64)u->channels[i]);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return -1;
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}

int dcu_scheme_store_report_apply(const dcu_report_op_t *op) {
    dcu_storage_t *st = s_storage;
    if (!op || !st || !st->db) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3 *db = st->db;
    int rc = 0;
    switch (op->method) {
        case 127:
            if (op->unit_count == 0 || op->unit_count > DCU_MAX_REPORT_SCHEMES)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (uint8_t i = 0; i < op->unit_count; i++) {
                rc = _rp_validate(&op->units[i]);
                if (rc != 0) return rc;
                for (uint8_t j = 0; j < i; j++) {
                    if (op->units[j].scheme_id == op->units[i].scheme_id)
                        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
                }
            }
            break;
        case 128:
            if (op->id_count == 0 || op->id_count > DCU_MAX_REPORT_SCHEMES)
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            for (uint8_t i = 0; i < op->id_count; i++) {
                if (_rp_exists(db, op->ids[i]) != 1) return EDGE_SLAVE_SRC_NOT_FOUND;
            }
            break;
        case 129:
            break;
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
    if (dcu_scheme_store_begin(db) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    switch (op->method) {
        case 127:
            for (uint8_t i = 0; i < op->unit_count && rc == 0; i++)
                rc = _rp_replace(db, &op->units[i]);
            break;
        case 128:
            for (uint8_t i = 0; i < op->id_count && rc == 0; i++) rc = _rp_delete(db, op->ids[i]);
            break;
        case 129:
            rc = dcu_scheme_store_exec(
                db, "DELETE FROM dcu_report_channel; DELETE FROM dcu_report_scheme;");
            break;
    }
    if (rc != 0 || dcu_scheme_store_commit(db) != 0) {
        dcu_scheme_store_rollback(db);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    return 0;
}

static int _rp_load(sqlite3 *db, uint8_t scheme_id, dcu_report_scheme_cfg_t *out) {
    memset(out, 0, sizeof(*out));
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT timeout_unit, timeout_value, max_times, content_type, "
                           "content_oad FROM dcu_report_scheme WHERE scheme_id = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return EDGE_SLAVE_SRC_NOT_FOUND;
    }
    out->scheme_id = scheme_id;
    out->timeout_unit = (uint8_t)sqlite3_column_int(stmt, 0);
    out->timeout_value = (uint16_t)sqlite3_column_int(stmt, 1);
    out->max_times = (uint8_t)sqlite3_column_int(stmt, 2);
    out->content_type = (dcu_report_content_t)sqlite3_column_int(stmt, 3);
    out->content_oad = (uint32_t)sqlite3_column_int64(stmt, 4);
    sqlite3_finalize(stmt);
    if (sqlite3_prepare_v2(db,
                           "SELECT oad FROM dcu_report_channel WHERE scheme_id = ? ORDER BY pos;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    sqlite3_bind_int(stmt, 1, scheme_id);
    uint8_t pos = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (pos >= DCU_MAX_REPORT_CHANNELS) {
            sqlite3_finalize(stmt);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        out->channels[pos++] = (uint32_t)sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    out->channel_count = pos;
    return 0;
}

int dcu_scheme_store_report_count(void) {
    dcu_storage_t *st = s_storage;
    if (!st || !st->db) return -1;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(st->db, "SELECT COUNT(*) FROM dcu_report_scheme;", -1, &stmt, NULL) !=
        SQLITE_OK) {
        return -1;
    }
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) rc = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_scheme_store_report_at(uint16_t pos, dcu_report_scheme_cfg_t *out) {
    dcu_storage_t *st = s_storage;
    if (!st || !st->db || !out) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(st->db,
                           "SELECT scheme_id FROM dcu_report_scheme ORDER BY scheme_id "
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
    return _rp_load(st->db, id, out);
}

int dcu_scheme_store_report_get(uint8_t scheme_id, dcu_report_scheme_cfg_t *out) {
    dcu_storage_t *st = s_storage;
    if (!st || !st->db || !out) return EDGE_SLAVE_SRC_TEMPORARY;
    return _rp_load(st->db, scheme_id, out);
}
