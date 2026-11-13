/**
 * @file edge_backup.h
 * @brief Platform backup bundle: SQLite snapshot + manifest, tar container.
 *
 * Format `edgeos-backup/1`: a ustar archive holding `manifest.json` and
 * `edge.db`. The database is copied through the SQLite backup API, so a live
 * product process keeps writing while the backup runs. The manifest records
 * the schema version, the creation time and the SHA-256 of every member.
 *
 * Restore is offline: stop the product process first. The current database is
 * renamed to `<db>.before-restore` before the bundle database is moved in.
 *
 * Not implemented here: encryption and archive signing. Deny-by-default still
 * applies because a bundle is only accepted from an operator-selected path.
 */
#ifndef EDGE_BACKUP_H
#define EDGE_BACKUP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_BACKUP_REASON_SIZE 200
#define EDGE_BACKUP_FORMAT "edgeos-backup/1"

/** Snapshot db_path into a new bundle at out_path. */
int edge_backup_create(const char *db_path, const char *out_path, char *reason,
                       size_t reason_size);

/** Check the archive, manifest and every member digest. */
int edge_backup_verify(const char *bundle_path, char *reason, size_t reason_size);

/** Verify, then replace db_path with the bundle database. */
int edge_backup_restore(const char *bundle_path, const char *db_path, char *reason,
                        size_t reason_size);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_BACKUP_H */
