/**
 * @file edge_history_archive.h
 * @brief History retention and verifiable export port.
 *
 * The port describes mechanism only: the consumer decides when to export,
 * what retention policy applies and where bundles live. Exported bundles use
 * format `edgeos-history/1` (ustar + manifest.json + per-member SHA-256).
 *
 * Retain never deletes active alarm rows. When an export gate is required,
 * retain needs a bundle directory and only deletes recovered rows whose
 * recovery time a verified bundle covers.
 */
#ifndef EDGE_HISTORY_ARCHIVE_H
#define EDGE_HISTORY_ARCHIVE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_HISTORY_REASON_SIZE 200
#define EDGE_HISTORY_FORMAT "edgeos-history/1"
#define EDGE_HISTORY_SHA256_SIZE 65

typedef enum {
    EDGE_HISTORY_OK = 0,
    EDGE_HISTORY_INVALID,
    EDGE_HISTORY_NO_SPACE,
    EDGE_HISTORY_BUSY,
    EDGE_HISTORY_CORRUPT,
    EDGE_HISTORY_IO,
    EDGE_HISTORY_VERIFY,
    /** The row is protected, for example an active alarm. */
    EDGE_HISTORY_PROTECTED,
    /** The source no longer matches the fingerprint of an existing bundle. */
    EDGE_HISTORY_CONFLICT,
} edge_history_result_t;

typedef struct {
    /** Inclusive window start in epoch milliseconds. */
    int64_t from_ms;
    /** Inclusive window end in epoch milliseconds. */
    int64_t to_ms;
    /** Final bundle path. Temporary files are derived next to it. */
    const char *out_path;
    /** Alarms: export active rows too. Retention still never deletes them. */
    int include_active;
    /** Snapshot store: include the directory being written. */
    int include_current;
} edge_history_export_req_t;

typedef struct {
    uint64_t rows;
    unsigned long long bytes;
    /** Store-specific: alarms use the greatest exported recovery_time. */
    int64_t watermark_ms;
    char bundle_sha256[EDGE_HISTORY_SHA256_SIZE];
    /** Hash over the exported source rows; detects changed re-runs. */
    char source_fingerprint[EDGE_HISTORY_SHA256_SIZE];
    /** 1 when an existing verified bundle already covers this request. */
    int already;
} edge_history_export_info_t;

typedef struct {
    /** Retention clock supplied by the caller; retention never reads the wall clock. */
    int64_t now_ms;
    /** Keep at most this many recovered rows; 0 disables the limit. */
    uint32_t max_rows;
    /** Delete recovered rows older than this; 0 disables the limit. */
    uint32_t max_age_days;
    /**
     * 1 requires a verified export before deleting: export_dir is required,
     * and deletions stay inside windows that verified bundles cover.
     */
    int require_verified_export;
    /**
     * Caller-asserted upper cap on deletion when require_verified_export is
     * set; 0 disables the cap. It never authorizes deletion by itself and
     * cannot replace bundle coverage.
     */
    int64_t verified_watermark_ms;
    /** Rows per delete statement; 0 selects the adapter default. */
    uint32_t chunk_rows;
    /**
     * Directory holding verified bundles; required when
     * require_verified_export is set. Retain re-verifies each bundle, unions
     * the windows they cover and re-checks each covering bundle's
     * source_fingerprint before deleting.
     */
    const char *export_dir;
} edge_history_retention_t;

typedef struct {
    uint32_t deleted_rows;
    uint64_t freed_bytes;
    int64_t oldest_remaining_ms;
} edge_history_retention_result_t;

typedef struct {
    uint64_t rows;
    uint64_t active_rows;
    uint64_t bytes;
    int64_t oldest_ms;
    int64_t newest_ms;
} edge_history_stats_t;

typedef struct {
    void *ctx;
    /**
     * Export [from_ms, to_ms] to req->out_path. An existing verified bundle
     * with the same source returns already=1; changed source returns
     * CONFLICT without touching the bundle.
     */
    edge_history_result_t (*export_range)(void *ctx, const edge_history_export_req_t *req,
                                          edge_history_export_info_t *info, char *reason,
                                          size_t reason_size);
    /** Check the archive, manifest and every member digest. */
    edge_history_result_t (*verify_export)(void *ctx, const char *path,
                                           edge_history_export_info_t *info, char *reason,
                                           size_t reason_size);
    /** Deletes run in one transaction; a failed verify leaves rows untouched. */
    edge_history_result_t (*retain)(void *ctx, const edge_history_retention_t *policy,
                                    edge_history_retention_result_t *result, char *reason,
                                    size_t reason_size);
    edge_history_result_t (*stats)(void *ctx, edge_history_stats_t *out);
} edge_history_admin_port_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGE_HISTORY_ARCHIVE_H */
