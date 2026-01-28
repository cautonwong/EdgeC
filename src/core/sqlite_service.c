/**
 * @file sqlite_service.c
 * @brief 终极版工业级持久化引擎 (支持 BLOB 参数、时序分表、自动迁移)
 */

#include "sqlite_service.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "edge_log.h"

// 数据库架构版本 (V4: 支持 BLOB 参数)
#define EDGE_DB_SCHEMA_VERSION 4

typedef struct {
    PipeMsg *buffer;
    int count;
    int capacity;
} data_batch_t;

struct sqlite_svc_s {
    sqlite3 *db;
    uv_loop_t *loop;
    sqlite_config_t config;
    data_batch_t *active_buf;
    data_batch_t *flush_buf;
    uv_mutex_t buf_lock;
    uv_work_t work_req;
    uv_timer_t flush_timer;
    uv_timer_t gc_timer; /**< [Phase 9] 存储审计定时器 */
    int is_flushing;
    char current_shard[64];
};

// --- 存储空间审计 (Task 8) ---
#include <sys/stat.h>

static void _on_storage_gc(uv_timer_t *handle) {
    sqlite_svc_t *svc = (sqlite_svc_t *)handle->data;
    if (svc->config.max_db_size_mb == 0) return;

    struct stat st;
    if (stat(svc->config.db_path, &st) == 0) {
        uint64_t size_mb = (uint64_t)st.st_size / (1024 * 1024);
        if (size_mb > svc->config.max_db_size_mb) {
            LOG_WARN(LOG_CAT_CORE, "DB: Size %lu MB exceeds limit %u MB. Triggering GC...",
                     (unsigned long)size_mb, svc->config.max_db_size_mb);

            if (svc->config.strategy == STRATEGY_SHARDING) {
                // [AET-2.2] 分表策略：删除最旧的一张表 (telemetry_YYYYMM)
                sqlite3_stmt *stmt;
                const char *sql =
                    "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE "
                    "'telemetry_%%' ORDER BY name ASC LIMIT 1;";
                if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                    if (sqlite3_step(stmt) == SQLITE_ROW) {
                        const char *old_table = (const char *)sqlite3_column_text(stmt, 0);
                        char drop_sql[128];
                        snprintf(drop_sql, sizeof(drop_sql), "DROP TABLE %s;", old_table);
                        sqlite3_exec(svc->db, drop_sql, NULL, NULL, NULL);
                        LOG_INFO(LOG_CAT_CORE, "DB GC: Dropped oldest shard %s", old_table);
                    }
                    sqlite3_finalize(stmt);
                }
            } else {
                // 简单策略：删除最早的 10% 数据
                sqlite3_exec(
                    svc->db,
                    "DELETE FROM telemetry WHERE ts < (SELECT MIN(ts) + (MAX(ts)-MIN(ts))/10 "
                    "FROM telemetry);",
                    NULL, NULL, NULL);
            }
            // 释放空间
            sqlite3_exec(svc->db, "PRAGMA incremental_vacuum;", NULL, NULL, NULL);
        }
    }
}

// --- 1. 内部工具 ---

static void _get_shard_table(uint64_t ts, char *out) {
    time_t sec = (time_t)(ts / 1000);
    struct tm t;
    gmtime_r(&sec, &t);
    snprintf(out, 64, "telemetry_%04d%02d", t.tm_year + 1900, t.tm_mon + 1);
}

static void _ensure_shard_exists(sqlite_svc_t *svc, const char *table_name) {
    if (strcmp(table_name, svc->current_shard) == 0) return;
    char sql[256];
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS %s (ts INT, tag_id INT, val REAL, q INT, PRIMARY KEY(ts, "
             "tag_id)) WITHOUT ROWID;",
             table_name);
    if (sqlite3_exec(svc->db, sql, 0, 0, 0) == SQLITE_OK) {
        strcpy(svc->current_shard, table_name);
    }
}

// --- 2. 模式迁移 (Migration) ---

static int _db_run_migration(sqlite_svc_t *svc, int current_ver) {
    // [终极防御] 仿真模式下，即便版本号不对，也禁止迁移
    if (getenv("EDGE_DB_PATH")) {
        LOG_INFO(LOG_CAT_CORE, "DB: Matrix Sandbox mode. Migration blocked.");
        return 0;
    }
    char *err = NULL;
    int ret = 0;
    LOG_INFO(LOG_CAT_CORE, "DB: Current version %d, migrating to %d", current_ver,
             EDGE_DB_SCHEMA_VERSION);

    switch (current_ver) {
        case 0:
            ret |= sqlite3_exec(svc->db,
                                "CREATE TABLE IF NOT EXISTS telemetry (ts INT, tag_id INT, val "
                                "REAL, q INT, PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;"
                                "CREATE TABLE IF NOT EXISTS inventory (id TEXT PRIMARY KEY, type "
                                "TEXT, addr INT, protocol TEXT, enabled INT DEFAULT 1);",
                                NULL, NULL, &err);
            // fallthrough
        case 1:
            ret |= sqlite3_exec(svc->db,
                                "CREATE TABLE IF NOT EXISTS edge_settings (product TEXT, tag_id "
                                "INT, val BLOB, PRIMARY KEY(product, tag_id));",
                                NULL, NULL, &err);
            // fallthrough
        case 2:
            ret |= sqlite3_exec(
                svc->db, "CREATE INDEX IF NOT EXISTS idx_telemetry_tag ON telemetry(tag_id);", NULL,
                NULL, &err);
            // fallthrough
        case 3:
            // 确保 edge_settings 的 val 确实是 BLOB
            ret |= sqlite3_exec(svc->db,
                                "DROP TABLE IF EXISTS edge_settings;"
                                "CREATE TABLE edge_settings (product TEXT, tag_id INT, val BLOB, "
                                "PRIMARY KEY(product, tag_id));",
                                NULL, NULL, &err);
            break;
    }

    if (ret != 0) {
        LOG_ERROR(LOG_CAT_CORE, "DB Migration Error: %s", err ? err : "Unknown");
        if (err) sqlite3_free(err);
        return -1;
    }

    char sql[64];
    snprintf(sql, sizeof(sql), "PRAGMA user_version=%d;", EDGE_DB_SCHEMA_VERSION);
    sqlite3_exec(svc->db, sql, NULL, NULL, NULL);
    return 0;
}

static int _migrate_schema(sqlite_svc_t *svc) {
    sqlite3_stmt *stmt;
    int current_ver = 0;
    if (sqlite3_prepare_v2(svc->db, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) current_ver = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    if (current_ver < EDGE_DB_SCHEMA_VERSION) return _db_run_migration(svc, current_ver);
    return 0;
}

// --- 3. 异步冲刷逻辑 ---

static void _worker_flush_cb(uv_work_t *req) {
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    data_batch_t *batch = svc->flush_buf;
    if (!batch || batch->count == 0) return;

    sqlite3_exec(svc->db, "BEGIN IMMEDIATE;", 0, 0, 0);

    // [AET-2.1 Optimization] 预编译语句在循环外定义，减少解析开销
    sqlite3_stmt *stmt = NULL;
    const char *current_table = NULL;

    for (int i = 0; i < batch->count; i++) {
        PipeMsg *msg = &batch->buffer[i];
        const char *table = "telemetry";
        char shard_name[64];
        if (svc->config.strategy == STRATEGY_SHARDING) {
            _get_shard_table(msg->ts, shard_name);
            _ensure_shard_exists(svc, shard_name);
            table = shard_name;
        }

        if (current_table == NULL || strcmp(table, current_table) != 0) {
            if (stmt) sqlite3_finalize(stmt);
            char sql[128];
            snprintf(sql, sizeof(sql), "INSERT OR REPLACE INTO %s VALUES (?,?,?,?)", table);
            if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                continue;
            }
            current_table = table;
        }

        sqlite3_bind_int64(stmt, 1, (sqlite3_int64)msg->ts);
        sqlite3_bind_int(stmt, 2, (int)msg->tag_id);
        sqlite3_bind_double(stmt, 3, msg->payload.val.f);
        sqlite3_bind_int(stmt, 4, (int)msg->quality);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);  // [AET-2.1] 重置语句以便复用
    }

    if (stmt) sqlite3_finalize(stmt);
    sqlite3_exec(svc->db, "COMMIT;", 0, 0, 0);
}

static void _after_flush_cb(uv_work_t *req, int status) {
    (void)status;
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    svc->flush_buf->count = 0;
    svc->is_flushing = 0;
}

static int _trigger_flush(sqlite_svc_t *svc) {
    if (svc->is_flushing || !svc->active_buf || svc->active_buf->count == 0) return -1;
    uv_mutex_lock(&svc->buf_lock);
    data_batch_t *tmp = svc->active_buf;
    svc->active_buf = svc->flush_buf;
    svc->flush_buf = tmp;
    uv_mutex_unlock(&svc->buf_lock);
    svc->is_flushing = 1;
    svc->work_req.data = svc;
    return uv_queue_work(svc->loop, &svc->work_req, _worker_flush_cb, _after_flush_cb);
}

static void _on_timer(uv_timer_t *handle) {
    _trigger_flush((sqlite_svc_t *)handle->data);
}

// --- 4. 公开 API (核心补强) ---

sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg) {
    sqlite_svc_t *svc = calloc(1, sizeof(sqlite_svc_t));
    svc->loop = loop;
    svc->config = *cfg;
    if (sqlite3_open(cfg->db_path, &svc->db) != SQLITE_OK) {
        free(svc);
        return NULL;
    }
    sqlite3_exec(svc->db, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    _migrate_schema(svc);
    if (cfg->strategy == STRATEGY_SERIES || cfg->strategy == STRATEGY_SHARDING) {
        svc->active_buf = calloc(1, sizeof(data_batch_t));
        svc->flush_buf = calloc(1, sizeof(data_batch_t));
        int bsize = cfg->batch_size > 0 ? cfg->batch_size : 100;
        svc->active_buf->buffer = malloc(sizeof(PipeMsg) * (size_t)bsize);
        svc->flush_buf->buffer = malloc(sizeof(PipeMsg) * (size_t)bsize);
        svc->active_buf->capacity = bsize;
        svc->flush_buf->capacity = bsize;
        uv_mutex_init(&svc->buf_lock);
        uv_timer_init(loop, &svc->flush_timer);
        svc->flush_timer.data = svc;
        int interval = cfg->flush_interval_ms > 0 ? cfg->flush_interval_ms : 1000;
        uv_timer_start(&svc->flush_timer, _on_timer, (uint64_t)interval, (uint64_t)interval);
    }

    // 初始化 GC 定时器 (每分钟检查一次)
    uv_timer_init(loop, &svc->gc_timer);
    svc->gc_timer.data = svc;
    uv_timer_start(&svc->gc_timer, _on_storage_gc, 60000, 60000);

    return svc;
}

int sqlite_svc_set_param(sqlite_svc_t *svc, const char *product, uint32_t tag_id, const void *val,
                         size_t len) {
    if (!svc || !product || !val) return -1;
    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT OR REPLACE INTO edge_settings (product, tag_id, val) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_text(stmt, 1, product, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)tag_id);
    sqlite3_bind_blob(stmt, 3, val, (int)len, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int sqlite_svc_get_param(sqlite_svc_t *svc, const char *product, uint32_t tag_id, void *buf,
                         size_t buflen) {
    if (!svc || !product || !buf) return -1;
    sqlite3_stmt *stmt;
    int bytes_read = -1;
    const char *sql = "SELECT val FROM edge_settings WHERE product=? AND tag_id=?;";
    if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, product, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, (int)tag_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const void *blob = sqlite3_column_blob(stmt, 0);
            int len = sqlite3_column_bytes(stmt, 0);
            if (blob && len > 0) {
                bytes_read = (len < (int)buflen) ? len : (int)buflen;
                memcpy(buf, blob, (size_t)bytes_read);
            }
        }
        sqlite3_finalize(stmt);
    }
    return bytes_read;
}

int sqlite_svc_insert(sqlite_svc_t *svc, const PipeMsg *msg) {
    if (!svc || !msg) return -1;
    if (svc->config.strategy == STRATEGY_LITE) {
        char sql[256];
        snprintf(sql, sizeof(sql), "INSERT OR REPLACE INTO telemetry VALUES (%llu, %u, %f, %u);",
                 (unsigned long long)msg->ts, msg->tag_id, msg->payload.val.f, msg->quality);
        return sqlite3_exec(svc->db, sql, 0, 0, 0) == SQLITE_OK ? 0 : -1;
    }
    uv_mutex_lock(&svc->buf_lock);
    if (svc->active_buf->count < svc->active_buf->capacity)
        svc->active_buf->buffer[svc->active_buf->count++] = *msg;
    uv_mutex_unlock(&svc->buf_lock);
    return 0;
}

int sqlite_svc_query(sqlite_svc_t *svc, uint64_t start_ts, uint64_t end_ts, sqlite_svc_query_cb cb,
                     void *user_data) {
    if (!svc || !cb) return -1;
    char sql[512];
    if (svc->config.strategy == STRATEGY_SHARDING) {
        char shard[64];
        _get_shard_table(start_ts, shard);
        snprintf(sql, sizeof(sql), "SELECT ts, tag_id, val, q FROM %s WHERE ts>=? AND ts<=?;",
                 shard);
    } else {
        strcpy(sql, "SELECT ts, tag_id, val, q FROM telemetry WHERE ts>=? AND ts<=?;");
    }
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)start_ts);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)end_ts);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PipeMsg m = {.ts = (uint64_t)sqlite3_column_int64(stmt, 0),
                     .tag_id = (uint32_t)sqlite3_column_int(stmt, 1),
                     .payload = {.val = {.f = sqlite3_column_double(stmt, 2)}},
                     .quality = (uint8_t)sqlite3_column_int(stmt, 3),
                     .type = PIPEMSG_TYPE_SCALAR};
        if (cb(user_data, &m) != 0) break;
    }
    sqlite3_finalize(stmt);
    return 0;
}

int sqlite_svc_begin_trans(sqlite_svc_t *svc) {
    if (!svc || !svc->db) return -1;
    return sqlite3_exec(svc->db, "BEGIN EXCLUSIVE TRANSACTION;", NULL, NULL, NULL) == SQLITE_OK
               ? 0
               : -1;
}

int sqlite_svc_commit_trans(sqlite_svc_t *svc) {
    if (!svc || !svc->db) return -1;
    return sqlite3_exec(svc->db, "COMMIT;", NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

void sqlite_svc_destroy(sqlite_svc_t *svc) {
    if (!svc) return;

    uv_timer_stop(&svc->gc_timer);
    if (!uv_is_closing((uv_handle_t *)&svc->gc_timer)) {
        uv_close((uv_handle_t *)&svc->gc_timer, NULL);
    }

    if (svc->active_buf) {
        uv_timer_stop(&svc->flush_timer);
        while (svc->is_flushing) uv_run(svc->loop, UV_RUN_ONCE);
        if (svc->active_buf->count > 0) {
            data_batch_t *old_flush = svc->flush_buf;
            svc->flush_buf = svc->active_buf;
            uv_work_t req = {.data = svc};
            _worker_flush_cb(&req);
            svc->flush_buf = old_flush;
        }
        free(svc->active_buf->buffer);
        free(svc->active_buf);
        if (svc->flush_buf) {
            free(svc->flush_buf->buffer);
            free(svc->flush_buf);
        }
        uv_mutex_destroy(&svc->buf_lock);
    }
    if (svc->db) sqlite3_close(svc->db);
    free(svc);
}

typedef struct {
    sqlite_svc_t *svc;
    char *sql;
    sqlite_exec_cb cb;
    void *user_data;
    int status;
    char *errmsg;
} exec_task_t;
static void _exec_work_cb(uv_work_t *req) {
    exec_task_t *t = (exec_task_t *)req->data;
    t->status = sqlite3_exec(t->svc->db, t->sql, NULL, NULL, &t->errmsg);
}
static void _exec_after_cb(uv_work_t *req, int status) {
    (void)status;
    exec_task_t *t = (exec_task_t *)req->data;
    if (t->cb) t->cb(t->user_data, t->status, t->errmsg ? &t->errmsg : NULL);
    if (t->errmsg) sqlite3_free(t->errmsg);
    free(t->sql);
    free(t);
    free(req);
}
int sqlite_svc_exec(sqlite_svc_t *svc, const char *sql, sqlite_exec_cb cb, void *user_data) {
    if (!svc || !sql) return -1;
    uv_work_t *req = malloc(sizeof(uv_work_t));
    exec_task_t *task = calloc(1, sizeof(exec_task_t));
    task->svc = svc;
    task->sql = strdup(sql);
    task->cb = cb;
    task->user_data = user_data;
    req->data = task;
    return uv_queue_work(svc->loop, req, _exec_work_cb, _exec_after_cb);
}
void *sqlite_svc_get_db_handle(sqlite_svc_t *svc) {
    return svc ? svc->db : NULL;
}
