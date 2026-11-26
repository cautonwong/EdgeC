/**
 * @file edge_digest.h
 * @brief SHA-256 over a file or a stream of chunks, shared by backup and export.
 */
#ifndef EDGE_DIGEST_H
#define EDGE_DIGEST_H

#include <stddef.h>

/** Lowercase hex digest of path; size_out receives the byte count. */
int edge_sha256_file(const char *path, char hex[65], unsigned long long *size_out);

/** Streaming SHA-256 for callers hashing many small records. */
typedef struct edge_sha256_s edge_sha256_t;

edge_sha256_t *edge_sha256_start(void);

/** Returns 0 or -1. */
int edge_sha256_update(edge_sha256_t *ctx, const void *data, size_t len);

/** Finalize and write 64 lowercase hex digits plus NUL. Returns 0 or -1. */
int edge_sha256_finish_hex(edge_sha256_t *ctx, char hex[65]);

void edge_sha256_free(edge_sha256_t *ctx);

#endif /* EDGE_DIGEST_H */
