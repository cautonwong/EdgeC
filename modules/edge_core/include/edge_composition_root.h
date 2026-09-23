/**
 * @file edge_composition_root.h
 * @brief EdgeOS Single Composition Root & Dependency Injection Framework (AEP-751 / Issue #336)
 *
 * Provides a standardized Composition Root for EdgeOS products (DCU, IEMS, SmartBox),
 * ensuring that object instantiation, memory pooling, and port injection are centralized
 * in one place, eliminating global service locators and hidden singletons.
 */

#ifndef EDGE_COMPOSITION_ROOT_H
#define EDGE_COMPOSITION_ROOT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "edge_port.h"
#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Composition Configuration & Structures                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Configuration for initializing the Composition Root Container
 */
typedef struct {
    const char *product_name;           /**< Product name (e.g. "IEMS", "DCU", "SmartBox") */
    size_t datahub_capacity;            /**< DataHub shadow table tag capacity */
    size_t memory_pool_buffer_size;     /**< Size of each pre-allocated buffer (bytes) */
    uint32_t memory_pool_capacity;      /**< Number of pre-allocated buffers */
    bool enable_dma_buf;                /**< Enable DMA-BUF zero-copy allocation */
    bool enable_anti_corruption;        /**< Initialize Anti-Corruption Layer */
} edge_composition_config_t;

/**
 * @brief Composition Container Telemetry & Integrity Metrics
 */
typedef struct {
    char product_name[32];              /**< Name of assembled product */
    uint32_t active_consumers;          /**< Number of consumer modules injected */
    uint64_t port_read_ops;             /**< Cumulative reads via Reader Port */
    uint64_t port_write_ops;            /**< Cumulative writes via Writer Port */
    uint64_t port_alloc_ops;            /**< Cumulative buffer acquires via Memory Port */
    bool zero_alloc_runtime;            /**< True if no dynamic heap calls in execution phase */
    bool initialized;                   /**< True if container is healthy and running */
} edge_composition_metrics_t;

typedef struct edge_composition_container edge_composition_container_t;

/* -------------------------------------------------------------------------- */
/* Public APIs                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Build and assemble a Single Composition Root container
 */
int edge_composition_root_init(edge_composition_container_t **out_container,
                               const edge_composition_config_t *cfg);

/**
 * @brief Destroy the Composition Root and cleanly teardown all owned providers
 */
int edge_composition_root_destroy(edge_composition_container_t *container);

/**
 * @brief Retrieve the assembled Consumer Ports bundle from the Composition Root
 */
int edge_composition_root_get_ports(edge_composition_container_t *container,
                                    edge_consumer_ports_t *out_ports);

/**
 * @brief Invalidate / wire a consumer module by injecting Ports explicitly
 */
int edge_composition_root_inject_consumer(edge_composition_container_t *container,
                                         void *consumer_ctx,
                                         int (*wire_fn)(void *ctx, const edge_consumer_ports_t *ports));

/**
 * @brief Get DataHub handle owned by the Composition Root
 */
datahub_t *edge_composition_root_get_datahub(edge_composition_container_t *container);

/**
 * @brief Retrieve Composition Root runtime metrics
 */
int edge_composition_root_get_metrics(edge_composition_container_t *container,
                                      edge_composition_metrics_t *metrics);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_COMPOSITION_ROOT_H */
