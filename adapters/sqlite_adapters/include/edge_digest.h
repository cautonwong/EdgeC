/**
 * @file edge_digest.h
 * @brief SHA-256 over a file, shared by backup and export.
 */
#ifndef EDGE_DIGEST_H
#define EDGE_DIGEST_H

#include <stddef.h>

/** Lowercase hex digest of path; size_out receives the byte count. */
int edge_sha256_file(const char *path, char hex[65], unsigned long long *size_out);

#endif /* EDGE_DIGEST_H */
