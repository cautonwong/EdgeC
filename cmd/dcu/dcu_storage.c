#include "dcu_storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "contract_registry.h"
#include "edge_log.h"

/**
 * @brief 定时批量事务合并刷写回调 (libuv timer 驱动)
 */
static void _flush_timer_cb(uv_timer_t* timer) {
    dcu_storage_t* storage = (dcu_storage_t*)timer->data;
    if (storage) {
        dcu_storage_flush(storage);
    }
}

/**
 * @brief 初始化 SQLite 数据库与事务合并存储服务 (单线程模型 - 绝无死锁)
 */
int dcu_storage_init(dcu_storage_t* storage, uv_loop_t* loop, const char* db_path) {
    if (!storage || !loop || !db_path) return -1;

    storage->loop = loop;
    storage->cache_count = 0;
    memset(storage->cache, 0, sizeof(storage->cache));

    // 1. 直接打开 SQLite
    int ret = sqlite3_open(db_path, &storage->db);
    if (ret != SQLITE_OK) {
        return -3;
    }

    // 开启 5 秒繁忙重试窗口
    sqlite3_busy_timeout(storage->db, 5000);

    // 3. 【生产级寿命保护】开启 WAL 日志模式
    sqlite3_exec(storage->db, "PRAGMA journal_mode = WAL;", NULL, NULL, NULL);
    sqlite3_exec(storage->db, "PRAGMA synchronous = NORMAL;", NULL, NULL, NULL);

    // 4. 原子化建表
    const char* sql_init =
        "CREATE TABLE IF NOT EXISTS dcu_meters (meter_address TEXT PRIMARY KEY, protocol_type "
        "TEXT, channel_id INTEGER);"
        "CREATE TABLE IF NOT EXISTS dcu_telemetry_latest (meter_address TEXT, oad INTEGER, value "
        "REAL, updated_at INTEGER, PRIMARY KEY (meter_address, oad));"
        "CREATE TABLE IF NOT EXISTS dcu_freeze_history (meter_address TEXT, freeze_oad INTEGER, "
        "freeze_time INTEGER, source_oad INTEGER, frozen_value REAL, PRIMARY KEY (meter_address, "
        "freeze_oad, freeze_time, source_oad));"
        "CREATE TABLE IF NOT EXISTS telemetry (timestamp INTEGER, tag_id INTEGER, value REAL, "
        "device_id INTEGER, quality INTEGER);"
        "CREATE TABLE IF NOT EXISTS event_history (meter_address TEXT, oi INTEGER, start_time "
        "INTEGER, end_time INTEGER, snapshot_value REAL, PRIMARY KEY (meter_address, oi, "
        "start_time));";

    sqlite3_exec(storage->db, sql_init, NULL, NULL, NULL);

    // 创建高性能索引
    sqlite3_exec(storage->db,
                 "CREATE INDEX IF NOT EXISTS idx_freeze_query ON dcu_freeze_history "
                 "(meter_address, freeze_oad, freeze_time);",
                 NULL, NULL, NULL);
    sqlite3_exec(storage->db,
                 "CREATE INDEX IF NOT EXISTS idx_telemetry_ts ON telemetry(timestamp);", NULL, NULL,
                 NULL);

    // 6. 初始化并挂接闲时 10 秒合并落盘定时器 (libuv)
    uv_timer_init(loop, &storage->flush_timer);
    storage->flush_timer.data = storage;
    uv_timer_start(&storage->flush_timer, _flush_timer_cb, 10000, 10000);
    storage->timer_active = true;

    // [Optimization] 预编译冻结语句
    const char* freeze_sql =
        "INSERT OR REPLACE INTO dcu_freeze_history (meter_address, freeze_oad, freeze_time, "
        "source_oad, frozen_value) VALUES (?, ?, ?, ?, ?);";
    sqlite3_prepare_v2(storage->db, freeze_sql, -1, &storage->stmt_freeze, NULL);

    return 0;
}

int dcu_storage_write_telemetry(dcu_storage_t* storage, const char* addr, uint32_t device_id,
                                uint32_t oad, double value) {
    if (!storage || !addr) return -1;

    if (storage->cache_count >= STORAGE_CACHE_MAX) {
        dcu_storage_flush(storage);
    }

    telemetry_cache_entry_t* entry = &storage->cache[storage->cache_count];
    strncpy(entry->meter_address, addr, STORAGE_ADDR_LEN - 1);
    entry->meter_address[STORAGE_ADDR_LEN - 1] = '\0';
    entry->oad = oad;
    entry->value = value;
    entry->ts = (uint64_t)time(NULL);
    entry->device_id = device_id;

    storage->cache_count++;

    if (storage->cache_count >= STORAGE_CACHE_MAX) {
        dcu_storage_flush(storage);
    }

    return 0;
}

int dcu_storage_write_freeze(dcu_storage_t* storage, const char* addr, uint32_t freeze_oad,
                             uint64_t freeze_time, uint32_t src_oad, double value) {
    if (!storage || !addr || !storage->stmt_freeze) return -1;
    sqlite3_stmt* stmt = storage->stmt_freeze;
    sqlite3_bind_text(stmt, 1, addr, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)freeze_oad);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)freeze_time);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)src_oad);
    sqlite3_bind_double(stmt, 5, value);
    int ret = sqlite3_step(stmt);
    sqlite3_reset(stmt);
    return (ret == SQLITE_DONE) ? 0 : -3;
}

int dcu_storage_write_event(dcu_storage_t* storage, const char* addr, uint16_t oi,
                            uint64_t start_time, uint64_t end_time, float snapshot_val) {
    if (!storage || !addr) return -1;
    sqlite3_stmt* stmt;
    const char* sql =
        "INSERT OR REPLACE INTO event_history (meter_address, oi, start_time, end_time, "
        "snapshot_value) VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(storage->db, sql, -1, &stmt, NULL) != SQLITE_OK) return -2;
    sqlite3_bind_text(stmt, 1, addr, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, oi);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)start_time);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)end_time);
    sqlite3_bind_double(stmt, 5, (double)snapshot_val);
    int ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (ret == SQLITE_DONE) ? 0 : -3;
}

int dcu_storage_flush(dcu_storage_t* storage) {
    if (!storage || storage->cache_count == 0) return 0;

    sqlite3_exec(storage->db, "BEGIN IMMEDIATE TRANSACTION;", NULL, NULL, NULL);
    const char* upsert_sql =
        "INSERT OR REPLACE INTO dcu_telemetry_latest (meter_address, oad, value, updated_at) "
        "VALUES (?, ?, ?, ?);";
    const char* hist_sql =
        "INSERT INTO telemetry (timestamp, tag_id, value, device_id, quality) VALUES (?, ?, ?, ?, "
        "?);";
    sqlite3_stmt *stmt_up, *stmt_hist;
    if (sqlite3_prepare_v2(storage->db, upsert_sql, -1, &stmt_up, NULL) != SQLITE_OK) {
        sqlite3_exec(storage->db, "ROLLBACK;", NULL, NULL, NULL);
        storage->cache_count = 0;
        return -2;
    }
    if (sqlite3_prepare_v2(storage->db, hist_sql, -1, &stmt_hist, NULL) != SQLITE_OK) {
        sqlite3_finalize(stmt_up);
        sqlite3_exec(storage->db, "ROLLBACK;", NULL, NULL, NULL);
        storage->cache_count = 0;
        return -3;
    }
    for (uint16_t i = 0; i < storage->cache_count; i++) {
        telemetry_cache_entry_t* entry = &storage->cache[i];
        sqlite3_bind_text(stmt_up, 1, entry->meter_address, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt_up, 2, (sqlite3_int64)entry->oad);
        sqlite3_bind_double(stmt_up, 3, entry->value);
        sqlite3_bind_int64(stmt_up, 4, (sqlite3_int64)entry->ts);
        sqlite3_step(stmt_up);
        sqlite3_reset(stmt_up);
        sqlite3_bind_int64(stmt_hist, 1, (sqlite3_int64)entry->ts);
        sqlite3_bind_int64(stmt_hist, 2, (sqlite3_int64)entry->oad);
        sqlite3_bind_double(stmt_hist, 3, entry->value);
        sqlite3_bind_int64(stmt_hist, 4, (sqlite3_int64)entry->device_id);
        sqlite3_bind_int(stmt_hist, 5, 0);
        sqlite3_step(stmt_hist);
        sqlite3_reset(stmt_hist);
    }
    sqlite3_finalize(stmt_up);
    sqlite3_finalize(stmt_hist);
    sqlite3_exec(storage->db, "COMMIT;", NULL, NULL, NULL);
    storage->cache_count = 0;
    return 0;
}

int dcu_storage_query_freeze(dcu_storage_t* storage, const char* addr, uint32_t freeze_oad,
                             uint64_t start_time, uint64_t end_time,
                             dcu_freeze_record_t* out_records, uint16_t max_records,
                             uint16_t* out_count) {
    if (!storage || !addr || !out_records || !out_count) return -1;
    *out_count = 0;
    sqlite3_stmt* stmt;
    const char* query_sql =
        "SELECT meter_address, freeze_oad, freeze_time, source_oad, frozen_value FROM "
        "dcu_freeze_history WHERE meter_address = ? AND freeze_oad = ? AND freeze_time >= ? AND "
        "freeze_time <= ? ORDER BY freeze_time ASC;";
    if (sqlite3_prepare_v2(storage->db, query_sql, -1, &stmt, NULL) != SQLITE_OK) return -2;
    sqlite3_bind_text(stmt, 1, addr, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)freeze_oad);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)start_time);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)end_time);
    while (sqlite3_step(stmt) == SQLITE_ROW && *out_count < max_records) {
        dcu_freeze_record_t* rec = &out_records[*out_count];
        strncpy(rec->meter_address, (const char*)sqlite3_column_text(stmt, 0),
                STORAGE_ADDR_LEN - 1);
        rec->meter_address[STORAGE_ADDR_LEN - 1] = '\0';
        rec->freeze_oad = (uint32_t)sqlite3_column_int64(stmt, 1);
        rec->freeze_time = (uint64_t)sqlite3_column_int64(stmt, 2);
        rec->source_oad = (uint32_t)sqlite3_column_int64(stmt, 3);
        rec->frozen_value = sqlite3_column_double(stmt, 4);
        (*out_count)++;
    }
    sqlite3_finalize(stmt);
    return 0;
}

int dcu_storage_count_freezes(dcu_storage_t* storage, uint16_t* out_count) {
    if (!storage || !out_count) return -1;
    sqlite3_stmt* stmt;
    int ret =
        sqlite3_prepare_v2(storage->db, "SELECT count(*) FROM dcu_freeze_history", -1, &stmt, NULL);
    if (ret != SQLITE_OK) return -3;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *out_count = (uint16_t)sqlite3_column_int(stmt, 0);
    } else {
        *out_count = 0;
    }
    sqlite3_finalize(stmt);
    return 0;
}

void dcu_storage_destroy(dcu_storage_t* storage) {
    if (!storage) return;
    if (storage->timer_active) {
        uv_timer_stop(&storage->flush_timer);
        storage->timer_active = false;
    }
    dcu_storage_flush(storage);
    if (storage->stmt_freeze) {
        sqlite3_finalize(storage->stmt_freeze);
        storage->stmt_freeze = NULL;
    }
    if (storage->db) {
        sqlite3_close(storage->db);
        storage->db = NULL;
    }
}
