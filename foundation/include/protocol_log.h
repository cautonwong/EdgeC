/**
 * @file protocol_log.h
 * @brief Protocol-local diagnostics for pure codecs.
 *
 * Protocol codecs must not link edge_core just to emit a diagnostic. These
 * macros keep the same severity information on stderr without a log service.
 */
#ifndef PROTOCOL_LOG_H
#define PROTOCOL_LOG_H

#include <stdio.h>

#define PROTO_LOG_DEBUG(fmt, ...) fprintf(stderr, "[PROTO][DEBUG] " fmt "\n", ##__VA_ARGS__)
#define PROTO_LOG_INFO(fmt, ...)  fprintf(stderr, "[PROTO][INFO] " fmt "\n", ##__VA_ARGS__)
#define PROTO_LOG_WARN(fmt, ...)  fprintf(stderr, "[PROTO][WARN] " fmt "\n", ##__VA_ARGS__)
#define PROTO_LOG_ERROR(fmt, ...) fprintf(stderr, "[PROTO][ERROR] " fmt "\n", ##__VA_ARGS__)

#endif /* PROTOCOL_LOG_H */
