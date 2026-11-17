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

#define IEMS_MAX_POINT_SUBSCRIPTIONS 64 /* matches DATAHUB_MAX_SUBSCRIBERS */

/* One DataHub registration. Written on the loop thread before subscribe
 * returns; immutable until unsubscribe has returned and the slot is cleared. */
typedef struct {
    int32_t sub_id; /**< 0 = free */
    app_host_event_cb_t on_event;
    void *event_ctx;
} iems_point_sub_t;

typedef struct {
    const char *app_id;
    datahub_t *hub;
    sqlite_svc_t *db;
    const edge_tag_meta_t *product_tags;
    uint32_t product_tag_count;
    uint32_t app_base;
    iems_point_sub_t subscriptions[IEMS_MAX_POINT_SUBSCRIPTIONS];
} iems_app_host_ctx_t;

/** Wire the host callbacks to ctx. ctx must outlive the server. */
void iems_app_host_ports_init(app_host_ports_t *ports, iems_app_host_ctx_t *ctx);

#endif /* IEMS_APP_HOST_PORTS_H */
