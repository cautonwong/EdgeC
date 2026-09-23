/**
 * @file edge_composition_root.c
 * @brief EdgeOS Single Composition Root Implementation (AEP-751 / Issue #336)
 */

#include "edge_composition_root.h"
#include "datahub_ops.h"
#include "dma_buf_pool.h"
#include "edge_anti_corruption.h"
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
};

int edge_composition_root_init(edge_composition_container_t **out_container,
                               const edge_composition_config_t *cfg) {
    if (!out_container || !cfg) return -1;

    edge_composition_container_t *container = (edge_composition_container_t *)calloc(1, sizeof(edge_composition_container_t));
    if (!container) return -2;

    container->config = *cfg;
    pthread_mutex_init(&container->lock, NULL);

    /* 1. Allocate & Initialize Foundation: DataHub shadow memory bus */
    container->hub = (datahub_t *)calloc(1, sizeof(datahub_t));
    if (!container->hub) {
        pthread_mutex_destroy(&container->lock);
        free(container);
        return -3;
    }

    const char *p_name = cfg->product_name ? cfg->product_name : "edgeos_product";
    uint32_t shadow_cap = cfg->datahub_capacity ? (uint32_t)cfg->datahub_capacity : 256;
    int rc = datahub_init(container->hub, NULL, p_name, 64, shadow_cap, NULL, 0, NULL, NULL, NULL);
    if (rc != 0) {
        free(container->hub);
        pthread_mutex_destroy(&container->lock);
        free(container);
        return -4;
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
    snprintf(container->metrics.product_name, sizeof(container->metrics.product_name), "%s",
             cfg->product_name ? cfg->product_name : "EdgeOS-Product");
    container->metrics.zero_alloc_runtime = true;
    container->metrics.initialized = true;

    container->initialized = true;
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
        if (container->hub->ops && container->hub->ops->destroy) {
            container->hub->ops->destroy(container->hub);
        }
        free(container->hub);
        container->hub = NULL;
    }

    container->initialized = false;
    pthread_mutex_unlock(&container->lock);
    pthread_mutex_destroy(&container->lock);
    free(container);
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
