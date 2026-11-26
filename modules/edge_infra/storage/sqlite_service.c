/**
 * @file sqlite_service.c
 * @brief 终极版工业级持久化引擎 (支持 BLOB 参数、时序分表、自动迁移)
 */

#include "sqlite_service.h"
#include <uv.h>

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>

#include "edge_log.h"

// 数据库架构版本 (V9: 轻量任务)
#define EDGE_DB_SCHEMA_VERSION 18

typedef struct {
    PipeMsg *buffer;
    /** 每条记录在插入边界采样的墙钟 (epoch µs)；零分配路径没有这块内存，为 NULL。 */
    uint64_t *wall_ts_us;
    int count;
    int capacity;
} data_batch_t;

struct sqlite_svc_s {
    sqlite3 *db;
    sqlite3 *flush_db; /**< 仅刷新 worker 使用；:memory: 时复用 db */
    uv_loop_t *loop;
    sqlite_config_t config;
    data_batch_t *active_buf;
    data_batch_t *flush_buf;
    data_batch_t static_active_buf;
    data_batch_t static_flush_buf;
    uv_mutex_t buf_lock;
    uv_work_t work_req;
    uv_timer_t flush_timer;
    uv_timer_t gc_timer; /**< [Phase 9] 存储审计定时器 */
    _Atomic uint64_t last_wall_us; /**< 本库最近一次写入的墙钟 (epoch µs) */
    
    // --- [Backpressure] 背压控制 ---
    atomic_bool is_busy;      /**< [新增] 标记后台线程是否正在忙于刷盘 */
    uint64_t total_dropped;   /**< [新增] 因背压丢弃的消息计数 */

    int is_flushing;
    int last_flush_rc; /**< 异步 worker 的最近结果；非 0 时保留批次重试 */
    bool is_allocated;
    bool are_buffers_allocated;
    uint8_t *batch_block; /**< SERIES/SHARDING 的 PipeMsg+墙钟合并分配块 */
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

/* 分表按月命名，月份来自记录墙钟 (epoch µs)。 */
static void _get_shard_table_us(uint64_t wall_ts_us, char *out) {
    time_t sec = (time_t)(wall_ts_us / 1000000ULL);
    struct tm t;
    gmtime_r(&sec, &t);
    snprintf(out, 64, "telemetry_%04d%02d", t.tm_year + 1900, t.tm_mon + 1);
}

static uint64_t _realtime_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

/* 插入边界的墙钟：严格递增，同一微秒的突发也不会撞 (wall_ts_us, tag_id) 主键。 */
static uint64_t _next_wall_us(sqlite_svc_t *svc) {
    uint64_t now = _realtime_us();
    uint64_t last = atomic_load_explicit(&svc->last_wall_us, memory_order_relaxed);
    for (;;) {
        uint64_t next = now > last ? now : last + 1;
        if (atomic_compare_exchange_weak_explicit(&svc->last_wall_us, &last, next,
                                                  memory_order_relaxed, memory_order_relaxed)) {
            return next;
        }
    }
}

static void _note_wall_us(sqlite_svc_t *svc, uint64_t wall_ts_us) {
    uint64_t last = atomic_load_explicit(&svc->last_wall_us, memory_order_relaxed);
    while (last < wall_ts_us &&
           !atomic_compare_exchange_weak_explicit(&svc->last_wall_us, &last, wall_ts_us,
                                                  memory_order_relaxed, memory_order_relaxed)) {
    }
}

static bool _table_exists(sqlite3 *db, const char *table_name) {
    sqlite3_stmt *stmt = NULL;
    bool exists = false;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM sqlite_master WHERE type='table' AND name = ?;", -1,
                           &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, table_name, -1, SQLITE_STATIC);
        exists = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
    }
    return exists;
}

static bool _table_has_wall_us(sqlite3 *db, const char *table_name) {
    sqlite3_stmt *stmt = NULL;
    bool has = false;
    if (sqlite3_prepare_v2(db,
                           "SELECT 1 FROM pragma_table_info(?) WHERE name = 'wall_ts_us';", -1,
                           &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, table_name, -1, SQLITE_STATIC);
        has = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
    }
    return has;
}

/* 旧形状分表不能静默复用：迁移没跑到或库被降级时 fail closed，让调用方看到失败。 */
static int _ensure_shard_exists(sqlite3 *db, const char *table_name, char *current_shard) {
    if (strcmp(table_name, current_shard) == 0) return 0;
    if (_table_exists(db, table_name)) {
        if (!_table_has_wall_us(db, table_name)) {
            LOG_ERROR(LOG_CAT_CORE,
                      "DB: shard %s has no wall_ts_us column (stale schema); reopen to migrate",
                      table_name);
            return -1;
        }
        strcpy(current_shard, table_name);
        return 0;
    }
    char sql[320];
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS %s (wall_ts_us INTEGER NOT NULL, tag_id INT NOT NULL, "
             "ts INT, val REAL, q INT, synced INT DEFAULT 0, "
             "PRIMARY KEY(wall_ts_us, tag_id)) WITHOUT ROWID;",
             table_name);
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_CORE, "DB: cannot create shard %s: %s", table_name, sqlite3_errmsg(db));
        return -1;
    }
    strcpy(current_shard, table_name);
    return 0;
}

// --- 2. 模式迁移 (Migration) ---
//
// 迁移只增不改：已发布的迁移 SQL 不可再编辑，否则 schema_migration 中的 checksum 会对不上。
// 需要变更结构时追加新版本，不要改写旧版本。

typedef struct {
    int version;
    const char *name;
    const char *sql;
} db_migration_t;

static const db_migration_t k_migrations[] = {
    {1, "telemetry_inventory",
     "CREATE TABLE IF NOT EXISTS telemetry (ts INT, tag_id INT, val REAL, q INT, synced INT "
     "DEFAULT 0, PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;"
     "CREATE TABLE IF NOT EXISTS inventory (id TEXT PRIMARY KEY, type TEXT, addr INT, protocol "
     "TEXT, enabled INT DEFAULT 1);"},
    {2, "edge_settings",
     "CREATE TABLE IF NOT EXISTS edge_settings (product TEXT, tag_id INT, val BLOB, PRIMARY "
     "KEY(product, tag_id));"},
    {3, "telemetry_tag_index",
     "CREATE INDEX IF NOT EXISTS idx_telemetry_tag ON telemetry(tag_id);"},
    // v3 原实现 DROP 了 edge_settings，会丢数据；v1/v2 建的表本来就有 BLOB 亲和性，这里只做幂等补建。
    {4, "edge_settings_blob_shape",
     "CREATE TABLE IF NOT EXISTS edge_settings (product TEXT, tag_id INT, val BLOB, PRIMARY "
     "KEY(product, tag_id));"},
    {5, "app_registry",
     "CREATE TABLE IF NOT EXISTS app_registry ("
     "id TEXT PRIMARY KEY, name TEXT, version TEXT, author TEXT, wasm_path TEXT, trigger_tag INT, "
     "entry_func TEXT, fault_policy INT, heap_limit_kb INT, timeout_ms INT, state INT, "
     "restart_count INT, created_at INT);"},
    {6, "app_grant_authz",
     "CREATE TABLE IF NOT EXISTS app_grant ("
     "app_id TEXT NOT NULL, capability TEXT NOT NULL, scope_json TEXT, state INTEGER, purpose TEXT, "
     "expires_at INTEGER, sensitivity_max INTEGER, granted_by TEXT, granted_at INTEGER, "
     "revoked_at INTEGER, PRIMARY KEY(app_id, capability));"
     "CREATE TABLE IF NOT EXISTS publisher_trust ("
     "publisher_id TEXT PRIMARY KEY, trust_level INTEGER, verified INTEGER);"
     "CREATE TABLE IF NOT EXISTS point_profile (scope TEXT PRIMARY KEY, sensitivity INTEGER);"},
    {7, "app_pressure_fields",
     "ALTER TABLE app_registry ADD COLUMN priority INTEGER NOT NULL DEFAULT 0;"
     "ALTER TABLE app_registry ADD COLUMN keep_alive INTEGER NOT NULL DEFAULT 0;"
     "ALTER TABLE app_registry ADD COLUMN desired_state INTEGER NOT NULL DEFAULT 1;"
     "ALTER TABLE app_registry ADD COLUMN last_active_ms INTEGER NOT NULL DEFAULT 0;"},
    {8, "app_secret",
     "CREATE TABLE IF NOT EXISTS app_secret ("
     "app_id TEXT NOT NULL, name TEXT NOT NULL, value BLOB NOT NULL, PRIMARY KEY(app_id, name));"},
    {9, "app_task",
     "CREATE TABLE IF NOT EXISTS app_task ("
     "id TEXT PRIMARY KEY, kind INTEGER NOT NULL, idempotency_key TEXT NOT NULL UNIQUE, "
     "state INTEGER NOT NULL, created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL, "
     "result_json TEXT);"},
    {10, "app_registry_prev_version",
     "ALTER TABLE app_registry ADD COLUMN prev_version TEXT NOT NULL DEFAULT '';"
     "ALTER TABLE app_registry ADD COLUMN prev_manifest_json TEXT NOT NULL DEFAULT '';"
     "ALTER TABLE app_registry ADD COLUMN prev_wasm_path TEXT NOT NULL DEFAULT '';"},
    {11, "registry_generation",
     "CREATE TABLE IF NOT EXISTS registry_meta (key TEXT PRIMARY KEY, value TEXT NOT NULL);"
     "INSERT OR IGNORE INTO registry_meta (key, value) VALUES ('generation', '0');"
     "CREATE TRIGGER IF NOT EXISTS app_registry_gen_insert AFTER INSERT ON app_registry BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"
     "CREATE TRIGGER IF NOT EXISTS app_registry_gen_update AFTER UPDATE ON app_registry BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"
     "CREATE TRIGGER IF NOT EXISTS app_registry_gen_delete AFTER DELETE ON app_registry BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"
     "CREATE TRIGGER IF NOT EXISTS app_grant_gen_insert AFTER INSERT ON app_grant BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"
     "CREATE TRIGGER IF NOT EXISTS app_grant_gen_update AFTER UPDATE ON app_grant BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"
     "CREATE TRIGGER IF NOT EXISTS app_grant_gen_delete AFTER DELETE ON app_grant BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"},
    {12, "app_point_slots",
     "CREATE TABLE IF NOT EXISTS app_point ("
     "app_id TEXT NOT NULL, name TEXT NOT NULL, slot INTEGER NOT NULL, type INTEGER NOT NULL, "
     "unit TEXT, contract_version INTEGER NOT NULL, retention INTEGER NOT NULL, "
     "stale_after_ms INTEGER NOT NULL, stability INTEGER NOT NULL, source INTEGER NOT NULL, "
     "PRIMARY KEY(app_id, name));"
     "CREATE UNIQUE INDEX IF NOT EXISTS idx_app_point_slot ON app_point(slot);"
     "CREATE TABLE IF NOT EXISTS app_slot_alloc (app_id TEXT PRIMARY KEY, "
     "base_slot INTEGER NOT NULL UNIQUE, slot_count INTEGER NOT NULL);"
     "CREATE TRIGGER IF NOT EXISTS app_point_gen_insert AFTER INSERT ON app_point BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"
     "CREATE TRIGGER IF NOT EXISTS app_point_gen_update AFTER UPDATE ON app_point BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"
     "CREATE TRIGGER IF NOT EXISTS app_point_gen_delete AFTER DELETE ON app_point BEGIN "
     "UPDATE registry_meta SET value = CAST(CAST(value AS INTEGER) + 1 AS TEXT) "
     "WHERE key = 'generation'; END;"},
    {13, "app_fault_window",
     "ALTER TABLE app_registry ADD COLUMN last_fault_ms INTEGER NOT NULL DEFAULT 0;"},
    {14, "system_time_trust",
     "CREATE TABLE IF NOT EXISTS system_time (key TEXT PRIMARY KEY, value INTEGER NOT NULL);"},
    {15, "app_limits",
     "CREATE TABLE IF NOT EXISTS app_limits ("
     "app_id TEXT PRIMARY KEY, heap_kb INTEGER NOT NULL DEFAULT 0, "
     "storage_kb INTEGER NOT NULL DEFAULT 0, cpu_ms_per_sec INTEGER NOT NULL DEFAULT 0, "
     "calls_per_sec INTEGER NOT NULL DEFAULT 0, publish_per_sec INTEGER NOT NULL DEFAULT 0, "
     "pids_max INTEGER NOT NULL DEFAULT 0);"},
    {16, "app_tick_ms",
     "ALTER TABLE app_registry ADD COLUMN tick_ms INTEGER NOT NULL DEFAULT 0;"},
    {17, "app_settings",
     "CREATE TABLE IF NOT EXISTS app_setting_schema ("
     "app_id TEXT PRIMARY KEY, schema_json TEXT NOT NULL);"
     "CREATE TABLE IF NOT EXISTS app_settings ("
     "app_id TEXT NOT NULL, key TEXT NOT NULL, value_json TEXT NOT NULL, "
     "PRIMARY KEY(app_id, key));"},
    /* telemetry 主键改为墙钟 (wall_ts_us, tag_id)。ts 仅保留单调时间用于排障与
     * 旧查询。旧库没有 shipped 数据，迁移直接重建表（打开后为空表）。 */
    {18, "telemetry_wall_key",
     "DROP TABLE IF EXISTS telemetry;"
     "CREATE TABLE telemetry ("
     "wall_ts_us INTEGER NOT NULL, tag_id INT NOT NULL, ts INT, val REAL, q INT, "
     "synced INT DEFAULT 0, PRIMARY KEY(wall_ts_us, tag_id)) WITHOUT ROWID;"
     "CREATE INDEX IF NOT EXISTS idx_telemetry_tag ON telemetry(tag_id);"
     "CREATE INDEX IF NOT EXISTS idx_telemetry_ts ON telemetry(ts);"
     "CREATE INDEX IF NOT EXISTS idx_telemetry_synced ON telemetry(synced) WHERE synced = 0;"},
};

int sqlite_service_schema_version(void) {
    return EDGE_DB_SCHEMA_VERSION;
}

#define DB_MIGRATION_COUNT (sizeof(k_migrations) / sizeof(k_migrations[0]))

_Static_assert(sizeof(k_migrations) / sizeof(k_migrations[0]) == EDGE_DB_SCHEMA_VERSION,
               "k_migrations must cover every schema version");

static const char *k_migration_table_sql =
    "CREATE TABLE IF NOT EXISTS schema_migration (version INTEGER PRIMARY KEY, name TEXT NOT "
    "NULL, checksum TEXT NOT NULL, applied_at INTEGER NOT NULL);";

// FNV-1a 64: 固定迁移内容指纹，防止已发布的迁移被改写。
static void _migration_checksum(const db_migration_t *m, char out[17]) {
    uint64_t h = 14695981039346656037ULL;
    for (const char *p = m->name; *p; p++) {
        h ^= (uint8_t)*p;
        h *= 1099511628211ULL;
    }
    for (const char *p = m->sql; *p; p++) {
        h ^= (uint8_t)*p;
        h *= 1099511628211ULL;
    }
    snprintf(out, 17, "%016llx", (unsigned long long)h);
}

static int _record_migration(sqlite_svc_t *svc, const db_migration_t *m, int applied_at) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(svc->db,
                           "INSERT OR IGNORE INTO schema_migration (version, name, checksum, "
                           "applied_at) VALUES (?, ?, ?, ?);",
                           -1, &stmt, NULL) != SQLITE_OK)
        return -1;
    char checksum[17];
    _migration_checksum(m, checksum);
    sqlite3_bind_int(stmt, 1, m->version);
    sqlite3_bind_text(stmt, 2, m->name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, checksum, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)applied_at);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

static int _verify_migration_records(sqlite_svc_t *svc) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(svc->db, "SELECT version, checksum FROM schema_migration;", -1, &stmt,
                           NULL) != SQLITE_OK)
        return -1;
    int rc = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int version = sqlite3_column_int(stmt, 0);
        const char *recorded = (const char *)sqlite3_column_text(stmt, 1);
        for (size_t i = 0; i < DB_MIGRATION_COUNT; i++) {
            if (k_migrations[i].version != version) continue;
            char expected[17];
            _migration_checksum(&k_migrations[i], expected);
            if (!recorded || strcmp(recorded, expected) != 0) {
                LOG_ERROR(LOG_CAT_CORE,
                          "DB: Migration v%d (%s) checksum mismatch (recorded %s, expected %s), "
                          "refusing to open",
                          version, k_migrations[i].name, recorded ? recorded : "(null)", expected);
                rc = -1;
            }
            break;
        }
        if (rc != 0) break;
    }
    sqlite3_finalize(stmt);
    return rc;
}

// 旧库（schema_migration 表出现之前）没有记录，按 user_version 补齐；INSERT OR IGNORE 不会覆盖已有记录。
static int _backfill_migration_records(sqlite_svc_t *svc, int current_ver) {
    for (size_t i = 0; i < DB_MIGRATION_COUNT; i++) {
        if (k_migrations[i].version > current_ver) continue;
        if (_record_migration(svc, &k_migrations[i], (int)time(NULL)) != 0) return -1;
    }
    return 0;
}

static int _read_user_version(sqlite_svc_t *svc) {
    sqlite3_stmt *stmt = NULL;
    int version = 0;
    if (sqlite3_prepare_v2(svc->db, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) version = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return version;
}

static bool _migration_recorded(sqlite_svc_t *svc, int version) {
    sqlite3_stmt *stmt = NULL;
    bool recorded = false;
    if (sqlite3_prepare_v2(svc->db, "SELECT 1 FROM schema_migration WHERE version = ?;", -1, &stmt,
                           NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, version);
        recorded = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
    }
    return recorded;
}

#define TELEMETRY_CREATE_SQL                                                                    \
    "CREATE TABLE %s (wall_ts_us INTEGER NOT NULL, tag_id INT NOT NULL, ts INT, val REAL, q INT, " \
    "synced INT DEFAULT 0, PRIMARY KEY(wall_ts_us, tag_id)) WITHOUT ROWID;"

static int _rebuild_one_telemetry(sqlite3 *db, const char *name, bool base) {
    char *quoted = sqlite3_mprintf("\"%w\"", name);
    if (!quoted) return -1;
    char *drop = sqlite3_mprintf("DROP TABLE %s;", quoted);
    char *create = sqlite3_mprintf(TELEMETRY_CREATE_SQL, quoted);
    int rc = -1;
    if (drop && create && sqlite3_exec(db, drop, NULL, NULL, NULL) == SQLITE_OK &&
        sqlite3_exec(db, create, NULL, NULL, NULL) == SQLITE_OK) {
        rc = 0;
        if (base) {
            sqlite3_exec(db,
                         "CREATE INDEX IF NOT EXISTS idx_telemetry_tag ON telemetry(tag_id);",
                         NULL, NULL, NULL);
            sqlite3_exec(db, "CREATE INDEX IF NOT EXISTS idx_telemetry_ts ON telemetry(ts);",
                         NULL, NULL, NULL);
            sqlite3_exec(db,
                         "CREATE INDEX IF NOT EXISTS idx_telemetry_synced ON telemetry(synced) "
                         "WHERE synced = 0;",
                         NULL, NULL, NULL);
        }
    }
    sqlite3_free(drop);
    sqlite3_free(create);
    sqlite3_free(quoted);
    return rc;
}

/* v18 的 DROP/CREATE 只覆盖基表；旧分表升级后仍可能是 ts 主键。这里幂等扫描
 * telemetry_YYYYMM，缺 wall_ts_us 就按同样策略 DROP 重建（无 shipped 数据）。 */
static int _rebuild_stale_telemetry(sqlite_svc_t *svc) {
    char names[64][64];
    int n = 0;
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT name FROM sqlite_master WHERE type='table' AND "
        "(name='telemetry' OR name GLOB 'telemetry_[0-9][0-9][0-9][0-9][0-9][0-9]') "
        "ORDER BY name;";
    if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_CORE, "DB: cannot scan telemetry shards: %s", sqlite3_errmsg(svc->db));
        return -1;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *name = (const char *)sqlite3_column_text(stmt, 0);
        if (!name) continue;
        if (n >= (int)(sizeof(names) / sizeof(names[0]))) {
            LOG_ERROR(LOG_CAT_CORE, "DB: too many telemetry shard tables (max %zu)",
                      sizeof(names) / sizeof(names[0]));
            sqlite3_finalize(stmt);
            return -1;
        }
        snprintf(names[n], sizeof(names[n]), "%s", name);
        n++;
    }
    sqlite3_finalize(stmt);

    for (int i = 0; i < n; i++) {
        if (_table_has_wall_us(svc->db, names[i])) continue;
        LOG_WARN(LOG_CAT_CORE,
                 "DB: rebuilding stale telemetry table %s (no wall_ts_us; no shipped data)",
                 names[i]);
        if (_rebuild_one_telemetry(svc->db, names[i], strcmp(names[i], "telemetry") == 0) != 0) {
            LOG_ERROR(LOG_CAT_CORE, "DB: cannot rebuild stale telemetry table %s", names[i]);
            return -1;
        }
    }
    return 0;
}

/**
 * 迁移在 BEGIN IMMEDIATE 之后重读 user_version 与 schema_migration，再决定应用哪些版本。
 * 事务外读到的版本可能已经过期：并发的另一个 opener 提交 v18 后，旧快照仍会执行
 * DROP/CREATE，把已提交的新数据删掉。锁内重读把迁移变成幂等操作。
 */
static int _db_run_migration(sqlite_svc_t *svc) {
    char *err = NULL;
    if (sqlite3_exec(svc->db, "BEGIN IMMEDIATE;", NULL, NULL, &err) != SQLITE_OK) {
        int code = sqlite3_extended_errcode(svc->db);
        if (code == SQLITE_BUSY || code == SQLITE_LOCKED) {
            LOG_ERROR(LOG_CAT_CORE,
                      "DB: another migration is in progress (database is locked); retry later");
        } else {
            LOG_ERROR(LOG_CAT_CORE, "DB: Cannot begin migration: %s", err ? err : "Unknown");
        }
        sqlite3_free(err);
        return -1;
    }

    int db_ver = _read_user_version(svc);
    if (db_ver > EDGE_DB_SCHEMA_VERSION) {
        LOG_ERROR(LOG_CAT_CORE,
                  "DB: Schema version %d is newer than supported %d, refusing to open", db_ver,
                  EDGE_DB_SCHEMA_VERSION);
        sqlite3_exec(svc->db, "ROLLBACK;", NULL, NULL, NULL);
        return SQLITE_SVC_ERR_SCHEMA_TOO_NEW;
    }
    LOG_INFO(LOG_CAT_CORE, "DB: Current version %d, migrating to %d", db_ver,
             EDGE_DB_SCHEMA_VERSION);

    if (sqlite3_exec(svc->db, k_migration_table_sql, NULL, NULL, &err) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_CORE, "DB: Cannot create schema_migration: %s", err ? err : "Unknown");
        sqlite3_free(err);
        sqlite3_exec(svc->db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }
    if (_verify_migration_records(svc) != 0) {
        sqlite3_exec(svc->db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }
    if (_backfill_migration_records(svc, db_ver) != 0) {
        LOG_ERROR(LOG_CAT_CORE, "DB: Cannot backfill schema_migration");
        sqlite3_exec(svc->db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }

    for (size_t i = 0; i < DB_MIGRATION_COUNT; i++) {
        const db_migration_t *m = &k_migrations[i];
        if (m->version <= db_ver) continue;
        if (_migration_recorded(svc, m->version)) continue;
        if (sqlite3_exec(svc->db, m->sql, NULL, NULL, &err) != SQLITE_OK) {
            LOG_ERROR(LOG_CAT_CORE, "DB Migration v%d (%s) failed: %s", m->version, m->name,
                      err ? err : "Unknown");
            sqlite3_free(err);
            sqlite3_exec(svc->db, "ROLLBACK;", NULL, NULL, NULL);
            return -1;
        }
        if (_record_migration(svc, m, (int)time(NULL)) != 0) {
            LOG_ERROR(LOG_CAT_CORE, "DB: Cannot record migration v%d (%s)", m->version, m->name);
            sqlite3_exec(svc->db, "ROLLBACK;", NULL, NULL, NULL);
            return -1;
        }
        LOG_INFO(LOG_CAT_CORE, "DB: Applied migration v%d (%s)", m->version, m->name);
    }

    /* 已记录 v18 但库里还留着旧形状分表时，也要在这次事务里重建。 */
    if (_rebuild_stale_telemetry(svc) != 0) {
        sqlite3_exec(svc->db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }

    char sql[64];
    snprintf(sql, sizeof(sql), "PRAGMA user_version=%d;", EDGE_DB_SCHEMA_VERSION);
    if (sqlite3_exec(svc->db, sql, NULL, NULL, &err) != SQLITE_OK ||
        sqlite3_exec(svc->db, "COMMIT;", NULL, NULL, &err) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_CORE, "DB: Cannot commit migration: %s", err ? err : "Unknown");
        sqlite3_free(err);
        sqlite3_exec(svc->db, "ROLLBACK;", NULL, NULL, NULL);
        return -1;
    }
    return 0;
}

static int _migrate_schema(sqlite_svc_t *svc) {
    // 先于任何写入拒绝过新的库，避免旧程序改写新库。
    int current_ver = _read_user_version(svc);
    if (current_ver > EDGE_DB_SCHEMA_VERSION) {
        LOG_ERROR(LOG_CAT_CORE, "DB: Schema version %d is newer than supported %d, refusing to open",
                  current_ver, EDGE_DB_SCHEMA_VERSION);
        return SQLITE_SVC_ERR_SCHEMA_TOO_NEW;
    }
    return _db_run_migration(svc);
}

// --- 3. 异步/同步冲刷逻辑 ---

/* 刷新 worker 用自己的连接。共用 loop 连接会让 worker 的 BEGIN/COMMIT
 * 把 loop 线程的一条语句卷进同一个事务；:memory: 无法跨连接共享，退回 db。 */
static sqlite3 *flush_connection(sqlite_svc_t *svc) {
    if (svc->flush_db) return svc->flush_db;
    if (svc->config.db_path[0] == '\0' || strcmp(svc->config.db_path, ":memory:") == 0) {
        return svc->db;
    }
    sqlite3 *db = NULL;
    if (sqlite3_open(svc->config.db_path, &db) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_CORE, "DB: flush connection open failed: %s",
                  db ? sqlite3_errmsg(db) : "out of memory");
        if (db) sqlite3_close(db);
        return svc->db;
    }
    sqlite3_busy_timeout(db, 5000);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL; PRAGMA temp_store=MEMORY;", NULL, NULL, NULL);
    svc->flush_db = db;
    return db;
}

static int _flush_batch_internal(sqlite_svc_t *svc, sqlite3 *db, data_batch_t *batch) {
    if (!svc || !db || !batch || batch->count == 0) return 0;

    char *err = NULL;
    if (sqlite3_exec(db, "BEGIN IMMEDIATE;", NULL, NULL, &err) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_CORE, "DB: flush cannot begin: %s", err ? err : "Unknown");
        sqlite3_free(err);
        return -1;
    }

    int rc = 0;
    sqlite3_stmt *stmt = NULL;
    const char *current_table = NULL;
    uint64_t prev_wall_us = 0;

    for (int i = 0; i < batch->count && rc == 0; i++) {
        PipeMsg *msg = &batch->buffer[i];
        uint64_t wall_ts_us = batch->wall_ts_us ? batch->wall_ts_us[i] : _realtime_us();
        if (!batch->wall_ts_us && wall_ts_us <= prev_wall_us) wall_ts_us = prev_wall_us + 1;
        prev_wall_us = wall_ts_us;
        const char *table = "telemetry";
        char shard_name[64];
        if (svc->config.strategy == STRATEGY_SHARDING) {
            _get_shard_table_us(wall_ts_us, shard_name);
            if (_ensure_shard_exists(db, shard_name, svc->current_shard) != 0) {
                rc = -1;
                break;
            }
            table = shard_name;
        }

        if (current_table == NULL || strcmp(table, current_table) != 0) {
            if (stmt) sqlite3_finalize(stmt);
            char sql[256];
            snprintf(sql, sizeof(sql),
                     "INSERT OR REPLACE INTO %s (wall_ts_us, tag_id, ts, val, q) "
                     "VALUES (?,?,?,?,?)",
                     table);
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
                LOG_ERROR(LOG_CAT_CORE, "DB: flush prepare failed for %s: %s", table,
                          sqlite3_errmsg(db));
                stmt = NULL;
                rc = -1;
                break;
            }
            current_table = table;
        }

        sqlite3_bind_int64(stmt, 1, (sqlite3_int64)wall_ts_us);
        sqlite3_bind_int(stmt, 2, (int)msg->tag_id);
        sqlite3_bind_int64(stmt, 3, (sqlite3_int64)msg->ts);
        sqlite3_bind_double(stmt, 4, msg->payload.val.f);
        sqlite3_bind_int(stmt, 5, (int)msg->quality);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            LOG_ERROR(LOG_CAT_CORE, "DB: flush insert failed for %s: %s", table,
                      sqlite3_errmsg(db));
            rc = -1;
            break;
        }
        sqlite3_reset(stmt);
    }

    if (stmt) sqlite3_finalize(stmt);

    if (rc == 0 && sqlite3_exec(db, "COMMIT;", NULL, NULL, &err) != SQLITE_OK) {
        LOG_ERROR(LOG_CAT_CORE, "DB: flush commit failed: %s", err ? err : "Unknown");
        sqlite3_free(err);
        rc = -1;
    }
    if (rc != 0) sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    return rc;
}

static void _worker_flush_cb(uv_work_t *req) {
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    data_batch_t *batch = svc->flush_buf;
    svc->last_flush_rc = 0;
    if (!batch || batch->count == 0) return;
    svc->last_flush_rc = _flush_batch_internal(svc, flush_connection(svc), batch);
    if (svc->last_flush_rc != 0) {
        LOG_ERROR(LOG_CAT_CORE, "DB: async flush failed; %d row(s) kept for retry", batch->count);
    }
}

int sqlite_svc_flush(sqlite_svc_t *svc) {
    if (!svc || !svc->db) return -1;
    int rc = 0;
    if (svc->active_buf) {
        uv_mutex_lock(&svc->buf_lock);
        if (svc->active_buf->count > 0) {
            rc = _flush_batch_internal(svc, svc->db, svc->active_buf);
            if (rc == 0) svc->active_buf->count = 0;
        }
        uv_mutex_unlock(&svc->buf_lock);
    }
    return rc;
}

static void _after_flush_cb(uv_work_t *req, int status) {
    (void)status;
    sqlite_svc_t *svc = (sqlite_svc_t *)req->data;
    if (svc->last_flush_rc == 0) {
        svc->flush_buf->count = 0;
    } else {
        LOG_ERROR(LOG_CAT_CORE, "DB: async flush failed; keeping %d row(s) for retry",
                  svc->flush_buf->count);
    }
    svc->is_flushing = 0;
    atomic_store_explicit(&svc->is_busy, false, memory_order_release);
}

static int _trigger_flush(sqlite_svc_t *svc) {
    if (svc->is_flushing || !svc->active_buf) return -1;
    if (svc->flush_buf->count == 0) {
        if (svc->active_buf->count == 0) return -1;
        uv_mutex_lock(&svc->buf_lock);
        data_batch_t *tmp = svc->active_buf;
        svc->active_buf = svc->flush_buf;
        svc->flush_buf = tmp;
        uv_mutex_unlock(&svc->buf_lock);
    }
    /* flush_buf 非空说明上一轮异步刷盘失败，优先重试它；新数据留在 active 下一轮。 */
    svc->is_flushing = 1;
    atomic_store_explicit(&svc->is_busy, true, memory_order_release);
    svc->work_req.data = svc;
    return uv_queue_work(svc->loop, &svc->work_req, _worker_flush_cb, _after_flush_cb);
}

static void _on_timer(uv_timer_t *handle) {
    _trigger_flush((sqlite_svc_t *)handle->data);
}

// --- 4. 公开 API (核心补强) ---

sqlite_svc_t *sqlite_svc_create_in(sqlite_svc_storage_t *storage, uv_loop_t *loop, sqlite_config_t *cfg,
                                   PipeMsg *active_buf_mem, int active_capacity,
                                   PipeMsg *flush_buf_mem, int flush_capacity) {
    if (!storage || !cfg) return NULL;
    _Static_assert(sizeof(struct sqlite_svc_s) <= sizeof(sqlite_svc_storage_t), "sqlite_svc_t exceeds storage");
    sqlite_svc_t *svc = (sqlite_svc_t *)storage;
    memset(svc, 0, sizeof(sqlite_svc_t));
    atomic_init(&svc->last_wall_us, 0);
    svc->loop = loop;
    svc->config = *cfg;
    svc->is_allocated = false;
    if (sqlite3_open(cfg->db_path, &svc->db) != SQLITE_OK) {
        return NULL;
    }
    sqlite3_busy_timeout(svc->db, 5000);
    if (_migrate_schema(svc) != 0) {
        sqlite3_close(svc->db);
        svc->db = NULL;
        return NULL;
    }
    sqlite3_exec(svc->db, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL; PRAGMA temp_store=MEMORY; PRAGMA wal_autocheckpoint=1000;", NULL, NULL, NULL);
    if (cfg->strategy == STRATEGY_SERIES || cfg->strategy == STRATEGY_SHARDING) {
        /* Caller-allocated storage contract: a placement constructor must not fall back
         * to the heap. Missing buffers are a programming error, not a reason to malloc
         * (#411). `sqlite_svc_create()` keeps the owning variant for dynamic use. */
        if (!active_buf_mem || active_capacity <= 0 || !flush_buf_mem || flush_capacity <= 0) {
            sqlite3_close(svc->db);
            svc->db = NULL;
            return NULL;
        }
        svc->active_buf = &svc->static_active_buf;
        svc->flush_buf = &svc->static_flush_buf;
        svc->active_buf->buffer = active_buf_mem;
        svc->active_buf->capacity = active_capacity;
        svc->flush_buf->buffer = flush_buf_mem;
        svc->flush_buf->capacity = flush_capacity;
        /* 零分配契约没有墙钟数组；该路径按 flush 时刻补采墙钟。SnF 走 LITE。 */
        svc->active_buf->wall_ts_us = NULL;
        svc->flush_buf->wall_ts_us = NULL;
        svc->are_buffers_allocated = false;
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

    atomic_init(&svc->is_busy, false);
    return svc;
}

sqlite_svc_t *sqlite_svc_create_ex(uv_loop_t *loop, sqlite_config_t *cfg, int *out_error) {
    if (out_error) *out_error = 0;
    if (!cfg) {
        if (out_error) *out_error = -1;
        return NULL;
    }
    sqlite_svc_t *svc = calloc(1, sizeof(sqlite_svc_t));
    if (!svc) {
        if (out_error) *out_error = -1;
        return NULL;
    }
    svc->is_allocated = true;
    svc->loop = loop;
    svc->config = *cfg;
    atomic_init(&svc->last_wall_us, 0);
    if (sqlite3_open(cfg->db_path, &svc->db) != SQLITE_OK) {
        if (svc->db) sqlite3_close(svc->db);
        free(svc);
        if (out_error) *out_error = -1;
        return NULL;
    }
    sqlite3_busy_timeout(svc->db, 5000);
    int migrate_rc = _migrate_schema(svc);
    if (migrate_rc != 0) {
        sqlite3_close(svc->db);
        free(svc);
        if (out_error) *out_error = migrate_rc;
        return NULL;
    }
    sqlite3_exec(svc->db, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL; PRAGMA temp_store=MEMORY; PRAGMA wal_autocheckpoint=1000;", NULL, NULL, NULL);
    if (cfg->strategy == STRATEGY_SERIES || cfg->strategy == STRATEGY_SHARDING) {
        svc->active_buf = &svc->static_active_buf;
        svc->flush_buf = &svc->static_flush_buf;
        int bsize = cfg->batch_size > 0 ? cfg->batch_size : 100;
        size_t msg_bytes = sizeof(PipeMsg) * (size_t)bsize;
        size_t wall_bytes = sizeof(uint64_t) * (size_t)bsize;
        /* 一个块装下 active/flush 的 PipeMsg 与墙钟数组：模块 heap 调用不增长，
         * flush 交换的是 data_batch_t 指针，释放时只认这个块基址。 */
        uint8_t *block = malloc(2 * (msg_bytes + wall_bytes));
        if (!block) {
            sqlite3_close(svc->db);
            free(svc);
            if (out_error) *out_error = -1;
            return NULL;
        }
        svc->batch_block = block;
        svc->active_buf->buffer = (PipeMsg *)block;
        svc->active_buf->wall_ts_us = (uint64_t *)(block + msg_bytes);
        svc->flush_buf->buffer = (PipeMsg *)(block + msg_bytes + wall_bytes);
        svc->flush_buf->wall_ts_us = (uint64_t *)(block + 2 * msg_bytes + wall_bytes);
        svc->active_buf->capacity = bsize;
        svc->flush_buf->capacity = bsize;
        svc->are_buffers_allocated = true;
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

    atomic_init(&svc->is_busy, false);
    return svc;
}

sqlite_svc_t *sqlite_svc_create(uv_loop_t *loop, sqlite_config_t *cfg) {
    return sqlite_svc_create_ex(loop, cfg, NULL);
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

static int _insert_impl(sqlite_svc_t *svc, const PipeMsg *msg, uint64_t wall_ts_us,
                        bool store_wall) {
    // [Backpressure] 简单背压：如果后台正在高载刷盘且缓冲区已满，则丢弃数据
    if (atomic_load_explicit(&svc->is_busy, memory_order_acquire)) {
        if (svc->active_buf && svc->active_buf->count >= svc->active_buf->capacity) {
            svc->total_dropped++;
            return -1;
        }
    }

    if (svc->config.strategy == STRATEGY_LITE) {
        char sql[256];
        snprintf(sql, sizeof(sql),
                 "INSERT OR REPLACE INTO telemetry (wall_ts_us, tag_id, ts, val, q) "
                 "VALUES (%llu, %u, %llu, %f, %u);",
                 (unsigned long long)wall_ts_us, msg->tag_id, (unsigned long long)msg->ts,
                 msg->payload.val.f, msg->quality);
        return sqlite3_exec(svc->db, sql, 0, 0, 0) == SQLITE_OK ? 0 : -1;
    }
    if (!svc->active_buf) return -1;
    if (store_wall && !svc->active_buf->wall_ts_us) {
        /* 零分配 SERIES 没有逐条墙钟存储：insert_at 不能静默改成 flush 时刻。 */
        LOG_ERROR(LOG_CAT_CORE,
                  "DB: insert_at needs the owning constructor (no wall storage in the "
                  "zero-alloc SERIES path); use sqlite_svc_insert() there");
        return -1;
    }
    uv_mutex_lock(&svc->buf_lock);
    if (svc->active_buf->count < svc->active_buf->capacity) {
        int slot = svc->active_buf->count;
        svc->active_buf->buffer[slot] = *msg;
        if (store_wall) svc->active_buf->wall_ts_us[slot] = wall_ts_us;
        svc->active_buf->count++;
    } else {
        // 缓冲区溢出，触发紧急刷盘 (同步降级)
        uv_mutex_unlock(&svc->buf_lock);
        _trigger_flush(svc);
        return -1;
    }
    uv_mutex_unlock(&svc->buf_lock);
    return 0;
}

int sqlite_svc_insert_at(sqlite_svc_t *svc, const PipeMsg *msg, uint64_t wall_ts_us) {
    if (!svc || !msg) return -1;
    _note_wall_us(svc, wall_ts_us);
    return _insert_impl(svc, msg, wall_ts_us, true);
}

int sqlite_svc_insert(sqlite_svc_t *svc, const PipeMsg *msg) {
    if (!svc || !msg) return -1;
    uint64_t wall_ts_us = _next_wall_us(svc);
    /* 零分配 SERIES 没有墙钟数组：insert() 的墙钟按 flush 时刻补采（见头文件）。 */
    bool store_wall = !(svc->active_buf && !svc->active_buf->wall_ts_us);
    return _insert_impl(svc, msg, wall_ts_us, store_wall);
}

int sqlite_svc_flush_sync(sqlite_svc_t *svc) {
    if (!svc || !svc->active_buf) return 0;
    int rc = 0;
    uv_mutex_lock(&svc->buf_lock);
    if (svc->active_buf->count > 0) {
        data_batch_t *old_flush = svc->flush_buf;
        svc->flush_buf = svc->active_buf;
        rc = _flush_batch_internal(svc, svc->db, svc->flush_buf);
        svc->flush_buf = old_flush;
        if (rc == 0) svc->active_buf->count = 0;
    }
    uv_mutex_unlock(&svc->buf_lock);
    return rc;
}

int sqlite_svc_query(sqlite_svc_t *svc, uint64_t start_ts, uint64_t end_ts, sqlite_svc_query_cb cb,
                     void *user_data) {
    if (!svc || !cb) return -1;
    if (svc->active_buf && svc->active_buf->count > 0) {
        if (sqlite_svc_flush_sync(svc) != 0) return -1;
    }
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

int sqlite_svc_init_from_contract(sqlite_svc_t *svc, const edge_contract_t *contract) {
    if (!svc || !contract) return -1;

    LOG_INFO(LOG_CAT_CORE, "DB: Initializing Schema and Seed data from Contract '%s'...",
             contract->name);

    // 1. 确保基础表结构存在 (利用已有的迁移逻辑)
    if (_migrate_schema(svc) != 0) return -1;

    // 2. 注入元数据标记
    sqlite3_exec(svc->db, "CREATE TABLE IF NOT EXISTS matrix_meta (key TEXT PRIMARY KEY, val TEXT);",
                 NULL, NULL, NULL);

    sqlite_svc_begin_trans(svc);

    // 清理旧的任务和设备（契约是唯一真理）
    sqlite3_exec(svc->db, "DELETE FROM inventory;", NULL, NULL, NULL);
    sqlite3_exec(svc->db, "DELETE FROM poll_tasks;", NULL, NULL, NULL);

    // 注入仿真标记，允许 main.c 里的 load_tasks 逻辑启动
    sqlite3_exec(svc->db, "DELETE FROM matrix_meta WHERE key='is_ready';", NULL, NULL, NULL);
    sqlite3_exec(svc->db, "INSERT INTO matrix_meta (key, val) VALUES ('is_ready', '1');", NULL,
                 NULL, NULL);

    // 3. 从契约的 Tags 中推导 Poll Tasks (如果 Tags 具有 poll_interval 属性)
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO poll_tasks (bus_id, device_id, tag_id) VALUES (?, ?, ?);";
    if (sqlite3_prepare_v2(svc->db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        // [深挖建议] 这里可以遍历 contract->instances 来填充 inventory 表
        // 此处先打通链路
        sqlite3_finalize(stmt);
    }

    sqlite_svc_commit_trans(svc);
    LOG_INFO(LOG_CAT_CORE, "DB: Contract-Native Initialization Complete.");

    return 0;
}

static void _on_svc_timer_close(uv_handle_t *handle) {
    int *pending = (int *)handle->data;
    if (pending) {
        (*pending)--;
    }
}

void sqlite_svc_destroy(sqlite_svc_t *svc) {
    if (!svc) return;

    int pending_closes = 0;

    if (!uv_is_closing((uv_handle_t *)&svc->gc_timer)) {
        uv_timer_stop(&svc->gc_timer);
        svc->gc_timer.data = &pending_closes;
        pending_closes++;
        uv_close((uv_handle_t *)&svc->gc_timer, _on_svc_timer_close);
    }

    if (svc->active_buf) {
        if (!uv_is_closing((uv_handle_t *)&svc->flush_timer)) {
            uv_timer_stop(&svc->flush_timer);
            svc->flush_timer.data = &pending_closes;
            pending_closes++;
            uv_close((uv_handle_t *)&svc->flush_timer, _on_svc_timer_close);
        }
        while (svc->is_flushing) uv_run(svc->loop, UV_RUN_ONCE);
        if (svc->active_buf->count > 0) {
            int rc = _flush_batch_internal(svc, svc->db, svc->active_buf);
            if (rc != 0) {
                LOG_ERROR(LOG_CAT_CORE, "DB: final flush failed; %d row(s) lost",
                          svc->active_buf->count);
            }
        }
        if (svc->are_buffers_allocated && svc->batch_block) {
            free(svc->batch_block); /* 同时包含 active/flush 的 PipeMsg 与墙钟数组 */
            svc->batch_block = NULL;
        }
        uv_mutex_destroy(&svc->buf_lock);
    }

    while (pending_closes > 0 && svc->loop) {
        uv_run(svc->loop, UV_RUN_ONCE);
    }

    if (svc->flush_db) {
        sqlite3_close(svc->flush_db);
        svc->flush_db = NULL;
    }
    if (svc->db) sqlite3_close(svc->db);
    if (svc->is_allocated) {
        free(svc);
    }
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
    char *sql_copy = strdup(sql);
    if (!req || !task || !sql_copy) {
        free(req);
        free(task);
        free(sql_copy);
        return -1;
    }
    task->svc = svc;
    task->sql = sql_copy;
    task->cb = cb;
    task->user_data = user_data;
    req->data = task;
    return uv_queue_work(svc->loop, req, _exec_work_cb, _exec_after_cb);
}
void *sqlite_svc_get_db_handle(sqlite_svc_t *svc) {
    return svc ? svc->db : NULL;
}


