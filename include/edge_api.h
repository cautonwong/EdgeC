#ifndef EDGE_API_H
#define EDGE_API_H

#include "bus_orchestrator.h"
#include "req_driver_legacy.h"
#include "req_raw_io.h"

/**
 * @brief Initializes the high-level IO API with the core orchestrator.
 *
 * @param orch The main bus orchestrator instance.
 * @param drv_mgr The main driver manager instance.
 */
void edge_api_init(bus_orchestrator_t* orch, driver_manager_t* drv_mgr);

/**
 * @brief Submits a high-level request to read a single tag value.
 */
int edge_api_read_tag(int bus_id, const char* protocol, uint32_t slave_id, uint32_t physical_tag_id,
                      legacy_on_complete_cb cb, void* user_data);

/**
 * @brief Submits a high-level request to write a single tag value.
 */
int edge_api_write_tag(int bus_id, const char* protocol, uint32_t slave_id,
                       uint32_t physical_tag_id, double value, legacy_on_complete_cb cb,
                       void* user_data);

/**
 * @brief Submits a raw I/O passthrough request.
 */
int edge_api_raw_io(int bus_id, net_buf_t* tx_buf, raw_io_on_complete_cb cb);

#endif
