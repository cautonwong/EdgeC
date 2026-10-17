/**
 * @file edge_composition_root.h
 * @brief EdgeOS Single Composition Root & Dependency Injection Framework (AEP-751 / AEP-752 / Issue #336 & #340)
 *
 * Provides a standardized Composition Root for EdgeOS products (DCU, IEMS, SmartBox),
 * ensuring that object instantiation, memory pooling, and port injection are centralized
 * in one place, eliminating global service locators and hidden singletons.
 *
 * Conforms to:
 * - Consumer-Defined Interfaces (edge_consumer_ports_t)
 * - Aggregate Root Encapsulation
 * - Composition Root Symmetric Lifecycle
 * - Caller-Allocated Storage & Opaque Pointers
 */

#ifndef EDGE_COMPOSITION_ROOT_H
#define EDGE_COMPOSITION_ROOT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>
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
    /* DataHub specific options */
    uv_loop_t *loop;                    /**< Event loop for DataHub async/timers */
    const char *shm_name;               /**< Explicit SHM name or NULL for default/env */
    size_t ring_capacity;               /**< Ring buffer capacity (default: 4096) */
    const edge_tag_meta_t *tags;        /**< Contract tags metadata array */
    uint32_t tag_count;                 /**< Number of contract tags */
    datahub_tag_map_cb map_fn;          /**< Tag to compact index mapping function */
    datahub_tag_map_cb_v2 map_fn_v2;    /**< V2 tag mapping function */
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

/**
 * @brief Opaque Container Handle for Composition Root
 */
typedef struct edge_composition_container edge_composition_container_t;

/**
 * @brief Caller-Allocated Storage for Composition Root (Physical Opaque Encapsulation)
 */
typedef struct {
    alignas(max_align_t) uint8_t raw[1024];
} edge_composition_container_storage_t;

/* -------------------------------------------------------------------------- */
/* Public APIs                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief In-place placement constructor for Composition Root Container
 *
 * @param store Caller-allocated storage buffer
 * @param cfg Container configuration parameters
 * @return edge_composition_container_t* handle pointing into store, or NULL on error
 */
edge_composition_container_t *edge_composition_root_create_in(edge_composition_container_storage_t *store,
                                                              const edge_composition_config_t *cfg);

/**
 * @brief Build and assemble a Single Composition Root container (Heap allocated fallback)
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

/**
 * @brief Wire Linux DMA-BUF pool to Consumer Memory Port
 */
struct dma_buf_pool;
typedef struct dma_buf_pool dma_buf_pool_t;
int edge_port_adapter_from_dma_pool(edge_memory_port_t *out_mem, dma_buf_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_COMPOSITION_ROOT_H */
