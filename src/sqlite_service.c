#include "sqlite_service.h"

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SCHEMA_VER_HEMS 1
#define SCHEMA_VER_IEMS 1
#define SCHEMA_VER_EVSE 1
#define SCHEMA_VER_SG 1
#define SCHEMA_VER_DCU 1

#define DB_CHECK(cmd, ctx, err_cnt)                                                            \
    do {                                                                                       \
        int rc = (cmd);                                                                        \
        if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE) {                        \
            fprintf(stderr, "[DB-ERR] %s (rc=%d): %s\n", #cmd, rc, sqlite3_errmsg((ctx)->db)); \
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

static void _get_shard_table(product_type_t type, uint64_t ts, char *out) {
    if (type == PROD_DCU) {
        snprintf(out, 64, "meter_data_%llu", (unsigned long long)ts);
    } else {
        strcpy(out, "telemetry");
    }
}

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
            return "CREATE TABLE current_state (tag_id INT PRIMARY KEY, val REAL, ts INT);";
        case PROD_IEMS:
            return "CREATE TABLE telemetry (ts INT, tag_id INT, val REAL, q INT, PRIMARY KEY(ts, "
                   "tag_id)) WITHOUT ROWID;";
        case PROD_EVSE:
            return "CREATE TABLE orders (id TEXT PRIMARY KEY, kwh REAL, status INT);";
        case PROD_SG_EL2025:
            return "CREATE TABLE waveforms (id INT PRIMARY KEY, ts INT, type INT, data BLOB);";
        case PROD_DCU:
            return "CREATE TABLE meter_meta (addr TEXT PRIMARY KEY, protocol INT);";
        default:
            return "";
    }
}

static int _migrate_schema(sqlite3 *db, product_type_t type) {
    int target_ver = _get_target_version(type);
    char sql[128];
    sqlite3_exec(db, _get_init_schema(type), NULL, NULL, NULL);
    snprintf(sql, sizeof(sql), "PRAGMA user_version=%d;", target_ver);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
    return 0;
}

static void _ensure_shard_exists(sqlite_svc_t *svc, const char *table_name) {
    if (strcmp(table_name, svc->current_shard) == 0) return;
    char sql[256];
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS %s (addr TEXT, tag_id INT, ts INT, val REAL);",
             table_name);
    if (sqlite3_exec(svc->db, sql, 0, 0, 0) == SQLITE_OK) strcpy(svc->current_shard, table_name);
}

static void _worker_flush_cb(uv_work_t *req) {
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    data_batch_t *batch = svc->flush_buf;
    if (batch->count == 0) return;
    int err_cnt = 0;
    sqlite3_stmt *stmt = NULL;
    sqlite3_exec(svc->db, "BEGIN IMMEDIATE;", 0, 0, 0);
    if (svc->config.type == PROD_DCU) {
        char table_name[64];
        _get_shard_table(PROD_DCU, batch->buffer[0].ts, table_name);
        _ensure_shard_exists(svc, table_name);
        char sql[256];
        snprintf(sql, sizeof(sql), "INSERT INTO %s VALUES (?,?,?,?)", table_name);
        sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL);
    } else {
        sqlite3_prepare_v2(svc->db, "INSERT INTO telemetry VALUES (?,?,?,?)", -1, &stmt, NULL);
    }
    for (int i = 0; i < batch->count; i++) {
        PipeMsg *msg = &batch->buffer[i];
        if (svc->config.type == PROD_DCU) {
            sqlite3_bind_text(stmt, 1, "meter_01", -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, (int)msg->tag_id);
            sqlite3_bind_int64(stmt, 3, (sqlite3_int64)msg->ts);
            sqlite3_bind_double(stmt, 4, msg->payload.val.f);
        } else {
            sqlite3_bind_int64(stmt, 1, (sqlite3_int64)msg->ts);
            sqlite3_bind_int(stmt, 2, (int)msg->tag_id);
            sqlite3_bind_double(stmt, 3, msg->payload.val.f);
            sqlite3_bind_int(stmt, 4, 0);
        }
        if (sqlite3_step(stmt) != SQLITE_DONE) err_cnt++;
        sqlite3_reset(stmt);
    }
    DB_CHECK(sqlite3_exec(svc->db, "COMMIT;", 0, 0, 0), svc, err_cnt);
    svc->stats.insert_ok += (uint64_t)(batch->count - err_cnt);
    if (stmt) sqlite3_finalize(stmt);
    return;

error_rollback:
    if (svc->db) {
        sqlite3_exec(svc->db, "ROLLBACK;", 0, 0, 0);
    }
    if (stmt) sqlite3_finalize(stmt);
}

static void _after_flush_cb(uv_work_t *req, int status) {
    (void)status;
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    svc->flush_buf->count = 0;
    svc->is_flushing = 0;
}

static void _trigger_flush(sqlite_svc_t *svc) {
    if (svc->is_flushing || svc->active_buf->count == 0) return;
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
        while (svc->is_flushing) uv_run(svc->loop, UV_RUN_ONCE);
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
    if (svc->config.type == PROD_HEMS) return 0;
    uv_mutex_lock(&svc->buf_lock);
    if (svc->active_buf->count >= svc->active_buf->capacity) {
        uv_mutex_unlock(&svc->buf_lock);
        return -1;
    }
    svc->active_buf->buffer[svc->active_buf->count++] = *msg;
    int full = (svc->active_buf->count >= svc->config.batch_size);
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
