/**
 * @file dcu_node.h
 * @brief DCU Edge Collector Domain Aggregate Root (Issue #375 / Clean Architecture)
 *
 * Encapsulates the Smart Meter Inventory, Poller Engine, Event Monitoring,
 * Storage Sink, and DataHub Shadow Bus into a single Aggregate Root.
 */

#ifndef DCU_NODE_H
#define DCU_NODE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <uv.h>

#include "datahub.h"
#include "dcu_event_monitor.h"
#include "dcu_inventory.h"
#include "dcu_poller.h"
#include "dcu_router.h"
#include "dcu_storage.h"
#include "edge_composition_root.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dcu_node_s dcu_node_t;

typedef struct {
    const char *product_name;  /**< Product identifier, default "DCU" */
    const char *db_path;       /**< Path to SQLite digital twin database */
    size_t datahub_capacity;   /**< DataHub tag capacity (e.g. 32768) */
    uint32_t poll_interval_ms; /**< Poller timer period in ms (default 1000) */
} dcu_node_config_t;

/**
 * @brief Create and assemble a DCU Node Aggregate Root
 */
dcu_node_t *dcu_node_create(uv_loop_t *loop, const dcu_node_config_t *cfg);

/**
 * @brief Start the DCU Node polling loop and event engines
 */
int dcu_node_start(dcu_node_t *node);

/**
 * @brief Stop the DCU Node polling loop
 */
int dcu_node_stop(dcu_node_t *node);

/**
 * @brief Perform a single polling pump cycle across active inventory
 */
int dcu_node_pump(dcu_node_t *node);

/**
 * @brief Access encapsulated Meter Inventory
 */
dcu_inventory_t *dcu_node_get_inventory(dcu_node_t *node);

/**
 * @brief Access encapsulated Poller Engine
 */
dcu_poller_t *dcu_node_get_poller(dcu_node_t *node);

/**
 * @brief Access encapsulated Storage Sink
 */
dcu_storage_t *dcu_node_get_storage(dcu_node_t *node);

/**
 * @brief Access encapsulated DataHub
 */
datahub_t *dcu_node_get_datahub(dcu_node_t *node);

/**
 * @brief Access Composition Root Container
 */
edge_composition_container_t *dcu_node_get_container(dcu_node_t *node);

/**
 * @brief Access Event Objects (Voltage Drop / Overcurrent)
 */
dcu_event_obj_t *dcu_node_get_event_3000(dcu_node_t *node);
dcu_event_obj_t *dcu_node_get_event_3005(dcu_node_t *node);

/**
 * @brief Destroy the DCU Node and clean up all resources
 */
void dcu_node_destroy(dcu_node_t *node);

#ifdef __cplusplus
}
#endif

#endif /* DCU_NODE_H */
