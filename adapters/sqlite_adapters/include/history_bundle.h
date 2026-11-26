/**
 * @file history_bundle.h
 * @brief Store-agnostic `edgeos-history/1` bundle writer and verifier.
 *
 * A bundle is a ustar archive holding data members first and `manifest.json`
 * last, followed by the two terminating zero blocks. Every member is pinned
 * by size and SHA-256 in the manifest; the manifest is pinned by the tar
 * header checksum and by the member set itself. out_path is never left
 * partially written.
 */
#ifndef HISTORY_BUNDLE_H
#define HISTORY_BUNDLE_H

#include <stddef.h>
#include <stdint.h>

#include "edge_history_archive.h"
#include "edge_tar.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /** Member path inside the archive, relative and under 100 bytes. */
    const char *name;
    const char *path;
    /** Data rows; only used when has_rows is set. */
    unsigned long long rows;
    int has_rows;
} edge_history_member_t;

typedef struct {
    /** Must be EDGE_HISTORY_FORMAT. */
    const char *format;
    const char *store;
    int schema_version;
    int64_t from_ms;
    int64_t to_ms;
    int64_t created_ms;
    int includes_active;
    /** Store-specific coverage mark, for alarms the max recovery_time. */
    int64_t watermark_ms;
    /** SHA-256 over the exported source rows, 64 lowercase hex. */
    const char *source_fingerprint;
    const char *source_db;
    const char *source_table;
    const char *source_schema;
    const edge_history_member_t *members;
    size_t member_count;
    unsigned long long row_count;
} edge_history_bundle_desc_t;

typedef struct {
    char store[32];
    int schema_version;
    int64_t from_ms;
    int64_t to_ms;
    int64_t created_ms;
    int64_t watermark_ms;
    int includes_active;
    unsigned long long row_count;
    char source_fingerprint[EDGE_HISTORY_SHA256_SIZE];
    uint64_t member_count;
} edge_history_bundle_info_t;

/**
 * Write desc to out_path. An existing verified bundle is not checked here;
 * the caller decides whether to reuse, rename or conflict.
 *
 * fail_after_bytes simulates a short write after that many payload bytes and
 * reports NO_SPACE; 0 disables it. It is the fault-injection seam for disk-full
 * tests, so tests never fill a real disk.
 */
edge_history_result_t edge_history_bundle_write(const char *out_path,
                                                const edge_history_bundle_desc_t *desc,
                                                unsigned long long fail_after_bytes, char *reason,
                                                size_t reason_size);

/**
 * Verify the archive, manifest, member set, sizes, digests and row counts.
 * expect_store NULL accepts any store. A manifest schema_version above
 * max_schema_version is rejected.
 */
edge_history_result_t edge_history_bundle_verify(const char *path, const char *expect_store,
                                                 int max_schema_version,
                                                 edge_history_bundle_info_t *info, char *reason,
                                                 size_t reason_size);

#ifdef __cplusplus
}
#endif

#endif /* HISTORY_BUNDLE_H */
