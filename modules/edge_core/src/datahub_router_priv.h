#ifndef DATAHUB_ROUTER_PRIV_H
#define DATAHUB_ROUTER_PRIV_H

#include "datahub_router.h"

struct datahub_router_s {
    datahub_t shm_hub;
    datahub_t dds_hub;
    datahub_t mqtt_hub;
    bool shm_active;
    bool dds_active;
    bool mqtt_active;
    datahub_router_policy_t write_policy;
};

#endif /* DATAHUB_ROUTER_PRIV_H */
