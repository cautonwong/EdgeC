#include "sqlite_service.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// PART 0: 工业级定义与宏 (Frozen Contracts)
// ============================================================================

#define SCHEMA_VER_HEMS 1
#define SCHEMA_VER_IEMS 1
#define SCHEMA_VER_EVSE 1
#define SCHEMA_VER_SG 1
#define SCHEMA_VER_DCU 1

// 错误处理宏：记录错误码，触发回调，但不崩溃
#define DB_CHECK(cmd, ctx, err_cnt)                                                       \
    do {                                                                                  \
        int rc = (cmd);                                                                   \
        if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {                   \
            if ((ctx)->err_cb)                                                            \
                (ctx)->err_cb(rc, sqlite3_errmsg((ctx)->db));                             \
            else                                                                          \
                fprintf(stderr, "[DB-ERR] (rc=%d): %s\n", rc, sqlite3_errmsg((ctx)->db)); \
            (err_cnt)++;                                                                  \
            goto error_rollback;                                                          \
        }                                                                                 \
    } while (0)

// 内部统计
typedef struct {
    uint64_t insert_ok;
    uint64_t insert_fail;
    uint64_t wal_size;
} db_stats_t;

// ============================================================================
// PART 1: 上下文定义
// ============================================================================

typedef struct {
    PipeMsg *buffer;
    int count;
    int capacity;
    uint32_t batch_id;
} data_batch_t;

struct sqlite_svc_s {
    sqlite3 *db;
    uv_loop_t *loop;
    sqlite_config_t config;

    // 异步写入状态
    data_batch_t *active_buf;
    data_batch_t *flush_buf;
    uv_mutex_t buf_lock;
    uv_work_t work_req;
    uv_timer_t flush_timer;
    int is_flushing;

    // 状态保持
    char current_shard[64];
    db_stats_t stats;

    // 错误回调
    sqlite_err_cb err_cb;
};

// ============================================================================
// PART 2: 时间与分表逻辑 (Time Authority)
// ============================================================================

static void _get_shard_table(product_type_t type, uint64_t ts, char *out) {
    if (type == PROD_DCU) {
        snprintf(out, 64, "meter_data_%06lu", ts);
    } else {
        strcpy(out, "telemetry");
    }
}

// ============================================================================
// PART 3: Schema 管理 (Migration Engine)
// ============================================================================

static int _get_target_version(product_type_t type) {
    switch (type) {
        case PROD_HEMS:
            return SCHEMA_VER_HEMS;
        case PROD_IEMS:
            return SCHEMA_VER_IEMS;
        case PROD_EVSE:
            return SCHEMA_VER_EVSE;
        case PROD_SG_EL2025:
            return SCHEMA_VER_SG;
        case PROD_DCU:
            return SCHEMA_VER_DCU;
        default:
            return 0;
    }
}

static const char *_get_init_schema(product_type_t type) {
    switch (type) {
        case PROD_HEMS:
            return "CREATE TABLE current_state (tag_id INT PRIMARY KEY, val REAL, ts INT);"
                   "CREATE TABLE hourly_stats (hour_key INT PRIMARY KEY, pv REAL, load REAL);";
        case PROD_IEMS:
            return "CREATE TABLE telemetry (ts INT, tag_id INT, val REAL, q INT, PRIMARY KEY(ts, "
                   "tag_id)) WITHOUT ROWID;";
        case PROD_EVSE:
            return "CREATE TABLE orders (id TEXT PRIMARY KEY, kwh REAL, status INT);"
                   "CREATE TABLE audit (id INT PRIMARY KEY, ts INT, event TEXT);";
        case PROD_SG_EL2025:
            return "CREATE TABLE waveforms (id INT PRIMARY KEY, ts INT, type INT, data BLOB);";
        case PROD_DCU:
            return "CREATE TABLE meter_meta (addr TEXT PRIMARY KEY, protocol INT);";
        default:
            return "";
    }
}

static int _migrate_schema(sqlite3 *db, product_type_t type) {
    int cur_ver = 0;
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            cur_ver = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    int target_ver = _get_target_version(type);
    if (cur_ver >= target_ver) return 0;
    char *err_msg = NULL;
    if (cur_ver == 0) {
        if (sqlite3_exec(db, _get_init_schema(type), NULL, NULL, &err_msg) != SQLITE_OK) {
            fprintf(stderr, "[DB] Init schema failed: %s\n", err_msg);
            sqlite3_free(err_msg);
            return -1;
        }
    }
    char sql[64];
    snprintf(sql, sizeof(sql), "PRAGMA user_version=%d;", target_ver);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    return 0;
}

// ============================================================================
// PART 4: 核心 Worker (运行在线程池)
// ============================================================================

static void _ensure_shard_exists(sqlite_svc_t *svc, const char *table_name) {
    if (strcmp(table_name, svc->current_shard) == 0) return;
    char sql[256];
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS %s ("
             "ts INTEGER, tag_id INTEGER, val REAL, quality INT, "
             "PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;",
             table_name);
    if (sqlite3_exec(svc->db, sql, 0, 0, 0) == SQLITE_OK) {
        strcpy(svc->current_shard, table_name);
    }
}

static void _worker_flush_cb(uv_work_t *req) {
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    data_batch_t *batch = svc->flush_buf;
    if (batch->count == 0) return;
    int err_cnt = 0;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_exec(svc->db, "BEGIN IMMEDIATE;", 0, 0, 0) != SQLITE_OK) {
        svc->stats.insert_fail += (uint64_t)batch->count;
        return;
    }
    if (svc->config.type == PROD_DCU) {
        char table_name[64];
        _get_shard_table(PROD_DCU, batch->buffer[0].ts, table_name);
        _ensure_shard_exists(svc, table_name);
        char sql[256];
        snprintf(sql, sizeof(sql), "INSERT INTO %s VALUES (?,?,?)", table_name);
        DB_CHECK(sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL), svc, err_cnt);
    } else if (svc->config.type == PROD_SG_EL2025) {
        DB_CHECK(
            sqlite3_prepare_v2(svc->db, "INSERT INTO waveforms (ts, type, data) VALUES (?,?,?)", -1,
                               &stmt, NULL),
            svc, err_cnt);
    } else {
        DB_CHECK(
            sqlite3_prepare_v2(svc->db, "INSERT INTO telemetry VALUES (?,?,?,?)", -1, &stmt, NULL),
            svc, err_cnt);
    }
    // 3. 执行 Batch
    for (int i = 0; i < batch->count; i++) {
        PipeMsg *msg = &batch->buffer[i];

        sqlite3_bind_int64(stmt, 1, (sqlite3_int64)msg->ts);
        
        if (svc->config.type == PROD_DCU) {
            // DCU: (addr, tag_id, ts, val)
            sqlite3_bind_text(stmt, 1, "meter_01", -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, (int)msg->tag_id);
            sqlite3_bind_int64(stmt, 3, (sqlite3_int64)msg->ts);
            sqlite3_bind_double(stmt, 4, msg->payload.val_f);
        } else if (msg->type == PIPEMSG_REF_DMA) {
            sqlite3_bind_int(stmt, 2, (int)msg->tag_id);
            // 这里假设 ref 模式下，我们暂时没有真正的指针传递
            // 简单绑定 0 或模拟数据
            sqlite3_bind_blob(stmt, 3, "", 0, SQLITE_STATIC);
        } else {
            sqlite3_bind_int(stmt, 2, (int)msg->tag_id);
            sqlite3_bind_double(stmt, 3, msg->payload.val_f);
            sqlite3_bind_int(stmt, 4, 0); // quality
        }

        if (sqlite3_step(stmt) != SQLITE_DONE) err_cnt++;
        sqlite3_reset(stmt);
    }
    DB_CHECK(sqlite3_exec(svc->db, "COMMIT;", 0, 0, 0), svc, err_cnt);
    svc->stats.insert_ok += (uint64_t)(batch->count - err_cnt);
    sqlite3_finalize(stmt);
    return;
error_rollback:
    sqlite3_exec(svc->db, "ROLLBACK;", 0, 0, 0);
    svc->stats.insert_fail += (uint64_t)batch->count;
    if (stmt) sqlite3_finalize(stmt);
}

// ============================================================================
// PART 5: 生命周期与 API (Lifecycle)
// ============================================================================

static void _after_flush_cb(uv_work_t *req, int status) {
    (void)status;
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    svc->flush_buf->count = 0;
    svc->is_flushing = 0;
}

static void _trigger_flush(sqlite_svc_t *svc) {
    if (svc->is_flushing) return;
    if (svc->active_buf->count == 0) return;
    uv_mutex_lock(&svc->buf_lock);
    data_batch_t *tmp = svc->active_buf;
    svc->active_buf = svc->flush_buf;
    svc->flush_buf = tmp;
    uv_mutex_unlock(&svc->buf_lock);
    svc->is_flushing = 1;
    svc->work_req.data = svc;
    uv_queue_work(svc->loop, &svc->work_req, _worker_flush_cb, _after_flush_cb);
}

static void _on_timer(uv_timer_t *handle) {
    _trigger_flush((sqlite_svc_t *)handle->data);
}

sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg) {
    sqlite_svc_t *svc = calloc(1, sizeof(sqlite_svc_t));
    svc->loop = loop;
    svc->config = *cfg;
    if (sqlite3_open(cfg->db_path, &svc->db) != SQLITE_OK) {
        free(svc);
        return NULL;
    }
    if (cfg->type == PROD_EVSE) {
        sqlite3_exec(svc->db, "PRAGMA journal_mode=DELETE; PRAGMA synchronous=FULL;", 0, 0, 0);
    } else {
        sqlite3_exec(svc->db, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;", 0, 0, 0);
    }
    _migrate_schema(svc->db, cfg->type);
    if (cfg->type != PROD_HEMS && cfg->type != PROD_EVSE) {
        svc->active_buf = calloc(1, sizeof(data_batch_t));
        svc->flush_buf = calloc(1, sizeof(data_batch_t));
        svc->active_buf->buffer = malloc(sizeof(PipeMsg) * (size_t)cfg->batch_size);
        svc->flush_buf->buffer = malloc(sizeof(PipeMsg) * (size_t)cfg->batch_size);
        svc->active_buf->capacity = cfg->batch_size;
        svc->flush_buf->capacity = cfg->batch_size;
        uv_mutex_init(&svc->buf_lock);
        uv_timer_init(loop, &svc->flush_timer);
        svc->flush_timer.data = svc;
        uv_timer_start(&svc->flush_timer, _on_timer, (uint64_t)cfg->flush_interval_ms,
                       (uint64_t)cfg->flush_interval_ms);
    }
    return svc;
}

void sqlite_svc_destroy(sqlite_svc_t *svc) {
    if (!svc) return;
    if (svc->config.type != PROD_HEMS && svc->config.type != PROD_EVSE) {
        uv_timer_stop(&svc->flush_timer);
        _trigger_flush(svc);
        while (svc->is_flushing) {
            uv_run(svc->loop, UV_RUN_ONCE);
        }
        free(svc->active_buf->buffer);
        free(svc->flush_buf->buffer);
        free(svc->active_buf);
        free(svc->flush_buf);
        uv_mutex_destroy(&svc->buf_lock);
    }
    sqlite3_close(svc->db);
    free(svc);
}

int sqlite_svc_insert(sqlite_svc_t *svc, const PipeMsg *msg) {
    if (svc->config.type == PROD_EVSE) return -1;
    if (svc->config.type == PROD_HEMS) {
        return 0;
    }
    uv_mutex_lock(&svc->buf_lock);
    data_batch_t *buf = svc->active_buf;
    if (buf->count >= buf->capacity) {
        uv_mutex_unlock(&svc->buf_lock);
        _trigger_flush(svc);
        return -1;
    }
    buf->buffer[buf->count++] = *msg;
    int full = (buf->count >= svc->config.batch_size);
    uv_mutex_unlock(&svc->buf_lock);
    if (full) _trigger_flush(svc);
    return 0;
}

int sqlite_svc_begin_trans(sqlite_svc_t *svc) {
    return sqlite3_exec(svc->db, "BEGIN EXCLUSIVE TRANSACTION;", 0, 0, 0);
}

int sqlite_svc_commit_trans(sqlite_svc_t *svc) {
    return sqlite3_exec(svc->db, "COMMIT;", 0, 0, 0);
}