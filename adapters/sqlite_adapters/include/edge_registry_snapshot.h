/**
 * @file edge_registry_snapshot.h
 * @brief Registry snapshot publishing and status JSON over the control database.
 *
 * The database is the durable source of truth. A snapshot is a cache: the
 * generation comes from registry_meta, which SQLite triggers advance on every
 * app_registry or app_grant write.
 */
#ifndef EDGE_REGISTRY_SNAPSHOT_H
#define EDGE_REGISTRY_SNAPSHOT_H

#include <stddef.h>

struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;

/**
 * Write <dir>/gen-<generation>.json through a temp file, fsync and rename, then
 * atomically update <dir>/current to the same file name. Returns 0 and the
 * published path on success, -1 with a reason otherwise.
 */
int edge_registry_snapshot_publish(sqlite_svc_t *db, const char *dir, char *out_path,
                                   size_t path_cap, char *reason, size_t reason_cap);

/**
 * One consumer's applied generation, as last reported by its heartbeat. The
 * host keeps these in memory; staleness is decided by the caller.
 */
typedef struct {
    char name[64];
    unsigned long long applied_generation;
    int stale;
} edge_registry_consumer_t;

/**
 * Status JSON for the management plane: generation, apps, consumers and the
 * convergence verdict. converged is true only when every consumer is fresh and
 * has applied the current generation, null when there are no consumers, false
 * otherwise. app_id limits the apps array to one id; NULL returns every app.
 * Returns a malloc'd string; the caller frees. NULL on failure.
 */
char *edge_registry_status_json(sqlite_svc_t *db, const edge_registry_consumer_t *consumers,
                                size_t consumer_count);

char *edge_registry_status_json_for(sqlite_svc_t *db, const edge_registry_consumer_t *consumers,
                                    size_t consumer_count, const char *app_id);

/** Current generation from registry_meta. Returns 0 on success. */
int edge_registry_generation(sqlite_svc_t *db, unsigned long long *out);

#endif /* EDGE_REGISTRY_SNAPSHOT_H */
