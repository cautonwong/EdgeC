/**
 * @file edge_tar.h
 * @brief Minimal ustar writer and reader shared by backup, export and history.
 */
#ifndef EDGE_TAR_H
#define EDGE_TAR_H

#include <stddef.h>
#include <stdio.h>

#define EDGE_TAR_BLOCK 512u
#define EDGE_TAR_NAME_SIZE 101
#define EDGE_TAR_MAX_MEMBERS 64

/** Fill one ustar header block for a regular file. */
void edge_tar_header(void *block, const char *name, unsigned long long size);

/** Write header, data and padding for one member. */
int edge_tar_write_entry(FILE *out, const char *name, const void *data, size_t size);

/** Write the two terminating zero blocks. */
int edge_tar_finish(FILE *out);

/**
 * Open path for writing with mode 0600 regardless of umask. Bundles contain
 * product data, so a umask 022 host must not expose them.
 */
FILE *edge_tar_fopen_private(const char *path, const char *mode);

/**
 * List regular-member names. Strict: exactly two zero blocks must end the
 * archive with no trailing bytes. names holds cap entries of
 * EDGE_TAR_NAME_SIZE bytes. Returns 0 or -1.
 */
int edge_tar_list(const char *path, char names[][EDGE_TAR_NAME_SIZE], size_t cap, size_t *count);

/**
 * Load one member into a malloc'd buffer (NUL appended) and hash it.
 * Members larger than max_size are rejected. Returns 0 or -1.
 */
int edge_tar_read_member(const char *path, const char *wanted, size_t max_size,
                         unsigned char **data, size_t *len, char sha[65]);

/** Hash one member without loading it. Returns 0 or -1. */
int edge_tar_hash_member(const char *path, const char *wanted, char sha[65],
                         unsigned long long *size_out);

/** Extract one member to dest as mode 0600 and fsync it. Returns 0 or -1. */
int edge_tar_copy_member(const char *path, const char *wanted, const char *dest);

#endif /* EDGE_TAR_H */
