/**
 * @file iems_app_manager_adapter.h
 * @brief Composition-side wiring for the IEMS app manager and its sqlite repo.
 */

#ifndef IEMS_APP_MANAGER_ADAPTER_H
#define IEMS_APP_MANAGER_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

#include "edgeos/edge_eapp.h"
#include "rest_control_port.h"

typedef struct uv_loop_s uv_loop_t;
typedef struct stream_engine_s stream_engine_t;
struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;
struct edge_app_mgr_s;
typedef struct edge_app_mgr_s edge_app_mgr_t;

/** Consumers heartbeat every 10 s; three missed periods mark them stale. */
#define IEMS_REGISTRY_CONSUMERS_MAX 8
#define IEMS_CONSUMER_STALE_MS 30000u

typedef struct {
    char name[64];
    unsigned long long applied_generation;
    uint64_t last_seen_ms;
} iems_registry_consumer_t;

typedef struct iems_app_runtime_s iems_app_runtime_t;

typedef struct {
    edge_app_mgr_t *mgr;
    stream_engine_t *engine;
    sqlite_svc_t *db;
    iems_app_runtime_t *app_runtime;
    iems_registry_consumer_t consumers[IEMS_REGISTRY_CONSUMERS_MAX];
    size_t consumer_count;
    edge_eapp_key_t package_keys[EDGE_EAPP_KEYRING_MAX]; /**< EDGEC_PACKAGE_KEYRING_FILE or single key */
    size_t package_key_count;
} iems_control_ctx_t;

/** Attach the app-host runtime; NULL keeps the legacy in-process wasm stage. */
void iems_control_set_app_runtime(iems_control_ctx_t *ctx, iems_app_runtime_t *runtime);

/** Record an unexpected app-host exit in the registry and publish a snapshot. */
void iems_control_app_faulted(iems_control_ctx_t *ctx, const char *app_id, int64_t status,
                              int term_signal);

/** Mark a scheduled restart as running again and publish a snapshot. */
void iems_control_app_restarted(iems_control_ctx_t *ctx, const char *app_id);

edge_app_mgr_t *iems_app_manager_create(uv_loop_t *loop, sqlite_svc_t *db,
                                        stream_engine_t *engine, void **storage_out);
void iems_app_manager_destroy(edge_app_mgr_t *mgr, void *storage);

void iems_control_port_init(rest_control_port_t *port, iems_control_ctx_t *ctx,
                            edge_app_mgr_t *mgr, stream_engine_t *engine, sqlite_svc_t *db);

#endif /* IEMS_APP_MANAGER_ADAPTER_H */
