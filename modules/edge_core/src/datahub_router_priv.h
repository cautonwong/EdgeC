#ifndef DATAHUB_ROUTER_PRIV_H
#define DATAHUB_ROUTER_PRIV_H

#include "datahub_priv.h"
#include "datahub_router.h"

enum {
    DATAHUB_ROUTER_BACKEND_SHM = 0,
    DATAHUB_ROUTER_BACKEND_DDS,
    DATAHUB_ROUTER_BACKEND_MQTT,
    DATAHUB_ROUTER_BACKEND_COUNT
};

typedef struct {
    struct datahub_s hub;
    bool active;
} datahub_router_backend_t;

struct datahub_router_s {
    datahub_router_backend_t backends[DATAHUB_ROUTER_BACKEND_COUNT];
    datahub_router_policy_t write_policy;
};

#endif /* DATAHUB_ROUTER_PRIV_H */
