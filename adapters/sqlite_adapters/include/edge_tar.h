/**
 * @file edge_tar.h
 * @brief Minimal ustar writer shared by the backup bundle and app export.
 */
#ifndef EDGE_TAR_H
#define EDGE_TAR_H

#include <stddef.h>
#include <stdio.h>

#define EDGE_TAR_BLOCK 512u

/** Fill one ustar header block for a regular file. */
void edge_tar_header(void *block, const char *name, unsigned long long size);

/** Write header, data and padding for one member. */
int edge_tar_write_entry(FILE *out, const char *name, const void *data, size_t size);

/** Write the two terminating zero blocks. */
int edge_tar_finish(FILE *out);

#endif /* EDGE_TAR_H */
