/**
 * @file edge_port.h
 * @brief EdgeOS Consumer-Defined Ports (Ports & Adapters / Hexagonal Architecture - AEP-751 / Issue #336)
 *
 * Defines explicit consumer-owned abstract port interfaces for decoupling business logic,
 * control loops, and high-performance algorithms from concrete physical buses, HAL, and RTOS.
 */

#ifndef EDGE_PORT_H
#define EDGE_PORT_H

#include "edge_port_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct datahub_s datahub_t;

/* -------------------------------------------------------------------------- */
/* Standard Glue Adapters (Wire Provider implementations to Consumer Ports)   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Wire DataHub shadow memory bus to Consumer Reader/Writer Ports
 */
int edge_port_adapter_from_datahub(edge_reader_port_t *out_reader,
                                  edge_writer_port_t *out_writer,
                                  datahub_t *hub);

/**
 * @brief Wire POSIX/Host monotonic system clock to Consumer Clock Port
 */
int edge_port_adapter_from_system_clock(edge_clock_port_t *out_clock);

/**
 * @brief Wire standard stdout/stderr console logger to Consumer Logger Port
 */
int edge_port_adapter_from_console_logger(edge_logger_port_t *out_logger);

/**
 * @brief Create pure mock/fake ports for 100% isolated host unit tests
 */
int edge_port_create_mock_ports(edge_consumer_ports_t *out_ports,
                                double initial_tag_val);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_PORT_H */
