#ifndef DCU_SCHEME_STORE_INTERNAL_H
#define DCU_SCHEME_STORE_INTERNAL_H

#include <sqlite3.h>

#include "dcu_storage.h"

void dcu_scheme_store_report_bind(dcu_storage_t *storage);

static inline int dcu_scheme_store_begin(sqlite3 *db) {
    return sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION;", NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

static inline int dcu_scheme_store_commit(sqlite3 *db) {
    return sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

static inline void dcu_scheme_store_rollback(sqlite3 *db) {
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
}

static inline int dcu_scheme_store_exec(sqlite3 *db, const char *sql) {
    return sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK ? 0 : -1;
}

#endif /* DCU_SCHEME_STORE_INTERNAL_H */
