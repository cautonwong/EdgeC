#include "db_engine.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BATCH_SIZE 1000 // Max records per transaction
#define FLUSH_INTERVAL_MS 1000 // Flush at least every 1 second

// Buffer for batching records
typedef struct {
    cedgex_datapoint_t items[BATCH_SIZE];
    int count;
} BatchBuffer;

// The main context for the DB engine
typedef struct {
    sqlite3 *db;
    uv_loop_t *loop;
    
    // Double buffering
    BatchBuffer *active_buf;
    BatchBuffer *flush_buf;
    
    uv_work_t work_req;
    uv_timer_t flush_timer;
    
    volatile int is_flushing;
    pthread_spinlock_t lock; // For protecting access to the active buffer
} DbEngineContext;

static DbEngineContext g_ctx;
static BatchBuffer g_buf1, g_buf2;

// --- Forward Declarations ---
static int run_migrations(sqlite3 *db);
static void _trigger_flush(uv_timer_t *handle);

// ==========================================
// 1. Background Worker Thread: Writes to SQLite
// ==========================================
static void db_worker_flush(uv_work_t *req) {
    DbEngineContext *ctx = (DbEngineContext *)req->data;
    BatchBuffer *buf_to_flush = ctx->flush_buf;
    
    if (buf_to_flush->count == 0) {
        return;
    }

    sqlite3_exec(ctx->db, "BEGIN TRANSACTION;", 0, 0, 0);
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO ts_data (ts, tag_id, val_num, quality) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(ctx->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[DB] Failed to prepare statement: %s\n", sqlite3_errmsg(ctx->db));
        sqlite3_exec(ctx->db, "ROLLBACK;", 0, 0, 0);
        return;
    }

    for (int i = 0; i < buf_to_flush->count; i++) {
        cedgex_datapoint_t *p = &buf_to_flush->items[i];
        sqlite3_bind_int64(stmt, 1, p->ts);
        sqlite3_bind_int(stmt, 2, p->tag_id);
        if (p->type == CEDGEX_VAL_TYPE_FLOAT) {
            sqlite3_bind_double(stmt, 3, p->val.f_val);
        } else {
            sqlite3_bind_double(stmt, 3, (double)p->val.i_val);
        }
        sqlite3_bind_int(stmt, 4, p->quality);
        
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_exec(ctx->db, "COMMIT;", 0, 0, 0);
}

// ==========================================
// 2. Main Loop Callback: After work is done
// ==========================================
static void after_db_worker_flush(uv_work_t *req, int status) {
    if (status != 0) {
        fprintf(stderr, "[DB] Worker flush failed with status %d\n", status);
    }
    
    DbEngineContext *ctx = (DbEngineContext *)req->data;
    ctx->flush_buf->count = 0; // Clear the flushed buffer
    ctx->is_flushing = 0;      // Release the lock
}

// ==========================================
// 3. Buffer Swap and Flush Trigger
// ==========================================
static void _trigger_flush(uv_timer_t *handle) {
    DbEngineContext *ctx = &g_ctx;

    if (ctx->is_flushing) {
        fprintf(stderr, "[DB] Warning: DB is busy, last flush not complete. Skipping this tick.\n");
        return;
    }
    
    pthread_spin_lock(&ctx->lock);
    if (ctx->active_buf->count == 0) {
        pthread_spin_unlock(&ctx->lock);
        return;
    }

    // Swap active and flush buffers
    BatchBuffer *temp = ctx->active_buf;
    ctx->active_buf = ctx->flush_buf;
    ctx->flush_buf = temp;
    pthread_spin_unlock(&ctx->lock);

    ctx->is_flushing = 1;
    ctx->work_req.data = ctx;
    uv_queue_work(ctx->loop, &ctx->work_req, db_worker_flush, after_db_worker_flush);
}

// ==========================================
// 4. Public API
// ==========================================
int db_engine_init(uv_loop_t *loop, const char *db_path) {
    g_ctx.loop = loop;
    g_ctx.active_buf = &g_buf1;
    g_ctx.flush_buf = &g_buf2;
    g_ctx.active_buf->count = 0;
    g_ctx.flush_buf->count = 0;
    g_ctx.is_flushing = 0;
    pthread_spin_init(&g_ctx.lock, PTHREAD_PROCESS_PRIVATE);

    // Open DB
    if (sqlite3_open(db_path, &g_ctx.db) != SQLITE_OK) {
        fprintf(stderr, "[DB] Can't open database: %s\n", sqlite3_errmsg(g_ctx.db));
        return -1;
    }

    // Set pragmas for performance and durability
    sqlite3_exec(g_ctx.db, "PRAGMA journal_mode=WAL;", 0, 0, 0);
    sqlite3_exec(g_ctx.db, "PRAGMA synchronous=NORMAL;", 0, 0, 0);
    sqlite3_exec(g_ctx.db, "PRAGMA cache_size=-4000;", 0, 0, 0); // 4MB cache

    // Run migrations
    if (run_migrations(g_ctx.db) != 0) {
        sqlite3_close(g_ctx.db);
        return -1;
    }

    // Start the periodic flush timer
    uv_timer_init(loop, &g_ctx.flush_timer);
    uv_timer_start(&g_ctx.flush_timer, _trigger_flush, FLUSH_INTERVAL_MS, FLUSH_INTERVAL_MS);

    printf("[DB] DB Engine initialized successfully.\n");
    return 0;
}

int db_engine_insert_async(cedgex_datapoint_t *dp) {
    pthread_spin_lock(&g_ctx.lock);
    BatchBuffer *buf = g_ctx.active_buf;
    if (buf->count >= BATCH_SIZE) {
        pthread_spin_unlock(&g_ctx.lock);
        // Trigger a flush immediately if the buffer is full
        _trigger_flush(NULL); 
        return -1; // Indicate buffer was full
    }
    buf->items[buf->count++] = *dp;
    pthread_spin_unlock(&g_ctx.lock);
    return 0;
}

void db_engine_cleanup(void) {
    printf("[DB] Cleaning up DB engine...\n");
    // Stop the timer and perform one final flush
    uv_timer_stop(&g_ctx.flush_timer);
    _trigger_flush(NULL);
    // Note: In a real app, you'd need to wait for the last flush to complete.
    // This is simplified for the example.
    
    // Wait a moment for the last job to finish
    uv_sleep(100); 

    sqlite3_close(g_ctx.db);
    pthread_spin_destroy(&g_ctx.lock);
}


// ==========================================
// 5. Database Migration Framework (from sql.md)
// ==========================================
#define TARGET_DB_VERSION 1 // Start with version 1

typedef int (*MigrationFunc)(sqlite3 *db);

int migrate_to_v1(sqlite3 *db) {
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS sys_config (key TEXT PRIMARY KEY, value TEXT, updated_at INTEGER);"
        "CREATE TABLE IF NOT EXISTS tags (id INTEGER PRIMARY KEY AUTOINCREMENT, tag_name TEXT UNIQUE NOT NULL, unit TEXT, desc TEXT);"
        "CREATE TABLE IF NOT EXISTS ts_data (ts INTEGER NOT NULL, tag_id INTEGER NOT NULL, val_num REAL, val_str TEXT, quality INTEGER DEFAULT 0, PRIMARY KEY (ts, tag_id)) WITHOUT ROWID;"
        "CREATE INDEX IF NOT EXISTS idx_ts_data_time ON ts_data(ts);"
        "CREATE TABLE IF NOT EXISTS alarms (id INTEGER PRIMARY KEY AUTOINCREMENT, ts INTEGER NOT NULL, tag_id INTEGER, level INTEGER, message TEXT, acked INTEGER DEFAULT 0);";
    
    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[DB Migration] V1 failed: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}


// Migration registry
MigrationFunc migrations[] = {
    NULL,          // v0 is a placeholder
    migrate_to_v1
};

static int run_migrations(sqlite3 *db) {
    sqlite3_stmt *stmt;
    int current_ver = 0;

    if (sqlite3_prepare_v2(db, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            current_ver = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    printf("[DB] DB Version: %d, Target Version: %d\n", current_ver, TARGET_DB_VERSION);

    if (current_ver >= TARGET_DB_VERSION) {
        return 0; // No migration needed
    }

    for (int v = current_ver + 1; v <= TARGET_DB_VERSION; v++) {
        printf("[DB] Migrating to version %d...\n", v);
        
        sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
        
        if (migrations[v](db) != 0) {
            fprintf(stderr, "[DB] Migration to version %d failed. Rolling back.\n", v);
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            return -1;
        }

        char ver_sql[64];
        snprintf(ver_sql, sizeof(ver_sql), "PRAGMA user_version = %d;", v);
        if (sqlite3_exec(db, ver_sql, NULL, NULL, NULL) != SQLITE_OK) {
             fprintf(stderr, "[DB] Failed to set user_version to %d.\n", v);
             sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
             return -1;
        }
        
        sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
        printf("[DB] Migration to version %d successful.\n", v);
    }
    
    return 0;
}
