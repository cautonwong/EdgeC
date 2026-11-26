#include "dcu_rule_store.h"

#include <sqlite3.h>
#include <string.h>

#include "dcu_scheme_store_internal.h"
#include "edge_log.h"

static const char *const k_schema =
    "CREATE TABLE IF NOT EXISTS dcu_rule ("
    " oad INTEGER NOT NULL, protocol INTEGER NOT NULL, role INTEGER NOT NULL,"
    " protocol_id INTEGER NOT NULL, scale REAL NOT NULL, length INTEGER NOT NULL,"
    " frame BLOB,"
    " PRIMARY KEY (oad, protocol, role));"
    "CREATE TABLE IF NOT EXISTS dcu_rule_meta (k TEXT PRIMARY KEY, v INTEGER NOT NULL);";

static int _is_seeded(sqlite3 *db) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT v FROM dcu_rule_meta WHERE k = 'seeded';", -1, &stmt, NULL) !=
        SQLITE_OK) {
        return -1;
    }
    int seeded = sqlite3_step(stmt) == SQLITE_ROW ? 1 : 0;
    sqlite3_finalize(stmt);
    return seeded;
}

static int _insert_seed(sqlite3 *db, const dcu_rule_row_t *seed, uint16_t count) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT OR IGNORE INTO dcu_rule "
                           "(oad, protocol, role, protocol_id, scale, length) "
                           "VALUES (?, ?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    for (uint16_t i = 0; i < count; i++) {
        sqlite3_reset(stmt);
        sqlite3_bind_int64(stmt, 1, (sqlite3_int64)seed[i].oad);
        sqlite3_bind_int(stmt, 2, seed[i].protocol);
        sqlite3_bind_int(stmt, 3, seed[i].role);
        sqlite3_bind_int64(stmt, 4, (sqlite3_int64)seed[i].protocol_id);
        sqlite3_bind_double(stmt, 5, (double)seed[i].scale);
        sqlite3_bind_int(stmt, 6, seed[i].length);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            return -1;
        }
    }
    sqlite3_finalize(stmt);
    return 0;
}

/* 旧库的 dcu_rule 已存在时 CREATE TABLE IF NOT EXISTS 不补列：缺 frame 需单独 ALTER。 */
static int _ensure_frame_column(sqlite3 *db) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "PRAGMA table_info(dcu_rule);", -1, &stmt, NULL) != SQLITE_OK) return -1;
    int has_frame = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        if (name && strcmp((const char *)name, "frame") == 0) has_frame = 1;
    }
    sqlite3_finalize(stmt);
    if (has_frame) return 0;
    return dcu_scheme_store_exec(db, "ALTER TABLE dcu_rule ADD COLUMN frame BLOB;");
}

int dcu_rule_store_init(dcu_storage_t *storage, const dcu_rule_row_t *seed, uint16_t count) {
    if (!storage || !storage->db || !seed) return -1;
    sqlite3 *db = storage->db;
    if (dcu_scheme_store_exec(db, k_schema) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "DCU rules: 建表失败");
        return -1;
    }
    if (_ensure_frame_column(db) != 0) return -1;
    int seeded = _is_seeded(db);
    if (seeded < 0) return -1;
    if (seeded) return 0;

    if (dcu_scheme_store_begin(db) != 0) return -1;
    int rc = _insert_seed(db, seed, count);
    if (rc == 0) {
        rc = dcu_scheme_store_exec(db, "INSERT OR REPLACE INTO dcu_rule_meta (k, v) "
                                        "VALUES ('seeded', 1);");
    }
    if (rc != 0 || dcu_scheme_store_commit(db) != 0) {
        dcu_scheme_store_rollback(db);
        return -1;
    }
    return 0;
}

int dcu_rule_store_load(dcu_storage_t *storage, dcu_rule_cache_t *out) {
    if (!storage || !storage->db || !out) return -1;
    memset(out, 0, sizeof(*out));
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(storage->db,
                           "SELECT oad, protocol, role, protocol_id, scale, length FROM dcu_rule "
                           "ORDER BY oad, protocol, role;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    int rc = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (out->count >= DCU_RULE_CACHE_MAX) {
            rc = -1;
            break;
        }
        dcu_rule_row_t *row = &out->rows[out->count++];
        row->oad = (uint32_t)sqlite3_column_int64(stmt, 0);
        row->protocol = (uint8_t)sqlite3_column_int(stmt, 1);
        row->role = (uint8_t)sqlite3_column_int(stmt, 2);
        row->protocol_id = (uint32_t)sqlite3_column_int64(stmt, 3);
        row->scale = (float)sqlite3_column_double(stmt, 4);
        row->length = (uint8_t)sqlite3_column_int(stmt, 5);
    }
    sqlite3_finalize(stmt);
    return rc;
}

static int _seed_covers(const dcu_rule_row_t *seed, uint16_t count, uint32_t oad) {
    for (uint16_t i = 0; i < count; i++)
        if (seed[i].oad == oad) return 1;
    return 0;
}

static int _scale_for(sqlite3 *db, const dcu_rule_row_t *seed, uint16_t seed_count, uint32_t oad,
                      float *scale, uint8_t *length) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT scale, length FROM dcu_rule "
                           "WHERE oad = ? AND protocol = ? AND role = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)oad);
    sqlite3_bind_int(stmt, 2, DCU_RULE_DLT645_2007);
    sqlite3_bind_int(stmt, 3, DCU_RULE_MAIN);
    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *scale = (float)sqlite3_column_double(stmt, 0);
        *length = (uint8_t)sqlite3_column_int(stmt, 1);
        found = 1;
    }
    sqlite3_finalize(stmt);
    if (found) return 1;
    for (uint16_t i = 0; i < seed_count; i++) {
        if (seed[i].oad != oad) continue;
        if (seed[i].protocol != DCU_RULE_DLT645_2007 || seed[i].role != DCU_RULE_MAIN) continue;
        *scale = seed[i].scale;
        *length = seed[i].length;
        return 1;
    }
    return 0;
}

static int _total_rows(sqlite3 *db, int *out) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM dcu_rule;", -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    int rc = sqlite3_step(stmt) == SQLITE_ROW ? 0 : -1;
    if (rc == 0) *out = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return rc;
}

static int _std_rows(sqlite3 *db, uint32_t oad, int *out) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT COUNT(*) FROM dcu_rule "
                           "WHERE oad = ? AND protocol IN (1, 2, 5);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)oad);
    int rc = sqlite3_step(stmt) == SQLITE_ROW ? 0 : -1;
    if (rc == 0) *out = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return rc;
}

static int _proto_owner(sqlite3 *db, uint8_t protocol, uint32_t protocol_id, uint32_t oad) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT 1 FROM dcu_rule "
                           "WHERE protocol = ? AND protocol_id = ? AND oad <> ? LIMIT 1;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, protocol);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)protocol_id);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)oad);
    int owner = sqlite3_step(stmt) == SQLITE_ROW ? 1 : 0;
    sqlite3_finalize(stmt);
    return owner;
}

static int _batch_dup(const dcu_rule_op_t *op, uint8_t upto, uint8_t protocol, uint32_t protocol_id) {
    for (uint8_t j = 0; j < upto; j++) {
        const dcu_rule_unit_t *u = &op->units[j];
        if (protocol == DCU_RULE_DLT645_2007) {
            if ((u->has_2007_main && u->di_2007_main == protocol_id) ||
                (u->has_2007_spare && u->di_2007_spare == protocol_id))
                return 1;
        } else if (protocol == DCU_RULE_DLT645_1997) {
            if ((u->has_1997_main && u->di_1997_main == protocol_id) ||
                (u->has_1997_spare && u->di_1997_spare == protocol_id))
                return 1;
        }
    }
    return 0;
}

static int _id_taken(sqlite3 *db, const dcu_rule_op_t *op, uint8_t upto, uint8_t protocol,
                     uint32_t protocol_id, uint32_t oad) {
    int owner = _proto_owner(db, protocol, protocol_id, oad);
    if (owner < 0) return EDGE_SLAVE_SRC_TEMPORARY;
    if (owner != 0 || _batch_dup(op, upto, protocol, protocol_id)) return EDGE_SLAVE_SRC_DENIED;
    return 0;
}

static int _internal_for_csd(sqlite3 *db, uint32_t csd, uint32_t *oad) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT oad FROM dcu_rule "
                           "WHERE protocol = ? AND role = ? AND protocol_id = ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, DCU_RULE_DLT698);
    sqlite3_bind_int(stmt, 2, DCU_RULE_MAIN);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)csd);
    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *oad = (uint32_t)sqlite3_column_int64(stmt, 0);
        found = 1;
    }
    sqlite3_finalize(stmt);
    return found;
}

static int _delete_std_rows(sqlite3 *db, uint32_t oad) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM dcu_rule WHERE oad = ? AND protocol IN (1, 2, 5);", -1,
                           &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)oad);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _insert_rule(sqlite3 *db, uint32_t oad, uint8_t protocol, uint8_t role,
                        uint32_t protocol_id, float scale, uint8_t length, const uint8_t *frame,
                        uint8_t frame_len) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_rule "
                           "(oad, protocol, role, protocol_id, scale, length, frame) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)oad);
    sqlite3_bind_int(stmt, 2, protocol);
    sqlite3_bind_int(stmt, 3, role);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)protocol_id);
    sqlite3_bind_double(stmt, 5, (double)scale);
    sqlite3_bind_int(stmt, 6, length);
    if (frame_len > 0) {
        sqlite3_bind_blob(stmt, 7, frame, frame_len, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 7);
    }
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _replace_unit(sqlite3 *db, const dcu_rule_unit_t *u, uint32_t oad, float scale,
                         uint8_t length) {
    if (_delete_std_rows(db, oad) != 0) return -1;
    int has_trans = u->has_trans || u->frame_len > 0;
    if (u->has_2007_main &&
        _insert_rule(db, oad, DCU_RULE_DLT645_2007, DCU_RULE_MAIN, u->di_2007_main, scale,
                     length, NULL, 0) != 0)
        return -1;
    if (u->has_2007_spare &&
        _insert_rule(db, oad, DCU_RULE_DLT645_2007, DCU_RULE_SPARE, u->di_2007_spare, scale,
                     length, NULL, 0) != 0)
        return -1;
    if (u->has_1997_main &&
        _insert_rule(db, oad, DCU_RULE_DLT645_1997, DCU_RULE_MAIN, u->di_1997_main, scale,
                     length, NULL, 0) != 0)
        return -1;
    if (u->has_1997_spare &&
        _insert_rule(db, oad, DCU_RULE_DLT645_1997, DCU_RULE_SPARE, u->di_1997_spare, scale,
                     length, NULL, 0) != 0)
        return -1;
    if (has_trans && _insert_rule(db, oad, DCU_RULE_TRANS, DCU_RULE_MAIN, 0, 0.0f, 0, u->frame,
                                  u->frame_len) != 0)
        return -1;
    return 0;
}

int dcu_rule_store_apply(dcu_storage_t *storage, const dcu_rule_row_t *seed, uint16_t seed_count,
                         const dcu_rule_op_t *op) {
    if (!storage || !storage->db || !op) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    sqlite3 *db = storage->db;

    if (op->method == 127) {
        if (op->unit_count == 0 || op->unit_count > DCU_RULE_BATCH_MAX) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        int total = 0;
        if (_total_rows(db, &total) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        int delta = 0;
        uint32_t internals[DCU_RULE_BATCH_MAX];
        for (uint8_t i = 0; i < op->unit_count; i++) {
            const dcu_rule_unit_t *u = &op->units[i];
            uint32_t oad = u->oad;
            if (u->csd != 0) {
                int mapped = _internal_for_csd(db, u->csd, &oad);
                if (mapped <= 0) return EDGE_SLAVE_SRC_TEMPORARY;
            }
            internals[i] = oad;
            if (!_seed_covers(seed, seed_count, oad)) return EDGE_SLAVE_SRC_TEMPORARY;
            for (uint8_t j = 0; j < i; j++) {
                if (internals[j] == oad) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            }
            int has07 = u->has_2007_main || u->has_2007_spare;
            int has97 = u->has_1997_main || u->has_1997_spare;
            int has_trans = u->has_trans || u->frame_len > 0;
            if (!has07 && !has97 && !has_trans) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (has07 && !u->has_2007_main) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            float scale = 0.0f;
            uint8_t length = 0;
            if ((has07 || has97) && !_scale_for(db, seed, seed_count, oad, &scale, &length)) {
                return EDGE_SLAVE_SRC_TEMPORARY;
            }
            int rc = 0;
            if (u->has_2007_main)
                rc = _id_taken(db, op, i, DCU_RULE_DLT645_2007, u->di_2007_main, oad);
            if (rc == 0 && u->has_2007_spare)
                rc = _id_taken(db, op, i, DCU_RULE_DLT645_2007, u->di_2007_spare, oad);
            if (rc == 0 && u->has_1997_main)
                rc = _id_taken(db, op, i, DCU_RULE_DLT645_1997, u->di_1997_main, oad);
            if (rc == 0 && u->has_1997_spare)
                rc = _id_taken(db, op, i, DCU_RULE_DLT645_1997, u->di_1997_spare, oad);
            if (rc != 0) return rc;
            int existing = 0;
            if (_std_rows(db, oad, &existing) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            delta += (int)(u->has_2007_main + u->has_2007_spare + u->has_1997_main +
                           u->has_1997_spare + has_trans) -
                     existing;
        }
        if (total + delta > (int)DCU_RULE_CACHE_MAX) return EDGE_SLAVE_SRC_TEMPORARY;

        if (dcu_scheme_store_begin(db) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        int rc = 0;
        for (uint8_t i = 0; i < op->unit_count && rc == 0; i++) {
            const dcu_rule_unit_t *u = &op->units[i];
            float scale = 0.0f;
            uint8_t length = 0;
            if (u->has_2007_main || u->has_2007_spare || u->has_1997_main || u->has_1997_spare)
                (void)_scale_for(db, seed, seed_count, internals[i], &scale, &length);
            rc = _replace_unit(db, u, internals[i], scale, length);
        }
        if (rc != 0 || dcu_scheme_store_commit(db) != 0) {
            dcu_scheme_store_rollback(db);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        return 0;
    }

    if (op->method == 128) {
        if (op->csd_count == 0 || op->csd_count > DCU_RULE_BATCH_MAX) {
            return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        }
        uint32_t internals[DCU_RULE_BATCH_MAX];
        for (uint8_t i = 0; i < op->csd_count; i++) {
            int mapped = _internal_for_csd(db, op->csds[i], &internals[i]);
            if (mapped <= 0) return EDGE_SLAVE_SRC_TEMPORARY;
            int rows = 0;
            if (_std_rows(db, internals[i], &rows) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
            if (rows == 0) return EDGE_SLAVE_SRC_NOT_FOUND;
        }
        if (dcu_scheme_store_begin(db) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        int rc = 0;
        for (uint8_t i = 0; i < op->csd_count && rc == 0; i++)
            rc = _delete_std_rows(db, internals[i]);
        if (rc != 0 || dcu_scheme_store_commit(db) != 0) {
            dcu_scheme_store_rollback(db);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        return 0;
    }

    if (op->method == 129) {
        if (dcu_scheme_store_begin(db) != 0) return EDGE_SLAVE_SRC_TEMPORARY;
        if (dcu_scheme_store_exec(db, "DELETE FROM dcu_rule WHERE protocol IN (1, 2, 5);") != 0 ||
            dcu_scheme_store_commit(db) != 0) {
            dcu_scheme_store_rollback(db);
            return EDGE_SLAVE_SRC_TEMPORARY;
        }
        return 0;
    }
    return EDGE_SLAVE_SRC_CLASS_MISMATCH;
}

int dcu_rule_store_projected_count(dcu_storage_t *storage, uint16_t *out) {
    if (!storage || !storage->db || !out) return -2;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(storage->db,
                           "SELECT COUNT(*) FROM ("
                           " SELECT DISTINCT r.oad FROM dcu_rule AS r"
                           " JOIN dcu_rule AS w ON w.oad = r.oad AND w.protocol = 3 AND w.role = 0"
                           " WHERE r.protocol IN (1, 2, 5));",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -2;
    }
    int rc = sqlite3_step(stmt) == SQLITE_ROW ? 0 : -2;
    if (rc == 0) {
        int n = sqlite3_column_int(stmt, 0);
        *out = (uint16_t)n;
    }
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_rule_store_projected_oad(dcu_storage_t *storage, uint16_t pos, uint32_t *out) {
    if (!storage || !storage->db || !out) return -2;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(storage->db,
                           "SELECT r.oad FROM dcu_rule AS r"
                           " JOIN dcu_rule AS w ON w.oad = r.oad AND w.protocol = 3 AND w.role = 0"
                           " WHERE r.protocol IN (1, 2, 5)"
                           " GROUP BY r.oad ORDER BY w.protocol_id LIMIT 1 OFFSET ?;",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -2;
    }
    sqlite3_bind_int(stmt, 1, pos);
    int rc = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *out = (uint32_t)sqlite3_column_int64(stmt, 0);
        rc = 0;
    }
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_rule_store_unit_load(dcu_storage_t *storage, uint32_t oad, dcu_rule_unit_t *out) {
    if (!storage || !storage->db || !out) return -2;
    memset(out, 0, sizeof(*out));
    out->oad = oad;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(storage->db,
                           "SELECT protocol, role, protocol_id, frame FROM dcu_rule "
                           "WHERE oad = ? AND protocol IN (1, 2, 3, 5);",
                           -1, &stmt, NULL) != SQLITE_OK) {
        return -2;
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)oad);
    int found = 0;
    int rc = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = 1;
        uint8_t protocol = (uint8_t)sqlite3_column_int(stmt, 0);
        uint8_t role = (uint8_t)sqlite3_column_int(stmt, 1);
        uint32_t pid = (uint32_t)sqlite3_column_int64(stmt, 2);
        if (protocol == DCU_RULE_DLT645_2007) {
            if (role == DCU_RULE_MAIN) {
                out->has_2007_main = 1;
                out->di_2007_main = pid;
            } else {
                out->has_2007_spare = 1;
                out->di_2007_spare = pid;
            }
        } else if (protocol == DCU_RULE_DLT645_1997) {
            if (role == DCU_RULE_MAIN) {
                out->has_1997_main = 1;
                out->di_1997_main = (uint16_t)pid;
            } else {
                out->has_1997_spare = 1;
                out->di_1997_spare = (uint16_t)pid;
            }
        } else if (protocol == DCU_RULE_DLT698) {
            if (role == DCU_RULE_MAIN) out->csd = pid;
        } else if (protocol == DCU_RULE_TRANS) {
            const void *blob = sqlite3_column_blob(stmt, 3);
            int bytes = sqlite3_column_bytes(stmt, 3);
            if (bytes < 0 || bytes > DCU_RULE_FRAME_MAX) {
                rc = -2;
                break;
            }
            out->has_trans = 1;
            out->frame_len = (uint8_t)bytes;
            if (bytes > 0 && blob) memcpy(out->frame, blob, (size_t)bytes);
        }
    }
    sqlite3_finalize(stmt);
    if (rc != 0) return rc;
    if (!found) return -1;
    if (out->csd == 0) return -2; /* 没有 698 行就给不出 CSD，不能上 601E/601F */
    return 0;
}
