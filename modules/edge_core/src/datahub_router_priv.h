#ifndef DATAHUB_ROUTER_PRIV_H
#define DATAHUB_ROUTER_PRIV_H

#include "datahub_priv.h"
#include "datahub_router.h"

struct datahub_router_s {
    struct datahub_s shm_hub;
    struct datahub_s dds_hub;
    struct datahub_s mqtt_hub;
    bool shm_active;
    bool dds_active;
    bool mqtt_active;
    datahub_router_policy_t write_policy;
};

#endif /* DATAHUB_ROUTER_PRIV_H */
