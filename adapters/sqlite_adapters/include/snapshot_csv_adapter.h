/**
 * @file snapshot_csv_adapter.h
 * @brief Filesystem writer and retention adapter for edge_snapshot_csv.h.
 *
 * Columns are copied at open.
 */
#ifndef SNAPSHOT_CSV_ADAPTER_H
#define SNAPSHOT_CSV_ADAPTER_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "edge_history_archive.h"
#include "edge_snapshot_csv.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open or create `<local_root>/<YYYY-MM-DD>/<file_stem>.csv` for local_now.
 * An existing file is validated: a crash-truncated BOM/header prefix is
 * rewritten, a trailing partial row is cut back to the last newline, a
 * different header returns CONFLICT and a missing BOM returns CORRUPT.
 * On failure out is zeroed and reason carries the cause.
 */
edge_history_result_t snapshot_csv_writer_open(const char *local_root, const struct tm *local_now,
                                               const char *file_stem,
                                               const edge_snapshot_column_t *columns,
                                               size_t column_count,
                                               edge_snapshot_writer_port_t *out, char *reason,
                                               size_t reason_size);

/** Close and zero port. Safe on a zeroed port. */
void snapshot_csv_writer_close(edge_snapshot_writer_port_t *port);

/**
 * Test seam: appends fail with NO_SPACE after this many payload bytes,
 * simulating a full disk; 0 disables the seam.
 */
void snapshot_csv_writer_set_fail_after(edge_snapshot_writer_port_t *port,
                                        unsigned long long bytes);

/** Test seam: override the backup usage percentage; 0 reads statvfs. */
void edge_snapshot_retention_set_usage_percent(uint32_t percent);

/** Test seam: force the cross-filesystem copy path even on one filesystem. */
void edge_snapshot_retention_set_force_copy(int on);

#ifdef __cplusplus
}
#endif

#endif /* SNAPSHOT_CSV_ADAPTER_H */
