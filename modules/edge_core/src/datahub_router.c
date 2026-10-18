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
            LOG_INFO(LOG_CAT_CORE, "DataHub Router: SHM backend active");
        }
    }

    if (config->enable_dds) {
        const datahub_ops_t *dds_ops = config->dds_ops ? config->dds_ops : datahub_get_backend(DATAHUB_BACKEND_DDS);
        router->dds_hub.ops = dds_ops;
        if (router->dds_hub.ops && router->dds_hub.ops->init) {
            int rc = router->dds_hub.ops->init(&router->dds_hub, &config->dds_config);
            if (rc == 0) {
                router->dds_active = true;
                LOG_INFO(LOG_CAT_CORE, "DataHub Router: DDS backend active");
            } else {
                router->dds_active = false;
                LOG_WARN(LOG_CAT_CORE, "DataHub Router: DDS backend disabled or failed to init (rc=%d), falling back gracefully", rc);
            }
        }
    }

    if (config->enable_mqtt) {
        const datahub_ops_t *mqtt_ops = config->mqtt_ops ? config->mqtt_ops : datahub_get_backend(DATAHUB_BACKEND_MQTT);
        router->mqtt_hub.ops = mqtt_ops;
        if (router->mqtt_hub.ops && router->mqtt_hub.ops->init) {
            int rc = router->mqtt_hub.ops->init(&router->mqtt_hub, &config->mqtt_config);
            if (rc == 0) {
                router->mqtt_active = true;
                LOG_INFO(LOG_CAT_CORE, "DataHub Router: MQTT backend active");
            } else {
                router->mqtt_active = false;
                LOG_WARN(LOG_CAT_CORE, "DataHub Router: MQTT backend disabled or failed to init (rc=%d), falling back gracefully", rc);
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

static int _datahub_router_ops_init(datahub_t *hub, const datahub_backend_config_t *config) {
    if (!hub || !config) return -1;
    datahub_router_t *router = (datahub_router_t *)calloc(1, sizeof(datahub_router_t));
    if (!router) return -1;

    datahub_router_config_t rcfg;
    memset(&rcfg, 0, sizeof(rcfg));
    rcfg.enable_shm = true;
    rcfg.enable_dds = (config->backend_type == DATAHUB_BACKEND_HYBRID || config->backend_type == DATAHUB_BACKEND_DDS);
    rcfg.enable_mqtt = (config->backend_type == DATAHUB_BACKEND_HYBRID || config->backend_type == DATAHUB_BACKEND_MQTT);
    rcfg.write_policy = DATAHUB_ROUTER_POLICY_BEST_EFFORT;
    rcfg.shm_config = *config;
    rcfg.shm_config.backend_type = DATAHUB_BACKEND_SHM;
    rcfg.dds_ops = datahub_get_backend(DATAHUB_BACKEND_DDS);
    rcfg.mqtt_ops = datahub_get_backend(DATAHUB_BACKEND_MQTT);

    int rc = datahub_router_init(router, hub->loop, &rcfg, hub->metadata_table, hub->metadata_count,
                                 hub->io_pool, hub->map_fn, hub->wakeup_fn);
    if (rc != 0) {
        free(router);
        return rc;
    }
    hub->backend_ctx = router;
    return 0;
}

static int _datahub_router_ops_publish(datahub_t *hub, const PipeMsg *msg) {
    if (!hub || !hub->backend_ctx) return -1;
    return datahub_router_publish((datahub_router_t *)hub->backend_ctx, msg);
}

static int _datahub_router_ops_write_shadow(datahub_t *hub, const PipeMsg *msg) {
    if (!hub || !hub->backend_ctx) return -1;
    return datahub_router_write_shadow((datahub_router_t *)hub->backend_ctx, msg);
}

static int _datahub_router_ops_read_shadow(datahub_t *hub, uint32_t tag_id, PipeMsg *out_msg) {
    if (!hub || !hub->backend_ctx) return -1;
    return datahub_router_read_shadow((datahub_router_t *)hub->backend_ctx, tag_id, out_msg);
}

static int _datahub_router_ops_read_shadow_device(datahub_t *hub, uint32_t tag_id, uint32_t device_id, PipeMsg *out_msg) {
    if (!hub || !hub->backend_ctx) return -1;
    datahub_router_t *router = (datahub_router_t *)hub->backend_ctx;
    if (router->shm_active) {
        return datahub_read_shadow_device(&router->shm_hub, tag_id, device_id, out_msg);
    }
    return datahub_router_read_shadow(router, tag_id, out_msg);
}

static void _datahub_router_ops_destroy(datahub_t *hub) {
    if (!hub || !hub->backend_ctx) return;
    datahub_router_destroy((datahub_router_t *)hub->backend_ctx);
    free(hub->backend_ctx);
    hub->backend_ctx = NULL;
}

const datahub_ops_t g_datahub_router_ops = {
    .init = _datahub_router_ops_init,
    .publish = _datahub_router_ops_publish,
    .write_shadow = _datahub_router_ops_write_shadow,
    .read_shadow = _datahub_router_ops_read_shadow,
    .read_shadow_device = _datahub_router_ops_read_shadow_device,
    .destroy = _datahub_router_ops_destroy,
};
