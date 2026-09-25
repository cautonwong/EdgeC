#include "datahub_router.h"
#include "datahub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

int datahub_router_init(datahub_router_t *router, uv_loop_t *loop, const datahub_router_config_t *config,
                       const edge_tag_meta_t *metadata, uint32_t metadata_len,
                       struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                       datahub_wakeup_filter_cb wakeup_fn) {
    if (!router || !config) return -1;
    memset(router, 0, sizeof(datahub_router_t));

    router->write_policy = config->write_policy;

    if (config->enable_shm) {
        int rc = datahub_init_with_config(&router->shm_hub, loop, &config->shm_config, metadata, metadata_len, io_pool, map_fn, wakeup_fn);
        if (rc == 0) {
            router->shm_active = true;
            LOG_INFO(CORE, "DataHub Router: SHM backend active");
        }
    }

    if (config->enable_dds) {
        if (config->dds_ops) {
            router->dds_hub.ops = config->dds_ops;
        }
        if (router->dds_hub.ops && router->dds_hub.ops->init) {
            int rc = router->dds_hub.ops->init(&router->dds_hub, &config->dds_config);
            if (rc == 0) {
                router->dds_active = true;
                LOG_INFO(CORE, "DataHub Router: DDS backend active");
            }
        }
    }

    if (config->enable_mqtt) {
        if (config->mqtt_ops) {
            router->mqtt_hub.ops = config->mqtt_ops;
        }
        if (router->mqtt_hub.ops && router->mqtt_hub.ops->init) {
            int rc = router->mqtt_hub.ops->init(&router->mqtt_hub, &config->mqtt_config);
            if (rc == 0) {
                router->mqtt_active = true;
                LOG_INFO(CORE, "DataHub Router: MQTT backend active");
            }
        }
    }

    return 0;
}

int datahub_router_publish(datahub_router_t *router, const pipe_msg_t *msg) {
    if (!router || !msg) return -1;
    int total_active = 0;
    int success_count = 0;
    int first_error = 0;

    if (router->shm_active) {
        total_active++;
        int rc = datahub_publish(&router->shm_hub, msg);
        if (rc == 0) success_count++;
        else if (first_error == 0) first_error = rc;
    }
    if (router->dds_active && router->dds_hub.ops) {
        total_active++;
        int rc = router->dds_hub.ops->publish(&router->dds_hub, msg);
        if (rc == 0) success_count++;
        else if (first_error == 0) first_error = rc;
    }
    if (router->mqtt_active && router->mqtt_hub.ops) {
        total_active++;
        int rc = router->mqtt_hub.ops->publish(&router->mqtt_hub, msg);
        if (rc == 0) success_count++;
        else if (first_error == 0) first_error = rc;
    }

    if (total_active == 0) return -1;
    if (router->write_policy == DATAHUB_ROUTER_POLICY_ALL_REQUIRED) {
        return (success_count == total_active) ? 0 : (first_error ? first_error : -1);
    }
    return (success_count > 0) ? 0 : (first_error ? first_error : -1);
}

int datahub_router_write_shadow(datahub_router_t *router, const pipe_msg_t *msg) {
    if (!router || !msg) return -1;
    int total_active = 0;
    int success_count = 0;
    int first_error = 0;

    if (router->shm_active) {
        total_active++;
        int rc = datahub_write_shadow(&router->shm_hub, msg);
        if (rc == 0) success_count++;
        else if (first_error == 0) first_error = rc;
    }
    if (router->dds_active && router->dds_hub.ops) {
        total_active++;
        int rc = router->dds_hub.ops->write_shadow(&router->dds_hub, msg);
        if (rc == 0) success_count++;
        else if (first_error == 0) first_error = rc;
    }
    if (router->mqtt_active && router->mqtt_hub.ops) {
        total_active++;
        int rc = router->mqtt_hub.ops->write_shadow(&router->mqtt_hub, msg);
        if (rc == 0) success_count++;
        else if (first_error == 0) first_error = rc;
    }

    if (total_active == 0) return -1;
    if (router->write_policy == DATAHUB_ROUTER_POLICY_ALL_REQUIRED) {
        return (success_count == total_active) ? 0 : (first_error ? first_error : -1);
    }
    return (success_count > 0) ? 0 : (first_error ? first_error : -1);
}

int datahub_router_read_shadow(datahub_router_t *router, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (!router || !out_msg) return -1;

    if (router->shm_active) {
        return datahub_read_shadow(&router->shm_hub, tag_id, out_msg);
    }
    if (router->dds_active && router->dds_hub.ops) {
        return router->dds_hub.ops->read_shadow(&router->dds_hub, tag_id, out_msg);
    }
    if (router->mqtt_active && router->mqtt_hub.ops) {
        return router->mqtt_hub.ops->read_shadow(&router->mqtt_hub, tag_id, out_msg);
    }

    return -1;
}

void datahub_router_destroy(datahub_router_t *router) {
    if (!router) return;

    if (router->shm_active) {
        datahub_destroy(&router->shm_hub);
        router->shm_active = false;
    }
    if (router->dds_active && router->dds_hub.ops) {
        router->dds_hub.ops->destroy(&router->dds_hub);
        router->dds_active = false;
    }
    if (router->mqtt_active && router->mqtt_hub.ops) {
        router->mqtt_hub.ops->destroy(&router->mqtt_hub);
        router->mqtt_active = false;
    }
}
