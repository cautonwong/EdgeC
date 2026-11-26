#ifndef ALARM_HISTORY_INTERNAL_H
#define ALARM_HISTORY_INTERNAL_H

#include <inttypes.h>
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "alarm_history_sqlite.h"
#include "edge_digest.h"

#define ALARM_PATH_MAX 512

typedef struct {
    sqlite3 *db;
    char db_path[ALARM_PATH_MAX];
    unsigned long long export_fail_after;
} alarm_history_store_t;

static inline edge_history_result_t fail(edge_history_result_t rc, char *reason, size_t reason_size,
                                         const char *text) {
    if (reason && reason_size) snprintf(reason, reason_size, "%s", text);
    return rc;
}

static inline edge_history_result_t db_fail(sqlite3 *db, edge_history_result_t rc, char *reason,
                                            size_t reason_size, const char *what) {
    if (reason && reason_size) snprintf(reason, reason_size, "%s: %s", what, sqlite3_errmsg(db));
    return rc;
}

static inline edge_history_result_t map_sqlite(int rc) {
    switch (rc & 0xff) {
        case SQLITE_FULL:
            return EDGE_HISTORY_NO_SPACE;
        case SQLITE_BUSY:
        case SQLITE_LOCKED:
            return EDGE_HISTORY_BUSY;
        case SQLITE_CORRUPT:
        case SQLITE_NOTADB:
            return EDGE_HISTORY_CORRUPT;
        default:
            return EDGE_HISTORY_IO;
    }
}

static inline edge_history_result_t exec_sql(sqlite3 *db, const char *sql) {
    int rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
    return rc == SQLITE_OK ? EDGE_HISTORY_OK : map_sqlite(rc);
}

static inline edge_history_result_t quick_check(sqlite3 *db) {
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, "PRAGMA quick_check;", -1, &stmt, NULL) != SQLITE_OK) {
        return map_sqlite(sqlite3_errcode(db));
    }
    edge_history_result_t rc = EDGE_HISTORY_CORRUPT;
    int step = sqlite3_step(stmt);
    if (step == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        if (text && strcmp((const char *)text, "ok") == 0) rc = EDGE_HISTORY_OK;
    } else if (step != SQLITE_DONE) {
        rc = map_sqlite(step);
    }
    sqlite3_finalize(stmt);
    return rc;
}

static inline int fp_update_u32(edge_sha256_t *ctx, uint32_t value) {
    unsigned char bytes[4];
    for (int i = 0; i < 4; i++) bytes[i] = (unsigned char)(value >> (24 - 8 * i));
    return edge_sha256_update(ctx, bytes, sizeof(bytes));
}

static inline int fp_update_u64(edge_sha256_t *ctx, uint64_t value) {
    unsigned char bytes[8];
    for (int i = 0; i < 8; i++) bytes[i] = (unsigned char)(value >> (56 - 8 * i));
    return edge_sha256_update(ctx, bytes, sizeof(bytes));
}

static inline int fp_update_text(edge_sha256_t *ctx, const unsigned char *text, int bytes) {
    if (bytes < 0) return -1;
    if (fp_update_u32(ctx, (uint32_t)bytes) != 0) return -1;
    if (bytes == 0) return 0;
    return edge_sha256_update(ctx, text, (size_t)bytes);
}

#endif /* ALARM_HISTORY_INTERNAL_H */
