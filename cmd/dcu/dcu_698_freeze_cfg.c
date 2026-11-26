#include "dcu_698_freeze_cfg.h"

#include <sqlite3.h>
#include <string.h>

#include "dcu_router.h"
#include "edge_log.h"
#include "dlt698_codec.h"

/* 属性 3 是 Class 9 的静态配置；默认集只在主站没动过该 OI 时生效。 */
static const uint32_t k_default_oads[] = {OAD_ACTIVE_ENERGY, OAD_VOLTAGE, OAD_CURRENT};

static dcu_storage_t *s_storage;
static dcu_freeze_cfgs_t *s_cfgs;
static dcu_freeze_scheduler_t *s_sched;

static void _default_one(dcu_freeze_cfg_t *cfg, uint16_t oi) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->oi = oi;
    cfg->count = (uint8_t)(sizeof(k_default_oads) / sizeof(k_default_oads[0]));
    for (uint8_t i = 0; i < cfg->count; i++) {
        cfg->items[i].oad = k_default_oads[i];
        cfg->items[i].period = 1;
        cfg->items[i].depth = 1;
    }
}

void dcu_freeze_cfgs_init(dcu_freeze_cfgs_t *cfgs) {
    if (!cfgs) return;
    for (uint8_t i = 0; i < DCU_FREEZE_OIS; i++) _default_one(&cfgs->ois[i], (uint16_t)(0x5000 + i));
}

const dcu_freeze_cfg_t *dcu_freeze_cfg_find(const dcu_freeze_cfgs_t *cfgs, uint16_t oi) {
    if (!cfgs || oi < 0x5000 || oi > 0x5000 + DCU_FREEZE_OIS - 1) return NULL;
    return &cfgs->ois[oi - 0x5000];
}

dcu_freeze_cfg_t *dcu_freeze_cfg_mut(dcu_freeze_cfgs_t *cfgs, uint16_t oi) {
    if (!cfgs || oi < 0x5000 || oi > 0x5000 + DCU_FREEZE_OIS - 1) return NULL;
    return &cfgs->ois[oi - 0x5000];
}

static int _exec(sqlite3 *db, const char *sql) {
    return sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

static int _ensure_schema(sqlite3 *db) {
    if (_exec(db,
              "CREATE TABLE IF NOT EXISTS dcu_freeze_attr3 ("
              "oi INTEGER NOT NULL, seq INTEGER NOT NULL, oad INTEGER NOT NULL, "
              "period INTEGER NOT NULL, depth INTEGER NOT NULL, PRIMARY KEY(oi, oad));") != 0)
        return -1;
    return _exec(db,
                 "CREATE TABLE IF NOT EXISTS dcu_freeze_attr3_meta ("
                 "oi INTEGER PRIMARY KEY, configured INTEGER NOT NULL);");
}

static bool _meta_configured(sqlite3 *db, uint16_t oi) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "SELECT configured FROM dcu_freeze_attr3_meta WHERE oi = ?;", -1,
                           &stmt, NULL) != SQLITE_OK)
        return false;
    sqlite3_bind_int(stmt, 1, oi);
    bool configured = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return configured;
}

static int _load_rows(sqlite3 *db, dcu_freeze_cfg_t *cfg) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "SELECT oad, period, depth FROM dcu_freeze_attr3 WHERE oi = ? "
                           "ORDER BY seq ASC;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt, 1, cfg->oi);
    cfg->configured = true;
    cfg->truncated = false;
    cfg->count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (cfg->count >= DCU_FREEZE_CFG_MAX) {
            cfg->truncated = true;
            continue;
        }
        dcu_freeze_attr_t *item = &cfg->items[cfg->count++];
        item->oad = (uint32_t)sqlite3_column_int64(stmt, 0);
        item->period = (uint32_t)sqlite3_column_int64(stmt, 1);
        item->depth = (uint32_t)sqlite3_column_int64(stmt, 2);
    }
    if (cfg->truncated) {
        LOG_WARN(LOG_CAT_CORE, "DCU freeze attr3 oi=%04X: DB has more than %u rows; extra ignored",
                 cfg->oi, (unsigned)DCU_FREEZE_CFG_MAX);
    }
    sqlite3_finalize(stmt);
    return 0;
}

static int _reload_one(dcu_freeze_cfgs_t *cfgs, dcu_storage_t *storage, uint16_t oi) {
    dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_mut(cfgs, oi);
    if (!cfg || !storage || !storage->db) return -1;
    if (!_meta_configured(storage->db, oi)) {
        _default_one(cfg, oi);
        return 0;
    }
    return _load_rows(storage->db, cfg);
}

int dcu_freeze_cfgs_load(dcu_freeze_cfgs_t *cfgs, dcu_storage_t *storage) {
    if (!cfgs || !storage || !storage->db) return -1;
    if (_ensure_schema(storage->db) != 0) return -1;
    dcu_freeze_cfgs_init(cfgs);
    for (uint8_t i = 0; i < DCU_FREEZE_OIS; i++) {
        if (_reload_one(cfgs, storage, (uint16_t)(0x5000 + i)) != 0) return -1;
    }
    return 0;
}

static int _mark_configured(sqlite3 *db, uint16_t oi) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_freeze_attr3_meta(oi, configured) VALUES(?, 1) "
                           "ON CONFLICT(oi) DO UPDATE SET configured = 1;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt, 1, oi);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _upsert_item(sqlite3 *db, uint16_t oi, const dcu_freeze_attr_t *item) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db,
                           "INSERT INTO dcu_freeze_attr3(oi, seq, oad, period, depth) "
                           "VALUES(?, COALESCE((SELECT MAX(seq) + 1 FROM dcu_freeze_attr3 "
                           "WHERE oi = ?), 0), ?, ?, ?) "
                           "ON CONFLICT(oi, oad) DO UPDATE SET period = excluded.period, "
                           "depth = excluded.depth;",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt, 1, oi);
    sqlite3_bind_int(stmt, 2, oi);
    sqlite3_bind_int64(stmt, 3, item->oad);
    sqlite3_bind_int64(stmt, 4, item->period);
    sqlite3_bind_int64(stmt, 5, item->depth);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

static int _delete_item(sqlite3 *db, uint16_t oi, uint32_t oad) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM dcu_freeze_attr3 WHERE oi = ? AND oad = ?;", -1, &stmt,
                           NULL) != SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt, 1, oi);
    sqlite3_bind_int64(stmt, 2, oad);
    int changed = sqlite3_step(stmt) == SQLITE_DONE ? sqlite3_changes(db) : -1;
    sqlite3_finalize(stmt);
    return changed == 0 ? EDGE_SLAVE_SRC_NOT_FOUND : 0;
}

static int _clear_items(sqlite3 *db, uint16_t oi) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "DELETE FROM dcu_freeze_attr3 WHERE oi = ?;", -1, &stmt, NULL) !=
        SQLITE_OK)
        return -1;
    sqlite3_bind_int(stmt, 1, oi);
    int rc = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return rc;
}

int dcu_freeze_cfg_encode(const dcu_freeze_cfg_t *cfg, uint8_t *buf, size_t cap, size_t *len) {
    if (!cfg || !buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    size_t off = 0;
    if (off + 2 > cap) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    buf[off++] = DLT698_TAG_ARRAY;
    buf[off++] = cfg->count;
    for (uint8_t i = 0; i < cfg->count; i++) {
        const dcu_freeze_attr_t *item = &cfg->items[i];
        if (off + 15 > cap) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        buf[off++] = DLT698_TAG_STRUCTURE;
        buf[off++] = 3;
        buf[off++] = DLT698_TAG_LONG_UNSIGNED;
        buf[off++] = (uint8_t)(item->period >> 8);
        buf[off++] = (uint8_t)item->period;
        buf[off++] = DLT698_TAG_OAD;
        buf[off++] = (uint8_t)(item->oad >> 24);
        buf[off++] = (uint8_t)(item->oad >> 16);
        buf[off++] = (uint8_t)(item->oad >> 8);
        buf[off++] = (uint8_t)item->oad;
        buf[off++] = DLT698_TAG_LONG_UNSIGNED;
        buf[off++] = (uint8_t)(item->depth >> 8);
        buf[off++] = (uint8_t)item->depth;
    }
    *len = off;
    return 0;
}

static int _decode_item(const dlt698_data_t *d, dcu_freeze_attr_t *out) {
    if (d->tag != DLT698_TAG_STRUCTURE || d->val.container.count != 3 || !d->val.container.items)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    const dlt698_data_t *f = d->val.container.items;
    if (f[0].tag != DLT698_TAG_LONG_UNSIGNED || f[1].tag != DLT698_TAG_OAD ||
        f[2].tag != DLT698_TAG_LONG_UNSIGNED)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    out->period = f[0].val.u16;
    out->oad = f[1].val.u32;
    out->depth = f[2].val.u16;
    return 0;
}

int dcu_698_freeze_decode_op(uint8_t method, const uint8_t *axdr, size_t len, dcu_freeze_op_t *op) {
    if (!axdr || !op || len == 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    memset(op, 0, sizeof(*op));
    op->method = method;

    dlt698_data_t d;
    memset(&d, 0, sizeof(d));
    uint16_t off = 0;
    if (dlt698_decode_data(axdr, (uint16_t)len, &off, &d) != 0)
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (!(off == len || (len - off == 1 && axdr[off] == DLT698_NO_TIME_TAG)))
        return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    switch (method) {
        case 3: /* 触发一次冻结：延时 long-unsigned（秒），0 = 立即 */
            if (d.tag != DLT698_TAG_LONG_UNSIGNED) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            op->delay_s = d.val.u16;
            return 0;
        case 4:
            if (_decode_item(&d, &op->items[0]) != 0) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            op->count = 1;
            return 0;
        case 5:
            if (d.tag != DLT698_TAG_OAD) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            op->oad = d.val.u32;
            return 0;
        case 7: { /* 批量添加：array 冻结对象 */
            if (d.tag != DLT698_TAG_ARRAY || (d.val.container.count > 0 && !d.val.container.items))
                return EDGE_SLAVE_SRC_TYPE_MISMATCH;
            if (d.val.container.count > DCU_FREEZE_CFG_MAX) return EDGE_SLAVE_SRC_DENIED;
            uint8_t n = (uint8_t)d.val.container.count;
            for (uint8_t i = 0; i < n; i++) {
                int rc = _decode_item(&d.val.container.items[i], &op->items[i]);
                if (rc != 0) return rc;
            }
            op->count = n;
            return 0;
        }
        case 8:
            return d.tag == DLT698_TAG_NULL ? 0 : EDGE_SLAVE_SRC_TYPE_MISMATCH;
        default:
            return EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
}

/* 主站第一次改属性 3 时，把“未配置缺省”落成显式行：默认集从此是主站配置的起点，
 * 后续删除/清空不会被下一轮加载重新补回默认。 */
static int _materialize_defaults(sqlite3 *db, dcu_freeze_cfgs_t *cfgs, uint16_t oi) {
    const dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_find(cfgs, oi);
    if (!cfg || cfg->configured) return 0;
    for (uint8_t i = 0; i < cfg->count; i++) {
        if (_upsert_item(db, oi, &cfg->items[i]) != 0) return -1;
    }
    return _mark_configured(db, oi);
}

static int _capacity_ok(const dcu_freeze_cfg_t *cfg, const dcu_freeze_op_t *op) {
    uint8_t total = cfg->count;
    for (uint8_t i = 0; i < op->count; i++) {
        bool exists = false;
        for (uint8_t k = 0; k < cfg->count && !exists; k++) {
            if (cfg->items[k].oad == op->items[i].oad) exists = true;
        }
        for (uint8_t k = 0; k < i && !exists; k++) {
            if (op->items[k].oad == op->items[i].oad) exists = true; /* 同帧重复只计一次 */
        }
        if (!exists) total++;
    }
    return total <= DCU_FREEZE_CFG_MAX;
}

static int _apply(uint16_t oi, const dcu_freeze_op_t *op) {
    if (!s_storage || !s_storage->db || !s_cfgs) return EDGE_SLAVE_SRC_TEMPORARY;
    sqlite3 *db = s_storage->db;

    if (op->method == 3) {
        if (!s_sched) return EDGE_SLAVE_SRC_TEMPORARY;
        return dcu_freeze_schedule(s_sched, oi, op->delay_s) == 0 ? 0
                                                                  : EDGE_SLAVE_SRC_TEMPORARY;
    }

    int rc = dcu_storage_begin_trans(s_storage);
    if (rc != 0) return EDGE_SLAVE_SRC_TEMPORARY;
    if (_materialize_defaults(db, s_cfgs, oi) != 0) {
        (void)dcu_storage_rollback_trans(s_storage);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    const dcu_freeze_cfg_t *cfg = dcu_freeze_cfg_find(s_cfgs, oi);
    if ((op->method == 4 || op->method == 7) && (!cfg || !_capacity_ok(cfg, op))) {
        (void)dcu_storage_rollback_trans(s_storage);
        return EDGE_SLAVE_SRC_DENIED;
    }
    if (op->method == 4 || op->method == 7) {
        rc = 0;
        for (uint8_t i = 0; i < op->count && rc == 0; i++) {
            if (_upsert_item(db, oi, &op->items[i]) != 0) rc = EDGE_SLAVE_SRC_TEMPORARY;
        }
        if (rc == 0) rc = _mark_configured(db, oi);
    } else if (op->method == 5) {
        rc = _delete_item(db, oi, op->oad);
        if (rc == 0) rc = _mark_configured(db, oi);
    } else if (op->method == 8) {
        rc = _clear_items(db, oi);
        if (rc == 0) rc = _mark_configured(db, oi);
    } else {
        rc = EDGE_SLAVE_SRC_CLASS_MISMATCH;
    }
    if (rc != 0) {
        (void)dcu_storage_rollback_trans(s_storage);
        return rc;
    }
    /* 事务内先把新快照载到临时对象，成功提交后才替换；失败回滚，缓存不动。 */
    dcu_freeze_cfgs_t next = *s_cfgs;
    if (_reload_one(&next, s_storage, oi) != 0) {
        (void)dcu_storage_rollback_trans(s_storage);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    if (dcu_storage_commit_trans(s_storage) != 0) {
        (void)dcu_storage_rollback_trans(s_storage);
        return EDGE_SLAVE_SRC_TEMPORARY;
    }
    *s_cfgs = next;
    return 0;
}

int dcu_698_freeze_execute(const dcu_698_object_t *obj, uint8_t method_id, const void *param,
                           void *result) {
    (void)result;
    const dcu_freeze_op_t *op = (const dcu_freeze_op_t *)param;
    if (!obj || !op || op->method != method_id) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
    if (obj->oi < 0x5000 || obj->oi > 0x5007) return EDGE_SLAVE_SRC_NOT_FOUND;
    return _apply(obj->oi, op);
}

const dcu_freeze_cfg_t *dcu_698_freeze_cfg_get(uint16_t oi) {
    return dcu_freeze_cfg_find(s_cfgs, oi);
}

void dcu_698_freeze_bind(dcu_storage_t *storage, dcu_freeze_cfgs_t *cfgs,
                         dcu_freeze_scheduler_t *sched) {
    s_storage = storage;
    s_cfgs = cfgs;
    s_sched = sched;
}
