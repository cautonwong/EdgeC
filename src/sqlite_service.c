#include "sqlite_service.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> // For fprintf, perror

// Static global for the SQLite database handle and uv_loop (for simplicity in this example)
// In a real application, these might be passed around or part of a context struct.
static sqlite3 *s_db = NULL;
static uv_loop_t *s_loop = NULL;

typedef struct {
    uv_work_t req;
    char *sql;
    sql_cb_t cb;
    void *user;
    int rc;
} sql_ctx_t;

int sqlite_service_init(uv_loop_t *loop, const char *db_path) {
    if (!loop || !db_path) {
        fprintf(stderr, "Invalid arguments for sqlite_service_init.\n");
        return -1;
    }
    if (s_db != NULL) {
        fprintf(stderr, "SQLite service already initialized.\n");
        return 0; // Already initialized
    }

    s_loop = loop;
    int rc = sqlite3_open(db_path, &s_db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(s_db));
        sqlite3_close(s_db);
        s_db = NULL;
        return -1;
    }
    fprintf(stdout, "SQLite database opened successfully: %s\n", db_path);
    return 0;
}

void sqlite_service_close(void) {
    if (s_db) {
        sqlite3_close(s_db);
        s_db = NULL;
        s_loop = NULL;
        fprintf(stdout, "SQLite database closed.\n");
    }
}

static void sql_work(uv_work_t *req) {
    sql_ctx_t *c = req->data;
    if (s_db == NULL) {
        c->rc = SQLITE_ERROR; // Indicate error if DB not open
        return;
    }
    // Execute the SQL statement. No callback for internal sqlite3_exec.
    // Errors will be captured in c->rc.
    c->rc = sqlite3_exec(s_db, c->sql, NULL, NULL, NULL);
}

static void sql_after(uv_work_t *req, int status) {
    sql_ctx_t *c = req->data;
    (void)status; // status indicates if req was cancelled, not used here.

    if (c->cb) {
        c->cb(c->rc, c->user);
    }
    free(c->sql); // Free the duplicated SQL string
    free(c);      // Free the context struct
}

int sqlite_exec_async(
    const char *sql,
    sql_cb_t cb,
    void *user
) {
    if (s_db == NULL || s_loop == NULL) {
        fprintf(stderr, "SQLite service not initialized.\n");
        return -1;
    }
    if (!sql) {
        fprintf(stderr, "SQL statement cannot be NULL.\n");
        return -1;
    }

    sql_ctx_t *c = calloc(1, sizeof(*c));
    if (!c) {
        perror("calloc for sql_ctx_t");
        return -1;
    }

    c->sql = strdup(sql);
    if (!c->sql) {
        perror("strdup for sql statement");
        free(c);
        return -1;
    }
    c->cb = cb;
    c->user = user;
    c->req.data = c; // Link req to its context

    // Queue the work to be executed in a worker thread
    return uv_queue_work(s_loop, &c->req, sql_work, sql_after);
}
