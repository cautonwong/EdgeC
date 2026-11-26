/**
 * @file alarm_history_sqlite.h
 * @brief SQLite adapter for the alarm history port.
 *
 * The database is a separate file opened by this adapter. The sqlite3 handle
 * stays private; callers only see edge_alarm_history_port_t.
 */
#ifndef ALARM_HISTORY_SQLITE_H
#define ALARM_HISTORY_SQLITE_H

#include <stddef.h>

#include "edge_alarm_history.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open or create db_path and fill out. On failure out is zeroed and reason
 * carries the cause. A database failing quick_check is renamed to
 * `<db_path>.corrupt-<epoch>` and CORRUPT is returned; it is never deleted.
 */
edge_history_result_t alarm_history_sqlite_open(const char *db_path, edge_alarm_history_port_t *out,
                                                char *reason, size_t reason_size);

/** Close the database and zero port. Safe on a zeroed port. */
void alarm_history_sqlite_close(edge_alarm_history_port_t *port);

/**
 * Test seam: exports fail with NO_SPACE after this many payload bytes,
 * simulating a full disk; 0 disables the seam.
 */
void alarm_history_sqlite_set_export_fail_after(edge_alarm_history_port_t *port,
                                                unsigned long long bytes);

/** v3 opcalarm.db import counters; active/recovered are source totals. */
typedef struct {
    uint64_t source_rows;
    uint64_t imported_rows;
    uint64_t skipped_rows;
    uint64_t active_rows;
    uint64_t recovered_rows;
} alarm_history_migration_result_t;

/**
 * Import a v3 opcalarm.db alarmRealTab into dst, which must come from
 * alarm_history_sqlite_open.
 *
 * The source is opened read-only and must carry the exact v3 eight-column
 * schema; it is never written, deleted or vacuumed. A row is identified by a
 * digest of its eight v3 values, so a rerun appends only rows the target has
 * not seen. Rows commit one by one with their state entry, so an interrupted
 * run leaves a prefix that a rerun completes. dry_run reports the same
 * counters without touching the target or creating migration state.
 * Counts that do not reconcile return VERIFY and leave the imported part in
 * place for a rerun.
 */
edge_history_result_t alarm_history_sqlite_migrate_v3(const char *src_db_path,
                                                      edge_alarm_history_port_t *dst, int dry_run,
                                                      alarm_history_migration_result_t *out,
                                                      char *reason, size_t reason_size);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_HISTORY_SQLITE_H */
