#include "sqlite_service.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uv_work_t req;
    sqlite3 *db;
    char *sql;
    sql_cb_t cb;
    void *user;
    int rc;
    int own_db; // flag to close db if we opened it
} sql_ctx_t;

static sqlite3 *g_db = NULL;

static void sql_work(uv_work_t *req) {
    sql_ctx_t *c = (sql_ctx_t *)req->data;
    
    // In a real production system, we might maintain a connection pool
    // or a persistent connection per thread. 
    // Here we strictly follow the user's pattern but try to reuse a global handle if initialized,
    // or open one ad-hoc (inefficient but safe for "snippet" logic).
    
    sqlite3 *db_to_use = c->db;
    if (!db_to_use) {
        // Fallback or explicit open
        if (sqlite3_open("test.db", &db_to_use) == SQLITE_OK) {
            c->own_db = 1;
        } else {
            c->rc = SQLITE_ERROR;
            return;
        }
    }

    char *err_msg = NULL;
    c->rc = sqlite3_exec(db_to_use, c->sql, NULL, NULL, &err_msg);
    if (c->rc != SQLITE_OK) {
        if (err_msg) {
            fprintf(stderr, "SQL Error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
    }

    if (c->own_db) {
        sqlite3_close(db_to_use);
    }
}

static void sql_after(uv_work_t *req, int status) {
    sql_ctx_t *c = (sql_ctx_t *)req->data;
    if (c->cb) c->cb(c->rc, c->user);
    free(c->sql);
    free(c);
}

int sqlite_exec_async(
    uv_loop_t *loop,
    const char *sql,
    sql_cb_t cb,
    void *user
) {
    sql_ctx_t *c = calloc(1, sizeof(*c));
    if (!c) return -1;

    // For this example, we let the worker thread open the DB to be safe
    // or use a thread-safe global handle. 
    // User snippet showed opening inside the structure. 
    // We'll leave c->db NULL so work callback opens it.
    c->db = NULL; 
    
    c->sql = strdup(sql);
    c->cb = cb;
    c->user = user;
    c->req.data = c;
    return uv_queue_work(loop, &c->req, sql_work, sql_after);
}