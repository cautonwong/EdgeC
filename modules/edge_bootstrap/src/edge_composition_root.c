/**
 * @file edge_composition_root.c
 * @brief EdgeOS Single Composition Root Implementation (AEP-751 / AEP-752 / Issue #336 & #340)
 */

#include "edge_composition_root.h"
#include "datahub_ops.h"
#include "dma_buf_pool.h"
#include "edge_anti_corruption.h"
#include "edge_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct edge_composition_container {
    edge_composition_config_t config;
    datahub_t *hub;
    dma_buf_pool_t *mem_pool;
    edge_anti_corruption_engine_t *acl_engine;
    edge_consumer_ports_t ports;
    edge_composition_metrics_t metrics;
    pthread_mutex_t lock;
    bool initialized;
    bool is_caller_allocated;
};

/* -------------------------------------------------------------------------- */
/* Compile-Time Size & Alignment Assertions (AEP-752)                          */
/* -------------------------------------------------------------------------- */
_Static_assert(sizeof(struct edge_composition_container) <= sizeof(edge_composition_container_storage_t),
               "edge_composition_container_storage_t buffer too small for struct edge_composition_container");
_Static_assert(_Alignof(struct edge_composition_container) <= _Alignof(edge_composition_container_storage_t),
               "edge_composition_container_storage_t alignment mismatch for struct edge_composition_container");

/* -------------------------------------------------------------------------- */
/* DMA-BUF Memory Pool Glue Adapter Implementation                            */
/* -------------------------------------------------------------------------- */

typedef struct {
    dma_buf_pool_t *pool;
    uint64_t total_acquires;
    uint64_t total_releases;
} dma_adapter_ctx_t;

static int dma_adapter_acquire(void *ctx, size_t size, void **out_ptr, int *out_fd) {
    if (!ctx || !out_ptr) return -1;
    (void)size;
    dma_adapter_ctx_t *ad = (dma_adapter_ctx_t *)ctx;
    if (!ad->pool) return -2;

    dma_buf_desc_t *desc = NULL;
    int rc = dma_buf_pool_acquire(ad->pool, &desc);
    if (rc != 0 || !desc) return rc ? rc : -3;

    *out_ptr = desc->vaddr;
    if (out_fd) *out_fd = desc->fd;
    ad->total_acquires++;
    return 0;
}

static int dma_adapter_release(void *ctx, void *ptr) {
    if (!ctx || !ptr) return -1;
    dma_adapter_ctx_t *ad = (dma_adapter_ctx_t *)ctx;
    if (!ad->pool) return -2;

    /* Search matching buffer descriptor by virtual address */
    dma_buf_desc_t temp_desc;
    memset(&temp_desc, 0, sizeof(temp_desc));
    temp_desc.vaddr = ptr;
    temp_desc.is_in_use = true;
    temp_desc.ref_count = 1;

    int rc = dma_buf_unref(ad->pool, &temp_desc);
    if (rc == 0) {
        ad->total_releases++;
    }
    return rc;
}

static dma_adapter_ctx_t s_default_dma_ad = {0};

int edge_port_adapter_from_dma_pool(edge_memory_port_t *out_mem, dma_buf_pool_t *pool) {
    if (!out_mem || !pool) return -1;
    s_default_dma_ad.pool = pool;
    out_mem->acquire_buffer = dma_adapter_acquire;
    out_mem->release_buffer = dma_adapter_release;
    out_mem->ctx = &s_default_dma_ad;
    return 0;
}

static int _init_container_resources(edge_composition_container_t *container, const edge_composition_config_t *cfg) {
    container->config = *cfg;
    pthread_mutex_init(&container->lock, NULL);

    /* 1. Allocate & Initialize Foundation: DataHub shadow memory bus */
    container->hub = (datahub_t *)calloc(1, sizeof(datahub_t));
    if (!container->hub) {
        pthread_mutex_destroy(&container->lock);
        return -3;
    }

    const char *env_shm = getenv("EDGE_SHM_NAME");
    const char *p_name = cfg->shm_name ? cfg->shm_name : (env_shm ? env_shm : (cfg->product_name ? cfg->product_name : "edgeos_bus"));
    uint32_t ring_cap = cfg->ring_capacity ? (uint32_t)cfg->ring_capacity : 4096;
    uint32_t shadow_cap = cfg->datahub_capacity ? (uint32_t)cfg->datahub_capacity : 1024;
    datahub_backend_type_t backend = cfg->backend_type;
    const char *env_b = getenv("EDGE_DATAHUB_BACKEND");
    if (env_b) {
        if (strcasecmp(env_b, "hybrid") == 0) backend = DATAHUB_BACKEND_HYBRID;
        else if (strcasecmp(env_b, "dds") == 0) backend = DATAHUB_BACKEND_DDS;
        else if (strcasecmp(env_b, "mqtt") == 0) backend = DATAHUB_BACKEND_MQTT;
        else if (strcasecmp(env_b, "shm") == 0) backend = DATAHUB_BACKEND_SHM;
    }

    /* Register available infra backends into the DataHub registry.
     * Each backend is optional at compile time (CONFIG_EDGE_INFRA_DDS / _MQTT), so the
     * symbol must be referenced under a guard: an unconditional reference resolves to an
     * undefined symbol in trimmed builds and breaks the link (#416). */
#if defined(CONFIG_EDGE_INFRA_DDS) && (CONFIG_EDGE_INFRA_DDS + 0)
    {
        extern const datahub_ops_t g_datahub_dds_ops;
        datahub_register_backend(DATAHUB_BACKEND_DDS, &g_datahub_dds_ops);
    }
#else
    LOG_WARN(LOG_CAT_CORE, "DDS backend not compiled in; DATAHUB_BACKEND_DDS unavailable");
#endif

#if defined(CONFIG_EDGE_INFRA_MQTT) && (CONFIG_EDGE_INFRA_MQTT + 0)
    {
        extern const datahub_ops_t g_datahub_mqtt_ops;
        datahub_register_backend(DATAHUB_BACKEND_MQTT, &g_datahub_mqtt_ops);
    }
#else
    LOG_WARN(LOG_CAT_CORE, "MQTT backend not compiled in; DATAHUB_BACKEND_MQTT unavailable");
#endif

    /* Degrade instead of failing the whole node when the requested backend is absent */
    if (backend != DATAHUB_BACKEND_SHM && !datahub_get_backend(backend)) {
        LOG_WARN(LOG_CAT_CORE, "DataHub backend %d unavailable, falling back to SHM", (int)backend);
        backend = DATAHUB_BACKEND_SHM;
    }

    int rc = 0;
    if (backend != DATAHUB_BACKEND_SHM) {
        datahub_config_t hub_conf = {
            .backend_type = backend,
            .ring_slots = ring_cap,
            .shadow_capacity = shadow_cap
        };
        strncpy(hub_conf.shm_name, p_name, sizeof(hub_conf.shm_name) - 1);
        rc = datahub_init_with_config(container->hub, cfg->loop, &hub_conf,
                                      cfg->tags, cfg->tag_count, NULL, cfg->map_fn, NULL);
    } else {
        rc = datahub_init(container->hub, cfg->loop, p_name, ring_cap, shadow_cap,
                          cfg->tags, cfg->tag_count, NULL, cfg->map_fn, NULL);
    }
    if (rc != 0) {
        free(container->hub);
        container->hub = NULL;
        pthread_mutex_destroy(&container->lock);
        return -4;
    }
    if (cfg->map_fn_v2) {
        datahub_set_tag_mapper_v2(container->hub, cfg->map_fn_v2);
    }

    /* 2. Pre-allocate Memory Pool for Zero-Copy execution */
    if (cfg->enable_dma_buf) {
        dma_buf_pool_config_t pool_cfg = {
            .buffer_size = cfg->memory_pool_buffer_size ? cfg->memory_pool_buffer_size : 4096,
            .pool_capacity = cfg->memory_pool_capacity ? cfg->memory_pool_capacity : 16,
            .enable_auto_flush = true,
            .alignment = 64
        };
        dma_buf_pool_init(&container->mem_pool, &pool_cfg);
    }

    /* 3. Initialize Anti-Corruption Engine */
    if (cfg->enable_anti_corruption) {
        container->acl_engine = (edge_anti_corruption_engine_t *)calloc(1, sizeof(edge_anti_corruption_engine_t));
        if (container->acl_engine) {
            edge_anti_corruption_init(container->acl_engine, 128);
        }
    }

    /* 4. Wire Providers to Consumer Ports (Glue Layer) */
    edge_port_adapter_from_datahub(&container->ports.reader, &container->ports.writer, container->hub);
    edge_port_adapter_from_system_clock(&container->ports.clock);
    if (container->mem_pool) {
        edge_port_adapter_from_dma_pool(&container->ports.memory, container->mem_pool);
    }
    edge_port_adapter_from_console_logger(&container->ports.logger);

    /* 5. Initialize container metrics */
    if (cfg->product_name) {
        strncpy(container->metrics.product_name, cfg->product_name, sizeof(container->metrics.product_name) - 1);
    } else {
        strncpy(container->metrics.product_name, "EdgeOS-Product", sizeof(container->metrics.product_name) - 1);
    }
    container->metrics.active_consumers = 0;
    container->metrics.port_read_ops = 0;
    container->metrics.port_write_ops = 0;
    container->metrics.port_alloc_ops = 0;
    container->metrics.zero_alloc_runtime = true;
    container->metrics.initialized = true;

    container->initialized = true;
    return 0;
}

edge_composition_container_t *edge_composition_root_create_in(edge_composition_container_storage_t *store,
                                                              const edge_composition_config_t *cfg) {
    if (!store || !cfg) return NULL;
    edge_composition_container_t *container = (edge_composition_container_t *)store->raw;
    memset(container, 0, sizeof(struct edge_composition_container));
    container->is_caller_allocated = true;

    int rc = _init_container_resources(container, cfg);
    if (rc != 0) return NULL;
    return container;
}

int edge_composition_root_init(edge_composition_container_t **out_container,
                               const edge_composition_config_t *cfg) {
    if (!out_container || !cfg) return -1;

    edge_composition_container_t *container = (edge_composition_container_t *)calloc(1, sizeof(edge_composition_container_t));
    if (!container) return -2;
    container->is_caller_allocated = false;

    int rc = _init_container_resources(container, cfg);
    if (rc != 0) {
        free(container);
        return rc;
    }

    *out_container = container;
    return 0;
}

int edge_composition_root_destroy(edge_composition_container_t *container) {
    if (!container) return -1;

    pthread_mutex_lock(&container->lock);

    if (container->acl_engine) {
        edge_anti_corruption_destroy(container->acl_engine);
        free(container->acl_engine);
        container->acl_engine = NULL;
    }

    if (container->mem_pool) {
        dma_buf_pool_destroy(container->mem_pool);
        container->mem_pool = NULL;
    }

    if (container->hub) {
        datahub_destroy(container->hub);
        free(container->hub);
        container->hub = NULL;
    }

    container->initialized = false;
    bool was_caller_alloc = container->is_caller_allocated;
    pthread_mutex_unlock(&container->lock);
    pthread_mutex_destroy(&container->lock);

    if (!was_caller_alloc) {
        free(container);
    }
    return 0;
}

int edge_composition_root_get_ports(edge_composition_container_t *container,
                                    edge_consumer_ports_t *out_ports) {
    if (!container || !out_ports) return -1;
    pthread_mutex_lock(&container->lock);
    *out_ports = container->ports;
    pthread_mutex_unlock(&container->lock);
    return 0;
}

int edge_composition_root_inject_consumer(edge_composition_container_t *container,
                                         void *consumer_ctx,
                                         int (*wire_fn)(void *ctx, const edge_consumer_ports_t *ports)) {
    if (!container || !consumer_ctx || !wire_fn) return -1;

    pthread_mutex_lock(&container->lock);
    int rc = wire_fn(consumer_ctx, &container->ports);
    if (rc == 0) {
        container->metrics.active_consumers++;
    }
    pthread_mutex_unlock(&container->lock);
    return rc;
}

datahub_t *edge_composition_root_get_datahub(edge_composition_container_t *container) {
    if (!container) return NULL;
    return container->hub;
}

int edge_composition_root_get_metrics(edge_composition_container_t *container,
                                      edge_composition_metrics_t *metrics) {
    if (!container || !metrics) return -1;
    pthread_mutex_lock(&container->lock);
    *metrics = container->metrics;
    pthread_mutex_unlock(&container->lock);
    return 0;
}
