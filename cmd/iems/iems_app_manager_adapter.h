/**
 * @file iems_app_manager_adapter.h
 * @brief Composition-side wiring for the IEMS app manager and its sqlite repo.
 */

#ifndef IEMS_APP_MANAGER_ADAPTER_H
#define IEMS_APP_MANAGER_ADAPTER_H

#include <stddef.h>

#include "rest_control_port.h"

typedef struct uv_loop_s uv_loop_t;
typedef struct stream_engine_s stream_engine_t;
struct sqlite_svc_s;
typedef struct sqlite_svc_s sqlite_svc_t;
struct edge_app_mgr_s;
typedef struct edge_app_mgr_s edge_app_mgr_t;

typedef struct {
    edge_app_mgr_t *mgr;
    stream_engine_t *engine;
} iems_control_ctx_t;

edge_app_mgr_t *iems_app_manager_create(uv_loop_t *loop, sqlite_svc_t *db,
                                        stream_engine_t *engine, void **storage_out);
void iems_app_manager_destroy(edge_app_mgr_t *mgr, void *storage);

void iems_control_port_init(rest_control_port_t *port, iems_control_ctx_t *ctx,
                            edge_app_mgr_t *mgr, stream_engine_t *engine);

#endif /* IEMS_APP_MANAGER_ADAPTER_H */
