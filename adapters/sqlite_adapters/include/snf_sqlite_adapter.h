#ifndef SNF_SQLITE_ADAPTER_H
#define SNF_SQLITE_ADAPTER_H

#include "snf_manager.h"
#include "sqlite_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Build a SnF store port backed by an existing sqlite service. */
snf_store_port_t snf_sqlite_store_port(sqlite_svc_t *db);

#ifdef __cplusplus
}
#endif

#endif /* SNF_SQLITE_ADAPTER_H */
