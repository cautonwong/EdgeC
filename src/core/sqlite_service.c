#include "sqlite_service.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCHEMA_VER 1

#define DB_CHECK(cmd, svc, err_cnt)                                                            \
    do {                                                                                       \
        int rc = (cmd);                                                                        \
        if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {                        \
            fprintf(stderr, "[DB-ERR] %s (rc=%d): %s\n", #cmd, rc, sqlite3_errmsg((svc)->db)); \
            (err_cnt)++;                                                                       \
            goto error_rollback;                                                               \
        }                                                                                      \
    } while (0)

typedef struct {
    uint64_t insert_ok;
    uint64_t insert_fail;
} db_stats_t;

typedef struct {
    PipeMsg *buffer;
    int count;
    int capacity;
} data_batch_t;

// 通用执行任务上下文
typedef struct {
    sqlite_svc_t *svc;
    char *sql;
    sqlite_exec_cb cb;
    void *user_data;
    int status;
    char *errmsg;
} exec_task_t;

struct sqlite_svc_s {
    sqlite3 *db;
    uv_loop_t *loop;
    sqlite_config_t config;
    data_batch_t *active_buf;
    data_batch_t *flush_buf;
    uv_mutex_t buf_lock;
    uv_work_t work_req;
    uv_timer_t flush_timer;
    int is_flushing;
    char current_shard[64];
    db_stats_t stats;
};

// ... (省略 _get_shard_table, _get_init_schema, _migrate_schema, _ensure_shard_exists 保持不变) ...
static void _get_shard_table(uint64_t ts, char *out) {
    time_t sec = (time_t)(ts / 1000);
    struct tm t;
    gmtime_r(&sec, &t);
    snprintf(out, 64, "telemetry_%04d%02d", t.tm_year + 1900, t.tm_mon + 1);
}

static const char *_get_init_schema(storage_strategy_t strategy) {
    switch (strategy) {
        case STRATEGY_LITE:
            return "CREATE TABLE IF NOT EXISTS current_state (tag_id INT PRIMARY KEY, val REAL, ts "
                   "INT);"
                   "CREATE TABLE IF NOT EXISTS collection_policies (interval_ms INT, slave_id INT, start_addr INT, reg_count INT);";
        case STRATEGY_TRANSACTION:
            return "CREATE TABLE IF NOT EXISTS orders (id TEXT PRIMARY KEY, kwh REAL, status INT);";
        default:
            return "CREATE TABLE IF NOT EXISTS telemetry (ts INT, tag_id INT, val REAL, q INT, "
                   "PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;"
                   "CREATE TABLE IF NOT EXISTS collection_policies (interval_ms INT, slave_id INT, start_addr INT, reg_count INT);";
    }
}

static int _migrate_schema(sqlite_svc_t *svc) {
    sqlite3_exec(svc->db, _get_init_schema(svc->config.strategy), NULL, NULL, NULL);
    char sql[64];
    snprintf(sql, sizeof(sql), "PRAGMA user_version=%d;", SCHEMA_VER);
    sqlite3_exec(svc->db, sql, NULL, NULL, NULL);
    return 0;
}

static void _ensure_shard_exists(sqlite_svc_t *svc, const char *table_name) {
    if (strcmp(table_name, svc->current_shard) == 0) return;
    char sql[256];
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS %s (ts INT, tag_id INT, val REAL, q INT, PRIMARY KEY(ts, "
             "tag_id)) WITHOUT ROWID;",
             table_name);
    if (sqlite3_exec(svc->db, sql, 0, 0, 0) == SQLITE_OK) strcpy(svc->current_shard, table_name);
}

// --- 通用 SQL 执行逻辑 ---

static void _exec_work_cb(uv_work_t *req) {
    exec_task_t *task = (exec_task_t *)req->data;
    // 在 Worker 线程中执行阻塞 SQL
    task->status = sqlite3_exec(task->svc->db, task->sql, NULL, NULL, &task->errmsg);
}

static void _exec_after_cb(uv_work_t *req, int status) {
    (void)status;
    exec_task_t *task = (exec_task_t *)req->data;

    if (task->cb) {
        task->cb(task->user_data, task->status, task->errmsg ? &task->errmsg : NULL);
    }

    if (task->errmsg) sqlite3_free(task->errmsg);
    free(task->sql);
    free(task);
    free(req);
}

int sqlite_svc_exec(sqlite_svc_t *svc, const char *sql, sqlite_exec_cb cb, void *user_data) {
    if (!svc || !sql) return -1;

    uv_work_t *req = malloc(sizeof(uv_work_t));
    if (!req) return -1;

    exec_task_t *task = calloc(1, sizeof(exec_task_t));
    if (!task) {
        free(req);
        return -1;
    }

    task->svc = svc;
    task->sql = strdup(sql);  // 必须拷贝，因为 sql 指针可能在异步执行前释放
    task->cb = cb;
    task->user_data = user_data;
    req->data = task;

    return uv_queue_work(svc->loop, req, _exec_work_cb, _exec_after_cb);
}

// ... (以下为原有 insert/query 逻辑，保持不变) ...

static void _worker_flush_cb(uv_work_t *req) {
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    data_batch_t *batch = svc->flush_buf;
    if (batch->count == 0) return;

    int err_cnt = 0;
    sqlite3_stmt *stmt = NULL;
    sqlite3_exec(svc->db, "BEGIN IMMEDIATE;", 0, 0, 0);

    const char *table = "telemetry";
    char shard_name[64];

    if (svc->config.strategy == STRATEGY_SHARDING) {
        _get_shard_table(batch->buffer[0].ts, shard_name);
        _ensure_shard_exists(svc, shard_name);
        table = shard_name;
    }

    char sql[256];
    snprintf(sql, sizeof(sql), "INSERT OR REPLACE INTO %s VALUES (?,?,?,?)", table);
    if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) != SQLITE_OK) goto error_rollback;

    for (int i = 0; i < batch->count; i++) {
        PipeMsg *msg = &batch->buffer[i];
        sqlite3_bind_int64(stmt, 1, (sqlite3_int64)msg->ts);
        sqlite3_bind_int(stmt, 2, (int)msg->tag_id);
        sqlite3_bind_double(stmt, 3, msg->payload.val.f);
        sqlite3_bind_int(stmt, 4, (int)msg->quality);

        if (sqlite3_step(stmt) != SQLITE_DONE) err_cnt++;
        sqlite3_reset(stmt);
    }

    sqlite3_exec(svc->db, "COMMIT;", 0, 0, 0);
    svc->stats.insert_ok += (uint64_t)(batch->count - err_cnt);
    if (stmt) sqlite3_finalize(stmt);
    return;

error_rollback:
    sqlite3_exec(svc->db, "ROLLBACK;", 0, 0, 0);
    if (stmt) sqlite3_finalize(stmt);
}

static void _after_flush_cb(uv_work_t *req, int status) {
    (void)status;
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    svc->flush_buf->count = 0;
    svc->is_flushing = 0;
}

static int _trigger_flush(sqlite_svc_t *svc) {
    if (svc->is_flushing || svc->active_buf->count == 0) return -1;
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

sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg) {
    sqlite_svc_t *svc = calloc(1, sizeof(sqlite_svc_t));
    if (!svc) return NULL;
    svc->loop = loop;
    svc->config = *cfg;
    svc->current_shard[0] = '\0';

    if (sqlite3_open(cfg->db_path, &svc->db) != SQLITE_OK) {
        free(svc);
        return NULL;
    }

    if (cfg->strategy == STRATEGY_SERIES || cfg->strategy == STRATEGY_SHARDING) {
        sqlite3_exec(svc->db, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;", NULL, NULL,
                     NULL);
    } else if (cfg->strategy == STRATEGY_TRANSACTION) {
        sqlite3_exec(svc->db, "PRAGMA journal_mode=DELETE; PRAGMA synchronous=FULL;", NULL, NULL,
                     NULL);
    }

    _migrate_schema(svc);

    if (cfg->strategy != STRATEGY_LITE && cfg->strategy != STRATEGY_TRANSACTION) {
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
    if (svc->config.strategy != STRATEGY_LITE && svc->config.strategy != STRATEGY_TRANSACTION) {
        uv_timer_stop(&svc->flush_timer);

        while (1) {
            if (!svc->is_flushing) {
                if (svc->active_buf->count > 0) {
                    _trigger_flush(svc);
                } else {
                    break;
                }
            }
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
    if (!svc || !msg) return -1;

    if (svc->config.strategy == STRATEGY_LITE) {
        char sql[128];
        snprintf(sql, sizeof(sql), "INSERT OR REPLACE INTO current_state VALUES (%u, %f, %llu)",
                 msg->tag_id, msg->payload.val.f, (unsigned long long)msg->ts);
        return sqlite3_exec(svc->db, sql, NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
    }

    if (svc->config.strategy == STRATEGY_TRANSACTION) {
        return 0;
    }

    uv_mutex_lock(&svc->buf_lock);
    if (svc->active_buf->count >= svc->active_buf->capacity) {
        uv_mutex_unlock(&svc->buf_lock);
        _trigger_flush(svc);
        return -1;
    }
    svc->active_buf->buffer[svc->active_buf->count++] = *msg;
    int full = (svc->active_buf->count >= svc->config.batch_size);
    uv_mutex_unlock(&svc->buf_lock);

    if (full) _trigger_flush(svc);
    return 0;
}

int sqlite_svc_query(sqlite_svc_t *svc, uint64_t start_ts, uint64_t end_ts, sqlite_svc_query_cb cb,
                     void *user_data) {
    if (!svc || !svc->db || !cb) return -1;

    char sql[512];
    if (svc->config.strategy == STRATEGY_SHARDING) {
        char shard_name[64];
        _get_shard_table(start_ts, shard_name);
        snprintf(sql, sizeof(sql),
                 "SELECT ts, tag_id, val, q FROM %s WHERE ts >= ? AND ts <= ? ORDER BY ts ASC;",
                 shard_name);
    } else {
        snprintf(
            sql, sizeof(sql),
            "SELECT ts, tag_id, val, q FROM telemetry WHERE ts >= ? AND ts <= ? ORDER BY ts ASC;");
    }

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)start_ts);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)end_ts);

    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        PipeMsg msg = {0};
        msg.ts = (uint64_t)sqlite3_column_int64(stmt, 0);
        msg.tag_id = (uint32_t)sqlite3_column_int(stmt, 1);
        msg.payload.val.f = sqlite3_column_double(stmt, 2);
        msg.quality = (uint8_t)sqlite3_column_int(stmt, 3);
        msg.type = PIPEMSG_TYPE_SCALAR;

        if (cb(user_data, &msg) != 0) {
            break;
        }
    }

    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE || rc == SQLITE_ROW) ? 0 : -1;
}

int sqlite_svc_begin_trans(sqlite_svc_t *svc) {
    return sqlite3_exec(svc->db, "BEGIN EXCLUSIVE TRANSACTION;", 0, 0, 0);
}

int sqlite_svc_commit_trans(sqlite_svc_t *svc) {
    return sqlite3_exec(svc->db, "COMMIT;", 0, 0, 0);
}

void *sqlite_svc_get_db_handle(sqlite_svc_t *svc) {
    return svc ? svc->db : NULL;
}