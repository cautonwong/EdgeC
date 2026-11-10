#include "datahub_router.h"
#include "datahub_router_priv.h"
#include "datahub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edge_log.h"

_Static_assert(sizeof(struct datahub_router_s) <= sizeof(datahub_router_storage_t),
               "datahub_router_s exceeds datahub_router_storage_t");

typedef struct {
    const char *name;
    const datahub_config_t *config;
    const datahub_ops_t *ops;
    bool enabled;
    bool remote;
} router_backend_spec_t;

static void _router_build_specs(const datahub_router_config_t *config,
                                router_backend_spec_t specs[DATAHUB_ROUTER_BACKEND_COUNT]) {
    specs[DATAHUB_ROUTER_BACKEND_SHM] = (router_backend_spec_t){
        .name = "SHM",
        .config = &config->shm_config,
        .enabled = config->enable_shm,
        .remote = false,
    };
    specs[DATAHUB_ROUTER_BACKEND_DDS] = (router_backend_spec_t){
        .name = "DDS",
        .config = &config->dds_config,
        .ops = config->dds_ops ? config->dds_ops : datahub_get_backend(DATAHUB_BACKEND_DDS),
        .enabled = config->enable_dds,
        .remote = true,
    };
    specs[DATAHUB_ROUTER_BACKEND_MQTT] = (router_backend_spec_t){
        .name = "MQTT",
        .config = &config->mqtt_config,
        .ops = config->mqtt_ops ? config->mqtt_ops : datahub_get_backend(DATAHUB_BACKEND_MQTT),
        .enabled = config->enable_mqtt,
        .remote = true,
    };
}

static void _router_start_backend(datahub_router_backend_t *backend,
                                  const router_backend_spec_t *spec, uv_loop_t *loop,
                                  const edge_tag_meta_t *metadata, uint32_t metadata_len,
                                  struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                                  datahub_wakeup_filter_cb wakeup_fn) {
    if (!spec->enabled) return;

    if (!spec->remote) {
        if (datahub_init_with_config(&backend->hub, loop, spec->config, metadata, metadata_len,
                                     io_pool, map_fn, wakeup_fn) != 0) {
            return;
        }
    } else {
        if (!spec->ops || !spec->ops->init) return;
        memset(&backend->hub, 0, sizeof(backend->hub));
        backend->hub.ops = spec->ops;
        int rc = spec->ops->init(&backend->hub, spec->config);
        if (rc != 0) {
            LOG_WARN(LOG_CAT_CORE,
                     "DataHub Router: %s backend disabled or failed to init (rc=%d), falling back gracefully",
                     spec->name, rc);
            return;
        }
    }

    backend->active = true;
    LOG_INFO(LOG_CAT_CORE, "DataHub Router: %s backend active", spec->name);
}

int datahub_router_init(datahub_router_t *router, uv_loop_t *loop, const datahub_router_config_t *config,
                       const edge_tag_meta_t *metadata, uint32_t metadata_len,
                       struct net_buf_pool *io_pool, datahub_tag_map_cb map_fn,
                       datahub_wakeup_filter_cb wakeup_fn) {
    if (!router || !config) return -1;
    memset(router, 0, sizeof(datahub_router_t));

    router->write_policy = config->write_policy;

    router_backend_spec_t specs[DATAHUB_ROUTER_BACKEND_COUNT];
    _router_build_specs(config, specs);
    for (int i = 0; i < DATAHUB_ROUTER_BACKEND_COUNT; i++) {
        _router_start_backend(&router->backends[i], &specs[i], loop, metadata, metadata_len,
                              io_pool, map_fn, wakeup_fn);
    }

    return 0;
}

datahub_router_policy_t datahub_router_get_policy(const datahub_router_t *router) {
    return router ? router->write_policy : DATAHUB_ROUTER_POLICY_BEST_EFFORT;
}

bool datahub_router_shm_active(const datahub_router_t *router) {
    return router ? router->backends[DATAHUB_ROUTER_BACKEND_SHM].active : false;
}

bool datahub_router_dds_active(const datahub_router_t *router) {
    return router ? router->backends[DATAHUB_ROUTER_BACKEND_DDS].active : false;
}

static int _router_fanout(datahub_router_t *router, const pipe_msg_t *msg, bool shadow) {
    int total_active = 0;
    int success_count = 0;
    int first_error = 0;

    for (int i = 0; i < DATAHUB_ROUTER_BACKEND_COUNT; i++) {
        datahub_router_backend_t *backend = &router->backends[i];
        if (!backend->active) continue;

        total_active++;
        int rc = shadow ? datahub_write_shadow(&backend->hub, msg)
                        : datahub_publish(&backend->hub, msg);
        if (rc == 0) success_count++;
        else if (first_error == 0) first_error = rc;
    }

    if (total_active == 0) return -1;
    if (router->write_policy == DATAHUB_ROUTER_POLICY_ALL_REQUIRED) {
        return (success_count == total_active) ? 0 : (first_error ? first_error : -1);
    }
    return (success_count > 0) ? 0 : (first_error ? first_error : -1);
}

int datahub_router_publish(datahub_router_t *router, const pipe_msg_t *msg) {
    if (!router || !msg) return -1;
    return _router_fanout(router, msg, false);
}

int datahub_router_write_shadow(datahub_router_t *router, const pipe_msg_t *msg) {
    if (!router || !msg) return -1;
    return _router_fanout(router, msg, true);
}

int datahub_router_read_shadow(datahub_router_t *router, uint32_t tag_id, pipe_msg_t *out_msg) {
    if (!router || !out_msg) return -1;

    for (int i = 0; i < DATAHUB_ROUTER_BACKEND_COUNT; i++) {
        if (router->backends[i].active) {
            return datahub_read_shadow(&router->backends[i].hub, tag_id, out_msg);
        }
    }

    return -1;
}

void datahub_router_destroy(datahub_router_t *router) {
    if (!router) return;

    for (int i = 0; i < DATAHUB_ROUTER_BACKEND_COUNT; i++) {
        datahub_router_backend_t *backend = &router->backends[i];
        if (!backend->active) continue;
        datahub_destroy(&backend->hub);
        backend->active = false;
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
    datahub_router_backend_t *shm = &router->backends[DATAHUB_ROUTER_BACKEND_SHM];
    if (shm->active) {
        return datahub_read_shadow_device(&shm->hub, tag_id, device_id, out_msg);
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
