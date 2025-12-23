#include "db_engine.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

typedef struct {
    db_datapoint_t *items;
    size_t count;
    size_t capacity;
} batch_buffer_t;

typedef struct {
    sqlite3 *db;
    uv_loop_t *loop;
    
    batch_buffer_t buffers[2]; // Double buffering
    _Atomic int active_buf_idx;
    
    uv_work_t work_req;
    uv_timer_t flush_timer;
    
    _Atomic bool is_flushing;
    char db_path[256];
} db_engine_ctx_t;

static db_engine_ctx_t g_db_ctx;

// --- Background worker thread for DB I/O ---
static void _worker_flush_to_disk(uv_work_t *req) {
    db_engine_ctx_t *ctx = (db_engine_ctx_t *)req->data;
    int flush_idx = (atomic_load(&ctx->active_buf_idx) + 1) % 2;
    batch_buffer_t *buf = &ctx->buffers[flush_idx];

    if (buf->count == 0) {
        return;
    }

    sqlite3_exec(ctx->db, "BEGIN TRANSACTION;", 0, 0, 0);
    
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(ctx->db, "INSERT INTO ts_data (ts, tag_id, val, quality) VALUES (?, ?, ?, ?);", -1, &stmt, NULL);

    for (size_t i = 0; i < buf->count; i++) {
        db_datapoint_t *p = &buf->items[i];
        sqlite3_bind_int64(stmt, 1, p->ts);
        sqlite3_bind_int(stmt, 2, p->tag_id);
        sqlite3_bind_double(stmt, 3, p->value);
        sqlite3_bind_int(stmt, 4, p->quality);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    
    sqlite3_finalize(stmt);
    sqlite3_exec(ctx->db, "COMMIT;", 0, 0, 0);

    // Clear the buffer that was just flushed
    buf->count = 0;
}

// --- Main thread callback after DB I/O is done ---
static void _after_flush_to_disk(uv_work_t *req, int status) {
    db_engine_ctx_t *ctx = (db_engine_ctx_t *)req->data;
    atomic_store(&ctx->is_flushing, false);
}

// --- Triggers a buffer swap and flush ---
static void _trigger_flush(db_engine_ctx_t *ctx) {
    if (atomic_load(&ctx->is_flushing)) {
        // Previous flush hasn't completed, data will wait in the active buffer.
        return; 
    }

    int active_idx = atomic_load(&ctx->active_buf_idx);
    if (ctx->buffers[active_idx].count == 0) {
        // Nothing to flush.
        return;
    }

    atomic_store(&ctx->is_flushing, true);
    
    // Swap buffers
    int next_active_idx = (active_idx + 1) % 2;
    atomic_store(&ctx->active_buf_idx, next_active_idx);

    // Queue the flush task to the libuv thread pool
    ctx->work_req.data = ctx;
    uv_queue_work(ctx->loop, &ctx->work_req, _worker_flush_to_disk, _after_flush_to_disk);
}

static void _on_timer_flush(uv_timer_t *handle) {
    _trigger_flush(&g_db_ctx);
}

// --- Public API ---
int db_engine_init(uv_loop_t *loop, const char *db_path, size_t batch_size, uint64_t flush_interval_ms) {
    memset(&g_db_ctx, 0, sizeof(g_db_ctx));
    g_db_ctx.loop = loop;
    strncpy(g_db_ctx.db_path, db_path, sizeof(g_db_ctx.db_path) - 1);
    atomic_init(&g_db_ctx.active_buf_idx, 0);
    atomic_init(&g_db_ctx.is_flushing, false);

    for (int i = 0; i < 2; ++i) {
        g_db_ctx.buffers[i].items = malloc(batch_size * sizeof(db_datapoint_t));
        if (!g_db_ctx.buffers[i].items) {
            // Handle allocation failure
            if (i > 0) free(g_db_ctx.buffers[0].items);
            return -1;
        }
        g_db_ctx.buffers[i].capacity = batch_size;
        g_db_ctx.buffers[i].count = 0;
    }

    if (sqlite3_open(g_db_ctx.db_path, &g_db_ctx.db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(g_db_ctx.db));
        free(g_db_ctx.buffers[0].items);
        free(g_db_ctx.buffers[1].items);
        return -1;
    }

    sqlite3_exec(g_db_ctx.db, "PRAGMA journal_mode=WAL;", 0, 0, 0);
    sqlite3_exec(g_db_ctx.db, "PRAGMA synchronous=NORMAL;", 0, 0, 0);
    
    const char *sql_create = 
        "CREATE TABLE IF NOT EXISTS ts_data (" 
        "ts INTEGER, tag_id INTEGER, val REAL, quality INTEGER, " 
        "PRIMARY KEY(ts, tag_id)) WITHOUT ROWID;";
    sqlite3_exec(g_db_ctx.db, sql_create, 0, 0, 0);

    uv_timer_init(loop, &g_db_ctx.flush_timer);
    uv_timer_start(&g_db_ctx.flush_timer, _on_timer_flush, flush_interval_ms, flush_interval_ms);
    
    return 0;
}

bool db_engine_insert(const db_datapoint_t *dp) {
    int active_idx = atomic_load(&g_db_ctx.active_buf_idx);
    batch_buffer_t *buf = &g_db_ctx.buffers[active_idx];
    
    if (buf->count >= buf->capacity) {
        _trigger_flush(&g_db_ctx);
        // Re-check after attempting a flush
        active_idx = atomic_load(&g_db_ctx.active_buf_idx);
        buf = &g_db_ctx.buffers[active_idx];
        if (buf->count >= buf->capacity) {
            fprintf(stderr, "[DB] Buffer overflow, dropping data!\n");
            return false;
        }
    }

    buf->items[buf->count++] = *dp;
    return true;
}

void db_engine_destroy(void) {
    uv_timer_stop(&g_db_ctx.flush_timer);
    _trigger_flush(&g_db_ctx);

    // Busy-wait for the last flush to complete.
    // In a real application, a more graceful shutdown with signaling is better.
    while(atomic_load(&g_db_ctx.is_flushing)) {
        usleep(10000); // Sleep for 10ms
    }

    sqlite3_close(g_db_ctx.db);
    free(g_db_ctx.buffers[0].items);
    free(g_db_ctx.buffers[1].items);
}
