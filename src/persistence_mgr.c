#include "persistence_mgr.h"
#include "sqlite_writer.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SQL_BUF_SIZE 4096

static void persistence_tick(uv_timer_t *t) {
    persistence_mgr_t *mgr = (persistence_mgr_t *)t->data;
    if (!mgr || !mgr->ring || !mgr->writer) return;
    
    PipeMsg msg;
    char *sql_buf = malloc(SQL_BUF_SIZE);
    if (!sql_buf) return;
    
    size_t offset = 0;
    offset += (size_t)snprintf(sql_buf + offset, SQL_BUF_SIZE - offset, "BEGIN TRANSACTION;");

    int count = 0;
    while (shm_ring_read(mgr->ring, &mgr->last_seq, &msg) == 0) {
        if (msg.type == PIPEMSG_SCALAR_F64) {
            offset += (size_t)snprintf(sql_buf + offset, SQL_BUF_SIZE - offset,
                "INSERT INTO readings(tag_id, value, ts) VALUES(%u, %.4f, %lu);",
                msg.tag_id, msg.payload.val_f, (unsigned long)msg.ts);
            count++;
        }
        // Prevent buffer overflow
        if (offset > SQL_BUF_SIZE - 256) break;
    }

    if (count > 0) {
        snprintf(sql_buf + offset, SQL_BUF_SIZE - offset, "COMMIT;");
        // Use the robust SQLite Writer
        // Note: The writer copies the SQL string, so we must free our buffer.
        sqlite_writer_submit(mgr->writer, sql_buf);
    }
    
    free(sql_buf);
}

int persistence_mgr_init(persistence_mgr_t *mgr, uv_loop_t *loop, shm_ring_t *ring, uint64_t interval_ms) {
    if (!mgr || !loop || !ring) return -1;
    
    mgr->ring = ring;
    mgr->loop = loop;
    mgr->last_seq = 0;

    // Initialize the writer
    if (sqlite_writer_init(&mgr->writer, "edge_data.db") != 0) {
        return -1;
    }

    // Create table synchronously to ensure it exists
    sqlite_writer_exec(mgr->writer, 
        "CREATE TABLE IF NOT EXISTS readings(tag_id INTEGER, value REAL, ts INTEGER);");

    uv_timer_init(loop, &mgr->timer);
    mgr->timer.data = mgr;
    return uv_timer_start(&mgr->timer, persistence_tick, interval_ms, interval_ms);
}

void persistence_mgr_close(persistence_mgr_t *mgr) {
    if (mgr) {
        if (!uv_is_closing((uv_handle_t*)&mgr->timer)) {
            uv_close((uv_handle_t*)&mgr->timer, NULL);
        }
        if (mgr->writer) {
            sqlite_writer_destroy(mgr->writer);
            mgr->writer = NULL;
        }
    }
}
