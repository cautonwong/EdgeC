/**
 * @file edge_port.h
 * @brief EdgeOS Consumer-Defined Ports (Ports & Adapters / Hexagonal Architecture - AEP-751 / Issue #336)
 *
 * Defines explicit consumer-owned abstract port interfaces for decoupling business logic,
 * control loops, and high-performance algorithms from concrete physical buses, HAL, and RTOS.
 */

#ifndef EDGE_PORT_H
#define EDGE_PORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct datahub_s datahub_t;

/* -------------------------------------------------------------------------- */
/* Consumer-Defined Port Interfaces                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Consumer Port: Real-time Telemetry & State Reader
 */
typedef struct edge_reader_port {
    int (*read_tag)(void *ctx, uint32_t tag_id, double *out_val, uint64_t *out_ts_us);
    int (*read_batch)(void *ctx, const uint32_t *tag_ids, double *out_vals, size_t count);
    void *ctx;
} edge_reader_port_t;

/**
 * @brief Consumer Port: Actuation, Setpoint & Telecontrol Writer
 */
typedef struct edge_writer_port {
    int (*write_tag)(void *ctx, uint32_t tag_id, double val, uint64_t ts_us);
    int (*write_batch)(void *ctx, const uint32_t *tag_ids, const double *vals, size_t count);
    void *ctx;
} edge_writer_port_t;

/**
 * @brief Consumer Port: Deterministic Wall-Clock & Monotonic Time Provider
 */
typedef struct edge_clock_port {
    uint64_t (*now_us)(void *ctx);
    uint64_t (*now_ms)(void *ctx);
    void *ctx;
} edge_clock_port_t;

/**
 * @brief Consumer Port: Zero-Copy Preallocated Buffer / DMA-BUF Memory Allocator
 */
typedef struct edge_memory_port {
    int (*acquire_buffer)(void *ctx, size_t size, void **out_ptr, int *out_fd);
    int (*release_buffer)(void *ctx, void *ptr);
    void *ctx;
} edge_memory_port_t;

/**
 * @brief Consumer Port: Structured Telemetry & Diagnostics Logger
 */
typedef struct edge_logger_port {
    void (*log_msg)(void *ctx, int level, const char *tag, const char *msg);
    void *ctx;
} edge_logger_port_t;

/**
 * @brief Unified Injected Consumer Ports Bundle
 */
typedef struct edge_consumer_ports {
    edge_reader_port_t reader;
    edge_writer_port_t writer;
    edge_clock_port_t clock;
    edge_memory_port_t memory;
    edge_logger_port_t logger;
} edge_consumer_ports_t;

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
