#ifndef EDGE_PARAMS_SQLITE_ADAPTER_H
#define EDGE_PARAMS_SQLITE_ADAPTER_H

#include "edge_params.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Build a parameter store port backed by an existing sqlite service. */
edge_params_store_t edge_params_sqlite_store(sqlite_svc_t *db);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_PARAMS_SQLITE_ADAPTER_H */
