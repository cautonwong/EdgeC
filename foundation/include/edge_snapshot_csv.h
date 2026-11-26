/**
 * @file edge_snapshot_csv.h
 * @brief Snapshot CSV writer port and date-directory retention policy.
 *
 * The writer formats caller-supplied labels and cells and appends them to
 * `<local_root>/<YYYY-MM-DD>/<file_stem>.csv`. It never reads the point table
 * or the wall clock: columns, values and local time come from the consumer.
 *
 * Quota deletion removes only backup-root date directories whose completion
 * marker verifies every CSV by size and SHA-256; unmarked or invalid ones are
 * kept. Local expiry moves a past date directory to the backup root after
 * sealing a local marker, or removes it in place when the backup already
 * holds byte-identical copies, with or without a marker. Only basenames
 * matching YYYY-MM-DD are managed; today and future dates are never moved or
 * deleted, and other entries are untouched except reserved .tmp-* and
 * .deleting-* directories under the backup root.
 *
 * Neither the writer nor edge_snapshot_maintain is thread safe. The consumer
 * runs one maintainer per root and closes every writer before maintenance.
 */
#ifndef EDGE_SNAPSHOT_CSV_H
#define EDGE_SNAPSHOT_CSV_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "edge_history_archive.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EDGE_SNAPSHOT_CELL_INT = 0, /**< v3 TYPE_TAG_INT: ",%lld". */
    EDGE_SNAPSHOT_CELL_FLOAT,   /**< v3 TYPE_TAG_FLOAT: ", %.2f". */
    EDGE_SNAPSHOT_CELL_EMPTY,   /**< v3 default and unsupported types: ", ". */
} edge_snapshot_cell_kind_t;

typedef struct {
    edge_snapshot_cell_kind_t kind;
    int64_t integer;
    double real;
} edge_snapshot_cell_t;

typedef struct {
    /** Consumer-built v3 label; opaque UTF-8 without a leading comma. */
    const char *label;
    /** Cell kind every row must supply for this column. */
    edge_snapshot_cell_kind_t kind;
} edge_snapshot_column_t;

typedef struct {
    void *ctx;
    /**
     * Append one row. count and every cell kind must match the columns passed
     * to open, and local_now must fall on the date the writer was opened for;
     * otherwise INVALID is returned and nothing is written.
     */
    edge_history_result_t (*append)(void *ctx, const struct tm *local_now,
                                    const edge_snapshot_cell_t *cells, size_t count, char *reason,
                                    size_t reason_size);
    /** fsync the file. Rows are not synced individually. */
    edge_history_result_t (*flush)(void *ctx, char *reason, size_t reason_size);
    /** Flush best effort, close and release ctx. */
    void (*close)(void *ctx);
} edge_snapshot_writer_port_t;

typedef struct {
    /** Required. Existing directory; date subdirectories are created here. */
    const char *local_root;
    /** NULL or "": local-only mode; nothing is moved or deleted. */
    const char *backup_root;
    /** Past date directories kept locally after backup; 0 keeps only today. */
    uint32_t keep_local_days;
    /** Backup usage trigger; 0 disables quota deletion. */
    uint32_t usage_threshold_percent;
    /** Maximum backup date directories deleted per maintain; 0 means 10. */
    uint32_t max_delete_dirs;
} edge_snapshot_retention_policy_t;

typedef struct {
    uint32_t moved_dirs;
    uint32_t deduped_local_dirs;
    uint32_t deleted_backup_dirs;
    uint32_t skipped_unknown;
    uint32_t conflicts;
    uint32_t failed_dirs;
    uint32_t usage_percent_after;
    int usage_known;
    /** Usage stayed above the threshold after this maintain; consumer warns. */
    int still_over_threshold;
    int backup_unavailable;
} edge_snapshot_retention_result_t;

/**
 * Move expired local date directories whose backup verifies, and enforce the
 * backup quota.
 *
 * Quota deletion only touches backup date directories whose completion marker
 * verifies. INVALID means the arguments or roots are unusable (including
 * local_root == backup_root) or a date directory cannot be sealed; IO means
 * the filesystem failed and nothing was deleted without proof; CORRUPT means
 * a local marker has invalid contents; CONFLICT means a marker is not a
 * regular file, a local directory has an unverified or conflicting backup, or
 * a backup marker failed quota verification. Non-regular markers are never
 * opened, so a FIFO cannot block maintenance. still_over_threshold is a
 * status for the consumer, not an error.
 */
edge_history_result_t edge_snapshot_maintain(const struct tm *local_now,
                                             const edge_snapshot_retention_policy_t *policy,
                                             edge_snapshot_retention_result_t *result, char *reason,
                                             size_t reason_size);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_SNAPSHOT_CSV_H */
