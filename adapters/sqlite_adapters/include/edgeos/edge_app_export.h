/**
 * @file edge_app_export.h
 * @brief App export archive: identity, points, grants and package digest.
 *
 * The ustar bundle holds `manifest.json`, `points.json`, `grants.json` and
 * `package.json`. Secrets and point history are deliberately absent; the
 * package itself is represented by its SHA-256 rather than copied.
 */
#ifndef EDGE_APP_EXPORT_H
#define EDGE_APP_EXPORT_H

#include <stddef.h>

#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_APP_EXPORT_REASON_SIZE 200

/** Write the export archive for app_id to out_path, atomically. */
int edge_app_export_archive(sqlite_svc_t *db, const char *app_id, const char *out_path,
                            char *reason, size_t reason_size);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_APP_EXPORT_H */
