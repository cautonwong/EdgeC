/**
 * @file iems_app_host_ports.h
 * @brief app_host_ports_t implementation over the iEMS DataHub, registry and authz.
 */
#ifndef IEMS_APP_HOST_PORTS_H
#define IEMS_APP_HOST_PORTS_H

#include <stdint.h>

#include "contract_data_types.h"
#include "datahub.h"
#include "edge_app_host.h"
#include "sqlite_service.h"

typedef struct {
    const char *app_id;
    datahub_t *hub;
    sqlite_svc_t *db;
    const edge_tag_meta_t *product_tags;
    uint32_t product_tag_count;
    uint32_t app_base;
} iems_app_host_ctx_t;

/** Wire the five host callbacks to ctx. ctx must outlive the server. */
void iems_app_host_ports_init(app_host_ports_t *ports, iems_app_host_ctx_t *ctx);

#endif /* IEMS_APP_HOST_PORTS_H */
