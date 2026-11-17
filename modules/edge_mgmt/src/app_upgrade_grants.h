#ifndef EDGE_APP_UPGRADE_GRANTS_H
#define EDGE_APP_UPGRADE_GRANTS_H

#include <stdbool.h>
#include <stddef.h>

#include "edgeos/edge_app_mgr.h"
#include "edgeos/edge_authz.h"

/* Grant sidecars of an interrupted upgrade. The snapshot is written after the
 * package swap and before the registry commit; the done marker is written by
 * the grant block. Recovery replays the snapshot when the done marker is
 * missing, then rolls the package and registry row back. */
void app_upgrade_grants_path(const char *target, char *out, size_t cap);
void app_upgrade_grants_done_path(const char *target, char *out, size_t cap);
void app_upgrade_markers_remove(const char *target);

int app_upgrade_grants_write_snapshot(const char *target,
                                      const app_capability_scope_t *capabilities,
                                      const edge_app_grant_t *snapshots, const bool *found,
                                      size_t count, char *reason, size_t reason_cap);
int app_upgrade_grants_write_done(const char *target);
int app_upgrade_grants_restore(const edge_authz_store_t *grants, const char *app_id,
                               const app_capability_scope_t *capabilities,
                               const edge_app_grant_t *snapshots, const bool *found, size_t count);
int app_upgrade_grants_restore_snapshot(const edge_authz_store_t *grants, const char *app_id,
                                        const char *snapshot_path);

#endif /* EDGE_APP_UPGRADE_GRANTS_H */
